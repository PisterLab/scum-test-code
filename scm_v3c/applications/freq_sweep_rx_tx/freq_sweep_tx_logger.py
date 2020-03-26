import pytest
import os
import time
import serial
import threading
import sys

# =========================== variables =======================================

BAUDRATE_OPENMOTE           = 115200
BAUDRATE_SCUM               = 19200

pytest.output_openmote      = ''
pytest.output_scum          = ''

LOG_FILE                    = 'freq_sweep_rx_tx_output_with_rssi_channel_11.txt'


NUM_PKT_PER_SETTING         = 3

#    4ms delay after sending each pkt (for file writing)
# 0.76ms for sending 22 bytes pkt
DURATION_PER_PKT            = 0.005 # second
NUM_CONFIG                  = 32*32*32
RUNNING_DURATION            = NUM_CONFIG*DURATION_PER_PKT*NUM_PKT_PER_SETTING

SCALE                       = 100

# =========================== class ===========================================

class serialReader(threading.Thread):

    def __init__(self,serialport=None, baudrate=None, log_file=None):
        
        self.goOn       = True
        
        self.serialport = serialport
        self.baudrate   = baudrate
        
        self.output     = ''
        self.log_file   = log_file
        
        threading.Thread.__init__(self)
        
    def run(self):
        
        self.serial = serial.Serial(self.serialport,self.baudrate,timeout=1)
        
        while self.goOn:
            output = self.serial.readline()
            if not (self.log_file is None):
                with open(self.log_file,'a') as lf:
                    lf.write(output)
                
        self.serial.close()
        
    def close(self):
        self.goOn = False
        
    def get_output(self):
        return self.output
        
# =========================== test ============================================
    
# ==== tests

if __name__ == '__main__':

    port_openmote   = os.environ.get('PORT_OPENMOTE')
    port_scum       = os.environ.get('PORT_SCUM')
    serial_openmote = serialReader(port_openmote, BAUDRATE_OPENMOTE, LOG_FILE)
    # serial_scum     = serialReader(port_scum, BAUDRATE_SCUM, LOG_FILE)
    
    # starting logging the serial output
    serial_openmote.start()
    # serial_scum.start()
    
    print "running for ", RUNNING_DURATION, 's...'
    
    for progress in range(NUM_CONFIG/SCALE):
        
        time.sleep(DURATION_PER_PKT*NUM_PKT_PER_SETTING*SCALE)
        sys.stdout.write("{0}/{1}\r".format((progress+1)*SCALE, NUM_CONFIG))
        sys.stdout.flush()
    
    serial_openmote.close()
    # serial_scum.close()