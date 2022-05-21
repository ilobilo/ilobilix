// Copyright (C) 2022  ilobilo

#include <cdi/lists.h>

extern "C"
{
    cdi_list_t cdi_list_create();
    void cdi_list_destroy(cdi_list_t list);
    cdi_list_t cdi_list_push(cdi_list_t list, void *value);
    void *cdi_list_pop(cdi_list_t list);
    size_t cdi_list_empty(cdi_list_t list);
    void *cdi_list_get(cdi_list_t list, size_t index);
    cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void *value);
    void *cdi_list_remove(cdi_list_t list, size_t index);
    size_t cdi_list_size(cdi_list_t list);
} // extern "C"