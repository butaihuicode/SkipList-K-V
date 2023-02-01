//
// Created by challway on 2023/1/11.
//

#ifndef K_V_SKIPLIST_K_V_SKIPLIST_H
#define K_V_SKIPLIST_K_V_SKIPLIST_H

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <string>
#include <cstring>
#include <mutex>
#include <fstream>

#define STORE_FILE "store/dumpFile"

std::mutex mtx;
std::string delimiter=":";



template<typename K,typename V>
class SkipListNode
{
public:
    //SkipListNode(){};
    SkipListNode(K key,V value,int level);
    ~SkipListNode()=default;

    void set_value(V value);
    K get_key() const;
    V get_value() const;

    //每一个节点至少要有第一层，然后有一个数组存储指向的元素
    SkipListNode<K,V> ** forward;
    //节点的等级
    int level_;
private:
    K key_;
    V value_;
};

template<typename K,typename V>
SkipListNode<K,V>::SkipListNode(K key,V value,int level):key_(key),value_(value),level_(level){
    //new一个大小为level+1的数组，里面放的都是SkipListNode的指针，这些指针还没分配空间。
    forward=new SkipListNode<K,V>*[level+1];
    memset(forward,0,sizeof(SkipListNode<K,V>*)*(level+1));
}

template<typename K,typename V>
K SkipListNode<K,V>::get_key()const
{
    return key_;
}

template<typename K,typename V>
V SkipListNode<K,V>::get_value()const
{
    return value_;
}

template<typename K,typename V>
void SkipListNode<K,V>::set_value(V value)
{
    value_=value;
}

template<typename K,typename V>
class SkipList{

public:
    SkipList(int max_level);
    ~SkipList();
    SkipListNode<K,V>* CreateNode(K,V,int);
    void InsertNode(K,V);
    bool SearchNode(K);
    void DeleteNode(K);
    //落盘
    void DumpFile();
    void LoadFile();

    void ShowList();

    int get_random_level();

private:

    void get_key_value_from_string(const std::string& str, std::string* key, std::string* value);
    bool is_vaild_string(const std::string& str);

    SkipListNode<K,V>* header_;

    //最大的层数
    int max_level_;
    //当前的层数
    int cur_level_;
    int element_count_;

    // file operator
    std::ofstream file_writer_;
    std::ifstream file_reader_;
};

template<typename K,typename V>
SkipListNode<K,V>* SkipList<K, V>::CreateNode(K key, V value, int level){
    SkipListNode<K,V>* node=new SkipListNode<K,V>(key,value,level);
    return node;
}
//insert的时间复杂度是？
template<typename K,typename V>
void SkipList<K,V>::InsertNode(K key, V value) {
    mtx.lock();
    SkipListNode<K,V>* cur_node=header_;
    //update是需要更新的节点汇总，每一个level都有一个要续在他后面的节点
    SkipListNode<K,V> *update[max_level_+1];
    memset(update, 0, sizeof(SkipListNode<K, V>*)*(max_level_+1));
    //start searching from highest level
    //level是forward的行数,判断时只需要取每行第一个，所以直接用数组首地址就可以
    for(int i=cur_level_;i>=0;i--)
    {
        //Insert，层数只会越来越低。forward[i]代表着每个节点i层的下一个节点
        //根本不需要往下指，因为本来就是一个节点，最高层到了位置就直接到下一层，节点没变，但每一层需要更新的节点不一样
        while(cur_node->forward[i]!= nullptr&&cur_node->forward[i]->get_key()<key)
        {
            cur_node=cur_node->forward[i];
        }
        update[i]=cur_node;
    }
    cur_node=cur_node->forward[0];
    //要插入的键已存在
    if(cur_node!= nullptr&&cur_node->get_key()==key)
    {
        std::cout << "key: " << key << ", exists" << std::endl;
        mtx.unlock();
        return;
    }
    //到尾部了或者要插入元素不一样
    if(cur_node== nullptr || cur_node->get_key()!=key){
        int random_level=get_random_level();
        //int random_level=10;
        if(random_level>cur_level_)
        {
            //如果随机层超过了当前层，先把空层填入update，也就是将要接受接到后面的节点，也就是head
            for(int i=cur_level_+1;i<=random_level;++i)
            {
                update[i]=header_;
            }
            cur_level_=random_level;
        }

        SkipListNode<K,V>* insert_node= CreateNode(key,value,random_level);
        for(int i=0;i<=random_level;++i)
        {
            //前后都要接上
            insert_node->forward[i]=update[i]->forward[i];
            update[i]->forward[i]=insert_node;
        }
        std::cout << "Successfully inserted key:" << key << ", value:" << value << std::endl;
        ++element_count_;
    }
    mtx.unlock();
    return;
}

