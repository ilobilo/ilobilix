// Copyright (C) 2022  ilobilo

#include <frg/manual_box.hpp>
#include <drivers/dtb.hpp>
#include <init/kernel.hpp>
#include <lib/log.hpp>
#include <cassert>

namespace dtb
{
    frg::manual_box<device_tree> tree;

    void init()
    {
        auto dtb = find_module("dtb");
        assert(dtb != nullptr, "Could not find DTB module!");

        auto header = reinterpret_cast<dtb::fdt::header*>(dtb->address);
        assert(header->magic.load() == 0xD00DFEED);

        log::infoln("DTB: Magic: 0x{:X}", header->magic.load());
        log::infoln("DTB: Total Size: {} Bytes", header->totalsize.load());
        log::infoln("DTB: Struct block offset: 0x{:X}", header->off_dt_struct.load());
        log::infoln("DTB: Strings block offset: 0x{:X}", header->off_dt_strings.load());
        log::infoln("DTB: Mem rsvmap block offset: 0x{:X}", header->off_mem_rsvmap.load());
        log::infoln("DTB: Version: {}", header->version.load());
        log::infoln("DTB: Last comp version: {}", header->last_comp_version.load());
        log::infoln("DTB: Phys CPU id: {}", header->boot_cpuid_phys.load());
        log::infoln("DTB: Strings block size: {} Bytes", header->size_dt_strings.load());
        log::infoln("DTB: Struct block size: {} Bytes", header->size_dt_struct.load());

        tree.initialize(dtb->address);
    }
} // namespace dtb