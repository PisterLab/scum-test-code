void gen_ble_packet(unsigned char *packet, unsigned char *AdvA, unsigned char channel);
void radio_init_tx_BLE(void);
void gen_test_ble_packet(unsigned char *packet);
void load_tx_arb_fifo(unsigned char *packet);
void transmit_tx_arb_fifo();
void radio_init_rx_ZCC_BLE(void);
void initialize_mote_ble(void);