template<typename K,typename V>
void SkipList<K,V>::DeleteNode(K key)
{
    mtx.lock();
    SkipListNode<K,V>* cur_node=header_;
    SkipListNode<K,V>* to_delete[max_level_+1];
    memset(to_delete, 0, sizeof(SkipListNode<K, V>*)*(max_level_+1));
    for(int i=cur_level_;i>=0;i--)
    {
        while(cur_node->forward[i]!=NULL&&cur_node->forward[i]->get_key()<key)
        {
            cur_node=cur_node->forward[i];
        }
        to_delete[i]=cur_node;
    }
    cur_node=cur_node->forward[0];
    if(cur_node!= nullptr && cur_node->get_key()==key)
    {
        for(int i=0;i<=cur_level_;++i)
        {
            if(to_delete[i]->forward[i]!=cur_node)
            {
                //说明到这一层没有了
                break;
            }
            to_delete[i]->forward[i]=cur_node->forward[i];
        }
        //如果删除的节点拥有唯一的最高层
        while(cur_level_>0&&header_->forward[cur_level_]==0)
        {
            --cur_level_;
        }
        std::cout << "Successfully deleted key "<< key << std::endl;
        --element_count_;
    }
    mtx.unlock();
}

template<typename K,typename V>
bool SkipList<K,V>::SearchNode(K key)
{
    SkipListNode<K,V>* cur_node=header_;
    for(int i=cur_level_;i>=0;--i)
    {
        while(cur_node->forward[i]!= nullptr&&cur_node->forward[i]->get_key()<key){
            cur_node=cur_node->forward[i];
        }
    }
    cur_node=cur_node->forward[0];
    if(cur_node->get_key()==key)
    {
        std::cout << "Found key: " << key << ", value: " << cur_node->get_value()<< std::endl;
        return true;
    }
    std::cout << "Not Found Key:" << key << std::endl;
    return false;
}

template<typename K,typename V>
void SkipList<K,V>::DumpFile()
{
    std::cout << "dump_file-----------------" << std::endl;
    file_writer_.open(STORE_FILE);
    SkipListNode<K,V>* node=header_->forward[0];

    while(node!= nullptr)
    {
        file_writer_<<node->get_key()<<":"<<node->get_value()<<std::endl;
        std::cout<<node->get_key()<<":"<<node->get_value()<<std::endl;
        node=node->forward[0];
    }
    //刷新缓冲区
    file_writer_.flush();
    file_writer_.close();
}

template<typename K,typename V>
void SkipList<K,V>::LoadFile() {
    file_reader_.open(STORE_FILE);
    std::cout << "load_file-----------------" << std::endl;
    std::string line,key,value;
    //getline默认分隔符为\n
    while(getline(file_reader_,line))
    {
        get_key_value_from_string(line,&key,&value);
        InsertNode(key,value);
        std::cout << "key:" << key << "value:" << value << std::endl;
    }
   file_reader_.close();
}

template<typename K,typename V>
void SkipList<K,V>::ShowList() {
    std::cout << "\n*****Skip List*****"<<"\n";
    for(int i=0;i<=cur_level_;i++) {
        SkipListNode<K, V> *node = header_->forward[i];
        std::cout <<"LEVEL"<<i<<" ";
        while (node != nullptr) {
            std::cout<< node->get_key() << ":" << node->get_value() << "; " ;
            node = node->forward[i];
        }
        std::cout<<std::endl;
    }
}

template<typename K, typename V>
void SkipList<K, V>::get_key_value_from_string(const std::string& str, std::string* key, std::string* value)
{
    if(!is_vaild_string(str))
    {
        return ;
    }
    std::string::size_type pos=str.find(delimiter);
    *key=str.substr(0,pos);
    *value=str.substr(pos+1,str.size()-pos);
}

template<typename K, typename V>
bool SkipList<K, V>::is_vaild_string(const std::string& str)
{
    if(str.size()==0)
    {
        return false;
    }
    if(str.find(delimiter)==std::string::npos)
    {
        return false;
    }
    return true;
}

template<typename K, typename V>
int SkipList<K, V>::get_random_level() {
    int k = 1;
    while (rand() % 2) {
        k++;
    }
    k = (k < max_level_) ? k : max_level_;
    return k;
}


template<typename K,typename V>
SkipList<K,V>::SkipList(int max_level):max_level_(max_level),cur_level_(0),element_count_(0) {
    K key;
    V value;
    header_=new SkipListNode<K,V>(key,value,max_level);
    //header_=header_->forward[0];
}

template<typename K, typename V>
SkipList<K, V>::~SkipList() {

    if (file_writer_.is_open()) {
        file_writer_.close();
    }
    if (file_reader_.is_open()) {
        file_reader_.close();
    }
    delete header_;
}









#endif //K_V_SKIPLIST_K_V_SKIPLIST_H
