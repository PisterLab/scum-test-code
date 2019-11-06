import pytest
import os
import time
import serial
import threading

# =========================== variables =======================================

BAUDRATE_OPENMOTE  = 115200
BAUDRATE_SCUM      = 19200

pytest.output_openmote   = ''
pytest.output_scum       = ''

# =========================== class ===========================================

class serialReader(threading.Thread):

    def __init__(self,serialport=None, baudrate=None):
        
        self.goOn       = True
        
        self.serialport = serialport
        self.baudrate   = baudrate
        
        self.output     = ''
        
        threading.Thread.__init__(self)
        
    def run(self):
        
        self.serial = serial.Serial(self.serialport,self.baudrate,timeout=1)
        
        while self.goOn:
            output = self.serial.read(100)
            self.output += str(output)
                
        self.serial.close()
        
    def close(self):
        self.goOn = False
        
    def get_output(self):
        return self.output
        
# =========================== test ============================================

def syscall(cmd):
    print '>>> {0}'.format(cmd)
    return os.system(cmd)
    
# ==== variables
    
@pytest.fixture
def serial_openmote():
    port_openmote   = os.environ.get('PORT_OPENMOTE')
    return serialReader(port_openmote, BAUDRATE_OPENMOTE)
    
@pytest.fixture
def serial_scum():
    port_scum       = os.environ.get('PORT_SCUM')
    return serialReader(port_scum, BAUDRATE_SCUM)
    
# ==== tests

def test_compilation():
    syscall("echo compilation...")
    result = syscall("%KEIL_UV_DIR%\\UV4.exe -b scm_v3c\\applications\\pingpong_test\\pingpong_test.uvprojx -o \"output.txt\"")
    assert result == 0
    
def test_bootload(serial_openmote, serial_scum):
    print "start to bootload OpenMote..."
    
    result = syscall("python scm_v3c\\bootload\\cc2538-bsl.py  -e --bootloader-invert-lines -w -b 400000 -p %PORT_OPENMOTE% scm_v3c\\applications\\pingpong_test\\openmote-b-24ghz.ihex")
    assert result == 0
    
    print "connecting to the serial port of SCuM and OpenMote..."
    
    # starting logging the serial output
    serial_openmote.start()
    serial_scum.start()
    
    print "stat to bootload SCuM..."
    
    result = syscall("python scm_v3c\\bootload\\bootload.py -tp %PORT_TEENSY% -i scm_v3c\\applications\\pingpong_test\\pingpong_test.bin")
    assert result == 0
    
    # waiting to let the code run for a while 
    time.sleep(10)
    serial_openmote.close()
    serial_scum.close()
    
    pytest.output_openmote  = serial_openmote.get_output()
    pytest.output_scum      = serial_scum.get_output()
    
    # use pytest -s to see the print out message
    print   'openmote_output\n', pytest.output_openmote
    print   'scum_output\n', pytest.output_scum
    

def test_communication():
    print "start communication test..."

    assert 'Locked' in pytest.output_scum
    
    print "SCuM Locked on the frequency of incoming single."

    assert 'Ptest' in pytest.output_openmote
