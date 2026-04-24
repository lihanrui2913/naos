#pragma once

#include <libs/klibc.h>

struct vfs_inode;
typedef struct vfs_inode vfs_node_t;

typedef struct cache_entry cache_entry_t;

typedef struct cache_stats {
    size_t block_pages;
    size_t page_pages;
    size_t dirty_pages;
    size_t writeback_pages;
} cache_stats_t;

cache_entry_t *cache_block_try_get(uint64_t drive, uint64_t page_index);
cache_entry_t *cache_block_get_or_create(uint64_t drive, uint64_t page_index,
                                         bool *created);

cache_entry_t *cache_page_try_get(vfs_node_t *node, uint64_t page_index);
cache_entry_t *cache_page_get_or_create(vfs_node_t *node, uint64_t page_index,
                                        bool *created);

void *cache_entry_data(cache_entry_t *entry);
uint64_t cache_entry_phys(cache_entry_t *entry);
size_t cache_entry_valid_bytes(const cache_entry_t *entry);

void cache_entry_mark_ready(cache_entry_t *entry, size_t valid_bytes);
void cache_entry_extend_valid(cache_entry_t *entry, size_t valid_bytes);
void cache_entry_abort_fill(cache_entry_t *entry);
void cache_entry_put(cache_entry_t *entry);

void cache_page_mark_dirty(cache_entry_t *entry);
void cache_page_clear_dirty(cache_entry_t *entry);
int cache_page_start_writeback(cache_entry_t *entry);
void cache_page_end_writeback(cache_entry_t *entry, int error);

void cache_block_invalidate_range(uint64_t drive, uint64_t start_offset,
                                  uint64_t len);
void cache_block_drop_drive(uint64_t drive);

void cache_page_invalidate_range(vfs_node_t *node, uint64_t start_offset,
                                 uint64_t len);
void cache_page_drop_inode(vfs_node_t *node);

void cache_get_stats(cache_stats_t *stats);

size_t cache_reclaim_pages(size_t target_pages);
