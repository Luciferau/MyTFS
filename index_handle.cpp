#include "index_handle.h"

namespace lucifer
{
    namespace tfs
    {
        IndexHandle::IndexHandle() {}

        IndexHandle::IndexHandle(const std::string &base_path, const int32_t &main_block_id)
        {

            // create file_op_ handle object
            std::stringstream tmp_stream;

            tmp_stream << base_path << INDEX_DIR_PREFIX << main_block_id;

            std::string index_path;
            tmp_stream >> index_path;

            file_op_ = new MMapFileOperation(index_path, O_CREAT | O_RDWR | O_LARGEFILE);

            this->is_load_ = false;
        }

        IndexHandle::~IndexHandle()
        {
            if (file_op_)
            {
                delete this->file_op_;
                this->file_op_ = nullptr;
            }
        }

        int IndexHandle::create(const int32_t logic_block_id, const int32_t bucket_size, const MMapOption map_option)
        {
            int ret;
            if (DEBUG)
            {
                printf("create index\n\tblock id: %u\n\tbucket size: %d\n\tmax_mmap_Size:%d\n\tper mmap size:%d\n\tfirst mmap size:%d\n\n",
                       logic_block_id, bucket_size, map_option.max_mmap_size_, map_option.per_mmap_size_, map_option.first_mmap_size_);
            }
            if (is_load_)
            {
                return EXIT_INDEX_ALREADY_LOADED_ERROR;
            }
            int64_t file_size = this->file_op_->get_file_size();
            if (file_size < 0)
            {
                return TFS_ERROR;
            }
            else if (file_size == 0)
            {
                IndexHeader i_header;

                i_header.block_info_.block_id_ = logic_block_id;
                i_header.block_info_.seq_no_ = 1;
                i_header.bucket_size_ = bucket_size;

                i_header.index_file_size_ = sizeof(IndexHandle) + bucket_size * sizeof(int32_t);
                // index_header + total buckets
                char *init_data = new char[i_header.index_file_size_];
                memcpy(init_data, &i_header, sizeof(IndexHeader));
                memset(init_data + sizeof(IndexHeader), 0, i_header.index_file_size_ - sizeof(IndexHeader));
                // write index header and buckets into index file

                ret = file_op_->pwrite_file(init_data, i_header.index_file_size_, 0);
                delete[] init_data;
                init_data = nullptr;

                if (ret != TFS_SUCCESS)
                {
                    return ret;
                }
                ret = file_op_->flush_file();
                if (ret != TFS_SUCCESS)
                {
                    return ret;
                }
            }
            else // file_size > 0 ,index already exist
            {
                return EXIT_META_UNEXPECT_FOUND_ERROR;
            }
            ret = this->file_op_->mmap_file(map_option);
            if (TFS_SUCCESS != ret)
            {
                return ret;
            }
            is_load_ = true;
            if (DEBUG)
            {

                printf("index successful\n\tinit blockid: %d\n\tdata file size: %d\n\tindex file size: %d\n\tbucket_size: %d\n\tfree head offset: %d\n\tseq no: %d\n\tsize: %d\n\tfilecount: %d\n\tdel size : %d\n\tdel_file_count: %d\n\tversion: %d\n\n",
                       logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_,
                       index_header()->bucket_size_, index_header()->free_head_offset_,
                       index_header()->block_info_.seq_no_, index_header()->block_info_.size_,
                       index_header()->block_info_.file_count_, index_header()->block_info_.del_size_,
                       index_header()->block_info_.del_file_count_, index_header()->block_info_.version_);
            }
            return TFS_SUCCESS;
        }
        IndexHeader *IndexHandle::index_header()
        {
            return reinterpret_cast<IndexHeader *>(this->file_op_->get_map_data());
        }

        BlockInfo *IndexHandle::block_info()
        {
            return reinterpret_cast<BlockInfo *>(this->file_op_->get_map_data());
        }

        int32_t IndexHandle::bucket_size() const
        {
            return reinterpret_cast<IndexHeader *>(this->file_op_->get_map_data())->bucket_size_;
        }

