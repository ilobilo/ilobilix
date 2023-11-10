// Copyright (C) 2022-2023  ilobilo

#pragma once

#include <algorithm>
#include <ranges>

inline constexpr bool erase_from(auto &&container, auto &&val)
{
    return container.erase(std::remove(std::ranges::begin(container), std::ranges::end(container), std::move(val)), std::ranges::end(container)) != std::ranges::end(container);
}

inline constexpr bool erase_from_if(auto &&container, auto pred)
{
    return container.erase(std::remove_if(std::ranges::begin(container), std::ranges::end(container), pred), std::ranges::end(container)) != std::ranges::end(container);
}

inline constexpr bool contains(auto &&container, auto &&val)
{
    return std::find(std::ranges::begin(container), std::ranges::end(container), std::move(val)) != std::ranges::end(container);
}

inline constexpr bool contains_if(auto &&container, auto pred)
{
    return std::find_if(std::ranges::begin(container), std::ranges::end(container), pred) != std::ranges::end(container);
}