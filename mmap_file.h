#ifndef TFS_MMAP_FILE_H
#define TFS_MMAP_FILE_H

#include "head.h"

#include <cerrno>
#include <cstdio>
#include <string.h>

namespace lucifer
{

    namespace tfs
    {

        //TODO
        enum Authority
        {
            READ_ONLY,
            WRITE_ONLY,
            READ_WRITE,
            DEFAULT,
        };

        class MMapFile

        {
        public:
            MMapFile();
            explicit MMapFile(const int fd);
            // explicit MMapFile(const char* path,const MMapOption &option);
            explicit MMapFile(const char *path);
            MMapFile(const MMapOption &option, const int fd);
            virtual ~MMapFile();
            bool syncFile();                               // sync th file
            bool mmapFile(const bool auth_write = false);  // Map files to memory and set the write authority with false
            void *getData() const;                         // Get the first address mapped to memory to data
            int32_t getSize() const;                       // Obtain the size of the mapping data
            bool remapFile(const bool auth_write = false); // REMap files to memory and set the write authority with false
            bool mumapFile();                              // unmap the file

        protected:
            bool ensureFileSize(const int32_t size);

        protected:

            void *data_ = nullptr;
            int32_t size_;
            int32_t fd_;
            Authority auth_ = Authority::DEFAULT;
            struct MMapOption mmap_file_option_;
            
        };

    }

}

#endif // !TFS_MMAP_FILE_H

// this file is mmap the file