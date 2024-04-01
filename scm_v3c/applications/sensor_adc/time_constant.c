#include "time_constant.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "adc_msb.h"
#include "fixed_point.h"

// Maximum number of ADC samples.
#define TIME_CONSTANT_MAX_NUM_ADC_SAMPLES 5000

// Maximum number of samples since the minimum ADC sample to determine whether
// sufficient ADC samples have been received.
#define TIME_CONSTANT_MAX_NUM_SAMPLES_SINCE_MINIMUM 150

// Number of samples to average at the end to find the minimum ADC sample.
#define TIME_CONSTANT_NUM_AVERAGES_FOR_MIN_ADC_SAMPLE 100

// Empirical ADC standard deviation in LSBs.
#define TIME_CONSTANT_ADC_STDDEV 5

// Time constant analysis state enum.
typedef enum {
    TIME_CONSTANT_STATE_INVALID = -1,
    TIME_CONSTANT_STATE_IDLE = 0,
    TIME_CONSTANT_STATE_RECEIVING = 1,
    TIME_CONSTANT_STATE_HAS_SUFFICIENT_SAMPLES = 2,
    TIME_CONSTANT_STATE_DONE = 3,
} time_constant_state_t;

// Time constant measurement period in milliseconds.
static uint32_t g_time_constant_sampling_period_ms = 0;  // ms

// Time constant state.
static time_constant_state_t g_time_constant_state =
    TIME_CONSTANT_STATE_INVALID;

// Number of ADC samples in the buffer.
static size_t g_time_constant_num_adc_samples = 0;

// Buffer for the ADC samples.
static uint16_t g_time_constant_adc_samples[TIME_CONSTANT_MAX_NUM_ADC_SAMPLES];

// Minimum ADC sample in the buffer.
static uint16_t g_time_constant_min_adc_sample = ADC_MAX_THEORETICAL_ADC_SAMPLE;

// Number of samples since the minimum ADC sample was updated.
static size_t g_time_constant_num_samples_since_minimum = 0;

