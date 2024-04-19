
#ifndef HEAD_H_INCLUDED
#define HEAD_H_INCLUDED

#include <cstdint>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>
#include <iostream>
#include <string>
#include <cstring>
#include <cassert>
#include <sstream>
 

namespace lucifer
{
    namespace tfs
    {

        // read or write length is less than required
        constexpr   const       int32_t         EXIT_DISK_OPER_INCOMPLETE                           =           -8012;
        constexpr   const       int32_t         TFS_SUCCESS                                         =           0;
        constexpr   const       int32_t         UNKNOWN_OPERATION                                   =           -8013;
        constexpr   const       bool            DEBUG                                               =           true;
        constexpr   const       int32_t         TFS_ERROR                                           =           -8014;
        static      const       std::string     MAINLOCK_DIR_PREFIX                                 =           "/mainblock/";
        static      const       std::string     INDEX_DIR_PREFIX                                    =           "/index/";
        static      const       mode_t          DIR_MODE                                            =           0755;
        constexpr   const       int32_t         EXIT_INDEX_ALREADY_LOADED_ERROR                     =           -8015;
        constexpr   const       int32_t         EXIT_META_UNEXPECT_FOUND_ERROR                      =           -8016;
        constexpr   const       int32_t         EXIT_INDEX_CORRUPT_ERROR                            =           -8017;
        constexpr   const       int32_t         EXIT_BLOCKID_CONFLICT_ERROR                         =           -8018;    
        constexpr   const       int32_t         EXIT_BUCKET_CONFLICT_ERROR                          =           -8019;      
        constexpr   const       int32_t         EXIT_MEFA_NOT_FOUND_ERROR                           =           -8020;  
        constexpr   const       int32_t         EXIT_BLOCKID_ZERO_ERROR                             =           -8021;
        constexpr   const       int32_t         UNKNOWN_ERROR                                       =           -8022;
        constexpr   const       int32_t         EXIT_BLOCK_NOT_EXIST                                =           -8023;
        constexpr   const       int32_t         EXIT_BLOCK_DEL_FILE_COUNT_LESSZERO                  =           -8024;
        constexpr   const       int32_t         EXIT_BLOCK_DEL_SIZE_LESSZEOR                        =           -8025;
        constexpr   const       int32_t         EXIT_META_NOT_FOUND_ERROR                           =           -8026;
        constexpr   const       int32_t         MAINBLOCK_DIR_PREFIX                                =           -8027;
        enum OperType{
            C_OPER_INSERT = 1,
            C_OPER_DELETE
        };

        struct MMapOption
        {

            int32_t max_mmap_size_;
            int32_t first_mmap_size_;
            int32_t per_mmap_size_;
        };

        struct BlockInfo
        {
            uint32_t block_id_;
            int32_t version_;
            int32_t file_count_;
            int32_t size_;
            int32_t del_file_count_;
            int32_t del_size_;
            uint32_t seq_no_;
            BlockInfo()
            {
                memset(this, 0, sizeof(BlockInfo));
            }
            inline bool operator==(const BlockInfo &b)
            {
                return (block_id_ == b.block_id_) && (version_ == b.version_) && (file_count_ == b.file_count_) && (size_ == b.size_) && (del_file_count_ == b.del_file_count_) && (del_size_ == b.del_size_) && (seq_no_ == b.seq_no_);
            }
            inline BlockInfo*      block_info(){return this;}
            
        };

        struct MetaInfo
        {
        private:
            uint64_t fileid_;
            struct  
            {
                int32_t inner_offset_;
                int32_t size_;
            } location_;

            int32_t next_meta_offset_;

        public:
            MetaInfo()
            {
                init();
            }
            MetaInfo(const uint64_t fileid, const int32_t inner_offset, const int32_t size,
                     const int32_t next_meta_offset)
            {

                fileid_ = fileid;
                location_.inner_offset_ = inner_offset;
                location_.size_ = size;
                this->next_meta_offset_ = next_meta_offset;
            }

            MetaInfo(const MetaInfo &other)
            {
                // 使用内存拷贝赋值
                memcpy(this, &other, sizeof(MetaInfo));
            }

            uint64_t get_key() const { return fileid_; }
            void set_key(const uint64_t key) { fileid_ = key; }
            uint64_t get_file_id() const { return fileid_; }
            void set_file_id(const uint64_t file_id) { fileid_ = file_id; }

            int32_t get_inner_offset() const { return location_.inner_offset_; }
            void set_inner_offset(const int32_t inner_offset) { location_.inner_offset_ = inner_offset; }

            int32_t get_size() const { return location_.size_; }
            void set_size(const int32_t file_size) { location_.size_ = file_size; }

            int32_t get_next_meta_offset() const { return next_meta_offset_; }
            void set_next_meta_offset(const int32_t next_meta_offset) { next_meta_offset_ = next_meta_offset; }

            MetaInfo &operator=(const MetaInfo &other)
            {
                if (this == &other)
                {
                    return *this;
                }

                fileid_ = other.fileid_;
                location_.inner_offset_ = other.location_.inner_offset_;
                location_.size_ = other.location_.size_;
                next_meta_offset_ = other.next_meta_offset_;
            }
            MetaInfo &clone(const MetaInfo &other)
            {
                assert(this != &other);

                fileid_ = other.fileid_;
                location_.inner_offset_ = other.location_.inner_offset_;
                location_.size_ = other.location_.size_;
                next_meta_offset_ = other.next_meta_offset_;
                return *this;
            }

            bool operator==(const MetaInfo &other) const
            {
                return (fileid_ == other.fileid_) && (location_.inner_offset_ == other.location_.inner_offset_) && (location_.size_ == other.location_.size_) && (next_meta_offset_ == other.next_meta_offset_);
            }

        private:
            void init()
            {
                /* fileid_ = 0;
                 location_.inner_offset_ = 0;
                 location_.size_ = 0;
                 next_meta_offset_ = 0;*/
                memset(this, 0, sizeof(MetaInfo));
            }
        };


    }
}

#endif // HEAD_H_INCLUDED