// Copyright (C) 2022-2024  ilobilo

#include <drivers/dtb.hpp>
#include <init/kernel.hpp>
#include <assert.h>

namespace dtb
{
    frg::manual_box<devicetree> tree;
    node devicetree::root()
    {
        return node(this, this->structs);
    }

    void init()
    {
        auto dtb = find_module("dtb");
        assert(dtb != nullptr, "Could not find DTB module!");
        assert(dtb->address != nullptr, "Could not find DTB module!");

        tree.initialize(dtb->address);
    }
} // namespace dtb