# freq_sweep_tx_simple

Sweeps LC frequencies while transmitting an 8 byte (10 including CRC) 802.15.4 packet

### To receive packets using an OpenMote:
- Follow steps [here](https://crystalfree.atlassian.net/wiki/spaces/SCUM/pages/2029879415/Basic+OpenMote+Setup+for+scum-test-code)
- Checkout `scum_freq_sweep_examples` branch of cloned pisterlab openwsn-fw repo
- Bootload OpenMote program in `projects/common/01bsp_radio_rx`
- Run (in `py2` env): `python projects/common/01bsp_radio_rx/01bsp_radio_rx.py <OPENMOTE_PORT> ASCII` to log packets that are received by the OpenMote
