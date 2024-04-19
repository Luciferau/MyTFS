#include "head.h"
#include "mmap_file_op.h"
#include <iostream>

using namespace std;
using namespace lucifer;
int main(void){


    const string file_name = "mmap_file_op.txt";
    tfs::MMapFileOperation * mmap_file_op =  new tfs::MMapFileOperation(file_name, O_CREAT|O_RDWR|O_LARGEFILE);

    int fd = mmap_file_op->open_file();
    if(fd < 0){
        std::cout<<"fd < 0 "<<std::endl;
        exit(-1);
    }

    char buffer[128];
    memset(buffer,'6',sizeof(buffer));

   

    mmap_file_op->close_file();
    return 0;
}