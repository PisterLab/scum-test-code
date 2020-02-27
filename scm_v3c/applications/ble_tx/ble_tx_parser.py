import os
import re

# =========================== variables =======================================

LOG_FILE                          = 'ble_tx_output.txt'

# =========================== helper ===========================================

# ==== tests

if __name__ == '__main__':

    num_tx = 0

    with (open(LOG_FILE), 'r') as lf:
        for line in lf:
            if re.match("Transmitting on \d+ \d+ \d+", line) is not None:
                num_tx += 1

    assert num_tx > 100, "Only received {0} TX lines".format(num_tx)
