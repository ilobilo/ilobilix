// Copyright (C) 2022  ilobilo

#include <cstdint>

namespace cpu::pic
{
    enum cmds
    {
        PIC1 = 0x20,
        PIC2 = 0xA0,
        PIC1_COMMAND = PIC1,
        PIC1_DATA = (PIC1 + 1),
        PIC2_COMMAND = PIC2,
        PIC2_DATA = (PIC2 + 1),
        PIC_EOI = 0x20
    };

    void eoi(uint64_t int_no);
    void disable();
    void init();
} // namespace cpu::pic