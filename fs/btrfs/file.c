#ifndef MY_ABC_HERE
#define MY_ABC_HERE
#endif
 
#include <linux/fs.h>
#include <linux/pagemap.h>
#include <linux/highmem.h>
#include <linux/time.h>
#include <linux/init.h>
#include <linux/string.h>
#include <linux/backing-dev.h>
#include <linux/mpage.h>
#include <linux/aio.h>
#include <linux/falloc.h>
#if defined(MY_DEF_HERE)
#include <linux/socket.h>
#include <net/sock.h>
#include <linux/net.h>
#endif  
#include <linux/swap.h>
#include <linux/writeback.h>
#include <linux/statfs.h>
#include <linux/compat.h>
#include <linux/slab.h>
#include <linux/btrfs.h>
#if defined(MY_ABC_HERE)
#include <net/sock.h>
#endif  
#include "ctree.h"
#include "disk-io.h"
#include "transaction.h"
#include "btrfs_inode.h"
#include "print-tree.h"
#include "tree-log.h"
#include "locking.h"
#include "volumes.h"
#include "qgroup.h"

static struct kmem_cache *btrfs_inode_defrag_cachep;
 
struct inode_defrag {
	struct rb_node rb_node;
	 
	u64 ino;
	 
	u64 transid;

	u64 root;

	u64 last_offset;

	int cycled;
};

static int __compare_inode_defrag(struct inode_defrag *defrag1,
				  struct inode_defrag *defrag2)
{
	if (defrag1->root > defrag2->root)
		return 1;
	else if (defrag1->root < defrag2->root)
		return -1;
	else if (defrag1->ino > defrag2->ino)
		return 1;
	else if (defrag1->ino < defrag2->ino)
		return -1;
	else
		return 0;
}

static int __btrfs_add_inode_defrag(struct inode *inode,
				    struct inode_defrag *defrag)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct inode_defrag *entry;
	struct rb_node **p;
	struct rb_node *parent = NULL;
	int ret;

	p = &root->fs_info->defrag_inodes.rb_node;
	while (*p) {
		parent = *p;
		entry = rb_entry(parent, struct inode_defrag, rb_node);

		ret = __compare_inode_defrag(defrag, entry);
		if (ret < 0)
			p = &parent->rb_left;
		else if (ret > 0)
			p = &parent->rb_right;
		else {
			 
			if (defrag->transid < entry->transid)
				entry->transid = defrag->transid;
			if (defrag->last_offset > entry->last_offset)
				entry->last_offset = defrag->last_offset;
			return -EEXIST;
		}
	}
	set_bit(BTRFS_INODE_IN_DEFRAG, &BTRFS_I(inode)->runtime_flags);
	rb_link_node(&defrag->rb_node, parent, p);
	rb_insert_color(&defrag->rb_node, &root->fs_info->defrag_inodes);
	return 0;
}

static inline int __need_auto_defrag(struct btrfs_root *root)
{
	if (!btrfs_test_opt(root, AUTO_DEFRAG))
		return 0;

	if (btrfs_fs_closing(root->fs_info))
		return 0;

	return 1;
}

int btrfs_add_inode_defrag(struct btrfs_trans_handle *trans,
			   struct inode *inode)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct inode_defrag *defrag;
	u64 transid;
	int ret;

	if (!__need_auto_defrag(root))
		return 0;

	if (test_bit(BTRFS_INODE_IN_DEFRAG, &BTRFS_I(inode)->runtime_flags))
		return 0;

	if (trans)
		transid = trans->transid;
	else
		transid = BTRFS_I(inode)->root->last_trans;

	defrag = kmem_cache_zalloc(btrfs_inode_defrag_cachep, GFP_NOFS);
	if (!defrag)
		return -ENOMEM;

	defrag->ino = btrfs_ino(inode);
	defrag->transid = transid;
	defrag->root = root->root_key.objectid;

	spin_lock(&root->fs_info->defrag_inodes_lock);
	if (!test_bit(BTRFS_INODE_IN_DEFRAG, &BTRFS_I(inode)->runtime_flags)) {
		 
		ret = __btrfs_add_inode_defrag(inode, defrag);
		if (ret)
			kmem_cache_free(btrfs_inode_defrag_cachep, defrag);
	} else {
		kmem_cache_free(btrfs_inode_defrag_cachep, defrag);
	}
	spin_unlock(&root->fs_info->defrag_inodes_lock);
	return 0;
}

static void btrfs_requeue_inode_defrag(struct inode *inode,
				       struct inode_defrag *defrag)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	int ret;

	if (!__need_auto_defrag(root))
		goto out;

	spin_lock(&root->fs_info->defrag_inodes_lock);
	ret = __btrfs_add_inode_defrag(inode, defrag);
	spin_unlock(&root->fs_info->defrag_inodes_lock);
	if (ret)
		goto out;
	return;
out:
	kmem_cache_free(btrfs_inode_defrag_cachep, defrag);
}

static struct inode_defrag *
btrfs_pick_defrag_inode(struct btrfs_fs_info *fs_info, u64 root, u64 ino)
{
	struct inode_defrag *entry = NULL;
	struct inode_defrag tmp;
	struct rb_node *p;
	struct rb_node *parent = NULL;
	int ret;

	tmp.ino = ino;
	tmp.root = root;

	spin_lock(&fs_info->defrag_inodes_lock);
	p = fs_info->defrag_inodes.rb_node;
	while (p) {
		parent = p;
		entry = rb_entry(parent, struct inode_defrag, rb_node);

		ret = __compare_inode_defrag(&tmp, entry);
		if (ret < 0)
			p = parent->rb_left;
		else if (ret > 0)
			p = parent->rb_right;
		else
			goto out;
	}

	if (parent && __compare_inode_defrag(&tmp, entry) > 0) {
		parent = rb_next(parent);
		if (parent)
			entry = rb_entry(parent, struct inode_defrag, rb_node);
		else
			entry = NULL;
	}
out:
	if (entry)
		rb_erase(parent, &fs_info->defrag_inodes);
	spin_unlock(&fs_info->defrag_inodes_lock);
	return entry;
}

void btrfs_cleanup_defrag_inodes(struct btrfs_fs_info *fs_info)
{
	struct inode_defrag *defrag;
	struct rb_node *node;

	spin_lock(&fs_info->defrag_inodes_lock);
	node = rb_first(&fs_info->defrag_inodes);
	while (node) {
		rb_erase(node, &fs_info->defrag_inodes);
		defrag = rb_entry(node, struct inode_defrag, rb_node);
		kmem_cache_free(btrfs_inode_defrag_cachep, defrag);

		if (need_resched()) {
			spin_unlock(&fs_info->defrag_inodes_lock);
			cond_resched();
			spin_lock(&fs_info->defrag_inodes_lock);
		}

		node = rb_first(&fs_info->defrag_inodes);
	}
	spin_unlock(&fs_info->defrag_inodes_lock);
}

#define BTRFS_DEFRAG_BATCH	1024

static int __btrfs_run_defrag_inode(struct btrfs_fs_info *fs_info,
				    struct inode_defrag *defrag)
{
	struct btrfs_root *inode_root;
	struct inode *inode;
	struct btrfs_key key;
	struct btrfs_ioctl_defrag_range_args range;
	int num_defrag;
	int index;
	int ret;

	key.objectid = defrag->root;
	btrfs_set_key_type(&key, BTRFS_ROOT_ITEM_KEY);
	key.offset = (u64)-1;

	index = srcu_read_lock(&fs_info->subvol_srcu);

	inode_root = btrfs_read_fs_root_no_name(fs_info, &key);
	if (IS_ERR(inode_root)) {
		ret = PTR_ERR(inode_root);
		goto cleanup;
	}

	key.objectid = defrag->ino;
	btrfs_set_key_type(&key, BTRFS_INODE_ITEM_KEY);
	key.offset = 0;
	inode = btrfs_iget(fs_info->sb, &key, inode_root, NULL);
	if (IS_ERR(inode)) {
		ret = PTR_ERR(inode);
		goto cleanup;
	}
	srcu_read_unlock(&fs_info->subvol_srcu, index);

	clear_bit(BTRFS_INODE_IN_DEFRAG, &BTRFS_I(inode)->runtime_flags);
	memset(&range, 0, sizeof(range));
	range.len = (u64)-1;
	range.start = defrag->last_offset;

	sb_start_write(fs_info->sb);
	num_defrag = btrfs_defrag_file(inode, NULL, &range, defrag->transid,
				       BTRFS_DEFRAG_BATCH);
	sb_end_write(fs_info->sb);
	 
	if (num_defrag == BTRFS_DEFRAG_BATCH) {
		defrag->last_offset = range.start;
		btrfs_requeue_inode_defrag(inode, defrag);
	} else if (defrag->last_offset && !defrag->cycled) {
		 
		defrag->last_offset = 0;
		defrag->cycled = 1;
		btrfs_requeue_inode_defrag(inode, defrag);
	} else {
		kmem_cache_free(btrfs_inode_defrag_cachep, defrag);
	}

	iput(inode);
	return 0;
cleanup:
	srcu_read_unlock(&fs_info->subvol_srcu, index);
	kmem_cache_free(btrfs_inode_defrag_cachep, defrag);
	return ret;
}

int btrfs_run_defrag_inodes(struct btrfs_fs_info *fs_info)
{
	struct inode_defrag *defrag;
	u64 first_ino = 0;
	u64 root_objectid = 0;

	atomic_inc(&fs_info->defrag_running);
	while (1) {
		 
		if (test_bit(BTRFS_FS_STATE_REMOUNTING,
			     &fs_info->fs_state))
			break;

		if (!__need_auto_defrag(fs_info->tree_root))
			break;

		defrag = btrfs_pick_defrag_inode(fs_info, root_objectid,
						 first_ino);
		if (!defrag) {
			if (root_objectid || first_ino) {
				root_objectid = 0;
				first_ino = 0;
				continue;
			} else {
				break;
			}
		}

		first_ino = defrag->ino + 1;
		root_objectid = defrag->root;

		__btrfs_run_defrag_inode(fs_info, defrag);
	}
	atomic_dec(&fs_info->defrag_running);

	wake_up(&fs_info->transaction_wait);
	return 0;
}

static noinline int btrfs_copy_from_user(loff_t pos, int num_pages,
					 size_t write_bytes,
					 struct page **prepared_pages,
					 struct iov_iter *i)
{
	size_t copied = 0;
	size_t total_copied = 0;
	int pg = 0;
	int offset = pos & (PAGE_CACHE_SIZE - 1);

	while (write_bytes > 0) {
		size_t count = min_t(size_t,
				     PAGE_CACHE_SIZE - offset, write_bytes);
		struct page *page = prepared_pages[pg];
		 
		pagefault_disable();
		copied = iov_iter_copy_from_user_atomic(page, i, offset, count);
		pagefault_enable();

		flush_dcache_page(page);

		if (!PageUptodate(page) && copied < count)
			copied = 0;

		iov_iter_advance(i, copied);
		write_bytes -= copied;
		total_copied += copied;

		if (unlikely(copied == 0))
			break;

		if (unlikely(copied < PAGE_CACHE_SIZE - offset)) {
			offset += copied;
		} else {
			pg++;
			offset = 0;
		}
	}
	return total_copied;
}

