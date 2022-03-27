# log_imu

Repeatedly logs accelerometer and gyroscope measurements from an IMU.

### GPIO setup (See configuration in spi.c):

IMU Chip select: GPO15

IMU Clock:       GPO14

IMU Data in:     GPI13

IMU Data out:    GPO12

### IMU setup:
Ensure IMU_VDDIO and IMU_VDD are set to 1.8V. On Sulu boards this can be accomplished putting pin headers on the VDDIO to IMM_VDDIO and VDDIO to IMM_VDD connections (assuming VDDIO = VBAT = 1.8V).