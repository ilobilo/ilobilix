// Copyright (C) 2022  ilobilo

#include <cdi/fs.h>

extern "C"
{
    void cdi_fs_driver_init(cdi_fs_driver *driver);
    void cdi_fs_driver_destroy(cdi_fs_driver *driver);
    void cdi_fs_driver_register(cdi_fs_driver *driver);

    size_t cdi_fs_data_read(cdi_fs_filesystem *fs, uint64_t start, size_t size, void *buffer);
    size_t cdi_fs_data_write(cdi_fs_filesystem *fs, uint64_t start, size_t size, const void *buffer);
} // extern "C"