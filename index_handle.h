#ifndef TFS_INDEX_HANDLE_H_
#define TFS_INDEX_HANDLE_H_

#include "head.h"
#include "mmap_file_op.h"
#include "mmap_file.h"

namespace lucifer
{

    namespace tfs
    {

        struct IndexHeader
        {

        public:
            BlockInfo   block_info_;        // meta block info
            int32_t     bucket_size_;       // hash bucket size
            int32_t     data_file_offset_;  // offset to wirte next data in block
            int32_t     index_file_size_;   // offset  after index+header + all buckets
            int32_t     free_head_offset_;  // free meta node list for reuse

            IndexHeader() { memset(this, 0, sizeof(IndexHeader)); }


        };

        class IndexHandle
        {
        public:

            IndexHandle(const std::string& base_path, const int32_t& main_block_id);
            IndexHandle();
            virtual ~IndexHandle();

            int             create(const int32_t logic_block_id,const int32_t bucket_size,const MMapOption map_option);
            int             load  (const int32_t logic_block_id,const int32_t bucket_size,const MMapOption map_option);
            int             remove(const uint32_t logic_block_id);
            int             flush ();
            void            commit_block_data_offset(const int file_size);
            bool            update_block_info(const OperType oper_type,const uint32_t modify_size);
            
            int32_t         bucket_size()const;
            int32_t         get_block_data_offset();
            int32_t         write_segment_meta(const uint64_t key,MetaInfo &meta);
            int32_t         read_segment_meta(const uint64_t key,MetaInfo &meta);
            int32_t         hash_find(const uint64_t key,int32_t &current_offset,int32_t &previous_offset);
            int32_t*        bucket_slot();
            int32_t         hash_insert(const uint64_t key,int32_t previous_offset,MetaInfo &meta);
            int32_t         delete_segment_meta(const uint64_t key);
            int32_t         free_head_offset()const;

            //整理快文件
            int32_t         space_reclamation(FileOperation*);
            
            IndexHeader*    index_header();    
            BlockInfo*      block_info();
        private:
            inline bool hash_compatre(const uint64_t left_key,const uint64_t right_key)
                {return (left_key == right_key);}
        
        private:
            MMapFileOperation*  file_op_;
            bool                is_load_;
            
        
        };

    }
}

#endif // TFS_INDEX_HANDLE_H_