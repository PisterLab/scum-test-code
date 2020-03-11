#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "memory_map.h"
#include "scm3c_hw_interface.h"
#include "radio.h"
#include "rftimer.h"

// raw_chip interrupt related
unsigned int chips[100];
unsigned int chip_index = 0;
int raw_chips;
int jj;
unsigned int acfg3_val;

// These coefficients are used for filtering frequency feedback information
// These are no necessarily the ideal values to use; situationally dependent
unsigned char FIR_coeff[11] = {4,16,37,64,87,96,87,64,37,16,4};
unsigned int IF_estimate_history[11] = {500,500,500,500,500,500,500,500,500,500};
signed short cdr_tau_history[11] = {0};

//=========================== definition ======================================

#define MAXLENGTH_TRX_BUFFER    128     // 1B length, 125B data, 2B CRC
#define NUM_CHANNELS            16

//===== default crc check result and rssi value

#define DEFAULT_CRC_CHECK        01     // this is an arbitrary value for now
#define DEFAULT_RSSI            -50     // this is an arbitrary value for now
#define DEFAULT_FREQ             11     // use the channel 11 for now

//===== for calibration

#define IF_FREQ_UPDATE_TIMEOUT   10
#define LO_FREQ_UPDATE_TIMEOUT   10
#define FILTER_WINDOWS_LEN       11
#define FIR_COEFF_SCALE          512 // determined by FIR_coeff

#define LC_CODE_RX      700 //Board Q3: tested at Inria A102 room (Oct, 16 2019)
#define LC_CODE_TX      707 //Board Q3: tested at Inria A102 room (Oct, 16 2019)

#define FREQ_UPDATE_RATE        15

//===== for recognizing panid

#define  LEN_PKT_INDEX           0x00
#define  PANID_LBYTE_PKT_INDEX   0x04
#define  PANID_HBYTE_PKT_INDEX   0x05
#define  DEFAULT_PANID           0xcafe

//=========================== variables =======================================

typedef struct {
    radio_capture_cbt   startFrame_tx_cb;
    radio_capture_cbt   endFrame_tx_cb;
    radio_capture_cbt   startFrame_rx_cb;
    radio_capture_cbt   endFrame_rx_cb;
            uint8_t     radio_tx_buffer[MAXLENGTH_TRX_BUFFER] __attribute__ \
                                                            ((aligned (4)));
            uint8_t     radio_rx_buffer[MAXLENGTH_TRX_BUFFER] __attribute__ \
                                                            ((aligned (4)));
            uint8_t     current_frequency;
            bool        crc_ok;
            
            uint32_t    rx_channel_codes[NUM_CHANNELS];
            uint32_t    tx_channel_codes[NUM_CHANNELS];
    
    // How many packets must be received before adjusting RX clock rates
    // Should be at least as long as the FIR filters
    volatile uint16_t   frequency_update_rate;
    volatile uint16_t   frequency_update_cooldown_timer;
} radio_vars_t;

radio_vars_t radio_vars;

//=========================== prototypes ======================================

void        setFrequencyTX(uint8_t channel);
void        setFrequencyRX(uint8_t channel);

uint32_t    build_RX_channel_table(uint32_t channel_11_LC_code);
void        build_TX_channel_table(
    uint32_t channel_11_LC_code, 
    uint32_t count_LC_RX_ch11
);

//=========================== public ==========================================


void radio_init(void) {

    // clear variables
    memset(&radio_vars,0,sizeof(radio_vars_t));
    
    //skip building a channel table for now; hardcode LC values
    radio_vars.tx_channel_codes[0] = LC_CODE_TX;
    radio_vars.rx_channel_codes[0] = LC_CODE_RX;
    
    radio_vars.frequency_update_rate = FREQ_UPDATE_RATE;

    // Enable radio interrupts in NVIC
    ISER = 0x40;
    
    // enable sfd done and send done interruptions of tranmission
    // enable sfd done and receiving done interruptions of reception
    RFCONTROLLER_REG__INT_CONFIG    = TX_LOAD_DONE_INT_EN           |   \
                                      TX_SFD_DONE_INT_EN            |   \
                                      TX_SEND_DONE_INT_EN           |   \
                                      RX_SFD_DONE_INT_EN            |   \
                                      RX_DONE_INT_EN;

    // Enable all errors
//    RFCONTROLLER_REG__ERROR_CONFIG  = TX_OVERFLOW_ERROR_EN          |   \
//                                      TX_CUTOFF_ERROR_EN            |   \
//                                      RX_OVERFLOW_ERROR_EN          |   \
//                                      RX_CRC_ERROR_EN               |   \
//                                      RX_CUTOFF_ERROR_EN;
                                      
    RFCONTROLLER_REG__ERROR_CONFIG  = RX_CRC_ERROR_EN;
}

