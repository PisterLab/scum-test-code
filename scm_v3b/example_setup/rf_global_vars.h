// Set aside sections of address space for the packet
char send_packet[127] __attribute__((aligned(4)));
char recv_packet[130] __attribute__((aligned(4)));
