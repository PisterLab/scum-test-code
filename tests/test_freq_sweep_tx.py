import pytest
import os
import time

# =========================== variables =======================================

LOG_FILE                    = 'freq_sweep_output.txt'
TARGET_STRING               = 'Ptest'

# =========================== test ============================================

def syscall(cmd):
    print '>>> {0}'.format(cmd)
    os.system(cmd)
    
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
    result = syscall("%KEIL_UV_DIR%\\UV4.exe -b scm_v3c\\applications\\freq_sweep_tx\\freq_sweep_tx.uvprojx")
    assert result == None
    
def test_bootload(serial_openmote, serial_scum):
    syscall("echo bootload...")
    
    result = syscall("python scm_v3c\\bootload\\bootload.py -tp %PORT_TEENSY% -i scm_v3c\\applications\\freq_sweep_tx\\freq_sweep_tx.bin")
    assert result == None

def test_communication():
    syscall("echo frequency_sweep_test...")

    result = syscall("python scm_v3c\\applications\\freq_sweep_tx\\freq_sweep_tx_logger.py")
    assert result == None
    
    with open(LOG_FILE,'rb') as f:
        assert TARGET_STRING in f.read()
        
    result = syscall("python scm_v3c\\applications\\freq_sweep_tx\\freq_sweep_parser.py")
    assert result == None
    
    syscall("echo Frequency Sweep Test complete!")
