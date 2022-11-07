// Copyright (C) 2022  ilobilo

#include <magic_enum_format.hpp>
#include <lib/syscall.hpp>

namespace syscall
{
    return_type wrapper::run(args_array args) const
    {
        this->print(args, this->name);
        auto ret = reinterpret_cast<return_type (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t)>
            (this->storage)(args[0], args[1], args[2], args[3], args[4], args[5]);

        auto [pid, tid] = proc::pid();
        log::infoln("syscall: [{}:{}] {} -> (0x{:X}, {})", pid, tid, this->name, ret.first, ret.second);
        return ret;
    }
} // namespace syscall