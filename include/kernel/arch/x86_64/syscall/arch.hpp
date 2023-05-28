#include <lib/types.hpp>

namespace arch
{
    uintptr_t sys_arch_prctl(int cmd, uintptr_t addr);
} // namespace arch