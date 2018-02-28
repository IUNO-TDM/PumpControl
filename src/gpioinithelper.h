#ifndef SRC_GPIOINITHELPER_H_
#define SRC_GPIOINITHELPER_H_

#ifdef OS_raspbian
#include <pigpio.h>
#else
const int PI_BAD_GPIO = -1;
const unsigned PI_OUTPUT = 0;
const unsigned PI_INPUT = 0;
const unsigned PI_PUD_UP = 0;
const unsigned PI_PUD_DOWN = 0;
const unsigned PI_PUD_OFF = 0;
int gpioSetMode(unsigned, unsigned);
int gpioSetPullUpDown(unsigned, unsigned);
int gpioRead(unsigned);
int gpioWrite(unsigned, unsigned);
int gpioSetPWMrange(unsigned, unsigned);
int gpioSetPWMfrequency(unsigned, unsigned);
int gpioPWM(unsigned, unsigned);
#endif

int wrapGpioInitialise();
void wrapGpioTerminate();

#endif /* SRC_GPIOINITHELPER_H_ */