// Natural logarithm lookup table.
static const fixed_point_short_t g_time_constant_ln_lookup_table[] = {
    0,      0,      11357,  18000,  22713,  26369,  29356,  31882,  34070,
    35999,  37726,  39287,  40713,  42024,  43238,  44369,  45426,  46419,
    47356,  48242,  49082,  49881,  50644,  51372,  52069,  52738,  53381,
    53999,  54595,  55170,  55725,  56262,  56783,  57287,  57776,  58251,
    58712,  59161,  59598,  60024,  60439,  60843,  61238,  61624,  62000,
    62368,  62728,  63081,  63426,  63764,  64095,  64419,  64737,  65049,
    65356,  65656,  65951,  66241,  66526,  66806,  67082,  67353,  67619,
    67881,  68139,  68393,  68643,  68890,  69132,  69372,  69607,  69840,
    70069,  70295,  70518,  70738,  70955,  71169,  71380,  71589,  71795,
    71999,  72200,  72398,  72595,  72788,  72980,  73169,  73357,  73542,
    73725,  73906,  74085,  74262,  74437,  74611,  74782,  74952,  75120,
    75286,  75451,  75614,  75776,  75935,  76094,  76250,  76406,  76560,
    76712,  76863,  77013,  77161,  77308,  77454,  77598,  77741,  77883,
    78023,  78163,  78301,  78438,  78574,  78709,  78843,  78975,  79107,
    79238,  79367,  79496,  79623,  79750,  79875,  80000,  80123,  80246,
    80368,  80489,  80609,  80728,  80846,  80964,  81080,  81196,  81311,
    81425,  81539,  81651,  81763,  81874,  81985,  82094,  82203,  82311,
    82419,  82525,  82631,  82737,  82842,  82946,  83049,  83152,  83254,
    83355,  83456,  83556,  83656,  83755,  83853,  83951,  84048,  84145,
    84241,  84337,  84432,  84526,  84620,  84713,  84806,  84898,  84990,
    85081,  85172,  85262,  85352,  85442,  85530,  85619,  85706,  85794,
    85881,  85967,  86053,  86139,  86224,  86309,  86393,  86477,  86560,
    86643,  86726,  86808,  86889,  86971,  87052,  87132,  87212,  87292,
    87371,  87450,  87529,  87607,  87685,  87762,  87839,  87916,  87993,
    88069,  88144,  88220,  88295,  88369,  88443,  88517,  88591,  88664,
    88737,  88810,  88882,  88954,  89026,  89097,  89169,  89239,  89310,
    89380,  89450,  89519,  89589,  89658,  89726,  89795,  89863,  89931,
    89998,  90066,  90133,  90199,  90266,  90332,  90398,  90464,  90529,
    90594,  90659,  90724,  90788,  90852,  90916,  90980,  91043,  91106,
    91169,  91232,  91294,  91356,  91418,  91480,  91541,  91603,  91664,
    91725,  91785,  91845,  91906,  91965,  92025,  92085,  92144,  92203,
    92262,  92320,  92379,  92437,  92495,  92553,  92610,  92668,  92725,
    92782,  92839,  92895,  92952,  93008,  93064,  93120,  93175,  93231,
    93286,  93341,  93396,  93451,  93505,  93560,  93614,  93668,  93722,
    93775,  93829,  93882,  93935,  93988,  94041,  94093,  94146,  94198,
    94250,  94302,  94354,  94405,  94457,  94508,  94559,  94610,  94661,
    94712,  94762,  94813,  94863,  94913,  94963,  95012,  95062,  95111,
    95161,  95210,  95259,  95308,  95356,  95405,  95453,  95501,  95550,
    95598,  95645,  95693,  95741,  95788,  95835,  95882,  95929,  95976,
    96023,  96070,  96116,  96163,  96209,  96255,  96301,  96347,  96392,
    96438,  96483,  96529,  96574,  96619,  96664,  96709,  96753,  96798,
    96842,  96887,  96931,  96975,  97019,  97063,  97107,  97150,  97194,
    97237,  97281,  97324,  97367,  97410,  97453,  97495,  97538,  97580,
    97623,  97665,  97707,  97749,  97791,  97833,  97875,  97917,  97958,
    97999,  98041,  98082,  98123,  98164,  98205,  98246,  98287,  98327,
    98368,  98408,  98448,  98489,  98529,  98569,  98609,  98648,  98688,
    98728,  98767,  98807,  98846,  98885,  98924,  98964,  99002,  99041,
    99080,  99119,  99157,  99196,  99234,  99273,  99311,  99349,  99387,
    99425,  99463,  99501,  99538,  99576,  99614,  99651,  99688,  99726,
    99763,  99800,  99837,  99874,  99911,  99948,  99984,  100021, 100057,
    100094, 100130, 100167, 100203, 100239, 100275, 100311, 100347, 100383,
    100418, 100454, 100490, 100525, 100561, 100596, 100631, 100666, 100701,
    100737, 100771, 100806, 100841, 100876, 100911, 100945, 100980, 101014,
    101049, 101083, 101117, 101151, 101185, 101219, 101253, 101287, 101321,
    101355, 101389, 101422, 101456, 101489, 101523, 101556, 101589, 101622,
    101655, 101689, 101722, 101754, 101787, 101820, 101853, 101886, 101918,
    101951, 101983, 102016, 102048, 102080, 102112, 102145, 102177, 102209,
    102241, 102273, 102304, 102336, 102368, 102400, 102431, 102463, 102494,
    102526, 102557, 102588, 102620, 102651, 102682, 102713, 102744, 102775,
    102806, 102837, 102867, 102898, 102929, 102959, 102990, 103020, 103051,
    103081, 103111, 103142, 103172, 103202, 103232, 103262, 103292, 103322,
    103352, 103382, 103411, 103441, 103471, 103500, 103530, 103559, 103589,
    103618, 103648, 103677, 103706, 103735, 103764, 103794, 103823, 103852,
    103880, 103909, 103938, 103967, 103996, 104024, 104053, 104081, 104110,
    104138, 104167, 104195, 104224, 104252, 104280, 104308, 104336, 104364,
    104392, 104420, 104448, 104476, 104504, 104532, 104560, 104587, 104615,
    104643, 104670, 104698, 104725, 104753, 104780, 104807, 104835, 104862,
    104889, 104916, 104943, 104970, 104997, 105024, 105051, 105078, 105105,
    105132, 105158, 105185, 105212, 105238, 105265, 105292, 105318, 105345,
    105371, 105397, 105424, 105450, 105476, 105502, 105528, 105555, 105581,
    105607, 105633, 105659, 105685, 105710, 105736, 105762, 105788, 105813,
    105839, 105865, 105890, 105916, 105941, 105967, 105992, 106018, 106043,
    106068, 106093, 106119, 106144, 106169, 106194, 106219, 106244, 106269,
    106294, 106319, 106344, 106369, 106394, 106418, 106443, 106468, 106493,
    106517, 106542, 106566, 106591, 106615, 106640, 106664, 106688, 106713,
    106737, 106761, 106786, 106810, 106834, 106858, 106882, 106906, 106930,
    106954, 106978, 107002, 107026, 107050, 107073, 107097, 107121, 107145,
    107168, 107192, 107215, 107239, 107263, 107286, 107309, 107333, 107356,
    107380, 107403, 107426, 107450, 107473, 107496, 107519, 107542, 107565,
    107588, 107611, 107634, 107657, 107680, 107703, 107726, 107749, 107772,
    107794, 107817, 107840, 107863, 107885, 107908, 107930, 107953, 107975,
    107998, 108020, 108043, 108065, 108088, 108110, 108132, 108155, 108177,
    108199, 108221, 108243, 108265, 108288, 108310, 108332, 108354, 108376,
    108398, 108420, 108441, 108463, 108485, 108507, 108529, 108550, 108572,
    108594, 108615, 108637, 108659, 108680, 108702, 108723, 108745, 108766,
    108788, 108809, 108831, 108852, 108873, 108894, 108916, 108937, 108958,
    108979, 109001, 109022, 109043, 109064, 109085, 109106, 109127, 109148,
    109169, 109190, 109211, 109231, 109252, 109273, 109294, 109315, 109335,
    109356, 109377, 109397, 109418, 109439, 109459, 109480, 109500, 109521,
    109541, 109562, 109582, 109602, 109623, 109643, 109663, 109684, 109704,
    109724, 109744, 109765, 109785, 109805, 109825, 109845, 109865, 109885,
    109905, 109925, 109945, 109965, 109985, 110005, 110025, 110045, 110065,
    110084, 110104, 110124, 110144, 110163, 110183, 110203, 110222, 110242,
    110261, 110281, 110301, 110320, 110340, 110359, 110378, 110398, 110417,
    110437, 110456, 110475, 110495, 110514, 110533, 110552, 110572, 110591,
    110610, 110629, 110648, 110667, 110687, 110706, 110725, 110744, 110763,
    110782, 110801, 110819, 110838, 110857, 110876, 110895, 110914, 110933,
    110951, 110970, 110989, 111008, 111026, 111045, 111064, 111082, 111101,
    111119, 111138, 111157, 111175, 111194, 111212, 111231, 111249, 111267,
    111286, 111304, 111323, 111341, 111359, 111377, 111396, 111414, 111432,
    111450, 111469, 111487, 111505, 111523, 111541, 111559, 111577, 111595,
    111613, 111631, 111649, 111667, 111685, 111703, 111721, 111739, 111757,
    111775, 111793, 111811, 111828, 111846, 111864, 111882, 111899, 111917,
    111935, 111952, 111970, 111988, 112005, 112023, 112040, 112058, 112076,
    112093, 112111, 112128, 112145, 112163, 112180, 112198, 112215, 112232,
    112250, 112267, 112284, 112302, 112319, 112336, 112354, 112371, 112388,
    112405, 112422, 112439, 112457, 112474, 112491, 112508, 112525, 112542,
    112559, 112576, 112593, 112610, 112627, 112644, 112661, 112678, 112695,
    112711, 112728, 112745, 112762, 112779, 112795, 112812, 112829, 112846,
    112862, 112879, 112896, 112912, 112929, 112946, 112962, 112979, 112995,
    113012, 113029, 113045, 113062, 113078, 113095, 113111, 113127, 113144,
    113160, 113177, 113193, 113209, 113226, 113242, 113258, 113275, 113291,
    113307, 113323, 113340, 113356, 113372, 113388, 113404, 113421, 113437,
    113453, 113469, 113485, 113501, 113517, 113533, 113549,
};

// Add an ADC sample.
static inline void time_constant_add_to_buffer(const uint16_t sample) {
    g_time_constant_adc_samples[g_time_constant_num_adc_samples] = sample;
    ++g_time_constant_num_adc_samples;
}

// Estimate the minimum ADC sample.
static inline uint16_t time_constant_estimate_min_sample(void) {
    // The minimum sample is the mean of the last few samples.
    uint32_t sum = 0;
    for (size_t i = 0; i < TIME_CONSTANT_NUM_AVERAGES_FOR_MIN_ADC_SAMPLE; ++i) {
        sum += g_time_constant_adc_samples[g_time_constant_num_adc_samples - 1 -
                                           i];
    }
    return sum / TIME_CONSTANT_NUM_AVERAGES_FOR_MIN_ADC_SAMPLE;
}

// Estimate the three tau index.
static inline size_t time_constant_estimate_three_tau_index(void) {
    const uint16_t max_adc_sample = g_time_constant_adc_samples[0];
    const uint16_t min_adc_sample = time_constant_estimate_min_sample();

    // The sample at 3tau corresponds to where the exponential has decayed by
    // 95%.
    const uint16_t three_tau_value =
        min_adc_sample + 0.05 * (max_adc_sample - min_adc_sample);
    for (size_t i = 0; i < g_time_constant_num_adc_samples; ++i) {
        if (g_time_constant_adc_samples[i] < three_tau_value) {
            return i;
        }
    }
    return g_time_constant_num_adc_samples;
}

