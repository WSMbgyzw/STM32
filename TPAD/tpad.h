#ifndef _TPAD_H
#define _TPAD_H
#include "./SYSTEM/sys/sys.h"
static void tpad_timx_cap_init(uint16_t psc);
static void tpad_reset(void);
static uint16_t tpad_get_val(void);
static uint16_t tpad_get_maxval(uint8_t n);
uint8_t tpad_init(uint16_t psc);
extern uint16_t g_tpad_default_val;
uint8_t tpad_scan(uint8_t mode);
uint8_t tpad_scan2(uint8_t mode);
#endif


