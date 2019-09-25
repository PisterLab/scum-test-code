% This script will open a COM port to SCM and wait to receive
% azimuth/elevation data points and will then plot them.

close all
clear all
clc
delete(instrfind)

% COM port to SCM
cortexCOM = 'COM12';

% Open COM port to SCM (assuming HCLK=10MHz)
cortex = serial(cortexCOM,'BaudRate',19200*2);
cortex.OutputBufferSize = 1e4;
cortex.InputBufferSize = 1e6;
cortex.TimeOut = 15;
fopen(cortex);

% Loop forever until Ctrl-C
while(1)

% How many plots to gather before each iteration of plotting
for ii = 1:10
    
% Cortex returns az/el values in pairs only when valid lighthouse pulses
% were found.  The returned values are the number of ticks of the 10 MHz
% HF_CLOCK/2 which occurred from the rising edge of the sync pulse to the
% rising edge of the sweep pulse.

% Read values
reply = fgetl(cortex);

    if(reply(1)=='a')
        azimuth_ticks(ii) = str2double(reply(3:end));

        reply = fgetl(cortex);
        elevation_ticks(ii) = str2double(reply(3:end));
    end
end

% Remove any zero-values data points
azimuth_ticks(azimuth_ticks==0)=[];
elevation_ticks(elevation_ticks==0)=[];

% Convert to degrees
azimuth_degrees = (azimuth_ticks / 10e6) * 377 * 180/pi;
elevation_degrees = (elevation_ticks / 10e6) * 377 * 180/pi;

% Plot
clf
scatter(azimuth_degrees,elevation_degrees,'x')
hold on
scatter(mean(azimuth_degrees),mean(elevation_degrees),'o')

xlabel('Azimuth [deg]');
ylabel('Elevation [deg]');
set(gca,'Fontsize',18)
axis([0 180 0 180])
grid on
axis square
axis([0 180 0 180])

pause(0.2)


end

fclose(cortex);
