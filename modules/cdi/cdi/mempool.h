/******************************************************************************
* Copyright (c) 2009 Kevin Wolf                                              *
*                                                                            *
* Permission is hereby granted,  free of charge,  to any  person obtaining a *
* copy of this software and associated documentation files (the "Software"), *
* to deal in the Software without restriction,  including without limitation *
* the rights to use,  copy, modify, merge, publish,  distribute, sublicense, *
* and/or sell copies  of the  Software,  and to permit  persons to whom  the *
* Software is furnished to do so, subject to the following conditions:       *
*                                                                            *
* The above copyright notice and this permission notice shall be included in *
* all copies or substantial portions of the Software.                        *
*                                                                            *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR *
* IMPLIED, INCLUDING  BUT NOT  LIMITED TO THE WARRANTIES OF MERCHANTABILITY, *
* FITNESS FOR A PARTICULAR  PURPOSE AND  NONINFRINGEMENT.  IN NO EVENT SHALL *
* THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
* LIABILITY,  WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING *
* FROM,  OUT OF  OR IN CONNECTION  WITH THE  SOFTWARE  OR THE  USE OR  OTHER *
* DEALINGS IN THE SOFTWARE.                                                  *
*****************************************************************************/

#ifndef CDI_MEMPOOL_H
#define CDI_MEMPOOL_H

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct cdi_mempool;

struct cdi_mempool *cdi_mempool_create(size_t pool_size, size_t object_size);
int cdi_mempool_get(struct cdi_mempool *pool, void **obj, uintptr_t *phys_obj);
int cdi_mempool_put(struct cdi_mempool *pool, void *obj);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
