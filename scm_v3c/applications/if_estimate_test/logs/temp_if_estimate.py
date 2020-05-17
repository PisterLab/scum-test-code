import matplotlib 
import matplotlib.pyplot as plt
import numpy as np
import os
import json

matplotlib.rcParams.update({'font.size': 12})

# =========================== variables =======================================

LOG_TXT     = "log_1.txt"


SETTING_OFFSET              = 23*32*32
NUM_CONFIG                  = 2*32*32


# data structure
data = {
    'temp':           [],
    'linear_config':  [],
    'IF_estimate':    [],
    'avg_estimate':   [],
    'LQI_chip_error': [],
}

# =========================== helper ===========================================

def converter_config_linear_2_text(linear_configure):
    coarse = (linear_configure >> 10) & 0x1f
    mid = (linear_configure >> 5  )& 0x1f
    fine   = (linear_configure       )& 0x1f
    output = "{0}.{1}.{2}".format(coarse, mid, fine)
    return output
    
def converter_config_text_2_linear(text_config):
    temp           = None
    linear_config  = None
    IF_estimate    = None
    LQI_chip_error = None
    try:
    
        info                = text_config.split(' ')
        temp                = info[3]
        coarse, mid, fine   = info[5].split('.')
        IF_estimate         = int(info[7])
        LQI_chip_error      = int(info[9][:-2])
        
        assert int(coarse) < 32 and int(mid) < 32 and int(fine) < 32
        linear_config = (int(coarse) << 10) | (int(mid) << 5) | int(fine)
        
    except:
        pass
    return temp, linear_config, IF_estimate, LQI_chip_error
    
# ==== tests

if __name__ == '__main__':

    for file in os.listdir("./"):
        if file == LOG_TXT:
            
            print "parsing {0}...".format(file)
            
            sweep_done = False
            temporal_list = []
            
            with open(file,'rb') as lf:
                for line in lf:
                
                    if line.startswith("Sweep Done!"):
                        sweep_done = True
                        
                    if sweep_done and line.startswith("pkt received"):
                
                        temp, linear_config, IF_estimate, LQI_chip_error = converter_config_text_2_linear(line)
                        if temp != None and linear_config != None and IF_estimate != None and LQI_chip_error != None:
                            data['temp'].append(temp)
                            data['linear_config'].append(linear_config)
                            data['IF_estimate'].append(IF_estimate)
                            data['LQI_chip_error'].append(LQI_chip_error)
                            
                    elif line.startswith("pkt received"):
                        
                        temp, linear_config, IF_estimate, LQI_chip_error = converter_config_text_2_linear(line)
                        if temp != None and linear_config != None and IF_estimate != None and LQI_chip_error != None:
                            if len(temporal_list) == 0:
                                temporal_list.append([linear_config])
                            elif (linear_config>>5) == (temporal_list[-1][0]>>5):
                                temporal_list[-1].append(linear_config)
                            else:
                                temporal_list.append([linear_config])
                                
                        
                    if line.startswith("avg_if_estimate"):
                        data['avg_estimate'].append(int(line.split(' ')[2]))
                        
    # verify data
    assert len(data['linear_config'])>0
    
    maxlength       = len(temporal_list[0])
    maxlength_list = []
    for l_sub in temporal_list:
        if len(l_sub)>maxlength:
            maxlength = len(l_sub)
            maxlength_list = l_sub
    
    print "{0}.{1}.{2} - {0}.{1}.{3} (len = {4})".format(
    
         ((maxlength_list[0]>>10) & 0x001f), ((maxlength_list[0]>>5) & 0x001f), (maxlength_list[0] & 0x001f), (maxlength_list[-1] & 0x001f), maxlength
         
    )
   
    
    for key, raw_data in data.items():
        
        fig, ax = plt.subplots(figsize=(16, 4))

        x_axis = [i for i in range(len(data[key]))]
        
        
        if key == 'linear_config':
            ax.plot(x_axis, data[key], '.', label='freq_setting')
            ax.plot(x_axis, [maxlength_list[0] for i in x_axis], 'k--')
            ax.plot(x_axis, [maxlength_list[-1] for i in x_axis], 'k--')
            
            yticks = [4*i + (maxlength_list[0] & 0xFFE0) for i in range(8)]
            ylabel = [converter_config_linear_2_text(i) for i in yticks]
            
            ax.set_yticks(yticks)
            ax.set_yticklabels(ylabel)
            
            
            ax2 = ax.twinx()  # instantiate a second axes that shares the same x-axis
            ax2.plot(x_axis, data['temp'], 'r-',label='temperature (C)')
            ax2.set_ylim(25, 40)
            ax2.set_ylabel('temperature')
            ax2.legend()
        else:
            ax.plot(x_axis, data[key], '.')
        
        if key == 'linear_config':
            ax.set_ylabel('frequency setting')
            ax.legend()
            ax.set_xlim(0,16000)
        else:
            ax.set_ylabel(key)
         
        ax.legend(markerscale=0.7, scatterpoints=1, loc=2)
        ax.grid(True)
        plt.tight_layout()
        plt.savefig('{0}.png'.format(key))
        plt.clf()
            