static void btrfs_drop_pages(struct page **pages, size_t num_pages)
{
	size_t i;
	for (i = 0; i < num_pages; i++) {
		 
		ClearPageChecked(pages[i]);
		unlock_page(pages[i]);
		mark_page_accessed(pages[i]);
		page_cache_release(pages[i]);
	}
}

int btrfs_dirty_pages(struct btrfs_root *root, struct inode *inode,
			     struct page **pages, size_t num_pages,
			     loff_t pos, size_t write_bytes,
			     struct extent_state **cached)
{
	int err = 0;
	int i;
	u64 num_bytes;
	u64 start_pos;
	u64 end_of_last_block;
	u64 end_pos = pos + write_bytes;
	loff_t isize = i_size_read(inode);

	start_pos = pos & ~((u64)root->sectorsize - 1);
	num_bytes = ALIGN(write_bytes + pos - start_pos, root->sectorsize);

	end_of_last_block = start_pos + num_bytes - 1;
	err = btrfs_set_extent_delalloc(inode, start_pos, end_of_last_block,
					cached);
	if (err)
		return err;

	for (i = 0; i < num_pages; i++) {
		struct page *p = pages[i];
		SetPageUptodate(p);
		ClearPageChecked(p);
		set_page_dirty(p);
	}

	if (end_pos > isize)
		i_size_write(inode, end_pos);
	return 0;
}

void btrfs_drop_extent_cache(struct inode *inode, u64 start, u64 end,
			     int skip_pinned)
{
	struct extent_map *em;
	struct extent_map *split = NULL;
	struct extent_map *split2 = NULL;
	struct extent_map_tree *em_tree = &BTRFS_I(inode)->extent_tree;
	u64 len = end - start + 1;
	u64 gen;
	int ret;
	int testend = 1;
	unsigned long flags;
	int compressed = 0;
	bool modified;

	WARN_ON(end < start);
	if (end == (u64)-1) {
		len = (u64)-1;
		testend = 0;
	}
	while (1) {
		int no_splits = 0;

		modified = false;
		if (!split)
			split = alloc_extent_map();
		if (!split2)
			split2 = alloc_extent_map();
		if (!split || !split2)
			no_splits = 1;

		write_lock(&em_tree->lock);
		em = lookup_extent_mapping(em_tree, start, len);
		if (!em) {
			write_unlock(&em_tree->lock);
			break;
		}
		flags = em->flags;
		gen = em->generation;
		if (skip_pinned && test_bit(EXTENT_FLAG_PINNED, &em->flags)) {
			if (testend && em->start + em->len >= start + len) {
				free_extent_map(em);
				write_unlock(&em_tree->lock);
				break;
			}
			start = em->start + em->len;
			if (testend)
				len = start + len - (em->start + em->len);
			free_extent_map(em);
			write_unlock(&em_tree->lock);
			continue;
		}
		compressed = test_bit(EXTENT_FLAG_COMPRESSED, &em->flags);
		clear_bit(EXTENT_FLAG_PINNED, &em->flags);
		clear_bit(EXTENT_FLAG_LOGGING, &flags);
		modified = !list_empty(&em->list);
		if (no_splits)
			goto next;

		if (em->start < start) {
			split->start = em->start;
			split->len = start - em->start;

			if (em->block_start < EXTENT_MAP_LAST_BYTE) {
				split->orig_start = em->orig_start;
				split->block_start = em->block_start;

				if (compressed)
					split->block_len = em->block_len;
				else
					split->block_len = split->len;
				split->orig_block_len = max(split->block_len,
						em->orig_block_len);
				split->ram_bytes = em->ram_bytes;
			} else {
				split->orig_start = split->start;
				split->block_len = 0;
				split->block_start = em->block_start;
				split->orig_block_len = 0;
				split->ram_bytes = split->len;
			}

			split->generation = gen;
			split->bdev = em->bdev;
			split->flags = flags;
			split->compress_type = em->compress_type;
			replace_extent_mapping(em_tree, em, split, modified);
			free_extent_map(split);
			split = split2;
			split2 = NULL;
		}
		if (testend && em->start + em->len > start + len) {
			u64 diff = start + len - em->start;

			split->start = start + len;
			split->len = em->start + em->len - (start + len);
			split->bdev = em->bdev;
			split->flags = flags;
			split->compress_type = em->compress_type;
			split->generation = gen;

			if (em->block_start < EXTENT_MAP_LAST_BYTE) {
				split->orig_block_len = max(em->block_len,
						    em->orig_block_len);

				split->ram_bytes = em->ram_bytes;
				if (compressed) {
					split->block_len = em->block_len;
					split->block_start = em->block_start;
					split->orig_start = em->orig_start;
				} else {
					split->block_len = split->len;
					split->block_start = em->block_start
						+ diff;
					split->orig_start = em->orig_start;
				}
			} else {
				split->ram_bytes = split->len;
				split->orig_start = split->start;
				split->block_len = 0;
				split->block_start = em->block_start;
				split->orig_block_len = 0;
			}

			if (extent_map_in_tree(em)) {
				replace_extent_mapping(em_tree, em, split,
						       modified);
			} else {
				ret = add_extent_mapping(em_tree, split,
							 modified);
				ASSERT(ret == 0);  
			}
			free_extent_map(split);
			split = NULL;
		}
next:
		if (extent_map_in_tree(em))
			remove_extent_mapping(em_tree, em);
		write_unlock(&em_tree->lock);

		free_extent_map(em);
		 
		free_extent_map(em);
	}
	if (split)
		free_extent_map(split);
	if (split2)
		free_extent_map(split2);
}

int __btrfs_drop_extents(struct btrfs_trans_handle *trans,
			 struct btrfs_root *root, struct inode *inode,
			 struct btrfs_path *path, u64 start, u64 end,
			 u64 *drop_end, int drop_cache,
			 int replace_extent,
			 u32 extent_item_size,
			 int *key_inserted)
{
	struct extent_buffer *leaf;
	struct btrfs_file_extent_item *fi;
	struct btrfs_key key;
	struct btrfs_key new_key;
	u64 ino = btrfs_ino(inode);
	u64 search_start = start;
	u64 disk_bytenr = 0;
	u64 num_bytes = 0;
	u64 extent_offset = 0;
	u64 extent_end = 0;
	int del_nr = 0;
	int del_slot = 0;
	int extent_type;
	int recow;
	int ret;
	int modify_tree = -1;
	int update_refs;
	int found = 0;
	int leafs_visited = 0;

	if (drop_cache)
		btrfs_drop_extent_cache(inode, start, end - 1, 0);

	if (start >= BTRFS_I(inode)->disk_i_size && !replace_extent)
		modify_tree = 0;

	update_refs = (test_bit(BTRFS_ROOT_REF_COWS, &root->state) ||
		       root == root->fs_info->tree_root);
	while (1) {
		recow = 0;
		ret = btrfs_lookup_file_extent(trans, root, path, ino,
					       search_start, modify_tree);
		if (ret < 0)
			break;
		if (ret > 0 && path->slots[0] > 0 && search_start == start) {
			leaf = path->nodes[0];
			btrfs_item_key_to_cpu(leaf, &key, path->slots[0] - 1);
			if (key.objectid == ino &&
			    key.type == BTRFS_EXTENT_DATA_KEY)
				path->slots[0]--;
		}
		ret = 0;
		leafs_visited++;
next_slot:
		leaf = path->nodes[0];
		if (path->slots[0] >= btrfs_header_nritems(leaf)) {
			BUG_ON(del_nr > 0);
			ret = btrfs_next_leaf(root, path);
			if (ret < 0)
				break;
			if (ret > 0) {
				ret = 0;
				break;
			}
			leafs_visited++;
			leaf = path->nodes[0];
			recow = 1;
		}

		btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
		if (key.objectid > ino ||
		    key.type > BTRFS_EXTENT_DATA_KEY || key.offset >= end)
			break;

		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		extent_type = btrfs_file_extent_type(leaf, fi);

		if (extent_type == BTRFS_FILE_EXTENT_REG ||
		    extent_type == BTRFS_FILE_EXTENT_PREALLOC) {
			disk_bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
			num_bytes = btrfs_file_extent_disk_num_bytes(leaf, fi);
			extent_offset = btrfs_file_extent_offset(leaf, fi);
			extent_end = key.offset +
				btrfs_file_extent_num_bytes(leaf, fi);
		} else if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
			extent_end = key.offset +
				btrfs_file_extent_inline_len(leaf,
						     path->slots[0], fi);
		} else {
			WARN_ON(1);
			extent_end = search_start;
		}

		if (extent_end == key.offset && extent_end >= search_start)
			goto delete_extent_item;

		if (extent_end <= search_start) {
			path->slots[0]++;
			goto next_slot;
		}

		found = 1;
		search_start = max(key.offset, start);
		if (recow || !modify_tree) {
			modify_tree = -1;
			btrfs_release_path(path);
			continue;
		}

		if (start > key.offset && end < extent_end) {
			BUG_ON(del_nr > 0);
			if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
				ret = -EOPNOTSUPP;
				break;
			}

			memcpy(&new_key, &key, sizeof(new_key));
			new_key.offset = start;
			ret = btrfs_duplicate_item(trans, root, path,
						   &new_key);
			if (ret == -EAGAIN) {
				btrfs_release_path(path);
				continue;
			}
			if (ret < 0)
				break;

			leaf = path->nodes[0];
			fi = btrfs_item_ptr(leaf, path->slots[0] - 1,
					    struct btrfs_file_extent_item);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							start - key.offset);

			fi = btrfs_item_ptr(leaf, path->slots[0],
					    struct btrfs_file_extent_item);

			extent_offset += start - key.offset;
			btrfs_set_file_extent_offset(leaf, fi, extent_offset);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							extent_end - start);
			btrfs_mark_buffer_dirty(leaf);

			if (update_refs && disk_bytenr > 0) {
				ret = btrfs_inc_extent_ref(trans, root,
						disk_bytenr, num_bytes, 0,
						root->root_key.objectid,
						new_key.objectid,
						start - extent_offset, 1);
				BUG_ON(ret);  
			}
			key.offset = start;
		}
		 
		if (start <= key.offset && end < extent_end) {
			if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
				ret = -EOPNOTSUPP;
				break;
			}

			memcpy(&new_key, &key, sizeof(new_key));
			new_key.offset = end;
			btrfs_set_item_key_safe(root, path, &new_key);

			extent_offset += end - key.offset;
			btrfs_set_file_extent_offset(leaf, fi, extent_offset);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							extent_end - end);
			btrfs_mark_buffer_dirty(leaf);
			if (update_refs && disk_bytenr > 0)
				inode_sub_bytes(inode, end - key.offset);
			break;
		}

		search_start = extent_end;
		 
		if (start > key.offset && end >= extent_end) {
			BUG_ON(del_nr > 0);
			if (extent_type == BTRFS_FILE_EXTENT_INLINE) {
				ret = -EOPNOTSUPP;
				break;
			}

			btrfs_set_file_extent_num_bytes(leaf, fi,
							start - key.offset);
			btrfs_mark_buffer_dirty(leaf);
			if (update_refs && disk_bytenr > 0)
				inode_sub_bytes(inode, extent_end - start);
			if (end == extent_end)
				break;

			path->slots[0]++;
			goto next_slot;
		}

		if (start <= key.offset && end >= extent_end) {
delete_extent_item:
			if (del_nr == 0) {
				del_slot = path->slots[0];
				del_nr = 1;
			} else {
				BUG_ON(del_slot + del_nr != path->slots[0]);
				del_nr++;
			}

			if (update_refs &&
			    extent_type == BTRFS_FILE_EXTENT_INLINE) {
				inode_sub_bytes(inode,
						extent_end - key.offset);
				extent_end = ALIGN(extent_end,
						   root->sectorsize);
			} else if (update_refs && disk_bytenr > 0) {
				ret = btrfs_free_extent(trans, root,
						disk_bytenr, num_bytes, 0,
						root->root_key.objectid,
						key.objectid, key.offset -
						extent_offset, 0);
				BUG_ON(ret);  
				inode_sub_bytes(inode,
						extent_end - key.offset);
			}

			if (end == extent_end)
				break;

			if (path->slots[0] + 1 < btrfs_header_nritems(leaf)) {
				path->slots[0]++;
				goto next_slot;
			}

			ret = btrfs_del_items(trans, root, path, del_slot,
					      del_nr);
			if (ret) {
				btrfs_abort_transaction(trans, root, ret);
				break;
			}

			del_nr = 0;
			del_slot = 0;

			btrfs_release_path(path);
			continue;
		}

		BUG_ON(1);
	}

	if (!ret && del_nr > 0) {
		 
		path->slots[0] = del_slot;
		ret = btrfs_del_items(trans, root, path, del_slot, del_nr);
		if (ret)
			btrfs_abort_transaction(trans, root, ret);
	}

	leaf = path->nodes[0];
	 
	if (!ret && replace_extent && leafs_visited == 1 &&
	    (path->locks[0] == BTRFS_WRITE_LOCK_BLOCKING ||
	     path->locks[0] == BTRFS_WRITE_LOCK) &&
	    btrfs_leaf_free_space(root, leaf) >=
	    sizeof(struct btrfs_item) + extent_item_size) {

		key.objectid = ino;
		key.type = BTRFS_EXTENT_DATA_KEY;
		key.offset = start;
		if (!del_nr && path->slots[0] < btrfs_header_nritems(leaf)) {
			struct btrfs_key slot_key;

			btrfs_item_key_to_cpu(leaf, &slot_key, path->slots[0]);
			if (btrfs_comp_cpu_keys(&key, &slot_key) > 0)
				path->slots[0]++;
		}
		setup_items_for_insert(root, path, &key,
				       &extent_item_size,
				       extent_item_size,
				       sizeof(struct btrfs_item) +
				       extent_item_size, 1);
		*key_inserted = 1;
	}

	if (!replace_extent || !(*key_inserted))
		btrfs_release_path(path);
	if (drop_end)
		*drop_end = found ? min(end, extent_end) : end;
	return ret;
}