void radio_setStartFrameTxCb(radio_capture_cbt cb) {
    radio_vars.startFrame_tx_cb    = cb;
}

void radio_setEndFrameTxCb(radio_capture_cbt cb) {
    radio_vars.endFrame_tx_cb      = cb;
}

void radio_setStartFrameRxCb(radio_capture_cbt cb) {
    radio_vars.startFrame_rx_cb    = cb;
}

void radio_setEndFrameRxCb(radio_capture_cbt cb) {
    radio_vars.endFrame_rx_cb      = cb;
}

void radio_reset(void) {
    // reset SCuM radio module
    RFCONTROLLER_REG__CONTROL = RF_RESET;
}

void radio_setFrequency(uint8_t frequency, radio_freq_t tx_or_rx) {
    
    radio_vars.current_frequency = DEFAULT_FREQ;
    
    switch(tx_or_rx){
    case FREQ_TX:
        setFrequencyTX(radio_vars.current_frequency);
        break;
    case FREQ_RX:
        setFrequencyRX(radio_vars.current_frequency);
        break;
    default:
        // shouldn't happen
        break;
    }
}

void radio_loadPacket(uint8_t* packet, uint16_t len){
    
    memcpy(&radio_vars.radio_tx_buffer[0],packet,len);

    // load packet in TXFIFO
    RFCONTROLLER_REG__TX_DATA_ADDR  = &(radio_vars.radio_tx_buffer[0]);
    RFCONTROLLER_REG__TX_PACK_LEN   = len;

    RFCONTROLLER_REG__CONTROL       = TX_LOAD;
}

// Turn on the radio for transmit
// This should be done at least ~50 us before txNow()
void radio_txEnable(){
    
    // Turn off polyphase and disable mixer
    ANALOG_CFG_REG__16 = 0x6;
    
    // Turn on LO, PA, and AUX LDOs
    ANALOG_CFG_REG__10 = 0x0028;
}

// Begin modulating the radio output for TX
// Note that you need some delay before txNow() to allow txLoad() to finish loading the packet
void radio_txNow(){
    
    RFCONTROLLER_REG__CONTROL = TX_SEND;
}

// Turn on the radio for receive
// This should be done at least ~50 us before rxNow()
void radio_rxEnable(){
    
    // Turn on LO, IF, and AUX LDOs via memory mapped register
    
    // Aux is inverted (0 = on)
    // Memory-mapped LDO control
    // ANALOG_CFG_REG__10 = AUX_EN | DIV_EN | PA_EN | IF_EN | LO_EN | PA_MUX | IF_MUX | LO_MUX
    // For MUX signals, '1' = FSM control, '0' = memory mapped control
    // For EN signals, '1' = turn on LDO
    ANALOG_CFG_REG__10 = 0x0018;
    
    // Enable polyphase and mixers via memory-mapped I/O
    ANALOG_CFG_REG__16 = 0x1;
    
    // Where packet will be stored in memory
    DMA_REG__RF_RX_ADDR = &(radio_vars.radio_rx_buffer[0]);;
    
    // Reset radio FSM
    RFCONTROLLER_REG__CONTROL = RF_RESET;
}

// Radio will begin searching for start of packet
void radio_rxNow(){
    
    // Reset digital baseband
    ANALOG_CFG_REG__4 = 0x2000;
    ANALOG_CFG_REG__4 = 0x2800;

    // Start RX FSM
    RFCONTROLLER_REG__CONTROL = RX_START;
}

void radio_getReceivedFrame(uint8_t* pBufRead,
                            uint8_t* pLenRead,
                            uint8_t  maxBufLen,
                             int8_t* pRssi,
                            uint8_t* pLqi) {
   
    //===== rssi
    *pRssi          = DEFAULT_RSSI;
    
    //===== length
    *pLenRead       = radio_vars.radio_rx_buffer[0];
    
    //===== packet 
    if (*pLenRead<=maxBufLen) {
        memcpy(pBufRead,&(radio_vars.radio_rx_buffer[1]),*pLenRead);
    }
}

