// Copyright (C) 2022  ilobilo

#pragma once

#include <lib/endian.hpp>

namespace dtb
{
    namespace fdt
    {
        struct header
        {
            endian_storage<uint32_t, endian::big> magic;
            endian_storage<uint32_t, endian::big> totalsize;
            endian_storage<uint32_t, endian::big> off_dt_struct;
            endian_storage<uint32_t, endian::big> off_dt_strings;
            endian_storage<uint32_t, endian::big> off_mem_rsvmap;
            endian_storage<uint32_t, endian::big> version;
            endian_storage<uint32_t, endian::big> last_comp_version;
            endian_storage<uint32_t, endian::big> boot_cpuid_phys;
            endian_storage<uint32_t, endian::big> size_dt_strings;
            endian_storage<uint32_t, endian::big> size_dt_struct;
        };

        struct reserve_entry
        {
            endian_storage<uint64_t, endian::big> address;
            endian_storage<uint64_t, endian::big> size;
        };
    } // namespace fdt

    // TODO: DTB
    struct device_tree
    {
        device_tree(void *data) { }
    };

    void init();
} // namespace dtb