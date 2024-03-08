#include <string.h>

#include "memory_map.h"
#include "optical.h"
#include "scm3c_hw_interface.h"
#include "spi.h"

//=========================== defines =========================================

#define CRC_VALUE (*((unsigned int*)0x0000FFFC))
#define CODE_LENGTH (*((unsigned int*)0x0000FFF8))

//=========================== variables =======================================

typedef struct {
    imu_data_t imu_measurement;
} app_vars_t;

app_vars_t app_vars;

//=========================== prototypes ======================================

//=========================== main ============================================

int main(void) {
    uint32_t i;

    memset(&app_vars, 0, sizeof(app_vars_t));

    initialize_mote();
    crc_check();
    perform_calibration();

    // Configure GPIO
    // Set GPI enables
    // Hex entry 1: 0x2 = 2 = 0b0010 = GPI 13 on for IMU
    GPI_enables(0x2000);

    // Set GPO enables
    // Hex entry 1: 0xD = 14 = 0b1101 = GPO 12,14,15 on for IMU
    GPO_enables(0xD000);

    // Program analog scan chain (update GPIO configs)
    analog_scan_chain_write();
    analog_scan_chain_load();

    initialize_imu();
    test_imu_life();

    while (1) {
        read_all_imu_data(&app_vars.imu_measurement);
        log_imu_data(&app_vars.imu_measurement);

        for (i = 0; i < 100000; i++);
    }
}

//=========================== public ==========================================

//=========================== private =========================================
