#include "head.h"
#include "file_op.h"
#include "index_handle.h"
 
 
using namespace lucifer;

const static tfs::MMapOption mmap_option = {1024000,4096,4096};//内存映射参数

const static uint32_t main_blcok_size = 1014*1024*64;//主块文件的大小

const static uint32_t bucket_size = 1000;//哈希桶的大小

static int32_t block_id = 1;

int main(int argc,char **argv)
{
    std::string mainblock_path;
    std::string index_path;
    int32_t ret = tfs::TFS_SUCCESS;

    std::cout<<"Type you bock id:"<<std::endl;
    std::cin>>block_id;
    if(block_id < 0){
        std::cerr<<"Invalid blockid,exie"<<std::endl;
        exit(-1);
    }
    //生成主块文件
    std::stringstream tmp_stream;
    tmp_stream << "."<< tfs::MAINLOCK_DIR_PREFIX  <<block_id;
    tmp_stream >> mainblock_path;
    
    tfs::FileOperation * mainblock = new tfs::FileOperation(mainblock_path,O_RDWR|O_LARGEFILE|O_CREAT);

    ret = mainblock->ftruncate_file(main_blcok_size);
    if(ret != 0){
        fprintf(stderr,"Fail to truncate file:%s\n",strerror(errno));
        
        std::cerr<<"Fail to truncate file"<<std::endl;
        delete mainblock;
        
        exit(-2);
    }
    
    //生成索引文件
    tfs::IndexHandle * index_handle = new tfs::IndexHandle(".",block_id);
    if(tfs::DEBUG)
        printf("Create index init\n");
    ret = index_handle->create(block_id,bucket_size,mmap_option);
    if(ret != tfs::TFS_SUCCESS)
    { 
        fprintf(stderr,"Fail to create index:%s\n",strerror(errno));
       
       
        delete mainblock;
        
        index_handle->remove(block_id);
        exit(-3);
    }
  
    mainblock->close_file();
    index_handle->flush();
    delete mainblock;
    delete index_handle;


    //other
    return 0;


 

}