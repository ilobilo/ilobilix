// Copyright (C) 2022-2024  ilobilo

#include <drivers/drivers.hpp>
#include <lib/log.hpp>

#include <uacpi/uacpi.h>
#include <uacpi/utilities.h>
#include <uacpi/resources.h>

#include <magic_enum.hpp>

#include <unordered_map>
#include <span>

namespace drivers
{
    namespace generic
    {
        namespace
        {
            static std::unordered_map<std::string_view, driver_t *> list;

            bool run_one(driver_t *driver)
            {
                std::string_view name(driver->name);

                for (size_t i = 0; i < driver->generic.num_deps && driver->generic.deps[i] != nullptr; i++)
                {
                    const auto &dep = driver->generic.deps[i];
                    if (name == dep)
                        continue;

                    if (list.contains(dep) == false)
                    {
                        log::errorln("DRIVERS: Dependency '{}' of '{}' not found", dep, name);
                        return false;
                    }

                    auto depdriver = list[dep];
                    if (depdriver->initialised == false)
                    {
                        log::infoln("DRIVERS: Initialising dependency '{}' of '{}'", dep, name);
                        run_one(depdriver);

                        if (depdriver->initialised == false)
                        {
                            log::errorln("DRIVERS: Could not initialise dependency '{}' of '{}'", dep, name);
                            return false;
                        }
                        else log::infoln("DRIVERS: Dependency '{}' of '{}' initialised", dep, name);
                    }
                }

                bool ret = false;
                if (driver->generic.init)
                {
                    log::infoln("DRIVERS: Calling '{}' init()", name);
                    ret = driver->generic.init();
                }
                else log::errorln("DRIVERS: '{}' does not have init() function", name);

                if (ret == true)
                {
                    driver->initialised = true;
                    driver->failed = false;
                }
                else
                {
                    driver->initialised = false;
                    driver->failed = true;
                }

                return ret;
            }

            bool destroy_one(driver_t *driver)
            {
                if (driver->initialised == false)
                    return false;

                log::infoln("DRIVERS: Deinitialising '{}'", driver->name);

                if (driver->generic.fini)
                    driver->generic.fini();
                else
                    log::warnln("DRIVERS: '{}' does not have fini() function", driver->name);

                driver->initialised = false;
                return true;
            }

            bool add_internal(driver_t *driver, std::string_view name)
            {
                if (list.contains(name))
                {
                    log::errorln("DRIVERS: '{}' of type 'generic' already exists", name);
                    return false;
                }

                if (driver->generic.num_deps > 0 && driver->generic.deps[0] != nullptr)
                {
                    log::infoln(" Dependencies: ");
                    for (size_t d = 0; d < driver->generic.num_deps && driver->generic.deps[d] != nullptr; d++)
                        log::infoln("  - '{}'", driver->generic.deps[d]);
                }

                list[name] = driver;
                return true;
            }
        }

        void run_all()
        {
            for (const auto &[name, driver] : list)
            {
                bool result = false;
                if (driver->initialised == true || driver->failed == true)
                    continue;

                log::infoln("DRIVERS: Initialising '{}'", name);

                if (result == run_one(driver))
                    log::errorln("DRIVERS: Could not initialise '{}'", name);
                else
                    log::infoln("DRIVERS: '{}' initialised", name);
            }
        }

        bool destroy(std::string_view name)
        {
            if (list.contains(name) == false)
            {
                log::errorln("DRIVERS: '{}' not found", name);
                return false;
            }

            auto driver = list[name];
            return destroy_one(driver);
        }

        void destroy_all()
        {
            for (const auto [name, driver] : list)
                destroy(name);
        }
    } // namespace generic

    namespace acpi
    {
        namespace
        {
            static std::unordered_map<std::string_view, driver_t *> list;

            bool add_internal(driver_t *driver, std::string_view name)
            {
                if (list.contains(name))
                {
                    log::errorln("DRIVERS: '{}' of type 'acpi' already exists {}", name);
                    return false;
                }

                list[name] = driver;
                return true;
            }
        }

        void run_all()
        {
            uacpi_namespace_for_each_node_depth_first(uacpi_namespace_root(), [](void *, uacpi_namespace_node *node)
            {
                uacpi_namespace_node_info *info;
                uacpi_status ret = uacpi_get_namespace_node_info(node, &info);
                if (uacpi_unlikely_error(ret))
                {
                    auto path = uacpi_namespace_node_generate_absolute_path(node);;
                    log::errorln("UACPI: Unable to retrieve node '{}' information: {}", path, uacpi_status_to_string(ret));
                    uacpi_free_absolute_path(path);
                    return UACPI_NS_ITERATION_DECISION_CONTINUE;
                }

                if (info->type != UACPI_OBJECT_DEVICE)
                {
                    uacpi_free_namespace_node_info(info);
                    return UACPI_NS_ITERATION_DECISION_CONTINUE;
                }

                auto match = [&](std::string_view str)
                {
                    for (const auto &[name, driver] : list)
                    {
                        std::span<const char *> arr(driver->acpi.pnp_ids, driver->acpi.num_pnp_ids);
                        if (std::find_if(arr.begin(), arr.end(), [&str](const char *s) { return s && str == s; }) != arr.end())
                        {
                            if (driver->acpi.probe)
                            {
                                log::infoln("DRIVERS: Calling '{}' probe()", name);
                                driver->acpi.probe(node, info);
                            }
                            else log::errorln("DRIVERS: '{}' does not have probe() function", name);
                            return true;
                        }
                    }
                    return false;
                };

                bool found = false;
                if (info->flags & UACPI_NS_NODE_INFO_HAS_HID && info->hid.value)
                {
                    std::string_view str(info->hid.value, info->hid.size - 1);
                    found = match(str);
                }

                if (info->flags & UACPI_NS_NODE_INFO_HAS_CID)
                {
                    for (std::size_t i = 0; found == false && i < info->cid.num_ids; i++)
                    {
                        std::string_view str(info->cid.ids[i].value, info->cid.ids[i].size - 1);
                        found = match(str);
                    }
                }

                uacpi_free_namespace_node_info(info);
                return UACPI_NS_ITERATION_DECISION_CONTINUE;
            }, nullptr);
        }
    } // namespace acpi

    bool add_driver(driver_t *driver)
    {
        std::string_view name(driver->name);
        auto type = driver->type;

        if (magic_enum::enum_contains(type) == false)
        {
            log::errorln("DRIVERS: Invalid type '{}' for driver '{}'", uint8_t(type), name);
            return false;
        }
        log::infoln("DRIVERS: Registering driver '{}' of type '{}'", name, magic_enum::enum_name(type));

        bool added = false;
        switch (type)
        {
            case driver_type::generic:
                added = generic::add_internal(driver, name);
                break;
            case driver_type::acpi:
                added = acpi::add_internal(driver, name);
                break;
        };
        return added;
    }
} // namespace drivers