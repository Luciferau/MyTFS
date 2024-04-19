#include "mmap_file.h"

static bool debug = true;

namespace lucifer
{

    namespace tfs
    {

        MMapFile::MMapFile() : size_(0), data_(nullptr), fd_(-1) {}

        MMapFile::MMapFile(const int fd) : size_(0), data_(nullptr), fd_(fd) {}

        MMapFile::MMapFile(const MMapOption &mmap_option, const int fd) : size_(0), data_(nullptr), fd_(fd)
        {

            this->mmap_file_option_.max_mmap_size_ = mmap_option.max_mmap_size_;
            this->mmap_file_option_.per_mmap_size_ = mmap_option.per_mmap_size_;
            this->mmap_file_option_.first_mmap_size_ = mmap_option.first_mmap_size_;
        }

        MMapFile::~MMapFile()
        {
            if (this->data_)
            {
                if (debug)
                {
                    printf("mmap file destruct,fd:%d,maped size:%d,data%p\n", this->fd_, this->size_, this->data_);
                }

                msync(this->data_, this->size_, MS_SYNC);

                munmap(this->data_, this->size_);

                this->size_ = 0;
                this->data_ = nullptr;
                this->fd_ = -1;

                this->mmap_file_option_.first_mmap_size_ = 0;
                this->mmap_file_option_.max_mmap_size_ = 0;
                this->mmap_file_option_.per_mmap_size_ = 0;

                // delete[] this->data_;
            }
        }

        bool MMapFile::syncFile()
        {
            if (this->data_ != nullptr && this->size_ > 0)
            {
                return msync(this->data_, this->size_, MS_ASYNC) == 0;
            }
            return true;
        }

        bool MMapFile::mmapFile(const bool auth_write)
        {
            int flags = PROT_READ;
            if (auth_write)
            {
                flags |= PROT_WRITE;
            }
            if (this->fd_ < 0)
                return false;

            if (mmap_file_option_.max_mmap_size_ == 0)
                return false;
            if (this->size_ < this->mmap_file_option_.first_mmap_size_)
                this->size_ = this->mmap_file_option_.first_mmap_size_;
            else
                this->size_ = this->mmap_file_option_.max_mmap_size_;
            if (!this->ensureFileSize(size_))
            {
                fprintf(stderr, "ensureFileSize failed in mmapFile ,reason%s\n", strerror(errno));
                return false;
            }

            this->data_ = mmap(0, this->size_, flags, MAP_SHARED, this->fd_, 0);

            if (this->data_ == MAP_FAILED)
            {

                fprintf(stderr, "mmap file failed, resason%s fd:%d,maped size:%d,data%p\n", strerror(errno), this->fd_, this->size_, this->data_);
                this->size_ = 0;
                this->data_ = nullptr;
                this->fd_ = -1;

                return false;

                if (debug)
                    printf("mmap file success,fd:%d,maped size:%d,data%p\n", this->fd_, this->size_, this->data_);

                return true;
            }
            return true;
        }

        void *MMapFile::getData() const { return this->data_; }
        int32_t MMapFile::getSize() const { return this->size_; }

        bool MMapFile::mumapFile()
        {
            if (munmap(this->data_, this->size_) == 0)
                return true;
            else
                return false;
        }

        bool MMapFile::ensureFileSize(const int32_t size)
        {
            struct stat st;
            if (fstat(this->fd_, &st) < 0)
            {
                fprintf(stderr, "fstat erro  ,reason%s\n", strerror(errno));
                return false;
            }
            if (st.st_size < size)
            {
                if (ftruncate(this->fd_, size) < 0)
                {
                    fprintf(stderr, "ensureFileSize failed,reason%s\n", strerror(errno));
                    return false;
                }
                return true;
            }
            // 添加一个默认的返回值，或者修改函数返回类型为void
            return true;
        }

        bool MMapFile::remapFile(const bool auth_write)
        {

            if (this->fd_ < 0 || this->size_ == 0 || this->data_ == nullptr)
            {
                fprintf(stderr, "mremap not mapped yet\n");
                return false;
            }

            if (this->size_ == this->mmap_file_option_.max_mmap_size_)
            {
                if (debug)
                    printf("remap file failed,fd:%d,maped size:%d,data%p\n", this->fd_, this->size_, this->data_);
                fprintf(stderr, "alreadly mapped max size,now size:%d,max size :%d\n", this->size_, this->mmap_file_option_.max_mmap_size_);
                return false;
            }
            int32_t new_size = this->size_ + this->mmap_file_option_.per_mmap_size_;
            // if(new_size > this->mmap_file_option_.max_mmap_size_) new_size= this->mmap_file_option_.max_mmap_size_;
            new_size = (new_size > this->mmap_file_option_.max_mmap_size_) ? this->mmap_file_option_.max_mmap_size_ : new_size;

            if (!this->ensureFileSize(new_size))
            {
                fprintf(stderr, "ensureFileSize failed in remapFile ,reason%s\n", strerror(errno));
                return false;
            }
         
            void *new_map_data = mremap(this->data_, this->size_, new_size, MREMAP_MAYMOVE, auth_write ? PROT_WRITE : PROT_READ);

               if (debug)
                printf("remap file,fd:%d,maped size:%d,new size:%d old data :%p new data %p\n", this->fd_, this->size_, new_size,this->data_,new_map_data);

            if (MAP_FAILED == new_map_data)
            {
                fprintf(stderr, "mremap failed new_map_data is MAP_FAILD,reason%s new size :%d fd:%d \n", strerror(errno), new_size, this->fd_);
                return false;
            }
            else
            {

                if (debug)
                    printf("mremap success new_map_data:%p,new size:%d\n", new_map_data, new_size);
            }
            this->data_ = new_map_data;
            this->size_ = new_size;
            return true;
        }

    }
}