int btrfs_drop_extents(struct btrfs_trans_handle *trans,
		       struct btrfs_root *root, struct inode *inode, u64 start,
		       u64 end, int drop_cache)
{
	struct btrfs_path *path;
	int ret;

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
	ret = __btrfs_drop_extents(trans, root, inode, path, start, end, NULL,
				   drop_cache, 0, 0, NULL);
	btrfs_free_path(path);
	return ret;
}

static int extent_mergeable(struct extent_buffer *leaf, int slot,
			    u64 objectid, u64 bytenr, u64 orig_offset,
			    u64 *start, u64 *end)
{
	struct btrfs_file_extent_item *fi;
	struct btrfs_key key;
	u64 extent_end;

	if (slot < 0 || slot >= btrfs_header_nritems(leaf))
		return 0;

	btrfs_item_key_to_cpu(leaf, &key, slot);
	if (key.objectid != objectid || key.type != BTRFS_EXTENT_DATA_KEY)
		return 0;

	fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);
	if (btrfs_file_extent_type(leaf, fi) != BTRFS_FILE_EXTENT_REG ||
	    btrfs_file_extent_disk_bytenr(leaf, fi) != bytenr ||
	    btrfs_file_extent_offset(leaf, fi) != key.offset - orig_offset ||
	    btrfs_file_extent_compression(leaf, fi) ||
	    btrfs_file_extent_encryption(leaf, fi) ||
	    btrfs_file_extent_other_encoding(leaf, fi))
		return 0;

	extent_end = key.offset + btrfs_file_extent_num_bytes(leaf, fi);
	if ((*start && *start != key.offset) || (*end && *end != extent_end))
		return 0;

	*start = key.offset;
	*end = extent_end;
	return 1;
}

int btrfs_mark_extent_written(struct btrfs_trans_handle *trans,
			      struct inode *inode, u64 start, u64 end)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct extent_buffer *leaf;
	struct btrfs_path *path;
	struct btrfs_file_extent_item *fi;
	struct btrfs_key key;
	struct btrfs_key new_key;
	u64 bytenr;
	u64 num_bytes;
	u64 extent_end;
	u64 orig_offset;
	u64 other_start;
	u64 other_end;
	u64 split;
	int del_nr = 0;
	int del_slot = 0;
	int recow;
	int ret;
	u64 ino = btrfs_ino(inode);

	path = btrfs_alloc_path();
	if (!path)
		return -ENOMEM;
again:
	recow = 0;
	split = start;
	key.objectid = ino;
	key.type = BTRFS_EXTENT_DATA_KEY;
	key.offset = split;

	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
	if (ret < 0)
		goto out;
	if (ret > 0 && path->slots[0] > 0)
		path->slots[0]--;

	leaf = path->nodes[0];
	btrfs_item_key_to_cpu(leaf, &key, path->slots[0]);
	BUG_ON(key.objectid != ino || key.type != BTRFS_EXTENT_DATA_KEY);
	fi = btrfs_item_ptr(leaf, path->slots[0],
			    struct btrfs_file_extent_item);
	BUG_ON(btrfs_file_extent_type(leaf, fi) !=
	       BTRFS_FILE_EXTENT_PREALLOC);
	extent_end = key.offset + btrfs_file_extent_num_bytes(leaf, fi);
	BUG_ON(key.offset > start || extent_end < end);

	bytenr = btrfs_file_extent_disk_bytenr(leaf, fi);
	num_bytes = btrfs_file_extent_disk_num_bytes(leaf, fi);
	orig_offset = key.offset - btrfs_file_extent_offset(leaf, fi);
	memcpy(&new_key, &key, sizeof(new_key));

	if (start == key.offset && end < extent_end) {
		other_start = 0;
		other_end = start;
		if (extent_mergeable(leaf, path->slots[0] - 1,
				     ino, bytenr, orig_offset,
				     &other_start, &other_end)) {
			new_key.offset = end;
			btrfs_set_item_key_safe(root, path, &new_key);
			fi = btrfs_item_ptr(leaf, path->slots[0],
					    struct btrfs_file_extent_item);
			btrfs_set_file_extent_generation(leaf, fi,
							 trans->transid);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							extent_end - end);
			btrfs_set_file_extent_offset(leaf, fi,
						     end - orig_offset);
			fi = btrfs_item_ptr(leaf, path->slots[0] - 1,
					    struct btrfs_file_extent_item);
			btrfs_set_file_extent_generation(leaf, fi,
							 trans->transid);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							end - other_start);
			btrfs_mark_buffer_dirty(leaf);
			goto out;
		}
	}

	if (start > key.offset && end == extent_end) {
		other_start = end;
		other_end = 0;
		if (extent_mergeable(leaf, path->slots[0] + 1,
				     ino, bytenr, orig_offset,
				     &other_start, &other_end)) {
			fi = btrfs_item_ptr(leaf, path->slots[0],
					    struct btrfs_file_extent_item);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							start - key.offset);
			btrfs_set_file_extent_generation(leaf, fi,
							 trans->transid);
			path->slots[0]++;
			new_key.offset = start;
			btrfs_set_item_key_safe(root, path, &new_key);

			fi = btrfs_item_ptr(leaf, path->slots[0],
					    struct btrfs_file_extent_item);
			btrfs_set_file_extent_generation(leaf, fi,
							 trans->transid);
			btrfs_set_file_extent_num_bytes(leaf, fi,
							other_end - start);
			btrfs_set_file_extent_offset(leaf, fi,
						     start - orig_offset);
			btrfs_mark_buffer_dirty(leaf);
			goto out;
		}
	}

	while (start > key.offset || end < extent_end) {
		if (key.offset == start)
			split = end;

		new_key.offset = split;
		ret = btrfs_duplicate_item(trans, root, path, &new_key);
		if (ret == -EAGAIN) {
			btrfs_release_path(path);
			goto again;
		}
		if (ret < 0) {
			btrfs_abort_transaction(trans, root, ret);
			goto out;
		}

		leaf = path->nodes[0];
		fi = btrfs_item_ptr(leaf, path->slots[0] - 1,
				    struct btrfs_file_extent_item);
		btrfs_set_file_extent_generation(leaf, fi, trans->transid);
		btrfs_set_file_extent_num_bytes(leaf, fi,
						split - key.offset);

		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);

		btrfs_set_file_extent_generation(leaf, fi, trans->transid);
		btrfs_set_file_extent_offset(leaf, fi, split - orig_offset);
		btrfs_set_file_extent_num_bytes(leaf, fi,
						extent_end - split);
		btrfs_mark_buffer_dirty(leaf);

		ret = btrfs_inc_extent_ref(trans, root, bytenr, num_bytes, 0,
					   root->root_key.objectid,
					   ino, orig_offset, 1);
		BUG_ON(ret);  

		if (split == start) {
			key.offset = start;
		} else {
			BUG_ON(start != key.offset);
			path->slots[0]--;
			extent_end = end;
		}
		recow = 1;
	}

	other_start = end;
	other_end = 0;
	if (extent_mergeable(leaf, path->slots[0] + 1,
			     ino, bytenr, orig_offset,
			     &other_start, &other_end)) {
		if (recow) {
			btrfs_release_path(path);
			goto again;
		}
		extent_end = other_end;
		del_slot = path->slots[0] + 1;
		del_nr++;
		ret = btrfs_free_extent(trans, root, bytenr, num_bytes,
					0, root->root_key.objectid,
					ino, orig_offset, 0);
		BUG_ON(ret);  
	}
	other_start = 0;
	other_end = start;
	if (extent_mergeable(leaf, path->slots[0] - 1,
			     ino, bytenr, orig_offset,
			     &other_start, &other_end)) {
		if (recow) {
			btrfs_release_path(path);
			goto again;
		}
		key.offset = other_start;
		del_slot = path->slots[0];
		del_nr++;
		ret = btrfs_free_extent(trans, root, bytenr, num_bytes,
					0, root->root_key.objectid,
					ino, orig_offset, 0);
		BUG_ON(ret);  
	}
	if (del_nr == 0) {
		fi = btrfs_item_ptr(leaf, path->slots[0],
			   struct btrfs_file_extent_item);
		btrfs_set_file_extent_type(leaf, fi,
					   BTRFS_FILE_EXTENT_REG);
		btrfs_set_file_extent_generation(leaf, fi, trans->transid);
		btrfs_mark_buffer_dirty(leaf);
	} else {
		fi = btrfs_item_ptr(leaf, del_slot - 1,
			   struct btrfs_file_extent_item);
		btrfs_set_file_extent_type(leaf, fi,
					   BTRFS_FILE_EXTENT_REG);
		btrfs_set_file_extent_generation(leaf, fi, trans->transid);
		btrfs_set_file_extent_num_bytes(leaf, fi,
						extent_end - key.offset);
		btrfs_mark_buffer_dirty(leaf);

		ret = btrfs_del_items(trans, root, path, del_slot, del_nr);
		if (ret < 0) {
			btrfs_abort_transaction(trans, root, ret);
			goto out;
		}
	}
