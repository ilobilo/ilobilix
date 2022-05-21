#include <cdi.h>

#include "test.hpp"

#define DRIVER_NAME "test"

static int test_driver_init()
{
    printf("INIT WORKS! %s %d\n", "YAY", getstuff());
    return 0;
}

static int test_driver_destroy()
{
    printf("DESTROY WORKS! %s %d\n", "YAY", getstuff());
    return 0;
}

static cdi_driver test_driver = {
    .type           = CDI_VIDEO,
    .name           = DRIVER_NAME,
    .init           = test_driver_init,
    .destroy        = test_driver_destroy,
};

CDI_DRIVER(DRIVER_NAME, test_driver)