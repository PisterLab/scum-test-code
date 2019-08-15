import serial
import random
import os
import sys

# SCM-3B Bootload Script
# BW 11/17/18

# Set COM port and binary file path below
# Supports both optical and wired bootload modes, set flags accordingly

# Parameters
# ---------------------------------------------------------------------
#com_port = '/dev/cu.usbmodem3954801';
#com_port = '/dev/cu.usbmodem4943141';
#com_port = '/dev/cu.usbmodem4002061';
#com_port = '/dev/cu.usbmodem4702721';
#com_port = '/dev/cu.usbmodem3950511';
#com_port = '/dev/cu.usbmodem4072241'
#com_port = '/dev/cu.usbmodem4942921';
com_port = 'COM24'

#binFilePath = "/Users/Filip/Dropbox/Documents/Summer_2018/vmware_share/scm_v3c_154_TSCH_Demo/code.bin"
#binFilePath = r'C:\Users\Thomas\Desktop\SCuM\code.bin'
binFilePath = r'C:\Users\Thomas\Desktop\SCuM\scum-test-code\scm_v3c\code.bin'

# 1 = use optical program mode
# 0 = use wired 3-wire bus mode
boot_mode = 1;

# 1 = do not perform hard reset before optical programming
# 0 = perform hard reset before optical programming
skip_reset = 0;

# 1 = insert CRC for payload integrity checking
# 0 = do not insert CRC
# SCM C code must also be set up for CRC check for this to work
insert_CRC = 1;

# 1 = pad unused payload space with random data and check it with CRC
# 0 = pad with zeros, do not check integrity of padding
# This is useful to check for programming errors over full 64kB payload
pad_random_payload = 0;
# ---------------------------------------------------------------------

# Open COM port to teensy optical programmer
ser = serial.Serial(
    port=com_port,
    baudrate=19200,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    bytesize=serial.EIGHTBITS
)

# Read binary file from Keil
with open(binFilePath, 'rb') as f:
    bindata = bytearray(f.read())

# Need to know how long the binary payload is for computing CRC
code_length = len(bindata) - 1
pad_length = 65536 - code_length - 1

# Optional: pad out payload with random data if desired
# Otherwise pad out with zeros - uC must receive full 64kB
if(pad_random_payload == 1):
	for i in range(pad_length):
		bindata.append(random.randint(0,255))
	code_length = len(bindata) - 1 - 8
else:
	for i in range(pad_length):
		bindata.append(0)

if(insert_CRC == 1):
    # Insert code length at address 0x0000FFF8 for CRC calculation
    # Teensy will use this length value for calculating CRC
	bindata[65528] = code_length % 256 
	bindata[65529] = code_length // 256
	bindata[65530] = 0
	bindata[65531] = 0
	
# Transfer payload to Teensy
ser.write(b'transfersram\n')
print(ser.readline())
# Send the binary data over uart
ser.write(bindata)

if(insert_CRC == 1):
    # Have Teensy calculate 32-bit CRC over the code length 
    # It will store the 32-bit result at address 0x0000FFFC
	ser.write(b'insertcrc\n')

if(boot_mode == 1):
    # Configure parameters for optical TX
	ser.write(b'configopt\n')
	ser.write(b'80\n')	
	ser.write(b'80\n')
	ser.write(b'4\n')
	ser.write(b'80\n')
	
    # Encode the payload into 4B5B for optical transmission
	ser.write(b'encode4b5b\n')

	if(skip_reset == 0):
        # Do a hard reset and then optically boot
		ser.write(b'bootopt4b5b\n')

	else:
        # Skip the hard reset before booting
		ser.write(b'bootopt4b5bnorst\n')

    
    # Display confirmation message from Teensy
	print(ser.readline())

else:
    
    # Execute 3-wire bus bootloader on Teensy
	ser.write(b'boot3wb\n')

    # Display confirmation message from Teensy
	print(ser.readline())


ser.write(b'opti_cal\n');

ser.close() 