void radio_rfOff(){
    
    // Hold digital baseband in reset
    ANALOG_CFG_REG__4 = 0x2000;

    // Turn off LDOs
    ANALOG_CFG_REG__10 = 0x0000;
}

void radio_frequency_housekeeping(
    uint32_t IF_estimate,
    uint32_t LQI_chip_errors,
    int16_t cdr_tau_value
) {
    
    signed int sum      = 0;
    int jj;
    unsigned int IF_est_filtered;
    signed int chip_rate_error_ppm, chip_rate_error_ppm_filtered;
    unsigned short packet_len;
    signed int timing_correction;
    
    uint32_t IF_coarse;
    uint32_t IF_fine;
    
    IF_coarse           = scm3c_hw_interface_get_IF_coarse();
    IF_fine             = scm3c_hw_interface_get_IF_fine();
    
    packet_len = radio_vars.radio_rx_buffer[0];
    
    // When updating LO and IF clock frequncies, must wait long enough for the changes to propagate before changing again
    // Need to receive as many packets as there are taps in the FIR filter
    radio_vars.frequency_update_cooldown_timer++;
    
    // FIR filter for cdr tau slope
    sum = 0;
    
    // A tau value of 0 indicates there is no rate mistmatch between the TX and RX chip clocks
    // The cdr_tau_value corresponds to the number of samples that were added or dropped by the CDR
    // Each sample point is 1/16MHz = 62.5ns
    // Need to estimate ppm error for each packet, then FIR those values to make tuning decisions
    // error_in_ppm = 1e6 * (#adjustments * 62.5ns) / (packet length (bytes) * 64 chips/byte * 500ns/chip)
    // Which can be simplified to (#adjustments * 15625) / (packet length * 8)
                
    chip_rate_error_ppm = (cdr_tau_value * 15625) / (packet_len * 8);
    
    // Shift old samples
    for (jj=9; jj>=0; jj--){
        cdr_tau_history[jj+1] = cdr_tau_history[jj];
    }
    
    // New sample
    cdr_tau_history[0] = chip_rate_error_ppm;
    
    // Do FIR convolution
    for (jj=0; jj<=10; jj++){
        sum = sum + cdr_tau_history[jj] * FIR_coeff[jj];
    }
    
    // Divide by 512 (sum of the coefficients) to scale output
    chip_rate_error_ppm_filtered = sum / 512;
    
    //printf("%d -- %d\r\n",cdr_tau_value,chip_rate_error_ppm_filtered);
    
    // The IF clock frequency steps are about 2000ppm, so make an adjustment only if the error is larger than 1000ppm
    // Must wait long enough between changes for FIR to settle (at least 10 packets)
    // Need to add some handling here in case the IF_fine code will rollover with this change (0 <= IF_fine <= 31)
    if(radio_vars.frequency_update_cooldown_timer == radio_vars.frequency_update_rate){
        if(chip_rate_error_ppm_filtered > 1000) {
            set_IF_clock_frequency(IF_coarse, IF_fine++, 0);
        }
        if(chip_rate_error_ppm_filtered < -1000) {
            set_IF_clock_frequency(IF_coarse, IF_fine--, 0);
        }
    }
    scm3c_hw_interface_set_IF_coarse(IF_coarse);
    scm3c_hw_interface_set_IF_fine(IF_fine);
    
    
    // FIR filter for IF estimate
    sum = 0;
                
    // The IF estimate reports how many zero crossings (both pos and neg) there were in a 100us period
    // The IF should on average be 2.5 MHz, which means the IF estimate will return ~500 when there is no IF error
    // Each tick is roughly 5 kHz of error
    
    // Only make adjustments when the chip error rate is <10% (this value was picked as an arbitrary choice)
    // While packets can be received at higher chip error rates, the average IF estimate tends to be less accurate
    // Estimated chip_error_rate = LQI_chip_errors/256 (assuming the packet length was at least 8 Bytes)
    if(LQI_chip_errors < 25){
    
        // Shift old samples
        for (jj=9; jj>=0; jj--){
            IF_estimate_history[jj+1] = IF_estimate_history[jj];        
        }
        
        // New sample
        IF_estimate_history[0] = IF_estimate;

        // Do FIR convolution
        for (jj=0; jj<=10; jj++){
            sum = sum + IF_estimate_history[jj] * FIR_coeff[jj];        
        }
        
        // Divide by 512 (sum of the coefficients) to scale output
        IF_est_filtered = sum / 512;
        
        //printf("%d - %d, %d\r\n",IF_estimate,IF_est_filtered,LQI_chip_errors);
        
        // The LO frequency steps are about ~80-100 kHz, so make an adjustment only if the error is larger than that
        // These hysteresis bounds (+/- X) have not been optimized
        // Must wait long enough between changes for FIR to settle (at least as many packets as there are taps in the FIR)
        // For now, assume that TX/RX should both be updated, even though the IF information is only from the RX code
        if(radio_vars.frequency_update_cooldown_timer == radio_vars.frequency_update_rate){
            if(IF_est_filtered > 520){
                radio_vars.rx_channel_codes[radio_vars.current_frequency - 11]++; 
                radio_vars.tx_channel_codes[radio_vars.current_frequency - 11]++; 
            }
            if(IF_est_filtered < 480){
                radio_vars.rx_channel_codes[radio_vars.current_frequency - 11]--; 
                radio_vars.tx_channel_codes[radio_vars.current_frequency - 11]--; 
            }
            
            //printf("--%d - %d\r\n",IF_estimate,IF_est_filtered);

            radio_vars.frequency_update_cooldown_timer = 0;
        }
    }
}

