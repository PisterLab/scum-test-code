
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
fgen = visa('agilent','USB0::0x0957::0x2C07::MY57801384::0::INSTR');
buffer = 10e6;
set (fgen,'OutputBufferSize',(buffer+125));
set (fgen,'TimeOut',20);
fopen(fgen);
disp(query(fgen, '*IDN?')); % Agilent Technologies,33522B,MY57801384,4.00-1.19-2.00-58-00


dac_settling_code=0; % aka: ASC(823:-1:816)=dec2bin(dac_settling_code,8)-48;
pga_gain=9+1; % gain factor is X+1 if ASC value is X (gain is 0-indexed)
% not used in program, just used for notes

% voltages=.37:-0.001:.25;
% voltages=1.17:-0.001:.87;
% voltages=1.15:-0.0001:1.05;
% voltages=-0.05:0.01:1.2;
voltages=[0.1:0.0002:.4];% 1.2:-0.0001:-.2];
% voltages=.501;
%adc_value_local=zeros(numel(voltages),iterations);
%psu_value_local=zeros(numel(voltages),iterations);
%adc_mode =zeros(1,numel(voltages));

% fprintf(fgen,'OUTPUT2 OFF');
fprintf(fgen,'OUTPUT2:LOAD INF');
fprintf(fgen,'SOURCE2:FUNCTION DC');
fprintf(fgen,'OUTPUT2 ON');
pause(1)

iterations=1;
adc_result=zeros(iterations,numel(voltages));
watchdog=zeros(iterations,numel(voltages));
loop_times = zeros(1, numel(voltages));
tic;
for ii=1:numel(voltages)
    for jj=1:iterations
        % set new voltage
        fprintf(fgen,['SOURCE2:VOLTAGE:OFFSET ' num2str(voltages(ii))]);
        tstart = tic;
        % pause(0.1)
        % get ADC value
        fprintf(s,'a');
        adc_result_local = fgets(s);
        loop_times(ii) = toc(tstart);
        %disp(strcat('VOLTAGE: ', num2str(voltages(ii)), " ---- ", adc_result_local));
        watchdog_local=fgets(s);
        %     disp(adc_result_local);
        adc_result(jj,ii)=str2double(adc_result_local);
        watchdog(jj,ii)=str2double(watchdog_local);
    end
end

fprintf(fgen,'OUTPUT2 OFF');
fclose(s);
fclose(fgen);
filename = strcat('sensor_adc_PSU_loop_scm3b_PGAen_gain_',num2str(pga_gain),'_iter100round_',datestr(now,30));
save(filename);