// Return the natural logarithm of the given value.
static inline fixed_point_t time_constant_ln(const uint16_t value) {
    return g_time_constant_ln_lookup_table[value];
}

void time_constant_init(const uint32_t time_constant_sampling_period_ms) {
    g_time_constant_sampling_period_ms = time_constant_sampling_period_ms;
    g_time_constant_num_adc_samples = 0;
    g_time_constant_min_adc_sample = ADC_MAX_THEORETICAL_ADC_SAMPLE;
    g_time_constant_num_samples_since_minimum = 0;
    g_time_constant_state = TIME_CONSTANT_STATE_IDLE;
}

bool time_constant_add_sample(const uint16_t adc_sample) {
    switch (g_time_constant_state) {
        case TIME_CONSTANT_STATE_IDLE: {
            const uint16_t previous_adc_sample =
                g_time_constant_adc_samples[g_time_constant_num_adc_samples];
            if (previous_adc_sample == ADC_MAX_ADC_SAMPLE &&
                adc_sample < ADC_MAX_ADC_SAMPLE) {
                adc_msb_init(adc_sample + ADC_MAX_THEORETICAL_ADC_SAMPLE -
                             ADC_MAX_ADC_SAMPLE);
                const uint16_t disambiguated_adc_sample =
                    adc_msb_get_disambiguated_sample();
                time_constant_add_to_buffer(disambiguated_adc_sample);
                g_time_constant_min_adc_sample = disambiguated_adc_sample;
                g_time_constant_state = TIME_CONSTANT_STATE_RECEIVING;
            } else {
                g_time_constant_adc_samples[g_time_constant_num_adc_samples] =
                    adc_sample;
            }
            break;
        }
        case TIME_CONSTANT_STATE_RECEIVING: {
            adc_msb_disambiguate(adc_sample);
            const uint16_t disambiguated_adc_sample =
                adc_msb_get_disambiguated_sample();
            time_constant_add_to_buffer(disambiguated_adc_sample);

            // Update the minimum ADC sample.
            if (disambiguated_adc_sample < g_time_constant_min_adc_sample) {
                g_time_constant_min_adc_sample = disambiguated_adc_sample;
                g_time_constant_num_samples_since_minimum = 0;
            } else {
                ++g_time_constant_num_samples_since_minimum;

                // If the minimum ADC sample has not been updated for a while,
                // the exponential might have finished decaying.
                if (g_time_constant_num_samples_since_minimum >
                    TIME_CONSTANT_MAX_NUM_SAMPLES_SINCE_MINIMUM) {
                    g_time_constant_state =
                        TIME_CONSTANT_STATE_HAS_SUFFICIENT_SAMPLES;
                }
            }
            break;
        }
        case TIME_CONSTANT_STATE_INVALID:
        case TIME_CONSTANT_STATE_HAS_SUFFICIENT_SAMPLES:
        case TIME_CONSTANT_STATE_DONE:
        default: {
            return false;
        }
    }
    return true;
}

bool time_constant_has_sufficient_samples(void) {
    return g_time_constant_state == TIME_CONSTANT_STATE_HAS_SUFFICIENT_SAMPLES;
}

fixed_point_t time_constant_estimate(void) {
    const uint16_t min_adc_sample = time_constant_estimate_min_sample();
    const size_t three_tau_index = time_constant_estimate_three_tau_index();
    printf("Number of ADC samples: %d, three tau index: %d\n",
           g_time_constant_num_adc_samples, three_tau_index);

    // Calculate the exponential factor for the weights.
    const fixed_point_t factor_exponent = fixed_point_divide(
        fixed_point_init(-6), fixed_point_init(three_tau_index));
    // Approximate exp(x) as 1 + x + 1/2 * x^2.
    const fixed_point_t factor = fixed_point_add(
        fixed_point_init(1),
        fixed_point_add(
            factor_exponent,
            fixed_point_divide(
                fixed_point_multiply(factor_exponent, factor_exponent),
                fixed_point_init(2))));

    // Calculate the mean x-value, which could be approximated as fs * tau/2.
    // Ignore any multiplicative factors when calculating the weights.
    fixed_point_t weighted_sum_x_values = fixed_point_init(0);
    fixed_point_t sum_weights = fixed_point_init(0);
    fixed_point_t weight = fixed_point_init(1);
    for (size_t k = 0; k < three_tau_index; ++k) {
        weighted_sum_x_values =
            fixed_point_add(weighted_sum_x_values,
                            fixed_point_multiply(weight, fixed_point_init(k)));
        sum_weights = fixed_point_add(sum_weights, weight);
        weight = fixed_point_multiply(weight, factor);
    }
    const fixed_point_t x_mean =
        fixed_point_divide(weighted_sum_x_values, sum_weights);

    fixed_point_t time_constant_samples_numerator = fixed_point_init(0);
    fixed_point_t time_constant_samples_denominator = fixed_point_init(0);
    weight = fixed_point_init(1);
    for (size_t k = 0; k < three_tau_index; ++k) {
        const fixed_point_t k_minus_x_mean =
            fixed_point_subtract(fixed_point_init(k), x_mean);
        const fixed_point_t weighted_k_minus_x_mean =
            fixed_point_multiply(k_minus_x_mean, weight);
        time_constant_samples_denominator = fixed_point_add(
            time_constant_samples_denominator,
            fixed_point_multiply(
                weighted_k_minus_x_mean,
                time_constant_ln(g_time_constant_adc_samples[k] -
                                 min_adc_sample)));
        time_constant_samples_numerator = fixed_point_subtract(
            time_constant_samples_numerator,
            fixed_point_multiply(k_minus_x_mean, weighted_k_minus_x_mean));
        weight = fixed_point_multiply(weight, factor);
    }
    // The time constant in number of samples.
    const fixed_point_t time_constant_samples = fixed_point_divide(
        time_constant_samples_numerator, time_constant_samples_denominator);
    const fixed_point_t time_constant = fixed_point_multiply(
        fixed_point_divide(fixed_point_init(g_time_constant_sampling_period_ms),
                           fixed_point_init(1000)),
        time_constant_samples);
    return time_constant;
}