out:
	btrfs_free_path(path);
	return 0;
}

static int prepare_uptodate_page(struct page *page, u64 pos,
				 bool force_uptodate)
{
	int ret = 0;

	if (((pos & (PAGE_CACHE_SIZE - 1)) || force_uptodate) &&
	    !PageUptodate(page)) {
		ret = btrfs_readpage(NULL, page);
		if (ret)
			return ret;
		lock_page(page);
		if (!PageUptodate(page)) {
			unlock_page(page);
			return -EIO;
		}
	}
	return 0;
}

static noinline int prepare_pages(struct inode *inode, struct page **pages,
				  size_t num_pages, loff_t pos,
				  size_t write_bytes, bool force_uptodate)
{
	int i;
	unsigned long index = pos >> PAGE_CACHE_SHIFT;
	gfp_t mask = btrfs_alloc_write_mask(inode->i_mapping);
	int err = 0;
	int faili;

	for (i = 0; i < num_pages; i++) {
		pages[i] = find_or_create_page(inode->i_mapping, index + i,
					       mask | __GFP_WRITE);
		if (!pages[i]) {
			faili = i - 1;
			err = -ENOMEM;
			goto fail;
		}

		if (i == 0)
			err = prepare_uptodate_page(pages[i], pos,
						    force_uptodate);
		if (i == num_pages - 1)
			err = prepare_uptodate_page(pages[i],
						    pos + write_bytes, false);
		if (err) {
			page_cache_release(pages[i]);
			faili = i - 1;
			goto fail;
		}
		wait_on_page_writeback(pages[i]);
	}

	return 0;
fail:
	while (faili >= 0) {
		unlock_page(pages[faili]);
		page_cache_release(pages[faili]);
		faili--;
	}
	return err;

}

static noinline int
lock_and_cleanup_extent_if_need(struct inode *inode, struct page **pages,
				size_t num_pages, loff_t pos,
				u64 *lockstart, u64 *lockend,
				struct extent_state **cached_state)
{
	u64 start_pos;
	u64 last_pos;
	int i;
	int ret = 0;

	start_pos = pos & ~((u64)PAGE_CACHE_SIZE - 1);
	last_pos = start_pos + ((u64)num_pages << PAGE_CACHE_SHIFT) - 1;

	if (start_pos < inode->i_size) {
		struct btrfs_ordered_extent *ordered;
		lock_extent_bits(&BTRFS_I(inode)->io_tree,
				 start_pos, last_pos, 0, cached_state);
		ordered = btrfs_lookup_ordered_range(inode, start_pos,
						     last_pos - start_pos + 1);
		if (ordered &&
		    ordered->file_offset + ordered->len > start_pos &&
		    ordered->file_offset <= last_pos) {
			unlock_extent_cached(&BTRFS_I(inode)->io_tree,
					     start_pos, last_pos,
					     cached_state, GFP_NOFS);
			for (i = 0; i < num_pages; i++) {
				unlock_page(pages[i]);
				page_cache_release(pages[i]);
			}
			btrfs_start_ordered_extent(inode, ordered, 1);
			btrfs_put_ordered_extent(ordered);
			return -EAGAIN;
		}
		if (ordered)
			btrfs_put_ordered_extent(ordered);

		clear_extent_bit(&BTRFS_I(inode)->io_tree, start_pos,
				  last_pos, EXTENT_DIRTY | EXTENT_DELALLOC |
				  EXTENT_DO_ACCOUNTING | EXTENT_DEFRAG,
				  0, 0, cached_state, GFP_NOFS);
		*lockstart = start_pos;
		*lockend = last_pos;
		ret = 1;
	}

	for (i = 0; i < num_pages; i++) {
		if (clear_page_dirty_for_io(pages[i]))
			account_page_redirty(pages[i]);
		set_page_extent_mapped(pages[i]);
		WARN_ON(!PageLocked(pages[i]));
	}

	return ret;
}

static noinline int check_can_nocow(struct inode *inode, loff_t pos,
				    size_t *write_bytes)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct btrfs_ordered_extent *ordered;
	u64 lockstart, lockend;
	u64 num_bytes;
	int ret;

	ret = btrfs_start_write_no_snapshoting(root);
	if (!ret)
		return -ENOSPC;

	lockstart = round_down(pos, root->sectorsize);
	lockend = round_up(pos + *write_bytes, root->sectorsize) - 1;

	while (1) {
		lock_extent(&BTRFS_I(inode)->io_tree, lockstart, lockend);
		ordered = btrfs_lookup_ordered_range(inode, lockstart,
						     lockend - lockstart + 1);
		if (!ordered) {
			break;
		}
		unlock_extent(&BTRFS_I(inode)->io_tree, lockstart, lockend);
		btrfs_start_ordered_extent(inode, ordered, 1);
		btrfs_put_ordered_extent(ordered);
	}

	num_bytes = lockend - lockstart + 1;
	ret = can_nocow_extent(inode, lockstart, &num_bytes, NULL, NULL, NULL);
	if (ret <= 0) {
		ret = 0;
		btrfs_end_write_no_snapshoting(root);
	} else {
		*write_bytes = min_t(size_t, *write_bytes ,
				     num_bytes - pos + lockstart);
	}

	unlock_extent(&BTRFS_I(inode)->io_tree, lockstart, lockend);

	return ret;
}

static noinline ssize_t __btrfs_buffered_write(struct file *file,
					       struct iov_iter *i,
					       loff_t pos)
{
	struct inode *inode = file_inode(file);
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct page **pages = NULL;
	struct extent_state *cached_state = NULL;
	u64 release_bytes = 0;
	u64 lockstart;
	u64 lockend;
	unsigned long first_index;
	size_t num_written = 0;
	int nrptrs;
	int ret = 0;
	bool only_release_metadata = false;
	bool force_page_uptodate = false;
	bool need_unlock;

	nrptrs = min((iov_iter_count(i) + PAGE_CACHE_SIZE - 1) /
		     PAGE_CACHE_SIZE, PAGE_CACHE_SIZE /
		     (sizeof(struct page *)));
	nrptrs = min(nrptrs, current->nr_dirtied_pause - current->nr_dirtied);
	nrptrs = max(nrptrs, 8);
	pages = kmalloc(nrptrs * sizeof(struct page *), GFP_KERNEL);
	if (!pages)
		return -ENOMEM;

	first_index = pos >> PAGE_CACHE_SHIFT;

	while (iov_iter_count(i) > 0) {
		size_t offset = pos & (PAGE_CACHE_SIZE - 1);
		size_t write_bytes = min(iov_iter_count(i),
					 nrptrs * (size_t)PAGE_CACHE_SIZE -
					 offset);
		size_t num_pages = (write_bytes + offset +
				    PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
		size_t reserve_bytes;
		size_t dirty_pages;
		size_t copied;

		WARN_ON(num_pages > nrptrs);

		if (unlikely(iov_iter_fault_in_readable(i, write_bytes))) {
			ret = -EFAULT;
			break;
		}

		reserve_bytes = num_pages << PAGE_CACHE_SHIFT;
		ret = btrfs_check_data_free_space(inode, reserve_bytes);
		if (ret == -ENOSPC &&
		    (BTRFS_I(inode)->flags & (BTRFS_INODE_NODATACOW |
					      BTRFS_INODE_PREALLOC))) {
			ret = check_can_nocow(inode, pos, &write_bytes);
			if (ret > 0) {
				only_release_metadata = true;
				 
				num_pages = (write_bytes + offset +
					     PAGE_CACHE_SIZE - 1) >>
					PAGE_CACHE_SHIFT;
				reserve_bytes = num_pages << PAGE_CACHE_SHIFT;
				ret = 0;
			} else {
				ret = -ENOSPC;
			}
		}

		if (ret)
			break;

		ret = btrfs_delalloc_reserve_metadata(inode, reserve_bytes);
		if (ret) {
			if (!only_release_metadata)
				btrfs_free_reserved_data_space(inode,
							       reserve_bytes);
			else
				btrfs_end_write_no_snapshoting(root);
			break;
		}

		release_bytes = reserve_bytes;
		need_unlock = false;
again:
		 
		ret = prepare_pages(inode, pages, num_pages,
				    pos, write_bytes,
				    force_page_uptodate);
		if (ret)
			break;

		ret = lock_and_cleanup_extent_if_need(inode, pages, num_pages,
						      pos, &lockstart, &lockend,
						      &cached_state);
		if (ret < 0) {
			if (ret == -EAGAIN)
				goto again;
			break;
		} else if (ret > 0) {
			need_unlock = true;
			ret = 0;
		}

		copied = btrfs_copy_from_user(pos, num_pages,
					   write_bytes, pages, i);

		if (copied < write_bytes)
			nrptrs = 1;

		if (copied == 0) {
			force_page_uptodate = true;
			dirty_pages = 0;
		} else {
			force_page_uptodate = false;
			dirty_pages = (copied + offset +
				       PAGE_CACHE_SIZE - 1) >>
				       PAGE_CACHE_SHIFT;
		}

		if (num_pages > dirty_pages) {
			release_bytes = (num_pages - dirty_pages) <<
				PAGE_CACHE_SHIFT;
			if (copied > 0) {
				spin_lock(&BTRFS_I(inode)->lock);
				BTRFS_I(inode)->outstanding_extents++;
				spin_unlock(&BTRFS_I(inode)->lock);
			}
			if (only_release_metadata)
				btrfs_delalloc_release_metadata(inode,
								release_bytes);
			else
				btrfs_delalloc_release_space(inode,
							     release_bytes);
		}

		release_bytes = dirty_pages << PAGE_CACHE_SHIFT;

		if (copied > 0)
			ret = btrfs_dirty_pages(root, inode, pages,
						dirty_pages, pos, copied,
						NULL);
		if (need_unlock)
			unlock_extent_cached(&BTRFS_I(inode)->io_tree,
					     lockstart, lockend, &cached_state,
					     GFP_NOFS);
		if (ret) {
			btrfs_drop_pages(pages, num_pages);
			break;
		}

		release_bytes = 0;
		if (only_release_metadata)
			btrfs_end_write_no_snapshoting(root);

		if (only_release_metadata && copied > 0) {
			u64 lockstart = round_down(pos, root->sectorsize);
			u64 lockend = lockstart +
				(dirty_pages << PAGE_CACHE_SHIFT) - 1;

			set_extent_bit(&BTRFS_I(inode)->io_tree, lockstart,
				       lockend, EXTENT_NORESERVE, NULL,
				       NULL, GFP_NOFS);
			only_release_metadata = false;
		}

		btrfs_drop_pages(pages, num_pages);

		cond_resched();

		balance_dirty_pages_ratelimited(inode->i_mapping);
		if (dirty_pages < (root->leafsize >> PAGE_CACHE_SHIFT) + 1)
			btrfs_btree_balance_dirty(root);

		pos += copied;
		num_written += copied;
	}

	kfree(pages);

	if (release_bytes) {
		if (only_release_metadata) {
			btrfs_end_write_no_snapshoting(root);
			btrfs_delalloc_release_metadata(inode, release_bytes);
		} else {
			btrfs_delalloc_release_space(inode, release_bytes);
		}
	}

	return num_written ? num_written : ret;
}

static ssize_t __btrfs_direct_write(struct kiocb *iocb,
				    const struct iovec *iov,
				    unsigned long nr_segs, loff_t pos,
				    loff_t *ppos, size_t count, size_t ocount)
{
	struct file *file = iocb->ki_filp;
	struct iov_iter i;
	struct inode *inode = file_inode(file);
	ssize_t written;
	ssize_t written_buffered;
	loff_t endbyte;
	int err;

	written = generic_file_direct_write(iocb, iov, &nr_segs, pos, ppos,
					    count, ocount);

	if (written < 0 || written == count)
		return written;

	pos += written;
	count -= written;
	iov_iter_init(&i, iov, nr_segs, count, written);
	written_buffered = __btrfs_buffered_write(file, &i, pos);
	if (written_buffered < 0) {
		err = written_buffered;
		goto out;
	}
	endbyte = pos + written_buffered - 1;
	err = btrfs_fdatawrite_range(inode, pos, endbyte);
	if (err)
		goto out;
	err = filemap_fdatawait_range(inode->i_mapping, pos, endbyte);
	if (err)
		goto out;
	written += written_buffered;
	*ppos = pos + written_buffered;
	invalidate_mapping_pages(file->f_mapping, pos >> PAGE_CACHE_SHIFT,
				 endbyte >> PAGE_CACHE_SHIFT);
out:
	return written ? written : err;
}

static void update_time_for_write(struct inode *inode)
{
	struct timespec now;

	if (IS_NOCMTIME(inode))
		return;

	now = current_fs_time(inode->i_sb);
	if (!timespec_equal(&inode->i_mtime, &now))
		inode->i_mtime = now;

	if (!timespec_equal(&inode->i_ctime, &now))
		inode->i_ctime = now;

	if (IS_I_VERSION(inode))
		inode_inc_iversion(inode);
}

static ssize_t btrfs_file_aio_write(struct kiocb *iocb,
				    const struct iovec *iov,
				    unsigned long nr_segs, loff_t pos)
{
	struct file *file = iocb->ki_filp;
	struct inode *inode = file_inode(file);
	struct btrfs_root *root = BTRFS_I(inode)->root;
	loff_t *ppos = &iocb->ki_pos;
	u64 start_pos;
	u64 end_pos;
	ssize_t num_written = 0;
	ssize_t err = 0;
	size_t count, ocount;
	bool sync = (file->f_flags & O_DSYNC) || IS_SYNC(file->f_mapping->host);

	mutex_lock(&inode->i_mutex);

	err = generic_segment_checks(iov, &nr_segs, &ocount, VERIFY_READ);
	if (err) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}
	count = ocount;

	current->backing_dev_info = inode->i_mapping->backing_dev_info;
	err = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (err) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	if (count == 0) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	err = file_remove_suid(file);
	if (err) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	if (test_bit(BTRFS_FS_STATE_ERROR, &root->fs_info->fs_state)) {
		mutex_unlock(&inode->i_mutex);
		err = -EROFS;
		goto out;
	}

	update_time_for_write(inode);

	start_pos = round_down(pos, root->sectorsize);
	if (start_pos > i_size_read(inode)) {
		 
		end_pos = round_up(pos + count, root->sectorsize);
		err = btrfs_cont_expand(inode, i_size_read(inode), end_pos);
		if (err) {
			mutex_unlock(&inode->i_mutex);
			goto out;
		}
	}

	if (sync)
		atomic_inc(&BTRFS_I(inode)->sync_writers);

	if (unlikely(file->f_flags & O_DIRECT)) {
		num_written = __btrfs_direct_write(iocb, iov, nr_segs,
						   pos, ppos, count, ocount);
	} else {
		struct iov_iter i;

		iov_iter_init(&i, iov, nr_segs, count, num_written);

		num_written = __btrfs_buffered_write(file, &i, pos);
		if (num_written > 0)
			*ppos = pos + num_written;
	}

	mutex_unlock(&inode->i_mutex);

	spin_lock(&BTRFS_I(inode)->lock);
	BTRFS_I(inode)->last_sub_trans = root->log_transid;
	spin_unlock(&BTRFS_I(inode)->lock);
	if (num_written > 0 || num_written == -EIOCBQUEUED) {

		err = generic_write_sync(file, pos, num_written);
		if (err < 0 && num_written > 0)
			num_written = err;
	}

	if (sync)
		atomic_dec(&BTRFS_I(inode)->sync_writers);
out:
	current->backing_dev_info = NULL;
	return num_written ? num_written : err;
}

