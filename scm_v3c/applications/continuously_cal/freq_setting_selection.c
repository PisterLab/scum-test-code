
#define FREQ_OFFSET_TARGET      2
#define FREQ_OFFSET_THRESHOLD   5
#define MAX_MID_SETTINGS        32

#include "string.h"
#include "freq_setting_selection.h"


uint16_t freq_setting_selection_tx(
    uint16_t* setting_list, 
    int8_t* freq_offset_list
) {
    uint8_t i;
    int8_t diff;
    
    i = 0;
    while (freq_offset_list[i] != 0) {
        
        printf("freq_offset_list[%d] = %d\r\n", i, freq_offset_list[i]);
        
        diff = (int8_t)(freq_offset_list[i]-FREQ_OFFSET_TARGET);
        
        if (
            diff <=  FREQ_OFFSET_THRESHOLD &&
            diff >= -FREQ_OFFSET_THRESHOLD
        ) {
            break;
        }
        i++;
    }
    return setting_list[i];
    
}

uint16_t freq_setting_selection_rx(
    uint16_t* setting_list
) {
    
    uint8_t i;
    uint8_t mid;
    uint8_t mid_changed_at;
    uint8_t mid_settings_size;
    uint8_t max_mid_settings_size;
    
    uint16_t mid_settings[MAX_MID_SETTINGS];
    
    i                       = 0;
    mid                     = ((setting_list[0]>>5) & 0x001F);
    mid_changed_at          = 0;
    mid_settings_size       = 0;
    max_mid_settings_size   = 0;
    
    memset(mid_settings, 0, MAX_MID_SETTINGS*sizeof(uint16_t));
    while (setting_list[i]!=0) {
        
        printf(
            "%d: mid setting_list = %d %d %d\r\n",
            i,
            (setting_list[i] >> 10 ) & 0x001f,
            (setting_list[i] >> 5 )  & 0x001f,
            (setting_list[i] )       & 0x001f
        );
        
        if (((setting_list[i]>>5) & 0x001F) != mid) {
            if (mid_settings_size>max_mid_settings_size) {
                memcpy(
                    mid_settings, 
                    &setting_list[mid_changed_at],
                    mid_settings_size*sizeof(uint16_t)
                );
                max_mid_settings_size = mid_settings_size;
            }
            mid_changed_at = i;
            mid_settings_size = 1; // re-initiate to 1
            mid = (setting_list[i]>>5) & 0x001F;
        } else {
            mid_settings_size++;
        }
        i++;
    }
    
    if (mid_settings_size>max_mid_settings_size){
        memcpy(
            mid_settings, 
            &setting_list[mid_changed_at],
            mid_settings_size*sizeof(uint16_t)
        );
        max_mid_settings_size = mid_settings_size; 
    }
    
    // debug info
    
    i = 0;
    while (mid_settings[i] != 0) {

        i++;
    }
    
    // choose the median setting in mid_settings list
    
    i = 0;
    while(mid_settings[i]!=0){
        i++;
    }
    
    return mid_settings[i/2];
}