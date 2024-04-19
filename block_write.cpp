#include "head.h"
#include "file_op.h"
#include "index_handle.h"


using namespace lucifer;

const static tfs::MMapOption mmap_option = {1024000, 4096, 4096}; // 内存映射参数

const static uint32_t main_blcok_size = 1014 * 1024 * 64; // 主块文件的大小

const static uint32_t bucket_size = 1000; // 哈希桶的大小

const static bool debug = true;
static int32_t block_id = 1;

int main(int argc, char **argv)
{
    std::string mainblock_path;
    std::string index_path;
    int32_t ret = tfs::TFS_SUCCESS;

         
    
    std::cout << "Type your block id:" << std::endl;
    std::cin >> block_id;
 
    if (block_id < 1)
    {
        std::cerr << "Invalid block id , exit." << std::endl;
        exit(-1);
    }


    tfs::IndexHandle *index_handle = new tfs::IndexHandle(".", block_id); // 索引文件句柄
    
    if(tfs::DEBUG)
        printf("Load index in block_write\n");
 
    
    ret = index_handle->load(block_id, bucket_size, mmap_option);

    if (ret != tfs::TFS_SUCCESS){
        fprintf(stderr, "load index %d failed.\n", block_id);
        delete index_handle;
        exit(-2);
    }

    if (tfs::DEBUG)
        printf("load index successful \n");

    //写入主块文件
    std::stringstream tmp_stream;
    tmp_stream << "." << tfs::MAINLOCK_DIR_PREFIX << block_id;
    tmp_stream >> mainblock_path;

    tfs::FileOperation *mainblock = new tfs::FileOperation(mainblock_path, O_RDWR | O_LARGEFILE | O_CREAT);

    
    char buffer[4096];
    memset(buffer, '6', 4096);
    

   
    int32_t data_offset = index_handle->get_block_data_offset();
    uint32_t file_no = index_handle->block_info()->seq_no_;

    ret = mainblock->pwrite_file(buffer, sizeof(buffer), data_offset);

    if (ret != tfs::TFS_SUCCESS)
    {
        fprintf(stderr, "write to mainblock failed.ret:%d, reason:%s\n", ret, strerror(errno));
        mainblock->close_file();

        delete mainblock;
        delete index_handle;
        exit(-3);
    }

    //3.写入metaonfo索引
    struct tfs::MetaInfo meta;

    meta.set_file_id(file_no);
    meta.set_inner_offset(data_offset);
    meta.set_size(sizeof(buffer));

    ret = index_handle->write_segment_meta(meta.get_key(),meta);
    if(debug)
        printf("write segment meta ret is %d\n",ret);

    if(ret == tfs::TFS_SUCCESS){
        //succsssful updata block info
        //1. 更新索引头部信息
        index_handle->commit_block_data_offset(sizeof(buffer));

        //2.更新块信息
        index_handle->update_block_info(tfs::C_OPER_INSERT,sizeof(buffer));
        ret = index_handle->flush();
        if(debug)
            printf("write_segmentmeta is TFS_SUCCESS\n");
        if(ret!=tfs::TFS_SUCCESS)
        {
            fprintf(stdout,"flush mainblock %d faild. file no: %u \n",block_id,file_no);

        }
       

    }else{
        fprintf(stderr,"write_segment_meta - mainblock %d failed.file no %u\n",block_id,file_no);
    }


     if(ret != tfs::TFS_SUCCESS)
    {
        fprintf(stderr, "write to mainblock %d failed. file no:%u\n", block_id, file_no);
        mainblock->close_file();
    }
    else
    {
        if(tfs::DEBUG) printf("write successfully. file no: %u, block_id:%d\n", file_no, block_id);
    }
  

    mainblock->close_file();
    delete mainblock;
    delete index_handle;
    return 0;
}
