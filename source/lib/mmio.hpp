// Copyright (C) 2022  ilobilo

#pragma once

#include <cstdint>

void mmoutb(void *addr, uint8_t value);
void mmoutw(void *addr, uint16_t value);
void mmoutl(void *addr, uint32_t value);
void mmoutq(void *addr, uint64_t value);

uint8_t mminb(void *addr);
uint16_t mminw(void *addr);
uint32_t mminl(void *addr);
uint64_t mminq(void *addr);