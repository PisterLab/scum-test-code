import matplotlib 
import matplotlib.pyplot as plt
import numpy as np
import os
import json

matplotlib.rcParams.update({'font.size': 12})

# =========================== variables =======================================

LC_COUNT_TX_LOG                    = 'freq_sweep_tx_lc_count.txt'
LC_COUNT_RX_LOG                    = 'freq_sweep_rx_lc_count.txt'

result_to_write                    = 'freq_sweep_lc_count_result.json'

NUM_CONFIG                  = 32*32*32
LC_COUNT_SCALE              = 480*2/0.001


# data structure
data = {
    'q3': {
        'tx': [0 for i in range(NUM_CONFIG)],
        'rx': [0 for i in range(NUM_CONFIG)],
    },
    'q8': {
        'tx': [0 for i in range(NUM_CONFIG)],
        'rx': [0 for i in range(NUM_CONFIG)],
    }
}

# =========================== helper ===========================================

def converter_config_linear_2_text(linear_configure):
    coarse = (linear_configure >> 10) & 0x1f
    mid = (linear_configure >> 5  )& 0x1f
    fine   = (linear_configure       )& 0x1f
    output = "{0}.{1}.{2}".format(coarse, mid, fine)
    return output
    
def converter_config_text_2_linear(text_config):
    linear_config = None
    lc_count      = None
    try:
        coarse, mid, fine, lc_count = text_config.split('.')
        assert int(coarse) < 32 and int(mid) < 32 and int(fine) < 32
        linear_config = (int(coarse) << 10) | (int(mid) << 5) | int(fine)
        lc_count = int(lc_count)
    except:
        pass
    return linear_config, lc_count
    
# ==== tests

if __name__ == '__main__':

    for file in os.listdir("./"):
        if file.endswith('.txt'):
            print "parsing {0}...".format(file)
            
            with open(file,'rb') as lf:
                board = file.split('_')[0]
                name  = file.split('_')[3]
                for line in lf:
                    linear_config, lc_count = converter_config_text_2_linear(line)
                    if linear_config != None and lc_count != None:
                        if data[board][name][linear_config] == 0:
                            data[board][name][linear_config] = lc_count*LC_COUNT_SCALE
    
    # validating the dataset
    
    for board, file in data.items():
        for name, file_data in file.items():
            for i in file_data:
                assert i!=0
        
    # save data to file
    with open(result_to_write,'w') as json_file:
        json.dump(data,json_file)
        
    for board, file in data.items():
        fig, ax = plt.subplots(figsize=(16, 9))
                    
        for i in range(16):
            ax.plot(
                [2405000000+i*5000000 for j in range(len(file[file.keys()[0]]))], 
                '--',
                label='ch{0}'.format(i+11)
            )
        for key, item in file.items():
            ax.plot(item, label=key)
        
        
        # x_s_lim   = 0
        # x_e_lim   = NUM_CONFIG
        # SCALE     = 32*32
        
        x_s_lim   = 24*32*32 - 32
        x_e_lim   = 25*32*32 + 32
        SCALE     = 32
        
        ax.set_ylabel('Hz (converted from lc_count)')
        ax.set_xlabel('coarse.mid.fine')
        
        ax.set_xlim(x_s_lim,x_e_lim)
        ax.set_ylim(2400000000,2450000000)
        
        xticks = [x_s_lim+i*SCALE for i in range((x_e_lim-x_s_lim)/SCALE)]
        xlabel = [converter_config_linear_2_text(i) for i in xticks]
        
        ax.set_xticks(xticks)
        ax.set_xticklabels(xlabel,rotation = 45)
        
        ax.set_title(board)
        
        ax.legend(markerscale=0.7, scatterpoints=1, loc=2)
        ax.grid(True)
        plt.savefig('{0}_freq_sweep_lc_count.png'.format(board))