int btrfs_release_file(struct inode *inode, struct file *filp)
{
	if (filp->private_data)
		btrfs_ioctl_trans_end(filp);
	 
	if (test_and_clear_bit(BTRFS_INODE_ORDERED_DATA_CLOSE,
			       &BTRFS_I(inode)->runtime_flags))
			filemap_flush(inode->i_mapping);
	return 0;
}

static int start_ordered_ops(struct inode *inode, loff_t start, loff_t end)
{
	int ret;

	atomic_inc(&BTRFS_I(inode)->sync_writers);
	ret = btrfs_fdatawrite_range(inode, start, end);
	atomic_dec(&BTRFS_I(inode)->sync_writers);

	return ret;
}

int btrfs_sync_file(struct file *file, loff_t start, loff_t end, int datasync)
{
	struct dentry *dentry = file->f_path.dentry;
	struct inode *inode = dentry->d_inode;
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct btrfs_trans_handle *trans;
	struct btrfs_log_ctx ctx;
	int ret = 0;
	bool full_sync = 0;
	u64 len;

	len = (u64)end - (u64)start + 1;
	trace_btrfs_sync_file(file, datasync);

	ret = start_ordered_ops(inode, start, end);
	if (ret)
		return ret;

	mutex_lock(&inode->i_mutex);
	atomic_inc(&root->log_batch);
	full_sync = test_bit(BTRFS_INODE_NEEDS_FULL_SYNC,
			     &BTRFS_I(inode)->runtime_flags);
	 
	if (full_sync) {
		 
		ret = btrfs_wait_ordered_range(inode, start, len);
	} else {
		 
		ret = start_ordered_ops(inode, start, end);
	}
	if (ret) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}
	atomic_inc(&root->log_batch);

	smp_mb();
	if (btrfs_inode_in_log(inode, root->fs_info->generation) ||
	    (full_sync && BTRFS_I(inode)->last_trans <=
	     root->fs_info->last_trans_committed) ||
	    (!btrfs_have_ordered_extents_in_range(inode, start, len) &&
	     BTRFS_I(inode)->last_trans
	     <= root->fs_info->last_trans_committed)) {
		 
		clear_bit(BTRFS_INODE_NEEDS_FULL_SYNC,
			  &BTRFS_I(inode)->runtime_flags);
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	if (file->private_data)
		btrfs_ioctl_trans_end(file);

	trans = btrfs_start_transaction(root, 0);
	if (IS_ERR(trans)) {
		ret = PTR_ERR(trans);
		mutex_unlock(&inode->i_mutex);
		goto out;
	}
	trans->sync = true;

	btrfs_init_log_ctx(&ctx);

	ret = btrfs_log_dentry_safe(trans, root, dentry, start, end, &ctx);
	if (ret < 0) {
		 
		ret = 1;
	}

	mutex_unlock(&inode->i_mutex);

	if (ctx.io_err) {
		btrfs_end_transaction(trans, root);
		ret = ctx.io_err;
		goto out;
	}

	if (ret != BTRFS_NO_LOG_SYNC) {
		if (!ret) {
			ret = btrfs_sync_log(trans, root, &ctx);
			if (!ret) {
				ret = btrfs_end_transaction(trans, root);
				goto out;
			}
		}
		if (!full_sync) {
			ret = btrfs_wait_ordered_range(inode, start, len);
			if (ret) {
				btrfs_end_transaction(trans, root);
				goto out;
			}
		}
		ret = btrfs_commit_transaction(trans, root);
	} else {
		ret = btrfs_end_transaction(trans, root);
	}
out:
	return ret > 0 ? -EIO : ret;
}

static const struct vm_operations_struct btrfs_file_vm_ops = {
	.fault		= filemap_fault,
	.page_mkwrite	= btrfs_page_mkwrite,
	.remap_pages	= generic_file_remap_pages,
};

static int btrfs_file_mmap(struct file	*filp, struct vm_area_struct *vma)
{
	struct address_space *mapping = filp->f_mapping;

	if (!mapping->a_ops->readpage)
		return -ENOEXEC;

	file_accessed(filp);
	vma->vm_ops = &btrfs_file_vm_ops;

	return 0;
}

static int hole_mergeable(struct inode *inode, struct extent_buffer *leaf,
			  int slot, u64 start, u64 end)
{
	struct btrfs_file_extent_item *fi;
	struct btrfs_key key;

	if (slot < 0 || slot >= btrfs_header_nritems(leaf))
		return 0;

	btrfs_item_key_to_cpu(leaf, &key, slot);
	if (key.objectid != btrfs_ino(inode) ||
	    key.type != BTRFS_EXTENT_DATA_KEY)
		return 0;

	fi = btrfs_item_ptr(leaf, slot, struct btrfs_file_extent_item);

	if (btrfs_file_extent_type(leaf, fi) != BTRFS_FILE_EXTENT_REG)
		return 0;

	if (btrfs_file_extent_disk_bytenr(leaf, fi))
		return 0;

	if (key.offset == end)
		return 1;
	if (key.offset + btrfs_file_extent_num_bytes(leaf, fi) == start)
		return 1;
	return 0;
}

static int fill_holes(struct btrfs_trans_handle *trans, struct inode *inode,
		      struct btrfs_path *path, u64 offset, u64 end)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct extent_buffer *leaf;
	struct btrfs_file_extent_item *fi;
	struct extent_map *hole_em;
	struct extent_map_tree *em_tree = &BTRFS_I(inode)->extent_tree;
	struct btrfs_key key;
	int ret;
#ifdef MY_ABC_HERE
	int modify_slot = -1;
	int del_slot = -1;
	int update_offset = 0;
	u64 num_bytes = 0;
#endif  

	if (btrfs_fs_incompat(root->fs_info, NO_HOLES))
		goto out;

	key.objectid = btrfs_ino(inode);
	key.type = BTRFS_EXTENT_DATA_KEY;
	key.offset = offset;

#ifdef MY_ABC_HERE
	ret = btrfs_search_slot(trans, root, &key, path, -1, 1);
#else
	ret = btrfs_search_slot(trans, root, &key, path, 0, 1);
#endif  
	if (ret < 0)
		return ret;
	BUG_ON(!ret);

	leaf = path->nodes[0];
