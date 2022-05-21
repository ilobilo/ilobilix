/*
 * Copyright (c) 2010 Max Reitz
 *
 * This program is free software. It comes without any warranty, to
 * the extent permitted by applicable law. You can redistribute it
 * and/or modify it under the terms of the Do What The Fuck You Want
 * To Public License, Version 2, as published by Sam Hocevar. See
 * http://sam.zoy.org/projects/COPYING.WTFPL for more details.
 */


#ifndef _CDI_AUDIO_H_
#define _CDI_AUDIO_H_

#include <stddef.h>
#include <stdint.h>

#include <cdi.h>
#include <cdi/lists.h>
#include <cdi/mem.h>


typedef enum
{
    CDI_AUDIO_16SI = 0,
    CDI_AUDIO_8SI,
    CDI_AUDIO_32SI
} cdi_audio_sample_format_t;

typedef struct cdi_audio_position
{
    size_t buffer;
    size_t frame;
} cdi_audio_position_t;

typedef enum
{
    CDI_AUDIO_STOP = 0,
    CDI_AUDIO_PLAY = 1
} cdi_audio_status_t;

struct cdi_audio_device
{
    struct cdi_device dev;
    int record;
    cdi_list_t streams;
};


struct cdi_audio_stream
{
    struct cdi_audio_device* device;
    size_t num_buffers;
    size_t buffer_size;
    int fixed_sample_rate;
    cdi_audio_sample_format_t sample_format;
};

struct cdi_audio_driver
{
    struct cdi_driver drv;

    int (*transfer_data)(struct cdi_audio_stream *stream, size_t buffer, struct cdi_mem_area *memory, size_t offset);
    cdi_audio_status_t (*change_device_status)(struct cdi_audio_device *device, cdi_audio_status_t status);
    void (*set_volume)(struct cdi_audio_stream *stream, uint8_t volume);
    int (*set_sample_rate)(struct cdi_audio_stream *stream, int sample_rate);
    void (*get_position)(struct cdi_audio_stream *stream, cdi_audio_position_t *position);
    int (*set_number_of_channels)(struct cdi_audio_device *dev, int channels);
};

#ifdef __cplusplus
extern "C" {
#endif

void cdi_audio_buffer_completed(struct cdi_audio_stream *stream, size_t buffer);

#ifdef __cplusplus
} // extern "C"
#endif

#endif