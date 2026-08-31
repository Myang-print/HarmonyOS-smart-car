#ifndef HCSR04_DRIVER_H
#define HCSR04_DRIVER_H

#include <stdbool.h>

bool Hcsr04_Init(void);
bool Hcsr04_ReadMedian(float *distanceCm);

#endif
