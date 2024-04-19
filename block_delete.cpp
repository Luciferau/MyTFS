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
        fprintf(stderr, "Fail to load index:%s ret: %d\n", strerror(errno),ret);
         
        delete index_handle;

        index_handle->remove(block_id);
        exit(-2);
    }
    //删除指定文件的meta info
    uint64_t file_id;

    std::cout << "Type you file_id:" << std::endl;
    std::cin >> file_id;

   
    if (file_id < 1){
        std::cerr << "Invalid blockid,exit" << std::endl;
        exit(-2);    
    }

    ret = index_handle->delete_segment_meta(file_id);

    if(tfs::TFS_SUCCESS != ret){
        fprintf(stderr, "Fail to delete segment meta:%s ret: %d\n", strerror(errno),ret);

        delete index_handle;
        index_handle->remove(block_id);
        exit(-3);
    }

    ret = index_handle->flush();
    if(ret != tfs::TFS_SUCCESS){
        fprintf(stderr,"flush error");
    }

    printf("delete successful \n");
    
     
    //delete index_handle;
    return 0;


}