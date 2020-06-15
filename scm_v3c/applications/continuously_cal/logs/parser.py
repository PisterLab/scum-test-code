import matplotlib 
import matplotlib.pyplot as plt

LOG_FILE = 'log.txt'
SWEEP_CAL_START_STR        = 'setting_list'
CONTINUOUSLY_CAL_START_STR = 'TX setting:'

result = {
    'avg_if'    : [],
    'tx_setting': [],
    'avg_fo'    : [],
    'rx_setting': [],
    'temp'      : []
}

def converter_config_linear_2_text(linear_configure):
    coarse = (linear_configure >> 10) & 0x1f
    mid = (linear_configure >> 5  )& 0x1f
    fine   = (linear_configure       )& 0x1f
    output = "{0}.{1}.{2}".format(coarse, mid, fine)
    return output
    
def linearize_setting_text(line, txORrx):
    
    if txORrx == 'rx':
        coarse = int(line.split(' ')[-3])
        mid = int(line.split(' ')[-2])
        fine = int(line.split(' ')[-1])
    else:
        coarse = int(line.split(' ')[-4])
        mid = int(line.split(' ')[-3])
        fine = int(line.split(' ')[-2])
        
    linear_config = (int(coarse) << 10) | (int(mid) << 5) | int(fine)
    
    return linear_config
    
def parser_line(line):

    temp                = None
    rx_setting          = None
    tx_setting          = None
    avg_fo              = None
    avg_if              = None
    
    try:
    
        info                = line.split(' ')
        tx_setting          = (int(info[2])<<10) | (int(info[3])<<5)  | (int(info[4]))
        avg_if              = int(info[5].split('=')[-1][:-1])
        rx_setting          = (int(info[9])<<10)  | (int(info[10])<<5) | (int(info[11]))
        avg_fo              = int(info[12].split('=')[-1][:-1])
        temp                = int(info[13].split('=')[-1][:-1])
        
    except:
        print line.split(' ')
    return tx_setting, avg_if, rx_setting, avg_fo, temp
    
# find maxlength_setting_list

def get_max_length_setting_list(setting_list):

    maxlength       = len(setting_list[0])
    maxlength_list = []
    for l_sub in setting_list:
        if len(l_sub)>maxlength:
            maxlength = len(l_sub)
            maxlength_list = l_sub

    # print "{0}.{1}.{2} - {0}.{1}.{3} (len = {4})".format(

         # ((maxlength_list[0]>>10) & 0x001f), ((maxlength_list[0]>>5) & 0x001f), (maxlength_list[0] & 0x001f), (maxlength_list[-1] & 0x001f), maxlength
         
    # )
    
    return maxlength_list

setting_list_tx = []
setting_list_rx = []
tx_setting      = 0
with open(LOG_FILE, 'r') as f:
    for line in f:
        if 'SWEEP_TX' in line:
            tx_setting = 1
        if line.startswith(SWEEP_CAL_START_STR):
            if tx_setting == 0:
                
                linear_config = linearize_setting_text(line, 'rx')
                if len(setting_list_rx) == 0:
                    setting_list_rx.append([linear_config])
                elif (linear_config>>5) == (setting_list_rx[-1][0]>>5):
                    setting_list_rx[-1].append(linear_config)
                else:
                    setting_list_rx.append([linear_config])
            else:
                linear_config = linearize_setting_text(line, 'tx')
                if len(setting_list_tx) == 0:
                    setting_list_tx.append([linear_config])
                elif (linear_config>>5) == (setting_list_tx[-1][0]>>5):
                    setting_list_tx[-1].append(linear_config)
                else:
                    setting_list_tx.append([linear_config])
    
        if line.startswith(CONTINUOUSLY_CAL_START_STR):

            tx_setting, avg_if, rx_setting, avg_fo, temp = parser_line(line)
            result['avg_if'].append(avg_if)
            result['tx_setting'].append(tx_setting)
            result['avg_fo'].append(avg_fo)
            result['rx_setting'].append(rx_setting)
            result['temp'].append(temp)

maxlength_list_tx = get_max_length_setting_list(setting_list_tx)
maxlength_list_rx = get_max_length_setting_list(setting_list_rx)

# plot

for key, raw_data in result.items():
    
    fig, ax = plt.subplots(figsize=(16, 4))

    x_axis = [i for i in range(len(raw_data))]
    
    
    if 'setting' in key:
        ax.plot(x_axis, raw_data, '.', label='freq_setting')
        
        if key == 'tx_setting':
        
            ax.plot(x_axis, [maxlength_list_tx[0] for i in x_axis], 'k--')
            ax.plot(x_axis, [maxlength_list_tx[-1] for i in x_axis], 'k--')
            yticks = [4*i + (maxlength_list_tx[0] & 0xFFE0) for i in range(8)]
        
        if key == 'rx_setting':
        
            ax.plot(x_axis, [maxlength_list_rx[0] for i in x_axis], 'k--')
            ax.plot(x_axis, [maxlength_list_rx[-1] for i in x_axis], 'k--')
            yticks = [4*i + (maxlength_list_rx[0] & 0xFFE0) for i in range(8)]
        
        ylabel = [converter_config_linear_2_text(i) for i in yticks]
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(ylabel)
        ax.set_ylabel('frequency setting')
        ax.legend()
        
        ax2 = ax.twinx()  # instantiate a second axes that shares the same x-axis
        ax2.plot(x_axis, result['temp'], 'r-',label='temperature (C)')
        ax2.set_ylim(25, 40)
        ax2.set_ylabel('temperature')
        ax2.legend()
    else:
        ax.plot(x_axis, raw_data, '.')
        ax.set_ylabel(key)
    
    # ax.set_xlim(0,17000)
     
    ax.legend(markerscale=0.7, scatterpoints=1, loc=2)
    ax.grid(True)
    plt.tight_layout()
    plt.savefig('{0}.png'.format(key))
    plt.clf()