// Copyright (C) 2022-2024  ilobilo

#pragma once

#if defined(__x86_64__)
#  include <arch/x86_64/lib/io.hpp>
#  define CAN_LEGACY_IO 1
#else
#  define CAN_LEGACY_IO 0
#endif