import os
import time
import serial
import threading
import sys

# =========================== variables =======================================

BAUDRATE_OPENMOTE           = 115200
BAUDRATE_SCUM               = 19200

LOG_FILE                    = 'freq_sweep_tx_lc_count.txt'

#    4ms delay after sending each pkt (for file writing)
TIMER_PERIOD                = 0.001 # second
NUM_CONFIG                  = 32*32*32
NUM_SAMPLE                  = 10
RUNNING_DURATION            = NUM_CONFIG*TIMER_PERIOD*NUM_SAMPLE


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

    port_scum       = os.environ.get('PORT_SCUM')
    serial_scum     = serialReader(port_scum, BAUDRATE_SCUM, LOG_FILE)
    
    # starting logging the serial output
    serial_scum.start()
    
    print "running for ", RUNNING_DURATION, 's...'
    
    for progress in range(NUM_CONFIG):
        
        time.sleep(TIMER_PERIOD*NUM_SAMPLE)
        sys.stdout.write("{0}/{1}\r".format((progress+1), NUM_CONFIG))
        sys.stdout.flush()
    
    # wait for 10 seconds more
    time.sleep(10)
    
    serial_scum.close()