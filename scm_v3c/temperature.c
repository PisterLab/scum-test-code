#include "temperature.h"
#include "scm3c_hw_interface.h"
#include "Memory_map.h"
#include "rftimer.h"
#include "fixed-point.h"

/* Uses RF Timer to measure 2MHz and 32kHz clock counts over MEASUREMENT_TIME_MILLISECONDS. Then calculates the ratio
 * of these clocks and uses that ratio to calculate the returned value as follows:
 * return CLOCK_RATIO_VS_TEMP_OFFSET + (CLOCK_RATIO_VS_TEMP_SLOPE * ratio).
 * CLOCK_RATIO_VS_TEMP_OFFSET and CLOCK_RATIO_VS_TEMP_SLOPE represent the parameters of a linear regression model used
 * to relate the ratio fo the 2MHz and 32kHz clocks to a temperature value in degrees Celcius.
 */
double get_2MHz_32k_ratio_temp_estimate(unsigned int measurement_time_milliseconds, double clock_ratio_vs_temp_slope,
	double clock_ratio_vs_temp_offset) {
	unsigned int count_2M, count_32k;
	double ratio;
		
	// Measure the 2MHz and 32kHz counters over MEASUREMENT_TIME_MILLISECONDS	
	read_counters_duration(measurement_time_milliseconds);
	count_2M = scm3c_hw_interface_get_count_2M();
	count_32k = scm3c_hw_interface_get_count_32k();
	
	reset_counters();
	enable_counters();
		
	// Calculate the ratio between the 2M and the 32kHZz clocks
	ratio = fix_double(fix_div(fix_init(count_2M), fix_init(count_32k)));
	
	// Using our linear model that we fit based on fixed point temperature measurement
	// we can determine an estimate for temperature.
	return clock_ratio_vs_temp_slope * ratio + clock_ratio_vs_temp_offset;
}
