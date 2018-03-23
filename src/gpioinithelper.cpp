#include "gpioinithelper.h"

#ifdef OS_raspbian
#include <pigpio.h>
#else
int gpioInitialise(){return PI_BAD_GPIO;}
void gpioTerminate(){}
int gpioSetMode(unsigned, unsigned){return PI_BAD_GPIO;}
int gpioSetPullUpDown(unsigned, unsigned){return PI_BAD_GPIO;}
int gpioRead(unsigned){return PI_BAD_GPIO;}
int gpioWrite(unsigned, unsigned){return PI_BAD_GPIO;}
int gpioSetPWMrange(unsigned, unsigned){return PI_BAD_GPIO;}
int gpioSetPWMfrequency(unsigned, unsigned){return PI_BAD_GPIO;}
int gpioPWM(unsigned, unsigned){return PI_BAD_GPIO;}
#endif


static unsigned gpio_init_counter = 0;
static int gpio_init_rv = 0;

int wrapGpioInitialise(){
    if(gpio_init_counter == 0){
        gpio_init_rv = gpioInitialise();
    }
    gpio_init_counter++;
    return gpio_init_rv;
}

void wrapGpioTerminate(){
    if(gpio_init_counter == 1){
        gpioTerminate();
        gpio_init_rv = 0;
    }
    gpio_init_counter--;
}
