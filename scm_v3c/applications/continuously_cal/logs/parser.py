import matplotlib 
import matplotlib.pyplot as plt
import matplotlib.gridspec as gridspec

LOG_FILE = 'log_final.txt'
SWEEP_CAL_START_STR        = 'setting_list'
CONTINUOUSLY_CAL_START_STR = 'TX setting'
TEMP_OFFSET = 6

result = {
    'avg_if'        : [],
    'tx_setting'    : [],
    'avg_fo'        : [],
    'rx_setting'    : [],
    'rc_2m_setting' : [],
    'avg_2m_counts' : [],
    'temp'          : []
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
    rc_2m_setting       = None
    avg_fo              = None
    avg_if              = None
    avg_2m_counts       = None
    
    try:
    
        info                = line.split(' ')
        tx_setting          = (int(info[2])<<10) | (int(info[3])<<5)  | (int(info[4]))
        avg_fo              = int(info[5].split('=')[-1][:-1])
        rx_setting          = (int(info[9])<<10)  | (int(info[10])<<5) | (int(info[11]))
        avg_if              = int(info[12].split('=')[-1][:-1])
        rc_2m_setting       = (int(info[16])<<10)  | (int(info[17])<<5) | (int(info[18]))
        avg_2m_counts       = int(info[19].split('=')[-1][:-1])
        temp                = int(info[21].split('=')[-1][:-1]) - TEMP_OFFSET
        
    except:
        print(line.split(' '))
    return tx_setting, avg_if, rx_setting, avg_fo, rc_2m_setting, avg_2m_counts, temp
    
# find maxlength_setting_list

def get_max_length_setting_list(setting_list):

    maxlength       = len(setting_list[0])
    maxlength_list = []
    
    if len(setting_list) > 1:
        for l_sub in setting_list:
            if len(l_sub)>maxlength:
                maxlength = len(l_sub)
                maxlength_list = l_sub
    elif len(setting_list) == 1:
        maxlength_list = setting_list[0]

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

            tx_setting, avg_if, rx_setting, avg_fo, rc_2m_setting, avg_2m_counts, temp = parser_line(line)
            result['avg_if'].append(avg_if)
            result['tx_setting'].append(tx_setting)
            result['avg_fo'].append(avg_fo)
            result['rx_setting'].append(rx_setting)
            result['rc_2m_setting'].append(rc_2m_setting)
            result['avg_2m_counts'].append(avg_2m_counts)
            result['temp'].append(temp)

maxlength_list_tx = get_max_length_setting_list(setting_list_tx)
maxlength_list_rx = get_max_length_setting_list(setting_list_rx)

# counts drift

if_40_ppm   = 0
fo_40_ppm   = 0
twom_1000_ppm = 0

for i in result['avg_if']:
    if i>=481 and i<=519:
        if_40_ppm += 1

for i in result['avg_fo']:
    if i>=-10 and i<=14:
        fo_40_ppm += 1
        
for i in result['avg_2m_counts']:
    if i>=59940 and i<=60060:
        twom_1000_ppm += 1
        
all = float(len(result['avg_2m_counts']))
print("if_40_ppm = {0} fo_40_ppm = {1} 2m_count_1000_ppm = {2}".format(if_40_ppm/all, fo_40_ppm/all, twom_1000_ppm/all))

# plot

for key, raw_data in result.items():

    # roughly 2 samples per second
    x_axis = [i*0.5/60 for i in range(len(raw_data))]
    
    
    if 'setting' in key:
    
        fig, ax = plt.subplots(dpi=300)
    
        DOTSIZE  = 7
        
        if key == 'tx_setting':
            
            setting_line = ax.plot(x_axis, raw_data, '.',  color='#0000FF', label='LC OSC frequency setting (TX)', markersize=DOTSIZE)
            # ax.plot(x_axis, [maxlength_list_tx[0] for i in x_axis], 'k--', markersize=DOTSIZE)
            # ax.plot(x_axis, [maxlength_list_tx[-1] for i in x_axis], 'k--', markersize=DOTSIZE)
            yticks = [16*i + (maxlength_list_tx[0] & 0xFFE0) for i in range(9)]
        
        if key == 'rx_setting':
        
            setting_line = ax.plot(x_axis, raw_data, '.',  color='#0000FF', label='LC OSC frequency setting (RX)', markersize=DOTSIZE)
            # ax.plot(x_axis, [maxlength_list_rx[0] for i in x_axis], 'k--', markersize=DOTSIZE)
            # ax.plot(x_axis, [maxlength_list_rx[-1] for i in x_axis], 'k--', markersize=DOTSIZE)
            yticks = [16*i + (maxlength_list_rx[0] & 0xFFE0) for i in range(9)]
            
        if key == 'rc_2m_setting':
            
            setting_line = ax.plot(x_axis, raw_data, '.',  color='#0000FF', label='2M RC OSC frequency settings', markersize=DOTSIZE)
            yticks = [16*i + (raw_data[0] & 0xFFE0) for i in range(12)]
        
        ylabel = [converter_config_linear_2_text(i) for i in yticks]
        
        ax.set_yticks(yticks)
        ax.set_yticklabels(ylabel)
        ax.set_ylabel('frequency settings (coarse.mid.fine)')
        # ax.legend()
        
        ax2 = ax.twinx()  # instantiate a second axes that shares the same x-axis
        temp_line = ax2.plot(x_axis, result['temp'], 'r-',label='temperature ($^\circ$C)')
        ax2.set_ylim(25, 45)
        ax2.set_ylabel('temperature ($^\circ$C)')
        # ax2.legend()
        if 'failed' in LOG_FILE:
            ax.set_xlim(-0.1,3)
        else:
            ax.set_xlim(-2.5,50)
        ax.set_xlabel('time (minutes)')
        
        lines = setting_line+temp_line
        labels = [l.get_label() for l in lines]
        
        lgnd = ax.legend(lines, labels, markerscale=0.7, scatterpoints=1, loc=0)
        
        lgnd.legendHandles[0]._legmarker.set_markersize(7)
        
        ax.grid(True)
        
        fig.set_size_inches(8, 4)
        
    else:
    
        gs_kw = dict(width_ratios=[3,1])
    
        fig, (ax1, ax2)= plt.subplots(nrows=1, ncols=2,  gridspec_kw=gs_kw, dpi=300)
        
        DOTSIZE  = 5
        
        if key == 'avg_fo':
            bins = 64
            y_lable = "frequency offset"
            target_value = 2
            ppm_boundary = [-14,18]
            
        elif key == 'avg_if':
            bins = 120
            y_lable = "intermediate frequency counts"
            target_value = 500
            ppm_boundary = [480,520]
            
        elif key == 'avg_2m_counts':
            bins = 140
            y_lable = "2M RC frequency counts"
            target_value = 60000
            ppm_boundary = [59940,60060]
            
        target_x = [target_value for i in x_axis]
        
        ax1.plot(x_axis, raw_data, '.', color='#0000FF', markersize=DOTSIZE)
        # ax1.plot(x_axis, [ppm_boundary[0] for i in x_axis], 'r-')
        # ax1.plot(x_axis, [ppm_boundary[1] for i in x_axis], 'r-')
        ax1.set_ylabel(y_lable)
        ax2.hist(raw_data, bins=bins, color='#0000FF', density=False, label='bin={0}'.format(bins), orientation='horizontal')
        ax2.set_xlabel('number of samples')

        # ax1.plot(x_axis, target_x, '-', color='r', linewidth=3, label='target={0}'.format(target_value))
        # ax1.legend(loc=2)
        
        ax2.legend(markerscale=0.7, scatterpoints=1, loc=2)
        ax2.grid(True)
        
        ax1.set_xlim(-2.5,50)  
        ax1.set_xlabel('time (minutes)')

        ax1.grid(True)
        ax2.grid(True)
        
        fig.set_size_inches(16, 4)
    
    # ax.set_xlim(0,17000)
    
    plt.tight_layout()
    plt.savefig('{0}.png'.format(key))
    plt.clf()