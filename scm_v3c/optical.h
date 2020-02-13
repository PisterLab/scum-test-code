#ifndef __OPTICAL_H
#define __OPTICAL_H

#include <stdbool.h>
#include <stdint.h>

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

//==== admin
void optical_init(void);
void optical_enableLCCalibration(void);
bool optical_getCalibrationFinished(void);
void optical_setLCTarget(uint32_t LC_target);
uint8_t optical_getLCCoarse(void);
uint8_t optical_getLCMid(void);
uint8_t optical_getLCFine(void);
void optical_enable(void);

#endif