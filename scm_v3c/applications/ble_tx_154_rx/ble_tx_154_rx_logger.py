import pytest
import os
import time
import serial
import threading
import sys

# =========================== variables =======================================

BAUDRATE_SCUM               = 19200

LOG_FILE                    = 'ble_tx_154_rx_output.txt'

TIMER_PERIOD                = 0.4
RUNNING_DURATION            = 200*TIMER_PERIOD

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

    # port_openmote   = os.environ.get('PORT_OPENMOTE')
    port_scum       = os.environ.get('PORT_SCUM')
    # serial_openmote = serialReader(port_openmote, BAUDRATE_OPENMOTE, LOG_FILE)
    serial_scum     = serialReader(port_scum, BAUDRATE_SCUM, LOG_FILE)

    # starting logging the serial output
    # serial_openmote.start()
    serial_scum.start()

    print "running for ", RUNNING_DURATION, 's...'

    # serial_openmote.close()
    serial_scum.close()
