/* ------------------ */
/* --- SENSOR ADC --- */
/* ------------------ */
// Everything is MSB -> LSB
const unsigned int ASC_SENSOR_SEL_RESET = 242;
const unsigned int ASC_SENSORADC_SEL_CONVERT = 243;
const unsigned int ASC_SENSORADC_SEL_PGA_AMPLIFY = 244;
const unsigned int ASC_SENSORADC_PGA_GAIN[8] = {773, 880, 771, 770, 769, 768, 767, 766};
const unsigned int ASC_SENSORADC_ADC_SETTLE[8] = {823, 822, 821, 820, 819, 818, 817, 816};
const unsigned int ASC_SENSORADC_BGR_TUNE[7] = {779, 780, 781, 782, 783, 784, 778};
const unsigned int ASC_SENSORADC_CONSTGM_TUNE[8] = {758, 759, 760, 761, 762, 763, 764, 765};
const unsigned int ASC_SENSORADC_EN_VBATDIV = 798;
const unsigned int ASC_SENSORADC_EN_LDO = 801;
const unsigned int ASC_SENSORADC_SEL_INPUT_MUX[2] = {1087, 915};
const unsigned int ASC_SENSORADC_PGA_BYPASS = 1088;