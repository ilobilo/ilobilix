// Copyright (C) 2024-2025  ilobilo

import lib;

bool init() { return true; }
bool fini() { return true; }

declare_module(example) {
    "example", "an example module demonstrating blah blah blah description goes here",
    mod::generic { .init = init, .fini = fini },
    mod::deps { "test1", "test2" }
};