void radio_enable_interrupts(){
    
    // Enable radio interrupts in NVIC
    ISER = 0x40;
    
    // Enable all interrupts and pulses to radio timer
    //RFCONTROLLER_REG__INT_CONFIG = 0x3FF;   
        
    // Enable TX_SEND_DONE, RX_SFD_DONE, RX_DONE
    RFCONTROLLER_REG__INT_CONFIG = 0x1C;
    
    // Enable all errors
    //RFCONTROLLER_REG__ERROR_CONFIG = 0x1F;  
    
    // Enable only the RX CRC error
    RFCONTROLLER_REG__ERROR_CONFIG = 0x8;    //0x10; x10 is wrong? 
}

void radio_disable_interrupts(void){

    // Clear radio interrupts in NVIC
    ICER = 0x40;
}

bool radio_getCrcOk(void){
    return radio_vars.crc_ok;
}

uint32_t radio_getIFestimate(void){
    return read_IF_estimate();
}

uint32_t radio_getLQIchipErrors(void){
    return ANALOG_CFG_REG__21;
}

int16_t radio_get_cdr_tau_value(void){
    return ANALOG_CFG_REG__25;
}

//=========================== private =========================================

// SCM has separate setFrequency functions for RX and TX because of the way the
// radio is built. The LO needs to be set to a different frequency for TX vs RX.
void setFrequencyRX(uint8_t channel){
    
    // Set LO code for RX channel
    LC_monotonic(radio_vars.rx_channel_codes[channel-11]);
}

void setFrequencyTX(uint8_t channel){
    
    // Set LO code for TX channel
    LC_monotonic(radio_vars.tx_channel_codes[channel-11]);
}


uint32_t build_RX_channel_table(uint32_t channel_11_LC_code){
    
    uint32_t    rdata_lsb,rdata_msb;
    int32_t     i;
    uint32_t    t;
    uint32_t    count_LC[16];
    uint32_t    count_targets[17];
    
    memset(&count_LC[0],0,sizeof(count_LC));
    memset(&count_targets[0],0,sizeof(count_targets));
    
    radio_vars.rx_channel_codes[0] = channel_11_LC_code;
    
    i = 0;
    while(i<16) {
    
        LC_monotonic(radio_vars.rx_channel_codes[i]);
        //analog_scan_chain_write_3B_fromFPGA(&ASC[0]);
        //analog_scan_chain_load_3B_fromFPGA();
                    
        // Reset all counters
        ANALOG_CFG_REG__0 = 0x0000;
        
        // Enable all counters
        ANALOG_CFG_REG__0 = 0x3FFF;    
        
        // Count for some arbitrary amount of time
        for(t=1; t<16000; t++);
        
        // Disable all counters
        ANALOG_CFG_REG__0 = 0x007F;

        // Read count result
        rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
        rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
        count_LC[i] = rdata_lsb + (rdata_msb << 16);
    
        count_targets[i+1] = ((961+(i+1)*2) * count_LC[0]) / 961;
        
        // Adjust LC_code to match new target
        if(i>0){
            
            if(count_LC[i] < (count_targets[i] - 20)){
                radio_vars.rx_channel_codes[i]++;
            }
            else{
                radio_vars.rx_channel_codes[i+1] = radio_vars.rx_channel_codes[i] + 40;
                i++;
            }                    
        }
        
        if(i==0){
            radio_vars.rx_channel_codes[i+1] = radio_vars.rx_channel_codes[i] + 40;
            i++;
        }
    }
    
    for(i=0; i<16; i++){
//        printf(
//            "\r\nRX ch=%d, count_LC=%d, count_targets=%d, rx_channel_codes=%d",
//            i+11,count_LC[i],count_targets[i], radio_vars.rx_channel_codes[i]
//        );
    }
    
    return count_LC[0];
}


