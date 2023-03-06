#include <lib/types.hpp>

namespace vmm
{
    void *sys_mmap(void *addr, size_t length, int prot, int flags, int fdnum, off_t offset);
    int sys_munmap(void *addr, size_t length);
} // namespace vmm