#ifdef MY_ABC_HERE
	if (hole_mergeable(inode, leaf, path->slots[0] - 1, offset, end)) {
		fi = btrfs_item_ptr(leaf, path->slots[0] - 1,
				    struct btrfs_file_extent_item);
		num_bytes = btrfs_file_extent_num_bytes(leaf, fi) + end - offset;
		modify_slot = path->slots[0] - 1;
	}

	if (hole_mergeable(inode, leaf, path->slots[0], offset, end)) {
		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		if (modify_slot != -1) {
			num_bytes += btrfs_file_extent_num_bytes(leaf, fi);
			del_slot = path->slots[0];
		} else {
			num_bytes = btrfs_file_extent_num_bytes(leaf, fi) + end - offset;
			modify_slot = path->slots[0];
			update_offset = 1;
		}
	}
	if (modify_slot >= 0)
		goto out;
#else
	if (hole_mergeable(inode, leaf, path->slots[0]-1, offset, end)) {
		u64 num_bytes;

		path->slots[0]--;
		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		num_bytes = btrfs_file_extent_num_bytes(leaf, fi) +
			end - offset;
		btrfs_set_file_extent_num_bytes(leaf, fi, num_bytes);
		btrfs_set_file_extent_ram_bytes(leaf, fi, num_bytes);
		btrfs_set_file_extent_offset(leaf, fi, 0);
		btrfs_mark_buffer_dirty(leaf);
		goto out;
	}

	if (hole_mergeable(inode, leaf, path->slots[0], offset, end)) {
		u64 num_bytes;

		key.offset = offset;
		btrfs_set_item_key_safe(root, path, &key);
		fi = btrfs_item_ptr(leaf, path->slots[0],
				    struct btrfs_file_extent_item);
		num_bytes = btrfs_file_extent_num_bytes(leaf, fi) + end -
			offset;
		btrfs_set_file_extent_num_bytes(leaf, fi, num_bytes);
		btrfs_set_file_extent_ram_bytes(leaf, fi, num_bytes);
		btrfs_set_file_extent_offset(leaf, fi, 0);
		btrfs_mark_buffer_dirty(leaf);
		goto out;
	}
#endif  
	btrfs_release_path(path);

	ret = btrfs_insert_file_extent(trans, root, btrfs_ino(inode), offset,
				       0, 0, end - offset, 0, end - offset,
				       0, 0, 0);
	if (ret)
		return ret;

out:
#ifdef MY_ABC_HERE
	if (modify_slot >= 0) {
		fi = btrfs_item_ptr(leaf, modify_slot, struct btrfs_file_extent_item);

		btrfs_set_file_extent_num_bytes(leaf, fi, num_bytes);
		btrfs_set_file_extent_ram_bytes(leaf, fi, num_bytes);
		if (update_offset) {
			key.offset = offset;
			btrfs_set_item_key_safe(root, path, &key);
		}
		btrfs_set_file_extent_offset(leaf, fi, 0);
		btrfs_mark_buffer_dirty(leaf);
		if (del_slot >= 0) {
			ret = btrfs_del_items(trans, root, path, del_slot, 1);
			if (ret) {
				btrfs_abort_transaction(trans, root, ret);
				btrfs_release_path(path);
				return ret;
			}
		}
	}

#endif  
	btrfs_release_path(path);

	hole_em = alloc_extent_map();
	if (!hole_em) {
		btrfs_drop_extent_cache(inode, offset, end - 1, 0);
		set_bit(BTRFS_INODE_NEEDS_FULL_SYNC,
			&BTRFS_I(inode)->runtime_flags);
	} else {
		hole_em->start = offset;
		hole_em->len = end - offset;
		hole_em->ram_bytes = hole_em->len;
		hole_em->orig_start = offset;

		hole_em->block_start = EXTENT_MAP_HOLE;
		hole_em->block_len = 0;
		hole_em->orig_block_len = 0;
		hole_em->bdev = root->fs_info->fs_devices->latest_bdev;
		hole_em->compress_type = BTRFS_COMPRESS_NONE;
		hole_em->generation = trans->transid;

		do {
			btrfs_drop_extent_cache(inode, offset, end - 1, 0);
			write_lock(&em_tree->lock);
			ret = add_extent_mapping(em_tree, hole_em, 1);
			write_unlock(&em_tree->lock);
		} while (ret == -EEXIST);
		free_extent_map(hole_em);
		if (ret)
			set_bit(BTRFS_INODE_NEEDS_FULL_SYNC,
				&BTRFS_I(inode)->runtime_flags);
	}

	return 0;
}

static int find_first_non_hole(struct inode *inode, u64 *start, u64 *len)
{
	struct extent_map *em;
	int ret = 0;

	em = btrfs_get_extent(inode, NULL, 0, *start, *len, 0);
	if (IS_ERR_OR_NULL(em)) {
		if (!em)
			ret = -ENOMEM;
		else
			ret = PTR_ERR(em);
		return ret;
	}

	if (em->block_start == EXTENT_MAP_HOLE) {
		ret = 1;
		*len = em->start + em->len > *start + *len ?
		       0 : *start + *len - em->start - em->len;
		*start = em->start + em->len;
	}
	free_extent_map(em);
	return ret;
}

static int btrfs_punch_hole(struct inode *inode, loff_t offset, loff_t len)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct extent_state *cached_state = NULL;
	struct btrfs_path *path;
	struct btrfs_block_rsv *rsv;
	struct btrfs_trans_handle *trans;
	u64 lockstart;
	u64 lockend;
	u64 tail_start;
	u64 tail_len;
	u64 orig_start = offset;
	u64 cur_offset;
#ifdef MY_ABC_HERE
	u64 min_size = btrfs_calc_trans_metadata_size(root, 1);
#else
	u64 min_size = btrfs_calc_trunc_metadata_size(root, 1);
#endif  
	u64 drop_end;
	int ret = 0;
	int err = 0;
	int rsv_count;
	bool same_page;
	bool no_holes = btrfs_fs_incompat(root->fs_info, NO_HOLES);
	u64 ino_size;

	ret = btrfs_wait_ordered_range(inode, offset, len);
	if (ret)
		return ret;

	mutex_lock(&inode->i_mutex);
	ino_size = round_up(inode->i_size, PAGE_CACHE_SIZE);
	ret = find_first_non_hole(inode, &offset, &len);
	if (ret < 0)
		goto out_only_mutex;
	if (ret && !len) {
		 
		ret = 0;
		goto out_only_mutex;
	}

	lockstart = round_up(offset , BTRFS_I(inode)->root->sectorsize);
	lockend = round_down(offset + len,
			     BTRFS_I(inode)->root->sectorsize) - 1;
	same_page = ((offset >> PAGE_CACHE_SHIFT) ==
		    ((offset + len - 1) >> PAGE_CACHE_SHIFT));

	if (same_page && len < PAGE_CACHE_SIZE) {
		if (offset < ino_size)
			ret = btrfs_truncate_page(inode, offset, len, 0);
		goto out_only_mutex;
	}

	if (offset < ino_size) {
		ret = btrfs_truncate_page(inode, offset, 0, 0);
		if (ret) {
			mutex_unlock(&inode->i_mutex);
			return ret;
		}
	}

	if (offset == orig_start) {
		 
		len = offset + len - lockstart;
		offset = lockstart;
		ret = find_first_non_hole(inode, &offset, &len);
		if (ret < 0)
			goto out_only_mutex;
		if (ret && !len) {
			ret = 0;
			goto out_only_mutex;
		}
		lockstart = offset;
	}

	tail_start = lockend + 1;
	tail_len = offset + len - tail_start;
	if (tail_len) {
		ret = find_first_non_hole(inode, &tail_start, &tail_len);
		if (unlikely(ret < 0))
			goto out_only_mutex;
		if (!ret) {
			 
			if (tail_start + tail_len < ino_size) {
				ret = btrfs_truncate_page(inode,
						tail_start + tail_len, 0, 1);
				if (ret)
					goto out_only_mutex;
				}
		}
	}

	if (lockend < lockstart) {
		mutex_unlock(&inode->i_mutex);
		return 0;
	}

	while (1) {
		struct btrfs_ordered_extent *ordered;

		truncate_pagecache_range(inode, lockstart, lockend);

		lock_extent_bits(&BTRFS_I(inode)->io_tree, lockstart, lockend,
				 0, &cached_state);
		ordered = btrfs_lookup_first_ordered_extent(inode, lockend);

		if ((!ordered ||
		    (ordered->file_offset + ordered->len <= lockstart ||
		     ordered->file_offset > lockend)) &&
		     !btrfs_page_exists_in_range(inode, lockstart, lockend)) {
			if (ordered)
				btrfs_put_ordered_extent(ordered);
			break;
		}
		if (ordered)
			btrfs_put_ordered_extent(ordered);
		unlock_extent_cached(&BTRFS_I(inode)->io_tree, lockstart,
				     lockend, &cached_state, GFP_NOFS);
		ret = btrfs_wait_ordered_range(inode, lockstart,
					       lockend - lockstart + 1);
		if (ret) {
			mutex_unlock(&inode->i_mutex);
			return ret;
		}
	}

	path = btrfs_alloc_path();
	if (!path) {
		ret = -ENOMEM;
		goto out;
	}

	rsv = btrfs_alloc_block_rsv(root, BTRFS_BLOCK_RSV_TEMP);
	if (!rsv) {
		ret = -ENOMEM;
		goto out_free;
	}
#ifdef MY_ABC_HERE
	rsv->size = btrfs_calc_trans_metadata_size(root, 1);
#else
	rsv->size = btrfs_calc_trunc_metadata_size(root, 1);
#endif  
	rsv->failfast = 1;

	rsv_count = no_holes ? 2 : 3;
	trans = btrfs_start_transaction(root, rsv_count);
	if (IS_ERR(trans)) {
		err = PTR_ERR(trans);
		goto out_free;
	}

	ret = btrfs_block_rsv_migrate(&root->fs_info->trans_block_rsv, rsv,
				      min_size);
	BUG_ON(ret);
	trans->block_rsv = rsv;

	cur_offset = lockstart;
	len = lockend - cur_offset;
	while (cur_offset < lockend) {
		ret = __btrfs_drop_extents(trans, root, inode, path,
					   cur_offset, lockend + 1,
					   &drop_end, 1, 0, 0, NULL);
		if (ret != -ENOSPC)
			break;

		trans->block_rsv = &root->fs_info->trans_block_rsv;

		if (cur_offset < ino_size) {
			ret = fill_holes(trans, inode, path, cur_offset,
					 drop_end);
			if (ret) {
				err = ret;
				break;
			}
		}

		cur_offset = drop_end;

		ret = btrfs_update_inode(trans, root, inode);
		if (ret) {
			err = ret;
			break;
		}

		btrfs_end_transaction(trans, root);
		btrfs_btree_balance_dirty(root);

		trans = btrfs_start_transaction(root, rsv_count);
		if (IS_ERR(trans)) {
			ret = PTR_ERR(trans);
			trans = NULL;
			break;
		}

		ret = btrfs_block_rsv_migrate(&root->fs_info->trans_block_rsv,
					      rsv, min_size);
		BUG_ON(ret);	 
		trans->block_rsv = rsv;

		ret = find_first_non_hole(inode, &cur_offset, &len);
		if (unlikely(ret < 0))
			break;
		if (ret && !len) {
			ret = 0;
			break;
		}
	}

	if (ret) {
		err = ret;
		goto out_trans;
	}

	trans->block_rsv = &root->fs_info->trans_block_rsv;
	 
	if (cur_offset < ino_size && cur_offset < drop_end) {
		ret = fill_holes(trans, inode, path, cur_offset, drop_end);
		if (ret) {
			err = ret;
			goto out_trans;
		}
	}

