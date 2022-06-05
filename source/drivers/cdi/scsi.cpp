// Copyright (C) 2022  ilobilo

#include <cdi/scsi.h>

#include <lib/log.hpp>

extern "C"
{
    cdi_scsi_packet *cdi_scsi_packet_alloc(size_t size);
    void cdi_scsi_packet_free(cdi_scsi_packet *packet);

    void cdi_scsi_driver_init(cdi_scsi_driver *driver)
    {
        driver->drv.type = CDI_SCSI;
        cdi_driver_init(reinterpret_cast<cdi_driver*>(driver));
    }

    void cdi_scsi_driver_destroy(cdi_scsi_driver *driver)
    {
        cdi_driver_destroy(reinterpret_cast<cdi_driver*>(driver));
    }

    void cdi_scsi_driver_register(cdi_scsi_driver *driver);
    void cdi_scsi_device_init(cdi_scsi_device *device);
} // extern "C"