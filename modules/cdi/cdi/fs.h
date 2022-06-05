/*
 * Copyright (c) 2007 Antoine Kaufmann
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_FS_
#define _CDI_FS_

#include <cdi.h>
#include <cdi/lists.h>
#include <cdi/cache.h>

struct cdi_fs_filesystem;
struct cdi_fs_driver
{
    struct cdi_driver drv;
    int (*fs_probe)(struct cdi_fs_filesystem *fs, char **volname);
    int (*fs_init)(struct cdi_fs_filesystem *fs);
    int (*fs_destroy)(struct cdi_fs_filesystem *fs);
};

struct cdi_fs_res;
struct cdi_fs_filesystem
{
    struct cdi_fs_driver* driver;
    struct cdi_fs_res* root_res;
    int error;
    int read_only;
    void *opaque;
    cdi_fs_osdep osdep;
};

typedef enum
{
    CDI_FS_ERROR_NONE = 0,
    CDI_FS_ERROR_IO,
    CDI_FS_ERROR_ONS,
    CDI_FS_ERROR_RNF,
    CDI_FS_ERROR_EOF,
    CDI_FS_ERROR_RO,
    CDI_FS_ERROR_INTERNAL,
    CDI_FS_ERROR_NOT_IMPLEMENTED,
    CDI_FS_ERROR_UNKNOWN
} cdi_fs_error_t;

struct cdi_fs_stream
{
    struct cdi_fs_filesystem *fs;
    struct cdi_fs_res *res;
    cdi_fs_error_t error;
};

typedef enum
{
    CDI_FS_META_SIZE,
    CDI_FS_META_USEDBLOCKS,
    CDI_FS_META_BESTBLOCKSZ,
    CDI_FS_META_BLOCKSZ,
    CDI_FS_META_CREATETIME,
    CDI_FS_META_ACCESSTIME,
    CDI_FS_META_CHANGETIME
} cdi_fs_meta_t;


struct cdi_fs_res_flags
{
    int remove;
    int rename;
    int move;
    int read;
    int write;
    int execute;
    int browse;
    int read_link;
    int write_link;
    int create_child;
};

struct cdi_fs_res_res;
struct cdi_fs_res_file;
struct cdi_fs_res_dir;
struct cdi_fs_res_link;
struct cdi_fs_res_special;

typedef enum
{
    CDI_FS_BLOCK,
    CDI_FS_CHAR,
    CDI_FS_FIFO,
    CDI_FS_SOCKET
} cdi_fs_res_type_t;

typedef enum
{
    CDI_FS_CLASS_FILE,
    CDI_FS_CLASS_DIR,
    CDI_FS_CLASS_LINK,
    CDI_FS_CLASS_SPECIAL
} cdi_fs_res_class_t;

typedef enum
{
    CDI_FS_LOCK_NONE,
    CDI_FS_LOCK_WRITE,
    CDI_FS_LOCK_ALL
} cdi_fs_lock_t;

struct cdi_fs_res
{
    char* name;
    cdi_fs_lock_t lock;
    int loaded;
    int stream_cnt;
    struct cdi_fs_res* parent;
    cdi_list_t children;
    char* link_path;
    cdi_list_t acl;
    struct cdi_fs_res_flags flags;
    struct cdi_fs_res_res* res;
    struct cdi_fs_res_file* file;
    struct cdi_fs_res_dir* dir;
    struct cdi_fs_res_link* link;
    struct cdi_fs_res_special* special;
    cdi_fs_res_type_t type;
};

struct cdi_fs_res_res
{
    int (*load)(struct cdi_fs_stream *stream);
    int (*unload)(struct cdi_fs_stream *stream);
    int (*remove)(struct cdi_fs_stream *stream);
    int (*rename)(struct cdi_fs_stream *stream, const char *name);
    int (*move)(struct cdi_fs_stream *stream, struct cdi_fs_res *dest);
    int (*assign_class)(struct cdi_fs_stream *stream, cdi_fs_res_class_t _class);
    int (*remove_class)(struct cdi_fs_stream *stream, cdi_fs_res_class_t _class);
    int64_t (*meta_read)(struct cdi_fs_stream *stream, cdi_fs_meta_t meta);
    int (*meta_write)(struct cdi_fs_stream *stream, cdi_fs_meta_t meta, int64_t value);
};

struct cdi_fs_res_file
{
    int executable;
    size_t (*read)(struct cdi_fs_stream *stream, uint64_t start, size_t size, void *buffer);
    size_t (*write)(struct cdi_fs_stream *stream, uint64_t start, size_t size, const void *buffer);
    int (*truncate)(struct cdi_fs_stream *stream, uint64_t size);
};

struct cdi_fs_res_dir
{
    cdi_list_t (*list)(struct cdi_fs_stream *stream);
    int (*create_child)(struct cdi_fs_stream *stream, const char *name, struct cdi_fs_res *parent);
};

struct cdi_fs_res_link
{
    const char *(*read_link)(struct cdi_fs_stream *stream);
    int (*write_link)(struct cdi_fs_stream *stream, const char *path);
};

struct cdi_fs_res_special
{
    int (*dev_read)(struct cdi_fs_stream *stream, uint64_t *dev);
    int (*dev_write)(struct cdi_fs_stream *stream, uint64_t dev);
};

typedef enum
{
    CDI_FS_ACL_USER_NUMERIC,
    CDI_FS_ACL_USER_STRING,
    CDI_FS_ACL_GROUP_NUMERIC,
    CDI_FS_ACL_GROUP_STRING
} cdi_fs_acl_entry_type_t;

struct cdi_fs_acl_entry
{
    cdi_fs_acl_entry_type_t type;
    struct cdi_fs_res_flags flags;
};

struct cdi_fs_acl_entry_usr_num
{
    struct cdi_fs_acl_entry entry;
    int user_id;
};

struct cdi_fs_acl_entry_usr_str
{
    struct cdi_fs_acl_entry entry;
    char *user_name;
};

struct cdi_fs_acl_entry_grp_num
{
    struct cdi_fs_acl_entry entry;
    int group_id;
};

struct cdi_fs_acl_entry_grp_str
{
    struct cdi_fs_acl_entry entry;
    char* group_name;
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_fs_driver_init(struct cdi_fs_driver *driver);
void cdi_fs_driver_destroy(struct cdi_fs_driver *driver);
void cdi_fs_driver_register(struct cdi_fs_driver *driver);

size_t cdi_fs_data_read(struct cdi_fs_filesystem *fs, uint64_t start, size_t size, void *buffer);
size_t cdi_fs_data_write(struct cdi_fs_filesystem *fs, uint64_t start, size_t size, const void *buffer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif