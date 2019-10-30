import pytest
import os
import time

# =========================== variables =======================================

LOG_FILE                    = 'freq_sweep_rx_output_channel_11.txt'
TARGET_STRING               = 'test.'

# =========================== test ============================================

def syscall(cmd):
    print '>>> {0}'.format(cmd)
    os.system(cmd)
    
# ==== tests

def test_compilation():
    syscall("echo compilation...")
    result = syscall("%KEIL_UV_DIR%\\UV4.exe -b scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx.uvprojx")
    assert result == None
    
def test_bootload():
    syscall("echo bootload SCuM...")
    
    result = syscall("python scm_v3c\\bootload\\bootload.py -tp %PORT_TEENSY% -i scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx.bin")
    assert result == None
    
    syscall("echo bootload OpenMote...")
    
    result = syscall("python scm_v3c\\bootload\\cc2538-bsl.py  -e --bootloader-invert-lines -w -b 400000 -p %PORT_OPENMOTE% scm_v3c\\applications\\freq_sweep_rx\\openmote-b-24ghz.ihex")
    assert result == None
    
def test_logData():
    syscall("echo logging SCuM output...")

    result = syscall("python scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx_logger.py")
    assert result == None
    
    with open(LOG_FILE,'rb') as f:
        assert TARGET_STRING in f.read()
        
def test_verifyResult():
        
    result = syscall("python scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx_parser.py")
    assert result == None
    
    syscall("echo Frequency Sweep Rx Test complete!")
