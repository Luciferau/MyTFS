#ifndef TFS_FILE_OP_H_
#define TFS_FILE_OP_H_

#include "head.h"

namespace lucifer
{

    namespace tfs
    {

        class FileOperation
        {
        public:
            FileOperation();
            FileOperation(const std::string &file_name, const int open_flags = O_RDWR | O_LARGEFILE);
            virtual ~FileOperation();

            void close_file();
            int open_file();
            int flush_file(); // write thre file to the disk
            int unlink_file();
            virtual int pread_file(char *buf, const int32_t nbytes, const int64_t offset);
            virtual int pwrite_file(const char *buf, const int32_t nbytes, const int64_t offset);

            virtual int write_file(const char *buf, const int32_t nbytes);
            virtual int read_file( char *buf, const int32_t nbytes);
            int64_t get_file_size();

            int ftruncate_file(const int64_t length);
            int seek_file(const int64_t offset);
            
            int get_fd()const;

        protected:
            int fd_;
            int open_flags_;
            char* file_name_;
            int64_t file_size_;
            int disk_times_;
        protected:
            static constexpr mode_t OPEN_MODE = 0644;
            static constexpr int MAX_DISK_TIMES = 5;
        protected:
            int check_file();
        };

    }
}

#endif // TFS_FILE_OP_H_