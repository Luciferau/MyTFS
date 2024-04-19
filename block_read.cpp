#include "head.h"
#include "file_op.h"
#include "index_handle.h"

using namespace lucifer;

const static tfs::MMapOption mmap_option = {1024000, 4096, 4096}; // 内存映射参数

const static uint32_t main_blcok_size = 1014 * 1024 * 64; // 主块文件的大小

const static uint32_t bucket_size = 1000; // 哈希桶的大小

static int32_t block_id = 1;

int main(int argc, char **argv)
{
    std::string mainblock_path;
    std::string index_path;
    int32_t ret = tfs::TFS_SUCCESS;
    tfs::IndexHandle *index_handle = new tfs::IndexHandle(".", block_id);

    std::cout << "Type you bock id:" << std::endl;
    std::cin >> block_id;
   
    if (block_id < 0){
        std::cerr << "Invalid blockid,exit" << std::endl;
        exit(-1);    
    }


    ret = index_handle->load(block_id, bucket_size, mmap_option);
    
    if (ret != tfs::TFS_SUCCESS){
        fprintf(stderr, "Fail to load index:%s\n", strerror(errno));
        std::cerr << "Fail to load index ";
        std::cout << std::endl;
        delete index_handle;

        index_handle->remove(block_id);
        exit(-2);
    }
    
    //2.读取文件的meta info
    uint64_t file_id;

    std::cout << "Type you file_id:" << std::endl;
    std::cin >> file_id;

   
    if (file_id < 1){
        std::cerr << "Invalid blockid,exit" << std::endl;
        exit(-2);    
    }

    
    tfs::MetaInfo meta;

    ret = index_handle->read_segment_meta(file_id,meta);
    if(tfs::TFS_SUCCESS != ret)
    {
        fprintf(stderr,"read_segment_meta error,file_id %lu,ret %d",file_id,ret);
        exit(-3);
    }   
    //3根据meta info读取文件
    //读取主块文件
    std::stringstream tmp_stream;
    tmp_stream << "." << tfs::MAINLOCK_DIR_PREFIX << block_id;
    tmp_stream >> mainblock_path;

    tfs::FileOperation *mainblock = new tfs::FileOperation(mainblock_path, O_RDWR);

    char *buffer = new char[meta.get_size()+1];
    
    ret = mainblock->pread_file(buffer,meta.get_size(),meta.get_inner_offset());
    
    if(ret != tfs::TFS_SUCCESS)
    {
        fprintf(stderr, "read from mainblock %d failed. ret:%d\n", block_id,ret);
        mainblock->close_file();
        delete mainblock;
        delete index_handle;
    }
    else
    {
        if(tfs::DEBUG) printf("write successfully. ret : %u, block_id:%d\n", ret, block_id);
    }
  
    buffer[meta.get_size()] = '\0';
    
    printf("read size %d\ncontent : %s\n",meta.get_size(),buffer);
    mainblock->close_file();
    delete mainblock;
    
   
    delete index_handle;
    return 0;
}
