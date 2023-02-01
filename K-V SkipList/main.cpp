#include <iostream>
#include "skiplist.h"
#include<vector>
#define FILE_PATH "./store/dumpFile"
int main() {

    SkipList<int, std::string> skipList(100);
    skipList.InsertNode(1, "你");
    skipList.InsertNode(3, "好");
    skipList.InsertNode(8, "啊");
    skipList.InsertNode(7, "帅");
    skipList.DumpFile();

    // skipList.load_file();

    skipList.SearchNode(1);
    skipList.SearchNode(8);

    skipList.ShowList();

    skipList.DeleteNode(1);


    skipList.ShowList();


    return 0;
}
