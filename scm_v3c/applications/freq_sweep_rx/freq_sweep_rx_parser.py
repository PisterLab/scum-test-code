import matplotlib 
import matplotlib.pyplot as plt
import numpy as np
import os

matplotlib.rcParams.update({'font.size': 12})

# =========================== variables =======================================

LOG_FILE_START                    = 'freq_sweep_rx_output'
RSSI_READ_INDICATOR               = 'with_rssi'


NUM_PKT_PER_SETTING         = 3

#    4ms delay after sending each pkt (for file writing)
# 0.76ms for sending 22 bytes pkt
DURATION_PER_PKT            = 0.005 # second
NUM_CONFIG                  = 32*32*32
RUNNING_DURATION            = NUM_CONFIG*DURATION_PER_PKT*NUM_PKT_PER_SETTING

'''
data = {
    'channel_11':
        {
            'freq_offset'   : [],
            'linear_confg'  : []
        },
    'channel_12':
        {
            'freq_offset'   : [],
            'linear_confg'  : []
        },
        ...
}
'''
# data structure
data = {}
        
# =========================== helper ===========================================

def converter_config_linear_2_text(linear_configure):
    coarse = (linear_configure >> 10) & 0x1f
    middle = (linear_configure >> 5  )& 0x1f
    fine   = (linear_configure       )& 0x1f
    output = "{0}.{1}.{2}".format(coarse, middle, fine)
    return output
    
def converter_config_text_2_linear(text_config, has_rssi):
    linear_config = None
    freq_offset   = None
    rssi          = None
    try:
        if has_rssi:
            ptest, coarse, middle, fine, rssi = text_config.split('.')
            assert int(rssi)>-127
        else:
            ptest, coarse, middle, fine = text_config.split('.')
        assert int(coarse) < 32 and int(middle) < 32 and int(fine) < 32
        linear_config = (int(coarse) << 10) | (int(middle) << 5) | int(fine)
        freq_offset   = int(ptest.split(' ')[0])
    except:
        pass
    return freq_offset, linear_config, rssi
    
# ==== tests

if __name__ == '__main__':

    freq_offset_data = []
    config_data      = []

    for file in os.listdir("./"):
        if file.startswith(LOG_FILE_START) and file.endswith('.txt'):
            has_rssi = False
            if RSSI_READ_INDICATOR in file:
                has_rssi = True
                data[file] = {
                    'freq_offset':  [],
                    'linear_config':[],
                    'rssi':[]
                }
            else:
                data[file] = {
                    'freq_offset':  [],
                    'linear_config':[]
                }
            
            print "parsing {0}...".format(file)
            
            with open(file,'rb') as lf:
                for line in lf:
                    index = line.find('test.')
                    if index != -1:
                        freq_offset, linear_config, rssi = converter_config_text_2_linear(line, has_rssi)
                        if linear_config != None:
                            data[file]['linear_config'].append(linear_config)
                            if freq_offset != None: 
                                data[file]['freq_offset'].append(freq_offset)
                            if rssi != None:
                                data[file]['rssi'].append(rssi)
                            
    for key, item in data.items():
        
        fig, ax = plt.subplots(figsize=(16, 4))
        
        ax.hist(
            item['linear_config'],
            label="channel {0}".format(key.split('_')[-1].split('.')[0])
        )
    
        ax.set_ylabel('num_pkt_received')
        ax.set_xlabel('coarse.middle.fine')
        
        x_s_lim   = 0
        x_e_lim   = NUM_CONFIG
        SCALE     = 1000
        
        # x_s_lim   = 23*32*32
        # x_e_lim   = 29*32*32
        # SCALE     = 200
        
        # x_s_lim   = 22*32*32
        # x_e_lim   = 24*32*32
        # SCALE     = 100
        
        ax.set_xlim(x_s_lim,x_e_lim)
        
        # if item_key == 'freq_offset':
            # ax.set_ylim(-60,60)
        # elif item_key == 'rssi':
            # ax.set_ylim(-120,0)
        
        xticks = [x_s_lim+i*SCALE for i in range((x_e_lim-x_s_lim)/SCALE)]
        xlabel = [converter_config_linear_2_text(i) for i in xticks]
        
        ax.set_xticks(xticks)
        ax.set_xticklabels(xlabel,rotation = 45)
        
        ax.legend(markerscale=0.7, scatterpoints=1)
        ax.grid(True)
        plt.tight_layout()
        plt.savefig("{0}.png".format(key.split('.')[0]))