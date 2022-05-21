/*
 * Copyright (c) 2007 Kevin Wolf
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */

#ifndef _CDI_LISTS_
#define _CDI_LISTS_

#include <stddef.h>
#include <stdint.h>

typedef struct cdi_list_implementation* cdi_list_t;

#ifdef __cplusplus
extern "C" {
#endif

cdi_list_t cdi_list_create(void);
void cdi_list_destroy(cdi_list_t list);
cdi_list_t cdi_list_push(cdi_list_t list, void *value);
void *cdi_list_pop(cdi_list_t list);
size_t cdi_list_empty(cdi_list_t list);
void *cdi_list_get(cdi_list_t list, size_t index);
cdi_list_t cdi_list_insert(cdi_list_t list, size_t index, void *value);
void *cdi_list_remove(cdi_list_t list, size_t index);
size_t cdi_list_size(cdi_list_t list);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