void build_TX_channel_table(unsigned int channel_11_LC_code, unsigned int count_LC_RX_ch11){
    
    unsigned int rdata_lsb,rdata_msb;
    int t,i=0;
    unsigned int count_LC[16] = {0};
    unsigned int count_targets[17] = {0};
    
    unsigned short nums[16] = {802,904,929,269,949,434,369,578,455,970,139,297,587,109,373,159};
    unsigned short dens[16] = {801,901,924,267,940,429,364,569,447,951,136,290,572,106,362,154};    
        
    
    // Need to adjust here for shift from PA
    radio_vars.tx_channel_codes[0] = channel_11_LC_code;
    
    
    //for(i=0; i<16; i++){
    while(i<16) {
    
        LC_monotonic(radio_vars.tx_channel_codes[i]);
        //analog_scan_chain_write_3B_fromFPGA(&ASC[0]);
        //analog_scan_chain_load_3B_fromFPGA();
                    
        // Reset all counters
        ANALOG_CFG_REG__0 = 0x0000;
        
        // Enable all counters
        ANALOG_CFG_REG__0 = 0x3FFF;    
        
        // Count for some arbitrary amount of time
        for(t=1; t<16000; t++);
        
        // Disable all counters
        ANALOG_CFG_REG__0 = 0x007F;

        // Read count result
        rdata_lsb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x280000);
        rdata_msb = *(unsigned int*)(APB_ANALOG_CFG_BASE + 0x2C0000);
        count_LC[i] = rdata_lsb + (rdata_msb << 16);
        
        // Until figure out why modulation spacing is only 800kHz, only set 400khz above RF channel
        count_targets[i] = (nums[i] * count_LC_RX_ch11) / dens[i];
        //count_targets[i] = ((24054 + i*50) * count_LC_RX_ch11) / 24025;
        //count_targets[i] = ((24055 + i*50) * count_LC_RX_ch11) / 24025;
        
        if(count_LC[i] < (count_targets[i] - 5)){
            radio_vars.tx_channel_codes[i]++;
        }
        else{
            radio_vars.tx_channel_codes[i+1] = radio_vars.tx_channel_codes[i] + 40;
            i++;
        }
    }
    
    //for(i=0; i<16; i++){
    //    printf("\r\nTX ch=%d,  count_LC=%d,  count_targets=%d,  radio_vars.tx_channel_codes=%d",i+11,count_LC[i],count_targets[i],radio_vars.tx_channel_codes[i]);
    //}
    
}

void radio_build_channel_table(unsigned int channel_11_LC_code){
    
        unsigned int count_LC_RX_ch11;
    
        // Make sure in RX mode first
    
        count_LC_RX_ch11 = build_RX_channel_table(channel_11_LC_code);
    
        //printf("--\r\n");
    
        // Switch over to TX mode
    
        // Turn polyphase off for TX
        clear_asc_bit(971);

        // Hi-Z mixer wells for TX
        set_asc_bit(298);
        set_asc_bit(307);

        // Analog scan chain setup for radio LDOs for RX
        clear_asc_bit(504); // = gpio_pon_en_if
        set_asc_bit(506); // = gpio_pon_en_lo
        set_asc_bit(508); // = gpio_pon_en_pa

        build_TX_channel_table(channel_11_LC_code,count_LC_RX_ch11);
        
        radio_rfOff();
}

//=========================== intertupt =======================================

