#include "head.h"
#include "mmap_file.h"
#include "mmap_file_op.h"
#include "index_handle.h"

namespace lucifer
{
    namespace tfs

    {
        
        int32_t IndexHandle::space_reclamation(FileOperation* file_op ){

            //查找del_file_count
            //根据文件编号 逐步从头部开始写 hash_find就continue 有就往前写
            //截断文件
            //更新索引信息
            if(!file_op)
                return EXIT_BLOCK_NOT_EXIST;

            if(block_info()->del_file_count_ <= 0)        //块删除文件数量小于0
			{
				fprintf(stderr, "block id %u do not have del_file. del_file_count: %d\n", block_info()->block_id_, block_info()->del_file_count_);
				return EXIT_BLOCK_DEL_FILE_COUNT_LESSZERO;
			}
            	if(block_info()->del_size_ <= 0)             //块删除文件大小小于0
			{
				fprintf(stderr, "block id %u do not have del_file_size. del_file_size:%d\n", block_info()->block_id_, block_info()->del_size_);
				return EXIT_BLOCK_DEL_SIZE_LESSZEOR;
			}
			
            int32_t file_count = block_info()->file_count_;        //文件数量
			int32_t ret = TFS_SUCCESS;
			int32_t over_write_offset = 0;       //整个文件写入块后的偏移量
			int32_t current_write_offset = 0;    //文件未写全，块中的偏移量
		
            int64_t residue_bytes = 0;          //写入后还剩下需要写的字节数
			uint64_t key = 1;         //保存文件编号
			
            			//整理块
			for(int i = 1; i <= file_count; )
			{
				MetaInfo meta_info;            //保存临时读到的metainfo
				char buffer[4096] = { '0' };                 //保存的文件
				int nbytes = sizeof(buffer);        //该次需要写入的字节数
				
				ret = read_segment_meta(key, meta_info);
				
				current_write_offset = meta_info.get_inner_offset();   
				residue_bytes = meta_info.get_size(); 
			
				if(DEBUG) fprintf(stderr, "i: %d, file_id: %ld, key: %ld, ret: %d\n", i, meta_info.get_key(), key, ret);
				
				if(TFS_SUCCESS == ret)           //已经在哈希链表中读到
				{
					
					if(meta_info.get_size() <= sizeof(buffer))        //一次读完
					{	
						ret = file_op->pread_file(buffer, meta_info.get_size(), meta_info.get_inner_offset());      
						if(ret == TFS_SUCCESS)    //文件读成功,将文件重新写入块中
						{
							ret = file_op->pwrite_file(buffer, meta_info.get_size(), over_write_offset);
							if(ret == TFS_SUCCESS)          //文件写入成功
							{
								over_write_offset += meta_info.get_size();
								key++;
							}
							else         //文件未写成功 / 未写全
							{
								return ret;           //可以考虑将读取文件的地址传回(buffer的地址)
							}
						}
						else           //文件未读成功 / 未读全
						{
							return ret;
						}
					}
					else         //需要分多次读写
					{
						nbytes = sizeof(buffer);
						
						for(int j = 0; j < 1; )
						{
							ret = file_op->pread_file(buffer, nbytes, current_write_offset);     
							if(ret == TFS_SUCCESS)    		//文件读成功,将部分文件重新写入块中
							{
								//fprintf(stderr, "nbytes：%d\n", nbytes);
								ret = file_op->pwrite_file(buffer, nbytes, over_write_offset);
								if(ret == TFS_SUCCESS)          //文件写入成功
								{
									current_write_offset += nbytes;
									over_write_offset += nbytes;
									residue_bytes -= nbytes;
									
									//fprintf(stderr, "residue_bytes：%ld\n", residue_bytes);
									
									if(0 == residue_bytes) 
									{
										key++;       
										j++;          //结束循环
										continue;
									}
									
									if(nbytes > residue_bytes)
									{	
										nbytes = residue_bytes;
										continue;
									}
									
								}
								else         //文件未写成功 / 未写全
								{
									return ret;           //可以考虑将读取文件的地址传回(buffer的地址)
								}
							}
							else
							{
								return ret;   //文件未读成功 / 未读全
							}
						}
					}
					
				}
				else if(EXIT_META_NOT_FOUND_ERROR != ret)     //not found key(状态)
				{
					return ret;
				}
				else if(EXIT_META_NOT_FOUND_ERROR == ret)     //哈希链表中没有找到,该文件已被删除
				{
					key++;
					continue;
				}
				
				i++;


			}

            ret = file_op->flush_file();
			//截断文件
			ret = file_op->ftruncate_file(block_info()->size_);
			
			//更新索引文件信息
			index_header()->data_file_offset_ = block_info()->size_;
			
			//更新block info
			ret = block_info()->del_file_count_ = 0;
			ret = block_info()->del_size_ = 0;
			flush();
			
			return TFS_SUCCESS;
			
        }




    } // namespace tfs

    
} // namespace lucifer
 

 