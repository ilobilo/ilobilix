// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <lib/endian.hpp>

namespace dtb
{
    namespace fdt
    {
        struct header
        {
            endian_storage<uint32_t, std::endian::big> magic;
            endian_storage<uint32_t, std::endian::big> totalsize;
            endian_storage<uint32_t, std::endian::big> off_dt_struct;
            endian_storage<uint32_t, std::endian::big> off_dt_strings;
            endian_storage<uint32_t, std::endian::big> off_mem_rsvmap;
            endian_storage<uint32_t, std::endian::big> version;
            endian_storage<uint32_t, std::endian::big> last_comp_version;
            endian_storage<uint32_t, std::endian::big> boot_cpuid_phys;
            endian_storage<uint32_t, std::endian::big> size_dt_strings;
            endian_storage<uint32_t, std::endian::big> size_dt_struct;
        };

        struct reserve_entry
        {
            endian_storage<uint64_t, std::endian::big> address;
            endian_storage<uint64_t, std::endian::big> size;
        };
    } // namespace fdt

    // TODO: DTB
    struct device_tree
    {
        device_tree(void *data) { }
    };

    void init();
} // namespace dtb