out_trans:
	if (!trans)
		goto out_free;

	inode_inc_iversion(inode);
	inode->i_mtime = inode->i_ctime = CURRENT_TIME;

	trans->block_rsv = &root->fs_info->trans_block_rsv;
	ret = btrfs_update_inode(trans, root, inode);
	btrfs_end_transaction(trans, root);
	btrfs_btree_balance_dirty(root);
out_free:
	btrfs_free_path(path);
	btrfs_free_block_rsv(root, rsv);
out:
	unlock_extent_cached(&BTRFS_I(inode)->io_tree, lockstart, lockend,
			     &cached_state, GFP_NOFS);
out_only_mutex:
	mutex_unlock(&inode->i_mutex);
	if (ret && !err)
		err = ret;
	return err;
}

static long btrfs_fallocate(struct file *file, int mode,
			    loff_t offset, loff_t len)
{
	struct inode *inode = file_inode(file);
	struct extent_state *cached_state = NULL;
	struct btrfs_root *root = BTRFS_I(inode)->root;
	u64 cur_offset;
	u64 last_byte;
	u64 alloc_start;
	u64 alloc_end;
	u64 alloc_hint = 0;
	u64 locked_end;
	struct extent_map *em;
	int blocksize = BTRFS_I(inode)->root->sectorsize;
	int ret;

	alloc_start = round_down(offset, blocksize);
	alloc_end = round_up(offset + len, blocksize);

	if (mode & ~(FALLOC_FL_KEEP_SIZE | FALLOC_FL_PUNCH_HOLE))
		return -EOPNOTSUPP;

	if (mode & FALLOC_FL_PUNCH_HOLE)
		return btrfs_punch_hole(inode, offset, len);

	ret = btrfs_check_data_free_space(inode, alloc_end - alloc_start);
	if (ret)
		return ret;
	if (root->fs_info->quota_enabled) {
		ret = btrfs_qgroup_reserve(root, alloc_end - alloc_start);
		if (ret)
			goto out_reserve_fail;
	}

	mutex_lock(&inode->i_mutex);
	ret = inode_newsize_ok(inode, alloc_end);
	if (ret)
		goto out;

	if (alloc_start > inode->i_size) {
		ret = btrfs_cont_expand(inode, i_size_read(inode),
					alloc_start);
		if (ret)
			goto out;
	} else {
		 
		ret = btrfs_truncate_page(inode, inode->i_size, 0, 0);
		if (ret)
			goto out;
	}

	ret = btrfs_wait_ordered_range(inode, alloc_start,
				       alloc_end - alloc_start);
	if (ret)
		goto out;

	locked_end = alloc_end - 1;
	while (1) {
		struct btrfs_ordered_extent *ordered;

		lock_extent_bits(&BTRFS_I(inode)->io_tree, alloc_start,
				 locked_end, 0, &cached_state);
		ordered = btrfs_lookup_first_ordered_extent(inode,
							    alloc_end - 1);
		if (ordered &&
		    ordered->file_offset + ordered->len > alloc_start &&
		    ordered->file_offset < alloc_end) {
			btrfs_put_ordered_extent(ordered);
			unlock_extent_cached(&BTRFS_I(inode)->io_tree,
					     alloc_start, locked_end,
					     &cached_state, GFP_NOFS);
			 
			ret = btrfs_wait_ordered_range(inode, alloc_start,
						       alloc_end - alloc_start);
			if (ret)
				goto out;
		} else {
			if (ordered)
				btrfs_put_ordered_extent(ordered);
			break;
		}
	}

	cur_offset = alloc_start;
	while (1) {
		u64 actual_end;

		em = btrfs_get_extent(inode, NULL, 0, cur_offset,
				      alloc_end - cur_offset, 0);
		if (IS_ERR_OR_NULL(em)) {
			if (!em)
				ret = -ENOMEM;
			else
				ret = PTR_ERR(em);
			break;
		}
		last_byte = min(extent_map_end(em), alloc_end);
		actual_end = min_t(u64, extent_map_end(em), offset + len);
		last_byte = ALIGN(last_byte, blocksize);

		if (em->block_start == EXTENT_MAP_HOLE ||
		    (cur_offset >= inode->i_size &&
		     !test_bit(EXTENT_FLAG_PREALLOC, &em->flags))) {
			ret = btrfs_prealloc_file_range(inode, mode, cur_offset,
							last_byte - cur_offset,
							1 << inode->i_blkbits,
							offset + len,
							&alloc_hint);

			if (ret < 0) {
				free_extent_map(em);
				break;
			}
		} else if (actual_end > inode->i_size &&
			   !(mode & FALLOC_FL_KEEP_SIZE)) {
			 
			inode->i_ctime = CURRENT_TIME;
			i_size_write(inode, actual_end);
			btrfs_ordered_update_i_size(inode, actual_end, NULL);
		}
		free_extent_map(em);

		cur_offset = last_byte;
		if (cur_offset >= alloc_end) {
			ret = 0;
			break;
		}
	}
	unlock_extent_cached(&BTRFS_I(inode)->io_tree, alloc_start, locked_end,
			     &cached_state, GFP_NOFS);
out:
	mutex_unlock(&inode->i_mutex);
	if (root->fs_info->quota_enabled)
		btrfs_qgroup_free(root, alloc_end - alloc_start);
out_reserve_fail:
	 
	btrfs_free_reserved_data_space(inode, alloc_end - alloc_start);
	return ret;
}

static int find_desired_extent(struct inode *inode, loff_t *offset, int whence)
{
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct extent_map *em = NULL;
	struct extent_state *cached_state = NULL;
	u64 lockstart = *offset;
	u64 lockend = i_size_read(inode);
	u64 start = *offset;
	u64 len = i_size_read(inode);
	int ret = 0;

	lockend = max_t(u64, root->sectorsize, lockend);
	if (lockend <= lockstart)
		lockend = lockstart + root->sectorsize;

	lockend--;
	len = lockend - lockstart + 1;

	len = max_t(u64, len, root->sectorsize);
	if (inode->i_size == 0)
		return -ENXIO;

	lock_extent_bits(&BTRFS_I(inode)->io_tree, lockstart, lockend, 0,
			 &cached_state);

	while (start < inode->i_size) {
		em = btrfs_get_extent_fiemap(inode, NULL, 0, start, len, 0);
		if (IS_ERR(em)) {
			ret = PTR_ERR(em);
			em = NULL;
			break;
		}

		if (whence == SEEK_HOLE &&
		    (em->block_start == EXTENT_MAP_HOLE ||
		     test_bit(EXTENT_FLAG_PREALLOC, &em->flags)))
			break;
		else if (whence == SEEK_DATA &&
			   (em->block_start != EXTENT_MAP_HOLE &&
			    !test_bit(EXTENT_FLAG_PREALLOC, &em->flags)))
			break;

		start = em->start + em->len;
		free_extent_map(em);
		em = NULL;
		cond_resched();
	}
	free_extent_map(em);
	if (!ret) {
		if (whence == SEEK_DATA && start >= inode->i_size)
			ret = -ENXIO;
		else
			*offset = min_t(loff_t, start, inode->i_size);
	}
	unlock_extent_cached(&BTRFS_I(inode)->io_tree, lockstart, lockend,
			     &cached_state, GFP_NOFS);
	return ret;
}

static loff_t btrfs_file_llseek(struct file *file, loff_t offset, int whence)
{
	struct inode *inode = file->f_mapping->host;
	int ret;

	mutex_lock(&inode->i_mutex);
	switch (whence) {
	case SEEK_END:
	case SEEK_CUR:
		offset = generic_file_llseek(file, offset, whence);
		goto out;
	case SEEK_DATA:
	case SEEK_HOLE:
		if (offset >= i_size_read(inode)) {
			mutex_unlock(&inode->i_mutex);
			return -ENXIO;
		}

		ret = find_desired_extent(inode, &offset, whence);
		if (ret) {
			mutex_unlock(&inode->i_mutex);
			return ret;
		}
	}

	if (offset < 0 && !(file->f_mode & FMODE_UNSIGNED_OFFSET)) {
		offset = -EINVAL;
		goto out;
	}
	if (offset > inode->i_sb->s_maxbytes) {
		offset = -EINVAL;
		goto out;
	}

	if (offset != file->f_pos) {
		file->f_pos = offset;
		file->f_version = 0;
	}
out:
	mutex_unlock(&inode->i_mutex);
	return offset;
}

#if defined(MY_DEF_HERE) && !defined(MY_DEF_HERE)
#define MSG_KERNSPACE       0x1000000
#define MSG_NOCATCHSIG   0x2000000

