#include "time_constant.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "adc_msb.h"
#include "fixed_point.h"

// Maximum number of ADC samples.
#define TIME_CONSTANT_MAX_NUM_ADC_SAMPLES 5000

// Maximum number of samples since the minimum ADC sample to determine whether
// sufficient ADC samples have been received.
#define TIME_CONSTANT_MAX_NUM_SAMPLES_SINCE_MINIMUM 100

// Number of samples to average at the end to find the minimum ADC sample.
#define TIME_CONSTANT_NUM_AVERAGES_FOR_MIN_ADC_SAMPLE 10

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
static fixed_point_t g_time_constant_ln_lookup_table[] = {
    0,      0,      45426,  71999,  90852,  105476, 117425, 127527, 136278,
    143997, 150902, 157148, 162851, 168097, 172953, 177475, 181704, 185677,
    189423, 192967, 196328, 199526, 202575, 205488, 208277, 210952, 213523,
    215996, 218379, 220679, 222901, 225050, 227130, 229147, 231104, 233003,
    234849, 236645, 238393, 240095, 241754, 243373, 244952, 246494, 248001,
    249473, 250914, 252323, 253703, 255054, 256378, 257676, 258949, 260197,
    261422, 262625, 263805, 264965, 266105, 267225, 268327, 269410, 270476,
    271524, 272557, 273573, 274573, 275559, 276530, 277486, 278429, 279359,
    280276, 281180, 282071, 282951, 283819, 284676, 285521, 286356, 287180,
    287995, 288799, 289593, 290378, 291154, 291920, 292678, 293427, 294167,
    294900, 295624, 296340, 297048, 297749, 298443, 299129, 299808, 300480,
    301146, 301804, 302457, 303102, 303742, 304375, 305002, 305623, 306239,
    306848, 307452, 308051, 308644, 309232, 309814, 310392, 310964, 311531,
    312094, 312652, 313205, 313753, 314297, 314836, 315371, 315902, 316428,
    316951, 317469, 317983, 318493, 318999, 319501, 319999, 320494, 320985,
    321472, 321956, 322436, 322912, 323386, 323855, 324322, 324785, 325245,
    325702, 326155, 326606, 327053, 327497, 327939, 328377, 328812, 329245,
    329675, 330102, 330526, 330947, 331366, 331782, 332196, 332607, 333015,
    333421, 333824, 334225, 334623, 335019, 335413, 335804, 336193, 336580,
    336964, 337346, 337726, 338104, 338479, 338853, 339224, 339593, 339961,
    340326, 340689, 341050, 341409, 341766, 342121, 342475, 342826, 343175,
    343523, 343869, 344213, 344555, 344896, 345234, 345571, 345907, 346240,
    346572, 346902, 347231, 347557, 347883, 348206, 348528, 348849, 349168,
    349485, 349801, 350115, 350428, 350739, 351049, 351358, 351665, 351970,
    352274, 352577, 352878, 353178, 353477, 353774, 354070, 354364, 354658,
    354950, 355240, 355530, 355818, 356104, 356390, 356674, 356957, 357239,
    357520, 357799, 358078, 358355, 358631, 358906, 359179, 359452, 359723,
    359993, 360262, 360530, 360797, 361063, 361328, 361592, 361854, 362116,
    362377, 362636, 362895, 363152, 363409, 363664, 363919, 364172, 364425,
    364676, 364927, 365177, 365425, 365673, 365920, 366166, 366411, 366655,
    366898, 367140, 367382, 367622, 367862, 368101, 368339, 368576, 368812,
    369047, 369282, 369515, 369748, 369980, 370211, 370442, 370671, 370900,
    371128, 371355, 371581, 371807, 372032, 372256, 372479, 372702, 372923,
    373144, 373365, 373584, 373803, 374021, 374239, 374455, 374671, 374886,
    375101, 375315, 375528, 375740, 375952, 376163, 376373, 376583, 376792,
    377001, 377208, 377415, 377622, 377828, 378033, 378237, 378441, 378644,
    378847, 379049, 379250, 379451, 379651, 379850, 380049, 380248, 380445,
    380642, 380839, 381035, 381230, 381425, 381619, 381813, 382006, 382198,
    382390, 382582, 382772, 382963, 383152, 383341, 383530, 383718, 383906,
    384092, 384279, 384465, 384650, 384835, 385019, 385203, 385387, 385569,
    385752, 385934, 386115, 386296, 386476, 386656, 386835, 387014, 387192,
    387370, 387547, 387724, 387901, 388077, 388252, 388427, 388602, 388776,
    388949, 389122, 389295, 389467, 389639, 389810, 389981, 390152, 390322,
    390491, 390660, 390829, 390997, 391165, 391333, 391500, 391666, 391832,
    391998, 392163, 392328, 392493, 392657, 392820, 392983, 393146, 393309,
    393471, 393632, 393794, 393954, 394115, 394275, 394435, 394594, 394753,
    394911, 395069, 395227, 395384, 395541, 395698, 395854, 396010, 396165,
    396321, 396475, 396630, 396784, 396937, 397091, 397244, 397396, 397548,
    397700, 397852, 398003, 398154, 398304, 398455, 398604, 398754, 398903,
    399052, 399200, 399348, 399496, 399643, 399791, 399937, 400084, 400230,
    400376, 400521, 400666, 400811, 400956, 401100, 401244, 401387, 401530,
    401673, 401816, 401958, 402100, 402242, 402383, 402525, 402665, 402806,
    402946, 403086, 403225, 403365, 403504, 403642, 403781, 403919, 404057,
    404194, 404332, 404469, 404605, 404742, 404878, 405014, 405149, 405284,
    405419, 405554, 405689, 405823, 405957, 406090, 406224, 406357, 406489,
    406622, 406754, 406886, 407018, 407149, 407281, 407411, 407542, 407673,
    407803, 407933, 408062, 408192, 408321, 408450, 408578, 408707, 408835,
    408963, 409090, 409218, 409345, 409472, 409598, 409725, 409851, 409977,
    410103, 410228, 410353, 410478, 410603, 410727, 410851, 410975, 411099,
    411223, 411346, 411469, 411592, 411715, 411837, 411959, 412081, 412203,
    412324, 412446, 412567, 412687, 412808, 412928, 413048, 413168, 413288,
    413408, 413527, 413646, 413765, 413883, 414002, 414120, 414238, 414356,
    414473, 414591, 414708, 414825, 414941, 415058, 415174, 415290, 415406,
    415522, 415637, 415753, 415868, 415982, 416097, 416212, 416326, 416440,
    416554, 416668, 416781, 416894, 417007, 417120, 417233, 417346, 417458,
    417570, 417682, 417794, 417905, 418017, 418128, 418239, 418349, 418460,
    418571, 418681, 418791, 418901, 419010, 419120, 419229, 419338, 419447,
    419556, 419665, 419773, 419881, 419989, 420097, 420205, 420312, 420420,
    420527, 420634, 420741, 420847, 420954, 421060, 421166, 421272, 421378,
    421484, 421589, 421694, 421800, 421904, 422009, 422114, 422218, 422323,
    422427, 422531, 422634, 422738, 422841, 422945, 423048, 423151, 423254,
    423356, 423459, 423561, 423663, 423765, 423867, 423969, 424070, 424172,
    424273, 424374, 424475, 424576, 424676, 424777, 424877, 424977, 425077,
    425177, 425277, 425376, 425475, 425575, 425674, 425773, 425871, 425970,
    426069, 426167, 426265, 426363, 426461, 426559, 426656, 426754, 426851,
    426948, 427045, 427142, 427239, 427335, 427432, 427528, 427624, 427720,
    427816, 427912, 428008, 428103, 428198, 428294, 428389, 428484, 428578,
    428673, 428767, 428862, 428956, 429050, 429144, 429238, 429332, 429425,
    429519, 429612, 429705, 429798, 429891, 429984, 430076, 430169, 430261,
    430353, 430446, 430538, 430629, 430721, 430813, 430904, 430996, 431087,
    431178, 431269, 431360, 431450, 431541, 431631, 431722, 431812, 431902,
    431992, 432082, 432171, 432261, 432351, 432440, 432529, 432618, 432707,
    432796, 432885, 432973, 433062, 433150, 433239, 433327, 433415, 433503,
    433590, 433678, 433766, 433853, 433940, 434028, 434115, 434202, 434289,
    434375, 434462, 434548, 434635, 434721, 434807, 434893, 434979, 435065,
    435151, 435237, 435322, 435407, 435493, 435578, 435663, 435748, 435833,
    435917, 436002, 436087, 436171, 436255, 436339, 436423, 436507, 436591,
    436675, 436759, 436842, 436926, 437009, 437092, 437175, 437258, 437341,
    437424, 437507, 437589, 437672, 437754, 437836, 437919, 438001, 438083,
    438165, 438246, 438328, 438410, 438491, 438572, 438654, 438735, 438816,
    438897, 438978, 439058, 439139, 439220, 439300, 439380, 439461, 439541,
    439621, 439701, 439781, 439861, 439940, 440020, 440099, 440179, 440258,
    440337, 440416, 440495, 440574, 440653, 440732, 440810, 440889, 440967,
    441046, 441124, 441202, 441280, 441358, 441436, 441514, 441592, 441669,
    441747, 441824, 441901, 441979, 442056, 442133, 442210, 442287, 442363,
    442440, 442517, 442593, 442670, 442746, 442822, 442898, 442975, 443051,
    443126, 443202, 443278, 443354, 443429, 443505, 443580, 443655, 443730,
    443806, 443881, 443956, 444030, 444105, 444180, 444254, 444329, 444403,
    444478, 444552, 444626, 444700, 444774, 444848, 444922, 444996, 445069,
    445143, 445217, 445290, 445363, 445437, 445510, 445583, 445656, 445729,
    445802, 445875, 445947, 446020, 446092, 446165, 446237, 446309, 446382,
    446454, 446526, 446598, 446670, 446742, 446813, 446885, 446957, 447028,
    447100, 447171, 447242, 447313, 447384, 447456, 447526, 447597, 447668,
    447739, 447810, 447880, 447951, 448021, 448091, 448162, 448232, 448302,
    448372, 448442, 448512, 448582, 448652, 448721, 448791, 448860, 448930,
    448999, 449069, 449138, 449207, 449276, 449345, 449414, 449483, 449552,
    449620, 449689, 449758, 449826, 449895, 449963, 450031, 450100, 450168,
    450236, 450304, 450372, 450440, 450507, 450575, 450643, 450710, 450778,
    450845, 450913, 450980, 451047, 451115, 451182, 451249, 451316, 451383,
    451449, 451516, 451583, 451650, 451716, 451783, 451849, 451915, 451982,
    452048, 452114, 452180, 452246, 452312, 452378, 452444, 452510, 452575,
    452641, 452707, 452772, 452838, 452903, 452968, 453034, 453099, 453164,
    453229, 453294, 453359, 453424, 453488, 453553, 453618, 453682, 453747,
    453811, 453876, 453940, 454004, 454069, 454133, 454197,
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

void time_constant_init(void) {
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

    fixed_point_t numerator = fixed_point_init(0);
    fixed_point_t denominator = fixed_point_init(0);

    // Approximate mean x-value as tau/2.
    const fixed_point_t x_mean = fixed_point_divide(
        fixed_point_init(three_tau_index), fixed_point_init(6));
    const fixed_point_t factor_exponent = fixed_point_divide(
        fixed_point_init(-6), fixed_point_init(three_tau_index));
    // Approximate exp(x) as 1 + x + x^2.
    const fixed_point_t factor = fixed_point_add(
        fixed_point_init(1),
        fixed_point_add(
            factor_exponent,
            fixed_point_divide(
                fixed_point_multiply(factor_exponent, factor_exponent),
                fixed_point_init(2))));
    // Ignore any multiplicative factors.
    fixed_point_t weight = fixed_point_init(1);
    for (size_t k = 0; k < three_tau_index; ++k) {
        const fixed_point_t k_minus_x_mean =
            fixed_point_subtract(fixed_point_init(k), x_mean);
        const fixed_point_t weighted_k_minus_x_mean =
            fixed_point_multiply(weight, k_minus_x_mean);
        denominator = fixed_point_subtract(
            denominator, fixed_point_multiply(
                             time_constant_ln(g_time_constant_adc_samples[k] -
                                              min_adc_sample),
                             weighted_k_minus_x_mean));
        numerator = fixed_point_add(
            numerator,
            fixed_point_multiply(k_minus_x_mean, weighted_k_minus_x_mean));
        weight = fixed_point_multiply(weight, factor);
    }
    // The time constant in number of samples.
    const fixed_point_t time_constant_samples =
        fixed_point_divide(numerator, denominator);
    const fixed_point_t time_constant = fixed_point_multiply(
        fixed_point_divide(fixed_point_init(TIME_CONSTANT_SAMPLING_PERIOD_MS),
                           fixed_point_init(1000)),
        time_constant_samples);
    return time_constant;
}
