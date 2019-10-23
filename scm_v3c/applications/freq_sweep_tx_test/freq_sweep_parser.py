import matplotlib 
import matplotlib.pyplot as plt
import numpy as np

matplotlib.rcParams.update({'font.size': 15})

# =========================== variables =======================================

LOG_FILE                    = 'freq_sweep_output.txt'


NUM_PKT_PER_SETTING         = 3

#    4ms delay after sending each pkt (for file writing)
# 0.76ms for sending 22 bytes pkt
DURATION_PER_PKT            = 0.005 # second
NUM_CONFIG                  = 32*32*32
RUNNING_DURATION            = NUM_CONFIG*DURATION_PER_PKT*NUM_PKT_PER_SETTING

SCALE                       = 1000
        
# =========================== helper ===========================================

def converter_config_linear_2_text(linear_configure):
    coarse = (linear_configure >> 10) & 0x1f
    middle = (linear_configure >> 5  )& 0x1f
    fine   = (linear_configure       )& 0x1f
    output = "{0}.{1}.{2}".format(coarse, middle, fine)
    return output
    
def converter_config_text_2_linear(text_config):
    linear_config = None
    freq_offset   = None
    try:
        ptest, coarse, middle, fine = text_config.split('.')
        linear_config = (int(coarse) << 10) | (int(middle) << 5) | int(fine)
        freq_offset   = int(ptest.split(' ')[0])
    except:
        pass
    return freq_offset, linear_config
    
# ==== tests

if __name__ == '__main__':

    freq_offset_data = []
    config_data      = []

    with open(LOG_FILE,'r') as lf:
        for line in lf:
            index = line.find('Ptest.')
            if index != -1:
                freq_offset, linear_config = converter_config_text_2_linear(line)
                if freq_offset != None and linear_config != None:
                    freq_offset_data.append(freq_offset)
                    config_data.append(linear_config)
                    print freq_offset, linear_config
    
    fig, ax = plt.subplots()
    ax.scatter(config_data, freq_offset_data, alpha=0.6)
    
    ax.set_ylabel('FREQOFFSET on channel 11 (step 7800Hz)')
    ax.set_xlabel('coarse.middle.fine')
    
    ax.set_xlim(0,NUM_CONFIG)
    
    xticks = [i*SCALE for i in range(NUM_CONFIG/SCALE)]
    xlabel = [converter_config_linear_2_text(i*SCALE) for i in range(NUM_CONFIG/SCALE)]

    ax.set_xticks(xticks)
    ax.set_xticklabels(xlabel,rotation = 45)
    
    ax.grid(True)
    plt.show()