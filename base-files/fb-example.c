#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <linux/fb.h>
#include <sys/mman.h>

int main()
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;
    long int screensize = 0;
    char *fbp = 0;
    int x = 0, y = 0;
    long int location = 0;

    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1) {
        perror("opening /dev/fb0");
        return -1;
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo)) {
        printf("Error reading fixed information.\n");
        return -2;
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo)) {
        printf("Error reading variable information.\n");
        return -3;
    }

    printf("%dx%d, %dbpp\n", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel );

    // Figure out the size of the screen in bytes
    screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    // Map the device to memory
    fbp = (char *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if ((int)fbp == -1) {
        printf("Error: failed to map framebuffer device to memory.\n");
        return -4;
    }
    printf("The framebuffer device was mapped to memory successfully.\n");

    x = 100;
    y = 100;       // Where we are going to put the pixel

    // Figure out where in memory to put the pixel
    for ( y = 100; y < 300; y++ ) {
        for ( x = 100; x < 300; x++ ) {

            location = (x+vinfo.xoffset) * (vinfo.bits_per_pixel/8) + (y+vinfo.yoffset) * finfo.line_length;

            if ( vinfo.bits_per_pixel == 32 ) {
                *(fbp + location) = 100;        // Some blue
                *(fbp + location + 1) = 15+(x-100)/2;     // A little green
                *(fbp + location + 2) = 200-(y-100)/5;    // A lot of red
                *(fbp + location + 3) = 0;      // No transparency
            } else  { //assume 16bpp
                int b = 10;
                int g = (x-100)/6;     // A little green
                int r = 31-(y-100)/16;    // A lot of red
                unsigned short int t = r<<11 | g << 5 | b;
                *((unsigned short int*)(fbp + location)) = t;
            }

        }
    }
    munmap(fbp, screensize);
    close(fbfd);
    return 0;
}
