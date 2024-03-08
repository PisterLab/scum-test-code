
// Reverses (reflects) bits in a 32-bit word.
unsigned reverse(unsigned x) {
    x = ((x & 0x55555555) << 1) | ((x >> 1) & 0x55555555);
    x = ((x & 0x33333333) << 2) | ((x >> 2) & 0x33333333);
    x = ((x & 0x0F0F0F0F) << 4) | ((x >> 4) & 0x0F0F0F0F);
    x = (x << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | (x >> 24);
    return x;
}

// ----------------------------- crc32a --------------------------------

/* This is the basic CRC algorithm with no optimizations. It follows the
logic circuit as closely as possible. */

unsigned int crc32c(unsigned char* message, unsigned int length) {
    // unsigned int crc32b(unsigned int yy) {
    int i, j;
    unsigned int byte, crc;

    // unsigned int *message = 0x0000;
    // unsigned int length = 100;

    i = 0;
    crc = 0xFFFFFFFF;
    while (i < length) {
        byte = message[i];          // Get next byte.
        byte = reverse(byte);       // 32-bit reversal.
        for (j = 0; j <= 7; j++) {  // Do eight times.
            if ((int)(crc ^ byte) < 0)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc = crc << 1;
            byte = byte << 1;  // Ready next msg bit.
        }
        i = i + 1;
    }
    return reverse(~crc);
}
