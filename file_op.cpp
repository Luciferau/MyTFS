#include "file_op.h"
#include "head.h"

namespace lucifer
{

    namespace tfs
    {

        FileOperation::FileOperation() {}
        FileOperation::FileOperation(const std::string &file_name, const int open_flags)
            : fd_(-1), open_flags_(open_flags)
        {
            this->file_name_ = strdup(file_name.c_str());
        }
        FileOperation::~FileOperation()
        {
            if (this->fd_ > 0)
                ::close(this->fd_);
            if (nullptr != file_name_)
            {
                free(file_name_);
                file_name_ = nullptr;
            }
        }

        int FileOperation::open_file()
        {
            if (this->fd_ > 0)
            {
                close(fd_);
            }

            this->fd_ = ::open(this->file_name_, this->open_flags_, OPEN_MODE);

            if (this->fd_ < 0)
            {
                return -errno;
            }
            return fd_;
        }

        void FileOperation::close_file()
        {
            if (this->fd_ > 0)
            {
                ::close(this->fd_);
                this->fd_ = -1;
            }
            return;
        }

        int64_t FileOperation::get_file_size()
        {
            int fd = this->check_file();
            if (fd < 0)
                return -1;
            struct stat st;
            if (fstat(fd, &st) < 0)
                return -1;
            return st.st_size;
        }

        int FileOperation::check_file()
        {
            if (this->fd_ < 0)
            {
                this->fd_ = this->open_file();
            }

            return this->fd_;
        }

        int FileOperation::ftruncate_file(const int64_t length)
        {
            int fd = this->check_file();
            if (fd < 0)
                return -1;
            return ftruncate(fd, length);
        }
        // move
        int FileOperation::seek_file(const int64_t offset)
        {
            int fd = this->check_file();
            if (fd < 0)
                return -1;
            return lseek(fd, offset, SEEK_SET);
        }
        int FileOperation::flush_file()
        {
            if (this->open_flags_ & O_SYNC)
            {
                return 0;
            }
            int fd = this->check_file();
            if (fd < 0)
                return -1;
            return fsync(fd);
        }

        int FileOperation::unlink_file()
        {
            if (this->fd_ > 0)
            {
                this->close_file();
                this->fd_ = -1;
            }
            return ::unlink(this->file_name_);
        }

        int FileOperation::get_fd() const
        {
            return this->fd_;
        }

        int FileOperation::pread_file(char *buf, const int32_t nbytes, const int64_t offset)
        {
            int fd = this->check_file();
            if (fd < 0)
                return -1;

            int32_t left = nbytes; // 剩余字节
            int64_t read_offset = offset;
            int32_t read_len = 0;
            char *p_tmp = buf;
            int i = 0;
            while (left > 0)
            {
                ++i;
                if (i >= MAX_DISK_TIMES)
                    return -1;
                if (this->check_file() < 0)
                    return -errno;
                read_len = ::pread64(fd, p_tmp, left, read_offset);
                if (read_len < 0)
                {
                    read_len = -errno;
                    if (read_len == -EINTR || -EAGAIN == read_len)
                        continue;
                    else if (-EBADF == read_len)
                    {
                        // fd_ = -1;
                        return read_len;
                    }
                    else
                    {

                        return read_len;
                    }
                }
                else if (0 == read_len)
                {
                    break;
                }

                left -= read_len;
                p_tmp += read_len;
                read_offset += read_len;
            }
            if (0 != left)
            {
                return EXIT_DISK_OPER_INCOMPLETE;
            }
            return TFS_SUCCESS;
        }

        int FileOperation::pwrite_file(const char *buf, const int32_t nbytes, const int64_t offset)
        {
            int fd = this->check_file();
            if (fd < 0)
            {
                return -1;
            }

            int32_t left = nbytes; // 剩余字节
            int64_t write_offset = offset;
            int32_t write_len = 0;
            const char *p_tmp = buf;
            int i = 0;
            while (left > 0)
            {
                ++i;
                if (i >= MAX_DISK_TIMES)
                    return -1;
                write_len = ::pwrite64(fd, p_tmp, left, write_offset);
                if (write_len < 0)
                {
                    write_len = -errno;
                    if (-write_len == EINTR || EAGAIN == -write_len)
                        continue;
                    else if (EBADF == -write_len)
                    {
                        // fd_ = -1;
                        continue;
                    }
                    else
                    {
                        return write_len;
                    }
                }

                left -= write_len;
                p_tmp += write_len;
                write_offset += write_len;
            }
            if (0 == left)
                return TFS_SUCCESS;

            return EXIT_DISK_OPER_INCOMPLETE;
        }

        int FileOperation::write_file(const char *buf, const int32_t nbytes)
        {
            int fd = this->check_file();
            if (fd < 0)
                return -1;
            int32_t left = nbytes; // 剩余字节
            const char *p_tmp = buf;
            int i = 0;
            while (left > 0)
            {
                ++i;
                if (i >= MAX_DISK_TIMES)
                    return -1;
                int32_t write_len = ::write(fd, p_tmp, left);
                if (write_len < 0)
                {
                    write_len = -errno;
                    if (-write_len == EINTR || EAGAIN == -write_len)
                        continue;
                    else if (EBADF == -write_len)
                    {
                        // this->fd_ = -1; // 使用 fd 替换 fd_
                        continue;
                    }
                    else
                    {
                        return write_len;
                    }
                }

                left -= write_len;
                p_tmp += write_len;
            }
            if (0 == left)
            {
                return TFS_SUCCESS;
            }
            return EXIT_DISK_OPER_INCOMPLETE;
        }

        int FileOperation::read_file(char *buf, const int32_t nbytes)
        {
            int fd = this->check_file();
            if (fd < 0)
            {
                if (DEBUG)
                {
                    std::cout << "fd < 0" << std::endl;
                }
                return -1;
            }

            char *p_tmp = buf;
            int32_t left = nbytes;
            int i = 0;
            while (left > 0)
            {
                ++i;
                if (i >= MAX_DISK_TIMES)
                    return -1;
                int32_t read_len = ::read(fd, p_tmp, left);
                std::cout << "read_len: " << read_len << std::endl;
                if (read_len < 0)
                {
                    read_len = -errno;
                    if (-read_len == EINTR || EAGAIN == -read_len)
                        continue;
                    else if (EBADF == -read_len)
                    {
                        if (DEBUG)
                        {
                            std::cout << "is EBADF" << std::endl;
                        }
                        return -1; // 直接返回错误码
                    }
                    else
                    {
                        if (DEBUG)
                        {
                            std::cout << "is else" << std::endl;
                        }
                        return read_len;
                    }
                }
                else if (read_len == 0)
                {
                    // 文件读取结束
                    break;
                }
                left -= read_len;
                p_tmp += read_len;
            }
            std::cout<<"The left is "<<left<<std::endl;
            // 检查剩余字节数
            if (left != 0)
            {
                if (DEBUG)
                {
                    std::cout << "left != 0" << std::endl;
                }
                return EXIT_DISK_OPER_INCOMPLETE;
            }

            return TFS_SUCCESS;
        }

    }
    
}