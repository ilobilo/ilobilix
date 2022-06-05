/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_MISC_H_
#define _CDI_MISC_H_

#include <stdint.h>

#include <cdi.h>

#define CDI_GLUE(x,y) x ## y
#define CDI_BUILD_ASSERT(cnt) CDI_GLUE(__cdi_build_assert, cnt)
#define CDI_BUILD_BUG_ON(x) struct CDI_BUILD_ASSERT(__COUNTER__) { int assertion[(x) ? -1 : 1];  };

#ifdef __cplusplus
extern "C" {
#endif

#define cdi_barrier() do { asm volatile("" ::: "memory"); } while (0)

void cdi_register_irq(uint8_t irq, void (*handler)(struct cdi_device*), struct cdi_device *device);
int cdi_reset_wait_irq(uint8_t irq);
int cdi_wait_irq(uint8_t irq, uint32_t timeout);
int cdi_ioports_alloc(uint16_t start, uint16_t count);
int cdi_ioports_free(uint16_t start, uint16_t count);
void cdi_sleep_ms(uint32_t ms);
uint64_t cdi_elapsed_ms(void);

#define CDI_CONDITION_WAIT(cond, timeout) \
    do { \
        uint64_t start = cdi_elapsed_ms(); \
        while (!(cond) && cdi_elapsed_ms() - start < timeout); \
    } while (0)

#define CDI_CONDITION_WAIT_SLEEP(cond, timeout, sleep) \
    do { \
        uint64_t start = cdi_elapsed_ms(); \
        while (!(cond) && cdi_elapsed_ms() - start < timeout) { \
            cdi_sleep_ms(sleep); \
        } \
    } while (0)

#define CDI_UPCAST(object, to, field) \
    ((void)((object) - &((to*)0)->field), \
     (to*)((char*)(object) - (char*)&((to*)0)->field))

static inline uint16_t cdi_be16_to_cpu(uint16_t x)
{
    return (x >> 8) | (x << 8);
}

static inline uint32_t cdi_be32_to_cpu(uint32_t x)
{
    return ((uint32_t)cdi_be16_to_cpu(x) << 16) | (uint32_t)cdi_be16_to_cpu(x >> 16);
}

static inline uint64_t cdi_be64_to_cpu(uint64_t x)
{
    return ((uint64_t)cdi_be32_to_cpu(x) << 32) | (uint64_t)cdi_be32_to_cpu(x >> 32);
}

static inline uint16_t cdi_cpu_to_be16(uint16_t x)
{
    return cdi_be16_to_cpu(x);
}

static inline uint32_t cdi_cpu_to_be32(uint32_t x)
{
    return cdi_be32_to_cpu(x);
}

static inline uint64_t cdi_cpu_to_be64(uint64_t x)
{
    return cdi_be64_to_cpu(x);
}

static inline uint16_t cdi_le16_to_cpu(uint16_t x)
{
    return x;
}

static inline uint32_t cdi_le32_to_cpu(uint32_t x)
{
    return x;
}

static inline uint64_t cdi_le64_to_cpu(uint64_t x)
{
    return x;
}

static inline uint16_t cdi_cpu_to_le16(uint16_t x)
{
    return cdi_le16_to_cpu(x);
}

static inline uint32_t cdi_cpu_to_le32(uint32_t x)
{
    return cdi_le32_to_cpu(x);
}

static inline uint64_t cdi_cpu_to_le64(uint64_t x)
{
    return cdi_le64_to_cpu(x);
}

#ifdef __cplusplus
} // extern "C"
#endif

#endif