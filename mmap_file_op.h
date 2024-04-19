#ifndef TFS_MMAP_FILE_OPERATION_H
#define TFS_MMAP_FILE_OPERATION_H

#include "head.h"
#include "file_op.h"
#include "mmap_file.h"

namespace lucifer
{
    namespace tfs
    {

        class MMapFileOperation : public FileOperation
        {
        public:
        
            MMapFileOperation(const std::string& file_name, int open_flags = O_RDWR|O_LARGEFILE) 
                : FileOperation(file_name, open_flags), map_file_(nullptr), is_mapped(false) {}

            ~MMapFileOperation();

            int mmap_file(const MMapOption &mmap_option);
            int munmap_file();
            int pread_file(char *buf, const int32_t size, const int64_t offset);
            int pwrite_file(const char *buf, const int32_t size, const int64_t offset);
           
            void *get_map_data() const;
            int flush_file();

        private:
            MMapFile *map_file_;
            bool is_mapped;
        };

    }
}

#endif //! TFS_MMAP_FILE_OPERATION_H