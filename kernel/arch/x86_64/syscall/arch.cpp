#include <drivers/proc.hpp>

namespace arch
{
    uintptr_t sys_arch_prctl(int cmd, uintptr_t addr)
    {
        auto thread = this_thread();
        switch (cmd)
        {
            case arch_set_fs:
                thread->fs_base = addr;
                cpu::set_fs(thread->fs_base);
                break;
            case arch_set_gs:
                thread->gs_base = addr;
                cpu::set_kernel_gs(thread->gs_base);
                break;
            case arch_get_fs:
                *reinterpret_cast<uintptr_t*>(addr) = thread->fs_base;
                break;
            case arch_get_gs:
                *reinterpret_cast<uintptr_t*>(addr) = thread->gs_base;
                break;
            default:
                return_err(-1, EINVAL);
        }
        return 0;
    }
} // namespace arch