static ssize_t btrfs_splice_from_socket(struct file *file, struct socket *sock,
					loff_t __user *ppos, size_t count)
{
#if defined(MY_DEF_HERE)
	struct inode *inode = file_inode(file);
#else  
	struct inode *inode = fdentry(file)->d_inode;
#endif  
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct page **pages = NULL;
	struct kvec *iov = NULL;
	struct msghdr msg;
	long recvtimeo;
	ssize_t copied = 0;
	size_t offset, offset_tmp;
	int num_pages, dirty_pages;
	int err = 0;
	loff_t start_pos;
	loff_t pos = file->f_pos;
	int i;
	unsigned count_tmp = count;
	bool sync = (file->f_flags & O_DSYNC) || IS_SYNC(file->f_mapping->host);

#define ERROR_OUT do {mutex_unlock(&inode->i_mutex); goto out;} while(0)

	if (!count)
		return 0;

	if (ppos && copy_from_user(&pos, ppos, sizeof pos))
		return -EFAULT;
	offset = pos & (PAGE_CACHE_SIZE - 1);
	num_pages = (offset + count + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;
	start_pos = round_down(pos, root->sectorsize);

	if (!(pages = kmalloc(num_pages * sizeof(struct page *), GFP_KERNEL)) ||
		!(iov = kmalloc(num_pages * sizeof(*iov), GFP_KERNEL)))
		ERROR_OUT;

	current->backing_dev_info = inode->i_mapping->backing_dev_info;

	mutex_lock(&inode->i_mutex);

	if ((err = generic_write_checks(file, &pos, &count,
					S_ISBLK(inode->i_mode))))
		ERROR_OUT;

	if ((err = file_remove_suid(file)))
		ERROR_OUT;

	if (root->fs_info->fs_state & BTRFS_SUPER_FLAG_ERROR) {
                err = -EROFS;
		ERROR_OUT;
	}

	update_time_for_write(inode);

	if (start_pos > i_size_read(inode) &&
		(err = btrfs_cont_expand(inode, i_size_read(inode), start_pos)))
		ERROR_OUT;

	if ((err = btrfs_delalloc_reserve_space(inode,
					num_pages << PAGE_CACHE_SHIFT)))
		goto out_free;

#if defined(MY_DEF_HERE)
	if ((err = prepare_pages(file_inode(file), pages, num_pages,
					pos, count, false))) {
#else  
	if ((err = prepare_pages(root, file, pages, num_pages,
					pos, pos >> PAGE_CACHE_SHIFT,
					count, false))) {
#endif  
		btrfs_delalloc_release_space(inode,
					num_pages << PAGE_CACHE_SHIFT);
		goto out_free;
	}

	for (i = 0, offset_tmp = offset; i < num_pages; i++) {
		unsigned bytes = PAGE_CACHE_SIZE - offset_tmp;

		if (bytes > count_tmp)
			bytes = count_tmp;
		iov[i].iov_base = kmap(pages[i]) + offset_tmp;
		iov[i].iov_len = bytes;
		offset_tmp = 0;
		count_tmp -= bytes;
	}

	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = (struct iovec *)&iov[0];
	msg.msg_iovlen = num_pages;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = MSG_KERNSPACE;

	recvtimeo = sock->sk->sk_rcvtimeo;
	sock->sk->sk_rcvtimeo = 8 * HZ;
	copied = kernel_recvmsg(sock, &msg, iov, num_pages, count,
#if defined(MY_DEF_HERE)
                             MSG_WAITALL | MSG_NOCATCHSIGNAL);
#else  
                             MSG_WAITALL | MSG_NOCATCHSIG);
#endif  
	sock->sk->sk_rcvtimeo = recvtimeo;

	if (copied < 0) {
		err = copied;
		copied = 0;
	}

	dirty_pages = (copied + offset + PAGE_CACHE_SIZE - 1) >>
					PAGE_CACHE_SHIFT;

	for (i = 0; i < num_pages; i++)
		kunmap(pages[i]);
	if (dirty_pages < num_pages) {
		if (dirty_pages) {
			spin_lock(&BTRFS_I(inode)->lock);
			BTRFS_I(inode)->outstanding_extents++;
			spin_unlock(&BTRFS_I(inode)->lock);
		}
		btrfs_delalloc_release_space(inode,
                                        (num_pages - dirty_pages) <<
                                        PAGE_CACHE_SHIFT);
	}

	if (dirty_pages) {
		if ((err = btrfs_dirty_pages(root, inode, pages,
					dirty_pages, pos, copied, NULL))) {
			btrfs_delalloc_release_space(inode,
					dirty_pages << PAGE_CACHE_SHIFT);
			btrfs_drop_pages(pages, num_pages);
			goto out_free;
                }
	}

	btrfs_drop_pages(pages, num_pages);
	cond_resched();

	balance_dirty_pages_ratelimited(inode->i_mapping);
	if (dirty_pages < (root->leafsize >> PAGE_CACHE_SHIFT) + 1)
		btrfs_btree_balance_dirty(root);

	pos += copied;

out_free:
	mutex_unlock(&inode->i_mutex);

	if (copied > 0) {
		file->f_pos = pos;
		if (ppos && copy_to_user(ppos, &pos, sizeof *ppos))
			err = -EFAULT;
	}

	BTRFS_I(inode)->last_trans = root->fs_info->generation + 1;
	BTRFS_I(inode)->last_sub_trans = root->log_transid;
	if (copied > 0 || err == -EIOCBQUEUED)
		err = generic_write_sync(file, pos, copied);
	if (sync)
		atomic_dec(&BTRFS_I(inode)->sync_writers);
out:
	kfree(iov);
	kfree(pages);
	current->backing_dev_info = NULL;

	return err ? err : copied;
}
#endif  

#if defined(MY_ABC_HERE)
#if defined(MY_ABC_HERE)
extern int rw_verify_area(int read_write, struct file *file, loff_t *ppos, size_t count);
#endif  

static ssize_t btrfs_splice_from_socket(struct file *file, struct socket *sock,
					loff_t __user *ppos, size_t count)
{
	struct inode *inode = file_inode(file);
	struct btrfs_root *root = BTRFS_I(inode)->root;
	struct page **pages = NULL;
	struct kvec *iov = NULL;
	struct msghdr msg = { 0 };
	size_t offset, offset_tmp, remaining;
	size_t num_pages, dirty_pages;
	size_t copied = 0;
	u64 start_pos;
	int ret = 0;
	loff_t pos;
	int i;
	bool sync = (file->f_flags & O_DSYNC) || IS_SYNC(file->f_mapping->host);

	if (ppos && copy_from_user(&pos, ppos, sizeof(pos)))
		return -EFAULT;

	ret = rw_verify_area(WRITE, file, &pos, count);
	if (ret < 0)
		return ret;

	count = min_t(size_t, ret, SZ_128K);

	mutex_lock(&inode->i_mutex);

	current->backing_dev_info = inode->i_mapping->backing_dev_info;
	ret = generic_write_checks(file, &pos, &count, S_ISBLK(inode->i_mode));
	if (ret) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	if (count == 0) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	ret = file_remove_suid(file);
	if (ret) {
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	if (test_bit(BTRFS_FS_STATE_ERROR, &root->fs_info->fs_state)) {
		ret = -EROFS;
		mutex_unlock(&inode->i_mutex);
		goto out;
	}

	update_time_for_write(inode);

	start_pos = round_down(pos, root->sectorsize);
	if (start_pos > i_size_read(inode)) {
		ret = btrfs_cont_expand(inode, i_size_read(inode), start_pos);
		if (ret) {
			mutex_unlock(&inode->i_mutex);
			goto out;
		}
	}

	if (sync)
		atomic_inc(&BTRFS_I(inode)->sync_writers);

	offset = pos & (PAGE_CACHE_SIZE - 1);
	num_pages = (offset + count + PAGE_CACHE_SIZE - 1) >> PAGE_CACHE_SHIFT;

	ret = btrfs_delalloc_reserve_space(inode,
					   num_pages << PAGE_CACHE_SHIFT);
	if (ret)
		goto out_free;

	pages = kmalloc(num_pages * sizeof(*pages), GFP_NOFS);
	if (!pages) {
		ret = -ENOMEM;
		goto out_free;
	}

	iov = kmalloc(num_pages * sizeof(*iov), GFP_NOFS);
	if (!iov) {
		ret = -ENOMEM;
		goto out_free;
	}

#if defined(MY_ABC_HERE)
	ret = prepare_pages(file_inode(file), pages, num_pages,
			    pos, count, false);
#else  
	ret = prepare_pages(root, file, pages, num_pages,
			    pos, 0, count, false);
#endif  
	if (ret) {
		btrfs_delalloc_release_space(inode,
					     num_pages << PAGE_CACHE_SHIFT);
		goto out_free;
	}

	remaining = count;
	offset_tmp = offset;
	for (i = 0; i < num_pages; i++) {
		unsigned int bytes = min_t(unsigned int,
					   PAGE_CACHE_SIZE - offset_tmp, remaining);

		iov[i].iov_base = kmap(pages[i]) + offset_tmp;
		iov[i].iov_len = bytes;
		offset_tmp = 0;
		remaining -= bytes;
	}

	copied = kernel_recvmsg(sock, &msg, iov, num_pages, count, MSG_WAITALL);

	for (i = 0; i < num_pages; i++)
		kunmap(pages[i]);

	if (copied <= 0) {
		ret = copied;
		dirty_pages = copied = 0;
	} else
		dirty_pages = (copied + offset + PAGE_CACHE_SIZE - 1) >>
			PAGE_CACHE_SHIFT;

	if (num_pages > dirty_pages) {
		if (copied > 0) {
			spin_lock(&BTRFS_I(inode)->lock);
			BTRFS_I(inode)->outstanding_extents++;
			spin_unlock(&BTRFS_I(inode)->lock);
		}
		btrfs_delalloc_release_space(inode,
			     (num_pages - dirty_pages) << PAGE_CACHE_SHIFT);
	}

	if (copied > 0) {
		ret = btrfs_dirty_pages(root, inode, pages,
				dirty_pages, pos, copied,
				NULL);
		if (ret) {
			btrfs_delalloc_release_space(inode,
				     dirty_pages << PAGE_CACHE_SHIFT);
			btrfs_drop_pages(pages, num_pages);
			goto out_free;
		}
	}

	btrfs_drop_pages(pages, num_pages);

	cond_resched();

	balance_dirty_pages_ratelimited(inode->i_mapping);
	if (dirty_pages < (root->leafsize >> PAGE_CACHE_SHIFT) + 1)
		btrfs_btree_balance_dirty(root);

out_free:
	kfree(iov);
	kfree(pages);

	mutex_unlock(&inode->i_mutex);

	BTRFS_I(inode)->last_trans = root->fs_info->generation + 1;
	BTRFS_I(inode)->last_sub_trans = root->log_transid;
	if (copied > 0 || ret == -EIOCBQUEUED)
		ret = generic_write_sync(file, pos, copied);

	if (sync)
		atomic_dec(&BTRFS_I(inode)->sync_writers);

	pos += copied;
	if (ppos && copy_to_user(ppos, &pos, sizeof(*ppos)))
		ret = -EFAULT;
out:
	current->backing_dev_info = NULL;
	return ret ? ret : copied;
}
#endif  

const struct file_operations btrfs_file_operations = {
	.llseek		= btrfs_file_llseek,
	.read		= do_sync_read,
	.write		= do_sync_write,
	.aio_read       = generic_file_aio_read,
#if (defined(MY_DEF_HERE) && !defined(MY_DEF_HERE)) || \
	defined(MY_ABC_HERE)
	.splice_from_socket	= btrfs_splice_from_socket,
#endif  
	.splice_read	= generic_file_splice_read,
	.aio_write	= btrfs_file_aio_write,
	.mmap		= btrfs_file_mmap,
	.open		= generic_file_open,
	.release	= btrfs_release_file,
	.fsync		= btrfs_sync_file,
	.fallocate	= btrfs_fallocate,
	.unlocked_ioctl	= btrfs_ioctl,
#ifdef CONFIG_COMPAT
#ifdef MY_DEF_HERE
	.compat_ioctl	= btrfs_compat_ioctl,
#else
	.compat_ioctl	= btrfs_ioctl,
#endif  
#endif
};

void btrfs_auto_defrag_exit(void)
{
	if (btrfs_inode_defrag_cachep)
		kmem_cache_destroy(btrfs_inode_defrag_cachep);
}

int btrfs_auto_defrag_init(void)
{
	btrfs_inode_defrag_cachep = kmem_cache_create("btrfs_inode_defrag",
					sizeof(struct inode_defrag), 0,
					SLAB_RECLAIM_ACCOUNT | SLAB_MEM_SPREAD,
					NULL);
	if (!btrfs_inode_defrag_cachep)
		return -ENOMEM;

	return 0;
}

int btrfs_fdatawrite_range(struct inode *inode, loff_t start, loff_t end)
{
	int ret;

	ret = filemap_fdatawrite_range(inode->i_mapping, start, end);
	if (!ret && test_bit(BTRFS_INODE_HAS_ASYNC_EXTENT,
			     &BTRFS_I(inode)->runtime_flags))
		ret = filemap_fdatawrite_range(inode->i_mapping, start, end);

	return ret;
}
