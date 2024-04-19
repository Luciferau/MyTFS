#include "mmap_file_op.h"
#include <inttypes.h>
 
static int debug = 1;

namespace lucifer
{
    namespace tfs
    {

        MMapFileOperation::~MMapFileOperation()
        {
            if (map_file_)
            {
                delete (map_file_);
                map_file_ = nullptr;
            }
        }

       

        int MMapFileOperation::munmap_file()
        {
            if (this->is_mapped && this->map_file_ != nullptr)
            {
                delete (this->map_file_);
                this->is_mapped = false;
            }
            return TFS_SUCCESS;
        }

        int MMapFileOperation::mmap_file(const MMapOption &mmap_option)
        {
            if (mmap_option.max_mmap_size_ <= 0)
            {
                return TFS_ERROR;
            }
            if (mmap_option.max_mmap_size_ < mmap_option.first_mmap_size_)
                return TFS_ERROR;
            int fd = this->check_file();
            if (fd < 0)
            {
                fprintf(stderr, "MMapFileOpation::mmap_file checking faild ");
                return TFS_ERROR;
            }
            if (!is_mapped)
            {
                if (nullptr == this->map_file_)
                {
                    delete map_file_;
                }
                map_file_ = new MMapFile(mmap_option, fd);
                this->is_mapped = this->map_file_->mmapFile(true);
            }

            if (this->is_mapped)
            {
                return TFS_SUCCESS;
            }
            else
            {
                return TFS_ERROR;
            }
        }

        void *MMapFileOperation::get_map_data() const
        {

            if (this->is_mapped)
            {
                return this->map_file_->getData();
            }
            return nullptr;
        }

        int MMapFileOperation::pread_file(char *buf, const int32_t size, const int64_t offset)
        {
            // 如果读取不全可以考虑使用while
            // 内存已经映射
            if (this->is_mapped && (offset + size) > map_file_->getSize())
            {
                if (DEBUG)
                {
                    fprintf(stdout, "mmapFileOperation pread file, size: %d, offset: %" PRId64 ", map file size: %d. need remap\n", size, offset, map_file_->getSize());
                }
                this->map_file_->remapFile(); // 追加内存
            }
            if (is_mapped && (offset + size) <= this->map_file_->getSize())
            {
                memcpy(buf, (char *)map_file_->getData() + offset, size);

                return TFS_SUCCESS;
            }
            // 映射不全 内存没有映射或者是要读取的数据不全
            return FileOperation::pread_file(buf, size, offset);
        }

        int MMapFileOperation::pwrite_file(const char *buf, const int32_t size, const int64_t offset)
        {

            if (is_mapped && (offset + size) > this->map_file_->getSize())
            {
                if (DEBUG)
                {
                    fprintf(stdout, "mmapFileOperation pwrite file, size: %d, offset: %" PRId64 ", map file size: %d. need remap\n", size, offset, map_file_->getSize());
                }
                this->map_file_->remapFile(); // 追加内存
            }

            if (is_mapped && (offset + size) <= this->map_file_->getSize())
            {
               // memccpy((char*)map_file_->getData()+offset,buf,size);
                memcpy((char*)map_file_->getData()+offset,buf,size);
                return TFS_SUCCESS;
            }


            //内存没有映射or写入映射数据补全
            return FileOperation::pwrite_file(buf,size,offset);//or write
        }

        int MMapFileOperation::flush_file(){
            if(is_mapped){
                if(map_file_->syncFile())
                {
                    return TFS_SUCCESS;
                }else{
                    return TFS_ERROR;
                }
            }

          return int();
        }

    }

}