void radio_isr(void) {
    
    unsigned int interrupt = RFCONTROLLER_REG__INT;
    unsigned int error     = RFCONTROLLER_REG__ERROR;
    
    radio_vars.crc_ok   = true;
    if (error != 0) {
        
#ifdef ENABLE_PRINTF
        printf("Radio ERROR\r\n");
#endif
        
        if (error & 0x00000001) {
#ifdef ENABLE_PRINTF
            printf("TX OVERFLOW ERROR\r\n");
#endif
        }
        if (error & 0x00000002) {
#ifdef ENABLE_PRINTF
            printf("TX CUTOFF ERROR\r\n");
#endif
        }
        if (error & 0x00000004) {
#ifdef ENABLE_PRINTF
            printf("RX OVERFLOW ERROR\r\n");
#endif
        }
        
        if (error & 0x00000008) {
#ifdef ENABLE_PRINTF
            printf("RX CRC ERROR\r\n");
#endif
            radio_vars.crc_ok   = false;
        }
        if (error & 0x00000010) {
#ifdef ENABLE_PRINTF
            printf("RX CUTOFF ERROR\r\n");
#endif
        }
        
    }
    RFCONTROLLER_REG__ERROR_CLEAR = error;
    
    if (interrupt & 0x00000001) {
#ifdef ENABLE_PRINTF
        printf("TX LOAD DONE\r\n");
#endif
    }
    
    if (interrupt & 0x00000002) {
#ifdef ENABLE_PRINTF
        printf("TX SFD DONE\r\n");
#endif
        
        if (radio_vars.startFrame_tx_cb != 0) {
            radio_vars.startFrame_tx_cb(RFTIMER_REG__COUNTER);
        }
    }
    
    if (interrupt & 0x00000004){
#ifdef ENABLE_PRINTF
        printf("TX SEND DONE\r\n");
#endif
        
        if (radio_vars.endFrame_tx_cb != 0) {
            radio_vars.endFrame_tx_cb(RFTIMER_REG__COUNTER);
        }
    }
    
    if (interrupt & 0x00000008){
#ifdef ENABLE_PRINTF
        printf("RX SFD DONE\r\n");
#endif
        
        if (radio_vars.startFrame_rx_cb != 0) {
            radio_vars.startFrame_rx_cb(RFTIMER_REG__COUNTER);
        }
    }
    
    if (interrupt & 0x00000010) {
#ifdef ENABLE_PRINTF
        printf("RX DONE\r\n");
#endif
        
        if (radio_vars.endFrame_rx_cb != 0) {
            radio_vars.endFrame_rx_cb(RFTIMER_REG__COUNTER);
        }
    }
    
    RFCONTROLLER_REG__INT_CLEAR = interrupt;
}


// This ISR goes off when the raw chip shift register interrupt goes high
// It reads the current 32 bits and then prints them out after N cycles
void rawchips_32_isr() {
    
    unsigned int jj;
    unsigned int rdata_lsb, rdata_msb;
    
    // Read 32bit val
    rdata_lsb = ANALOG_CFG_REG__17;
    rdata_msb = ANALOG_CFG_REG__18;
    chips[chip_index] = rdata_lsb + (rdata_msb << 16);    
        
    chip_index++;
    
    //printf("x1\r\n");
    
    // Clear the interrupt
    //ANALOG_CFG_REG__0 = 1;
    //ANALOG_CFG_REG__0 = 0;
    acfg3_val |= 0x20;
    ANALOG_CFG_REG__3 = acfg3_val;
    acfg3_val &= ~(0x20);
    ANALOG_CFG_REG__3 = acfg3_val;

    if(chip_index == 10){    
        for(jj=1;jj<10;jj++){
            printf("%X\r\n",chips[jj]);
        }

        ICER = 0x0100;
        ISER = 0x0200;
        chip_index = 0;
        
        // Wait for print to complete
        for(jj=0;jj<10000;jj++);
        
        // Execute soft reset
        *(unsigned int*)(0xE000ED0C) = 0x05FA0004;
    }
}

// With HCLK = 5MHz, data rate of 1.25MHz tested OK
// For faster data rate, will need to raise the HCLK frequency
// This ISR goes off when the input register matches the target value
void rawchips_startval_isr() {
    
    unsigned int rdata_lsb, rdata_msb;
    
    // Clear all interrupts
    acfg3_val |= 0x60;
    ANALOG_CFG_REG__3 = acfg3_val;
    acfg3_val &= ~(0x60);
    ANALOG_CFG_REG__3 = acfg3_val;
        
    // Enable the interrupt for the 32bit 
    ISER = 0x0200;
    ICER = 0x0100;
    ICPR = 0x0200;
    
    // Read 32bit val
    rdata_lsb = ANALOG_CFG_REG__17;
    rdata_msb = ANALOG_CFG_REG__18;
    chips[chip_index] = rdata_lsb + (rdata_msb << 16);
    chip_index++;

    //printf("x2\r\n");

}
