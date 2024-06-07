// Copyright (C) 2022-2024  ilobilo

#pragma once

#include <module.hpp>
#include <string_view>

namespace drivers
{
    namespace generic
    {
        void run_all();

        bool destroy(std::string_view name);
        void destroy_all();
    } // namespace generic

    namespace acpi
    {
        void run_all();
    } // namespace acpi

    bool add_driver(driver_t *driver);
} // namespace drivers