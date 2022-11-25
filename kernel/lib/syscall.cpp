// Copyright (C) 2022  ilobilo

#include <magic_enum_format.hpp>
#include <magic_enum.hpp>

#include <lib/syscall.hpp>

namespace syscall
{
    uintptr_t wrapper::run(args_array args) const
    {
        auto [pid, tid] = proc::pid();

        this->print(args, this->name);
        auto ret = reinterpret_cast<uintptr_t (*)(uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t, uintptr_t)>
            (this->storage)(args[0], args[1], args[2], args[3], args[4], args[5]);

        if (auto val = magic_enum::enum_cast<errno_t>(-ret); val.has_value())
            log::infoln("syscall: [{}:{}] {} -> {}", pid, tid, this->name, val.value());
        else
            log::infoln("syscall: [{}:{}] {} -> {}", pid, tid, this->name, ret);
        return ret;
    }
} // namespace syscall