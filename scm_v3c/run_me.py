import numpy as np
import scipy as sp
import matplotlib.pyplot as plt
import visa
import csv
import serial
import time
import random

# Bootload imports
from bootload import *

# ADC imports
from sensor_adc.adc import *
from sensor_adc.adc_fsm import *
from sensor_adc.data_handling import *

if __name__ == "__main__":
    programmer_port = "COM5"
    scm_port = "COM16"
   
    # Programming SCM 
    if True:
        program_cortex_specs = dict(teensy_port=programmer_port,
                                    uart_port=None, #scm_port,
                                    file_binary='./code.bin',
                                    boot_mode='optical',
                                    skip_reset=False,
                                    insert_CRC=True,
                                    pad_random_payload=False)

        program_cortex(**program_cortex_specs)

    # ADC Settings
    control_mode = 'loopback'
    read_mode = 'uart'
    
    
    if False:
       test_adc_spot_specs = dict(
               port=scm_port,
               control_mode=control_mode,
               read_mode=read_mode,
               iterations=1000) 

       adc_out = test_adc_spot(**test_adc_spot_specs)

       

