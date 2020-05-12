/**
\brief General OpenWSN definitions
*/

#ifndef __SCUM_DEFS_H
#define __SCUM_DEFS_H

//=========================== define ==========================================

// LC_code used to the initial LC frequency, before optical calibration
#define DEFUALT_INIT_LC_CODE    600//630
//#define REFERENCE_LC_TARGET     250260
#define REFERENCE_LC_TARGET     250042//250573

// for tx we are supposed to be 500kHz above the target frequency (on channel 11 this is 250573)
// for rx we are supposed to be 2.5MHz aboce the target frequency

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================


#endif