clear; close all

openInst = instrfindall;
if ~isempty(openInst)
    fclose(openInst);
    delete(openInst)
end
clear openInst Instruments


s = serial('COM7','BaudRate',115200);         % Machine specific com port
s.OutputBufferSize = 2000; 
s.InputBufferSize = 2000; 
fopen(s);

% fgen = visa('agilent','GPIB1::10::INSTR');
% fgen = visa('agilent','USB0::0x0957::0x2C07::MY57801384::0::INSTR');
% buffer = 10e6;
% set (fgen,'OutputBufferSize',(buffer+125));
% set (fgen,'TimeOut',20);
% fopen(fgen);
% disp(query(fgen, '*IDN?')); % Agilent Technologies,33522B,MY57801384,4.00-1.19-2.00-58-00

s_TI = serial('COM22','BaudRate',19200);
fopen(s_TI);

dac_settling_code=0; % aka: ASC(823:-1:816)=dec2bin(dac_settling_code,8)-48;
pga_gain=26+1; % gain factor is X+1 if ASC value is X (gain is 0-indexed)
% not used in program, just used for notes

% duration=round(3*3600/1.17); % 100x adc loops takes, on avg, 1.17s
duration=round(3*3600/5.94);
% duration=10;

% fprintf(fgen,'OUTPUT2 OFF');
% fprintf(fgen,'OUTPUT2:LOAD INF');
% fprintf(fgen,'SOURCE2:FUNCTION DC');
% fprintf(fgen,'OUTPUT2 ON');
% pause(1)

iterations=100;
adc_result=zeros(iterations,duration);
watchdog=zeros(iterations,duration);
loop_times = zeros(1,duration);
temp_TI=zeros(iterations,duration);

for ii=1:duration
    for jj=1:iterations
        % set new voltage
        % fprintf(fgen,['SOURCE2:VOLTAGE:OFFSET ' num2str(voltages(ii))]);
        tstart = tic;
        %pause(0.1)
        % get ADC value
        fprintf(s_TI, 'temp');
        temp_TI(jj,ii) = eval(fgets(s_TI));
        fprintf(s,'a');
        adc_result_local = fgets(s);
        loop_times(ii) = toc(tstart);
        %disp(strcat('VOLTAGE: ', num2str(voltages(ii)), " ---- ", adc_result_local));
        watchdog_local=fgets(s);
        %     disp(adc_result_local);
        adc_result(jj,ii)=str2double(adc_result_local);
        watchdog(jj,ii)=str2double(watchdog_local);
    end
    disp(strcat('ADC CODE: ', adc_result_local, ' TEMP: ', num2str(temp_TI(jj,ii))));
    
end


% fprintf(fgen,'OUTPUT2 OFF');
fclose(s);
% fclose(fgen);
filename = strcat('sensor_adc_temp_loop_scm3b_PGAen_gain_',num2str(pga_gain),'_iter100round_',datestr(now,30));
save(filename);

%%
close
figure(1)
% plot(temp_TI(1,:),(max(adc_result,[],1))); hold on;% plot(voltages,watchdog/10e3);
% plot(temp_TI(1,:),(max(adc_result,[],1)),'.'); axis tight; grid on
plot(temp_TI(1,:),(adc_result),'.'); axis tight; grid on; hold on
plot(temp_TI(1,:),mean(adc_result,1),'k','linewidth',2); hold off;% plot(voltages,watchdog/10e3);
ylabel('ADC code')
xlabel('TI TMP102 probe temperature [\circC]')
title([{'SCM3B Temperature sensor w/ PGA gain = 10'} {'100 iterations plotted; black line is mean'}])
ylim([120 132])
xlim([-5 75])
% saveas(gcf,[filename '.png'])

% figure(2)
% plot(voltages,adc_mode,'x'); hold on
% plot(voltages,mean(adc_value_local,2),'.')
% legend('Mode','Mean')
% xlabel('Input voltage')
% ylabel('ADC code')
%figure(2)
%plot(voltages,adc_mode'-mean(adc_value_local,2))
