# OpenMote-CC2538 image

The 01bsp_radio_tx_prog.ihex image will let OpenMote-CC2538  send 20 bytes long frame at channel 11 every 100ms with contain of 1,2,3,4...

# how to program

Make sure OpenMote-CC2538 is in bootload mode, (all 4 leds are on dimly).
To do so, you can press the flash button on the mote or power cycle the mote.

Assuming the port name is COM3 after connecting the mote to PC, run the following command to program OpenMote-CC2538:

'''
 python cc2538-bsl.py -e --bootloader-invert-lines -w -b 400000 -p COM3 01bsp_radio_tx_prog.ihex
'''
