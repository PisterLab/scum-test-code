#ifndef __FREQ_SETTING_SELECTION_H
#define __FREQ_SETTING_SELECTION_H

#include <stdint.h>

//=========================== define ==========================================

//=========================== typedef =========================================

//=========================== variables =======================================

//=========================== prototypes ======================================

//==== admin
uint16_t freq_setting_selection_fo(uint16_t* setting_list,
                                   int8_t* freq_offset_list);

uint16_t freq_setting_selection_fo_alternative(uint16_t* setting_list,
                                               int8_t* freq_offset_list);

uint16_t freq_setting_selection_median(uint16_t* setting_list);

uint16_t freq_setting_selection_if(uint16_t* setting_list,
                                   uint16_t* if_count_list);

#endif