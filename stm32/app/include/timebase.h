/*
This module provides accurate no interrupt microsecond timebase.
*/

#pragma once
#include <stdint.h>

void timebase_init(void);
uint64_t timebase_now_us(void);