        int IndexHandle::load(const int32_t logic_block_id, const int32_t bucket_size,
                              const MMapOption map_option)
        {
            int ret = TFS_SUCCESS;

            if (is_load_)
            {
                return EXIT_INDEX_ALREADY_LOADED_ERROR;
            }

            int64_t file_size = this->file_op_->get_file_size();
            if (file_size < 0)
                return file_size;
            else if (file_size == 0)
            {
                printf("----------------file_size: %d\n", file_size);
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            MMapOption tmp_map_option = map_option;
            if (file_size > tmp_map_option.first_mmap_size_ && file_size <= tmp_map_option.max_mmap_size_)
            {
                tmp_map_option.first_mmap_size_ = file_size;
            }
            ret = this->file_op_->mmap_file(tmp_map_option);
            if (TFS_SUCCESS != ret)
            {
                return ret;
            }

            
            if (0 == this->bucket_size() || 0 == block_info()->block_id_)
            {
                if (DEBUG)
                    printf("bucket_size is 0 or block id is 0\n");
                fprintf(stderr, "index corrpurt ,block id: %u,bucket size: %d \n",
                        block_info()->block_id_, this->bucket_size());
                return EXIT_INDEX_CORRUPT_ERROR;
            }

            // check file size
            int32_t index_file_size = sizeof(IndexHeader) + this->bucket_size() * sizeof(int32_t);

            if (file_size < index_file_size)
            {
                fprintf(stderr, "index corrupt error,blockid: %u,\
                    this bucket size: %d,file size : %d,index file size : %d\n ",
                        block_info()->block_id_, this->bucket_size(), file_size, index_file_size);
                return EXIT_INDEX_CORRUPT_ERROR;
            }
            // check block id size
            if (logic_block_id != block_info()->block_id_)
            {
                fprintf(stderr, "block id confict : %u,index blockid : %u\n", logic_block_id, block_info()->block_id_);
                return EXIT_BLOCKID_CONFLICT_ERROR;
            }

            // check bucket size
            if (bucket_size != this->bucket_size())
            {

                fprintf(stderr, "Index configure error,old bucket size: %d,bew bucket size: %d\n", this->bucket_size(), bucket_size);
                return EXIT_BUCKET_CONFLICT_ERROR;
            }
            this->is_load_ = true;
            if (DEBUG)
            {

                printf("Load index handle successful \n\tload blockid:%d\n\tdata file size: %d\n\tindex file size: %d\n\tbucket_size: %d\n\tfree head offset: %d\n\tseq no: %d\n\tsize: %d\n\tfilecount: %d\n\tdel size : %d\n\tdel_file_count: %d\n\tversion: %d\n\n",
                       logic_block_id, index_header()->data_file_offset_, index_header()->index_file_size_,
                       index_header()->bucket_size_, index_header()->free_head_offset_,
                       index_header()->block_info_.seq_no_, index_header()->block_info_.size_,
                       index_header()->block_info_.file_count_, index_header()->block_info_.del_size_,
                       index_header()->block_info_.del_file_count_, index_header()->block_info_.version_);
            }

            return TFS_SUCCESS;
        }

        int IndexHandle::remove(const uint32_t logic_block_id)
        {
            if (is_load_)
            {
                if (logic_block_id != block_info()->block_id_)
                {
                    fprintf(stderr, "block id confict : %u,index blockid : %u\n", logic_block_id, block_info()->block_id_);
                    return EXIT_BLOCKID_CONFLICT_ERROR;
                }
            }
            auto ret = file_op_->munmap_file();

            if (TFS_SUCCESS != ret)
            {
                return ret;
            }

            ret = file_op_->unlink_file();
            return TFS_SUCCESS;
        }

        int IndexHandle::flush()
        {
            int ret = this->file_op_->flush_file();
            if (TFS_SUCCESS != ret)
            {
                fprintf(stderr, "index flush fail,ret:%d error desc:%s\n", ret, strerror(ret));
            }
            return ret;
        }

        int32_t IndexHandle::get_block_data_offset()
        {
            return reinterpret_cast<IndexHeader *>(file_op_->get_map_data())->data_file_offset_;
        }
           int32_t IndexHandle::free_head_offset() const
        {
            return reinterpret_cast<IndexHeader *>(file_op_->get_map_data())->free_head_offset_;
        }


        int32_t IndexHandle::write_segment_meta(const uint64_t key, MetaInfo &meta)
        {

            int32_t current_offset = 0;  // 查找的偏移
            int32_t previous_offset = 0; // 查找的偏移
            // key exist or not
            // find from file hashTbale is exist  (hash_find (key,current_offset))

            int ret = hash_find(key, current_offset, previous_offset);

            if (TFS_SUCCESS == ret)
            {
                return EXIT_META_UNEXPECT_FOUND_ERROR;
            }
            else if (EXIT_MEFA_NOT_FOUND_ERROR != ret)
            {
                return ret;
            }

            // if not exist ,then write the meta to file (hash insert(meta,slot))
            ret = hash_insert(key, previous_offset, meta);
            return ret;
        }

        int32_t *IndexHandle::bucket_slot()
        {
            return reinterpret_cast<int32_t *>(reinterpret_cast<char *>(file_op_->get_map_data()) + sizeof(IndexHeader));
        }

        int32_t IndexHandle::hash_find(const uint64_t key, int32_t &current_offset, int32_t &previous_offset)
        {
            int ret = TFS_SUCCESS;

            // ensure the key slot
            previous_offset = 0;
            current_offset = 0;

            MetaInfo meta_info;
            // 1. 确定key存放的桶（slot）的位置
            int32_t slot = static_cast<uint32_t>(key) % this->bucket_size();
            // 2. 读取桶的首节点存储的第一个节点的偏移量  如果偏移为0 ：EXIT_MEFA_NOT_FOUND_ERROR
            // 3. 根据偏移量读取存储的meta info
            // 4. 与key进行比较，相等就设置啧设置（——curoffset pre_offset 并且返回TFS_SUCCESS,否则执行5
            // 5. 从mefainfo取得下一个节点在文件中间的偏移量 如果偏移为0 ：EXIT_MEFA_NOT_FOUND_ERROR

            int32_t pos = this->bucket_slot()[slot];
            for (; pos != 0;)
            {
                ret = file_op_->pread_file(reinterpret_cast<char *>(&meta_info),
                                           sizeof(MetaInfo), pos);
                if (TFS_SUCCESS != ret)
                {
                    return ret;
                }

                if (hash_compatre(key, meta_info.get_key()))
                {
                    current_offset = pos;
                    return TFS_SUCCESS;
                }
                previous_offset = pos;
                pos = meta_info.get_next_meta_offset();
            }
            return EXIT_MEFA_NOT_FOUND_ERROR;
        }

        int32_t IndexHandle::hash_insert(const uint64_t key, int32_t previous_offset, MetaInfo &meta)
        {
            MetaInfo tmp_meta_info;
            int32_t ret = TFS_SUCCESS;
            int32_t current_offset = 0;
            // 1 确定key存放的slot的位置
            int32_t slot = static_cast<uint32_t>(key) % this->bucket_size();

            // 2 确定meta 节点存储在文件中的存放当前节点的偏移量

            if(free_head_offset()!= 0){
                ret = file_op_->pread_file(reinterpret_cast<char *>(&tmp_meta_info),sizeof(MetaInfo),free_head_offset());
                
                if(TFS_SUCCESS != ret)  return ret;

                current_offset = index_header()->free_head_offset_;
                index_header()->free_head_offset_ = tmp_meta_info.get_next_meta_offset();
                
                if(DEBUG)
                    printf("reuse metainfo ,current offset: %d\n",current_offset);
            }else{
               
                    
                current_offset   = index_header()->index_file_size_;
                index_header()->index_file_size_ += sizeof(MetaInfo);
            }

         

            // 3. 将metainfo写入索引文件中
            meta.set_inner_offset(0);
            ret = file_op_->pwrite_file(reinterpret_cast<const char *>(&meta),
                                        sizeof(MetaInfo), current_offset);
            if (TFS_SUCCESS != ret)
            {
                index_header()->index_file_size_ -= sizeof(MetaInfo);
                return ret;
            }
            // 4. 将meta节点插入哈希链表
           
            if (0 != previous_offset)
            {
                ret = file_op_->pread_file(reinterpret_cast<char *>(&tmp_meta_info), sizeof(MetaInfo), previous_offset);
                if (TFS_SUCCESS != ret)
                {
                    index_header()->index_file_size_ -= sizeof(MetaInfo);
                    return ret;
                }
                tmp_meta_info.set_next_meta_offset(current_offset);

                ret = file_op_->pwrite_file(reinterpret_cast<const char *>(&tmp_meta_info),
                                            sizeof(MetaInfo), current_offset);

                if (TFS_SUCCESS != ret)
                {
                    index_header()->index_file_size_ -= sizeof(MetaInfo);
                    return ret;
                }
            }

            else
            {
                bucket_slot()[slot] = current_offset;
            }

            return TFS_SUCCESS;
        }

        void IndexHandle::commit_block_data_offset(const int file_size)
        {
            reinterpret_cast<IndexHeader *>(this->file_op_->get_map_data())
                ->data_file_offset_ += file_size;
        }

        bool IndexHandle::update_block_info(const OperType oper_type, const uint32_t modify_size)
        {
            if (block_info()->block_id_ == 0)
                return EXIT_BLOCKID_ZERO_ERROR;
            if (oper_type == C_OPER_INSERT)
            {
                ++block_info()->version_;
                ++block_info()->file_count_;
                ++block_info()->seq_no_;
                block_info()->size_ += modify_size;
            }
            else if (oper_type == C_OPER_DELETE)
            {
                ++block_info()->version_;
                --block_info()->file_count_;
                block_info()->size_ -= modify_size;
                ++block_info()->del_file_count_;
                block_info()->del_size_ += modify_size;
            }
            if (DEBUG)
            {
                printf("\n\nupdate  block   info\n\tblockid: %u\n\tversion: %u\n\tfile_count: %u\n\tseq_no: %u\n\tsize: %u\n\tdel_size: %u\n\tdel_file_count: %u\n\n",
                       block_info()->block_id_, block_info()->version_, block_info()->file_count_, block_info()->seq_no_,
                       block_info()->size_, block_info()->del_size_, block_info()->del_file_count_);

                printf("type:%d,modify_size:%d\n", oper_type, modify_size);
            }
            return TFS_SUCCESS;
        }

        int32_t IndexHandle::read_segment_meta(const uint64_t key, MetaInfo &meta)
        {
            int32_t current_offset = 0;
            int32_t previous_offset = 0;

            auto ret = hash_find(key, current_offset, previous_offset);

            if (tfs::TFS_SUCCESS == ret)
            {
                ret = file_op_->pread_file(reinterpret_cast<char *>(&meta), sizeof(MetaInfo), current_offset);
                return ret;
            }
            else
            {
                if (tfs::DEBUG)
                    printf("read segment read not except?\n");
                return ret;
            }
        }

        int32_t IndexHandle::delete_segment_meta(const uint64_t key)
        {

            int32_t current_offset = 0;
            int32_t previous_offset = 0;

            int32_t ret = hash_find(key, current_offset, previous_offset);
            if (TFS_SUCCESS != ret)
                return ret;

            MetaInfo meta_info;
            ret = file_op_->pread_file(reinterpret_cast<char *>(&meta_info), sizeof(MetaInfo), current_offset);
            if (ret != TFS_SUCCESS)
                return ret;
            int32_t next_pos = meta_info.get_next_meta_offset();

            // 为首节点
            if (previous_offset == 0)
            {
                int32_t slot = static_cast<uint32_t>(key) % this->bucket_size();
                this->bucket_slot()[slot] = next_pos;
            }
            else
            { // 普通节点

                MetaInfo pre_meta_info;
                ret = file_op_->pread_file(reinterpret_cast<char *>(&pre_meta_info), sizeof(MetaInfo), previous_offset);
                if (ret != TFS_SUCCESS)
                    return ret;
                
                pre_meta_info.set_next_meta_offset(next_pos);
                ret = file_op_->pwrite_file(reinterpret_cast<const char *>(&pre_meta_info), sizeof(MetaInfo), previous_offset);
                
                if (ret != TFS_SUCCESS)
                    return ret;
            }

            // 加入可重用节点链表
            
            meta_info.set_next_meta_offset(free_head_offset());//index_header()->free_head_offset
            if(DEBUG){
                printf("delete_segment_meta- reuse meta_info,current_offset: %d\n",current_offset);

            }
            ret = file_op_->pwrite_file(reinterpret_cast<const char *>(&meta_info), sizeof(MetaInfo),current_offset);
            
            if(TFS_SUCCESS != ret) return ret;
            
            index_header()->free_head_offset_ = current_offset;
            update_block_info(C_OPER_DELETE, meta_info.get_size());

            return TFS_SUCCESS;
        }


     
    }

}