%%
figure(1)
plot(voltages,round(mean(adc_result,1))); hold on; % plot(voltages,watchdog/10e3);
plot(voltages,round(mean(adc_result,1)),'.'); axis tight
% figure(2)
% plot(voltages,adc_mode,'x'); hold on
% plot(voltages,mean(adc_value_local,2),'.')
% legend('Mode','Mean')
% xlabel('Input voltage')
% ylabel('ADC code')
%figure(2)
%plot(voltages,adc_mode'-mean(adc_value_local,2))
%%

%% inl dnl for adc
if(0)
    %%
range=9400:70000;
input=voltages(range); 
result=adc_result(1,range);
% code=result(range);   

subplot(4,1,[1:2 3 4]);
subplot(4,1,1:2)
plot(input,result,'s')
ylabel('ADC result scaled to 1V VREF (V)'); set(gca,'FontSize',14)
axis tight
grid on
title([{'A'},{'B'}]); set(gca,'FontSize',14)

subplot(4,1,3)
w_avg=(result(end)-result(1))/numel(result);

DNL=(diff(result) - w_avg)/w_avg;
plot(input(2:end),DNL)
title(['DNL max ' num2str(max(DNL),3) ' / min ' num2str(min(DNL),3)]); set(gca,'FontSize',14)
ylabel('DNL'); set(gca,'FontSize',14)
axis tight
% ylim([-4 4])

subplot(4,1,4)
INL=cumsum(DNL);
plot(input(2:end),INL)
title(['INL max ' num2str(max(INL),3) ' / min ' num2str(min(INL),3)]); set(gca,'FontSize',14)
ylabel('INL'); set(gca,'FontSize',14)
axis tight
% ylim([-1 8])
xlabel('Input voltage (V)')
set(gca,'FontSize',14)

end


if(0)
   %% 
load(    'sensor_adc_PSU_loop_gain2_20180711T180229.mat')
load(    'sensor_adc_PSU_loop_gain2_20180711T180229.mat')
load(    'sensor_adc_PSU_loop_gain2_20180711T180229.mat')
load(    'sensor_adc_PSU_loop_gain2_20180711T180229.mat')

cherrypick1=[2373:3828];

% voltage divider conversion ratio
voltage_divided=voltages*978.9/(11992+978.9);

% plot(voltages(cherrypick1),adc_value_local(cherrypick1,2),'.');

input=voltage_divided(cherrypick1); 
input=1:numel(cherrypick1);
result=adc_value_local(cherrypick1);   

subplot(4,1,[1:2 3 4]);
subplot(4,1,1:2)
plot(input,result,'s')
ylabel('ADC value'); set(gca,'FontSize',14)
axis tight
grid on
title([{'A'},{'B'}]); set(gca,'FontSize',14)
w_avg=(result(end)-result(1))/numel(result);
hold on
plot(input,result(1)+w_avg.*(1:numel(result)));
plot.xlabel('Vin');
plot.ylabel('ADC Code');
subplot(4,1,3)
mv_per_bit=1e3/((result(end)-result(1)) / (input(end)-input(1)))



DNL=(diff(result) - w_avg)/w_avg;
plot(input(2:end),DNL)
title(['DNL max ' num2str(max(DNL),3) ' / min ' num2str(min(DNL),3)]); set(gca,'FontSize',14)
ylabel('DNL'); set(gca,'FontSize',14)
axis tight
% ylim([-4 4])

subplot(4,1,4)
INL=cumsum(DNL);
plot(input(2:end),INL)
title(['INL max ' num2str(max(INL),3) ' / min ' num2str(min(INL),3)]); set(gca,'FontSize',14)
ylabel('INL'); set(gca,'FontSize',14)
axis tight
% ylim([-1 8])
xlabel('Input voltage (V)')
set(gca,'FontSize',14)

    
    
end

if(0)
    %%
    range=10:60;
    input=voltages(range);
    result=adc_result(range);
   
    subplot(4,1,[1:2 3 4]);
    subplot(4,1,1:2)
    plot(input,result,'.')
    ylabel('ADC value'); set(gca,'FontSize',14)
    axis tight
    grid on
    title([{'A'},{'B'}]); set(gca,'FontSize',14)
    
    subplot(4,1,3)
    w_avg=(result(end)-result(1))/numel(result);
    mv_per_bit=1e3/((result(end)-result(1)) / (input(end)-input(1)))
    
    
    
    DNL=(diff(result) - w_avg)/w_avg;
    plot(input(2:end),DNL,'x')
    title(['DNL max ' num2str(max(DNL),3) ' / min ' num2str(min(DNL),3)]); set(gca,'FontSize',14)
    ylabel('DNL'); set(gca,'FontSize',14)
    axis tight
    % ylim([-4 4])
    
    subplot(4,1,4)
    INL=cumsum(DNL);
    plot(input(2:end),INL,'s')
    title(['INL max ' num2str(max(INL),3) ' / min ' num2str(min(INL),3)]); set(gca,'FontSize',14)
    ylabel('INL'); set(gca,'FontSize',14)
    axis tight
    % ylim([-1 8])
    xlabel('Input voltage (V)')
    set(gca,'FontSize',14)
end



