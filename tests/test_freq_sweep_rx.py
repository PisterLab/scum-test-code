import pytest
import os
import time

# =========================== variables =======================================

# =========================== test ============================================

def syscall(cmd):
    print '>>> {0}'.format(cmd)
    return os.system(cmd)
    
# ==== tests

def test_compilation():
    '''
        using the UV4.exe executor to build the project. Pass when return code <=1
        ERRORLEVEL    Description
        0     No Errors or Warnings
        1     Warnings Only
        2     Errors
        3     Fatal Errors
        11    Cannot open project file for writing
        12    Device with given name in not found in database
        13    Error writing project file
        15    Error reading import XML file
        20    Error converting project
        http://www.keil.com/support/man/docs/uv4/uv4_commandline.htm
    '''
    syscall("echo compilation...")
    result = syscall("%KEIL_UV_DIR%\\UV4.exe -b scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx.uvprojx -o \"output.txt\"")
    assert result <= 1
    
def test_bootload():
    syscall("echo bootload SCuM...")
    
    result = syscall("python scm_v3c\\bootload\\bootload.py -tp %PORT_TEENSY% -i scm_v3c\\applications\\freq_sweep_rx\\Objects\\freq_sweep_rx.bin")
    assert result == 0
    
    syscall("echo bootload OpenMote...")
    
    result = syscall("python scm_v3c\\bootload\\cc2538-bsl.py  -e --bootloader-invert-lines -w -b 400000 -p %PORT_OPENMOTE% scm_v3c\\applications\\freq_sweep_rx\\openmote-b-24ghz.ihex")
    assert result == 0
    
def test_logData():
    syscall("echo logging SCuM output...")

    result = syscall("python scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx_logger.py")
    assert result == 0
        
def test_verifyResult():
        
    result = syscall("python scm_v3c\\applications\\freq_sweep_rx\\freq_sweep_rx_parser.py")
    assert result == 0
    
    syscall("echo Frequency Sweep Rx Test complete!")
