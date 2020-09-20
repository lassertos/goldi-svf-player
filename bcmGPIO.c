#include <bcm2835.h>
#include <stdio.h>

#define TCK 25
#define TMS 27
#define TDO 24
#define TDI 26

int initGPIO()
{
    // If you call this, it will not actually access the GPIO
    // Use for testing
    // bcm2835_set_debug(1);

    if (!bcm2835_init())
      return 1;

    bcm2835_gpio_fsel(TCK, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TMS, BCM2835_GPIO_FSEL_OUTP);
    bcm2835_gpio_fsel(TDO, BCM2835_GPIO_FSEL_INPT);
    bcm2835_gpio_fsel(TDI, BCM2835_GPIO_FSEL_OUTP);

    return 0;
}

void writeGPIO(int pin, unsigned data)
{
    bcm2835_gpio_write(pin, data);
}

unsigned readGPIO(int pin)
{
    return bcm2835_gpio_lev(pin);
}

void stopGPIO()
{
    bcm2835_close();
}
