#include "mmap_file.h" 
#include "head.h"

using namespace std;
using namespace lucifer;

static constexpr mode_t OPEN_MODE = 0644;

const static tfs::MMapOption mmap_option = {
    .max_mmap_size_ = 10240000,
    .first_mmap_size_ = 4096,
    .per_mmap_size_ = 4096  
  
};

int open_file(string file_name,int open_flags)
{
    int fd = open(file_name.c_str(), open_flags, OPEN_MODE);
    if (fd == -1)
    {
        perror("open");
        return -errno;
         
    }
    return fd;
}


int main(void)
{
     
     
    const char * filename = "./test.txt";
    int fd = open_file(filename, O_RDWR | O_CREAT|O_LARGEFILE);

    if (fd == -1)
    {
        fprintf(stderr, "open file error\n file name%s,error desc:%s\n", filename, strerror(-fd));
        return -1;
    }         
    
    tfs::MMapFile *map_file = new tfs::MMapFile(mmap_option,fd);
    
    bool is_mapped = map_file->mmapFile(true);
    if (!is_mapped)
    {
        fprintf(stderr, "mmap file error\n");
        return -1;
    }else
    {
        map_file->remapFile();
        memset(map_file->getData(), '6', map_file->getSize());
        map_file->syncFile();
        map_file->mumapFile();
    }
    
    close(fd);

    return 0;
}