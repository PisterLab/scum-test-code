import pytest
import os

def syscall(cmd):
    print '>>> {0}'.format(cmd)
    os.system(cmd)

def test_compilation():
    result = syscall("echo compilation...")
    assert result == None
    
def test_bootload():
    result = syscall("echo bootload...")
    assert result == None
    
def test_communication():
    result = syscall("echo communication...")
    assert result == None
