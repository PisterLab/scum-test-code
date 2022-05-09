The accompanying openmote project is located: https://github.com/filmak/openwsn-fw/tree/scum_channel_search/projects/common/01bsp_radio_scumhunt

For now, it is in filmak's fork of the openwsn-fw repo. A pull request is pending.

The goal of this project is to find all 16 (or some subset of the 16) 802.15.4 channel settings on a particular SCuM chip via calibration with an OpenMote.

The procedure is as follows:
SCuM will begin by sweeping through the range of TX LC codes as specified in the #define statements at the beginning of channel_cal.c
The contents of the packet are the LC codes. The OpenMote project that should be run alongside this project will wait for SCuM transmissions and log transmit codes.
After SCuM has finished all of its codes, it will do the same procedure for receive (sweeping from low frequency to high frequency). 
During this procedure, the OpenMote that was previously listening to SCuM is now transmitting and waiting for acknowledgements from SCuM.
When SCuM receives one of OpenMote's transmissions (which contain SCuM's transmit codes from the sweep) SCuM will send an ack to the OpenMote.
If SCuM is happy with its receive codes for a channel, it will indicate to the OpenMote to increment channel up and the procedure will continue until all 16 channels, both TX and RX, are found.
