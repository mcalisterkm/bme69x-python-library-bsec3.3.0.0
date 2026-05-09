#define _XOPEN_SOURCE 700
#define TEMP_OFFSET 0.0f
#ifndef BSEC
#define BSEC
#endif

#include "internal_functions.h"

struct bme69x_data data[3];
uint32_t del_period;
uint32_t time_ms;
uint8_t n_fields;
static int8_t rslt;

#ifdef BSEC
/* TVOC equivalent baseline tracker constants */
#define TVOC_EQUIVALENT_ENABLE    3
#define TVOC_EQUIVALENT_DISABLE   0
#define TVOC_CALIBRATION_TIME_SEC (30 * 60)  /* 30 minutes in seconds */

/* Global variable for baseline tracker */
static uint8_t baseline_tracker = TVOC_EQUIVALENT_DISABLE;
/* Global variable to track the current sample rate */
static float current_sample_rate = 0.0f;
/* TVOC calibration tracking variables */
static time_t tvoc_start_time = 0;
static bool tvoc_disable_flag = false;
static bool tvoc_calibration_started = false;
#endif

uint16_t
get_max(uint16_t array[], int8_t len)
{
    uint16_t max = 0;
    for (int i = 0; i < (ssize_t)len; i++)
    {
        if (array[i] > max)
        {
            max = array[i];
        }
    }
    return max;
}

void pi3g_delay_us(uint32_t duration_us, void *intf_ptr)
{
    struct timespec ts;
    ts.tv_sec = duration_us / 1000000;
    ts.tv_nsec = (duration_us % 1000000) * 1000;
    nanosleep(&ts, NULL);
}

int8_t pi3g_read(uint8_t regAddr, uint8_t *regData, uint32_t len, void *intf_ptr)
{
    rslt = BME69X_OK;
    int fd = *((int *)intf_ptr);
    if (write(fd, &regAddr, 1) != 1)
    {
        perror("pi3g_read register");
        rslt = -1;
    }
    if (read(fd, regData, len) != (ssize_t)len)
    {
        perror("pi3g_read data");
        rslt = -1;
    }

    return rslt;
}

int8_t pi3g_write(uint8_t regAddr, const uint8_t *regData, uint32_t len, void *intf_ptr)
{
    rslt = BME69X_OK;
    int fd = *((int *)intf_ptr);
    uint8_t reg[len + 1];
    reg[0] = regAddr;

    for (uint32_t i = 1; i < len + 1; i++)
        reg[i] = regData[i - 1];

    if (write(fd, reg, len + 1) != (ssize_t)(len + 1))
    {
        perror("pi3g_write");
        rslt = -1;
    }

    return rslt;
}

int8_t pi3g_set_conf(uint8_t os_hum, uint8_t os_pres, uint8_t os_temp, uint8_t filter, uint8_t odr, struct bme69x_conf *conf, struct bme69x_dev *bme, uint8_t debug_mode)
{
    int8_t rslt = BME69X_OK;

    rslt = bme69x_get_conf(conf, bme);
    if (rslt < 0)
    {
        perror("bme69x_get_conf");
    }

    conf->os_hum = os_hum;
    conf->os_pres = os_pres;
    conf->os_temp = os_temp;
    conf->filter = filter;
    conf->odr = odr;

    rslt = bme69x_set_conf(conf, bme);
    if (rslt != BME69X_OK)
    {
        perror("bme69x_set_conf");
    }
    if (debug_mode == 1)
    {
        printf("SET BME69X CONFIG\n");
    }
    return rslt;
}

int8_t pi3g_set_heater_conf_fm(uint8_t enable, uint16_t heatr_temp, uint16_t heatr_dur, struct bme69x_heatr_conf *heatr_conf, struct bme69x_dev *bme, uint8_t debug_mode)
{
    int8_t rslt = BME69X_OK;
    heatr_conf->enable = enable;
    heatr_conf->heatr_temp = heatr_temp;
    heatr_conf->heatr_dur = heatr_dur;
    rslt = bme69x_set_heatr_conf(BME69X_FORCED_MODE, heatr_conf, bme);
    if (rslt != BME69X_OK)
    {
        perror("bme69x_set_heatr_conf");
    }
    if (debug_mode == 1)
    {
        printf("SET HEATER CONFIG (FORCED MODE)\n");
    }
    return rslt;
}

int8_t pi3g_set_heater_conf_pm(uint8_t enable, uint16_t temp_prof[], uint16_t dur_prof[], uint8_t profile_len, struct bme69x_conf *conf, struct bme69x_heatr_conf *heatr_conf, struct bme69x_dev *bme, uint8_t debug_mode)
{
    int8_t rslt = BME69X_OK;
    heatr_conf->enable = enable;
    heatr_conf->heatr_temp_prof = temp_prof;
    heatr_conf->heatr_dur_prof = dur_prof;
    heatr_conf->shared_heatr_dur = 140 - (bme69x_get_meas_dur(BME69X_PARALLEL_MODE, conf, bme) / 1000);
    heatr_conf->profile_len = profile_len;
    rslt = bme69x_set_heatr_conf(BME69X_PARALLEL_MODE, heatr_conf, bme);
    if (rslt != BME69X_OK)
    {
        perror("bme69x_set_heatr_conf");
    }

    rslt = bme69x_set_op_mode(BME69X_PARALLEL_MODE, bme);
    if (rslt != BME69X_OK)
    {
        perror("bme69x_set_op_mode");
    }
    if (debug_mode == 1)
    {
        printf("SET HEATER CONFIG (PARALLEL MODE)\n");
    }

    return rslt;
}

int8_t pi3g_set_heater_conf_sm(uint8_t enable, uint16_t temp_prof[], uint16_t dur_prof[], uint8_t profile_len, struct bme69x_heatr_conf *heatr_conf, struct bme69x_dev *bme, uint8_t debug_mode)
{
    int8_t rslt = BME69X_OK;
    heatr_conf->enable = enable;
    heatr_conf->heatr_temp_prof = temp_prof;
    heatr_conf->heatr_dur_prof = dur_prof;
    heatr_conf->profile_len = profile_len;
    rslt = bme69x_set_heatr_conf(BME69X_SEQUENTIAL_MODE, heatr_conf, bme);
    if (rslt != BME69X_OK)
    {
        perror("bme69x_set_heatr_conf");
    }
    rslt = bme69x_set_op_mode(BME69X_SEQUENTIAL_MODE, bme);
    if (rslt != BME69X_OK)
    {
        perror("bme69x_set_op_mode");
    }
    if (debug_mode == 1)
    {
        printf("SET HEATER CONFIG (SEQUENTIAL MODE)\n");
    }
    return rslt;
}

int64_t pi3g_timestamp_ns()
{
    struct timespec spec;
    clock_gettime(CLOCK_MONOTONIC, &spec);

    int64_t time_ns = (int64_t)(spec.tv_sec) * (int64_t)1000000000 + (int64_t)(spec.tv_nsec);
    return time_ns;
}

uint32_t pi3g_timestamp_us()
{
    return (uint32_t)(pi3g_timestamp_ns() / 1000);
}

uint32_t pi3g_timestamp_ms()
{
    return (uint32_t)(pi3g_timestamp_us() / 1000);
}

#ifdef BSEC
static bool bsec_output_is_included(uint8_t sensor_id)
{
    if (sensor_id == 0 || sensor_id > 31)
    {
        return false;
    }

    return ((BSEC_OUTPUT_INCLUDED >> (sensor_id - 1U)) & 0x01U) != 0U;
}

bsec_library_return_t bsec_set_sample_rate(void *bme, float sample_rate)
{
    /* Store the sample rate for later use */
    current_sample_rate = sample_rate;

    /* Max 14 outputs including optional TVOC in LP mode. */
    bsec_sensor_configuration_t requested_virtual_sensors[14];
    uint8_t n_requested_virtual_sensors = 0;

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    /* BSEC_OUTPUT_BREATH_VOC_EQUIVALENT is intentionally absent.
     * Per BSEC v3.3.0.0 release notes: "BREATH_VOC_EQUIVALENT output disabled
     * for BME690 sensors.  TVOC_EQUIVALENT output enabled with Auto calibration
     * feature."  TVOC_EQUIVALENT is subscribed separately in LP mode below. */
    const uint8_t base_outputs[] = {
        BSEC_OUTPUT_IAQ,
        BSEC_OUTPUT_STATIC_IAQ,
        BSEC_OUTPUT_CO2_EQUIVALENT,
        BSEC_OUTPUT_RAW_TEMPERATURE,
        BSEC_OUTPUT_RAW_PRESSURE,
        BSEC_OUTPUT_RAW_HUMIDITY,
        BSEC_OUTPUT_RAW_GAS,
        BSEC_OUTPUT_STABILIZATION_STATUS,
        BSEC_OUTPUT_RUN_IN_STATUS,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE,
        BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY,
        BSEC_OUTPUT_GAS_PERCENTAGE
    };

    for (size_t i = 0; i < (sizeof(base_outputs) / sizeof(base_outputs[0])); ++i)
    {
        if (bsec_output_is_included(base_outputs[i]))
        {
            requested_virtual_sensors[n_requested_virtual_sensors].sensor_id = base_outputs[i];
            requested_virtual_sensors[n_requested_virtual_sensors].sample_rate = sample_rate;
            ++n_requested_virtual_sensors;
        }
    }

    /* TVOC is only supported in LP mode */
    /* Use tolerance for floating-point comparison */
    float sample_rate_diff = fabs(sample_rate - BSEC_SAMPLE_RATE_LP);
   /** printf("Sample rate: %.5f, LP rate: %.5f, diff: %.5f, test result: %s\n", 
           sample_rate, BSEC_SAMPLE_RATE_LP, sample_rate_diff, 
           (sample_rate_diff < 0.01f) ? "PASS (TVOC enabled)" : "FAIL (TVOC disabled)");
           **/
    if ((sample_rate_diff < 0.01f) && bsec_output_is_included(BSEC_OUTPUT_TVOC_EQUIVALENT))
    {
        printf("TVOC sensor enabled - adding to subscription (LP mode detected)\n"); 
        requested_virtual_sensors[n_requested_virtual_sensors].sensor_id = BSEC_OUTPUT_TVOC_EQUIVALENT;
        requested_virtual_sensors[n_requested_virtual_sensors].sample_rate = sample_rate;
        ++n_requested_virtual_sensors;
    }
    else
    {
        printf("TVOC sensor NOT enabled - not in LP mode\n");
    }
    
    printf("Total requested virtual sensors: %d\n", n_requested_virtual_sensors);

    return bsec_update_subscription((void *)bme, requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);
}

bsec_library_return_t bsec_set_sample_rate_ai(void *bme,  uint8_t variant_id, struct bme69x_heatr_conf *bme69x_heatr_conf, uint8_t num_ai_classes)
{
    if (variant_id == BME69X_VARIANT_GAS_LOW)
    {
        perror("bsec_set_sample_rate_ai");
        printf("AI features are not available for BME690\n");
        return BSEC_OK;
    }
    uint8_t n_requested_virtual_sensors;
    n_requested_virtual_sensors = 13;
    bsec_sensor_configuration_t requested_virtual_sensors[n_requested_virtual_sensors];

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    printf("SHARED HEATR DUR IN SET SAMPLE RATE AI %d\n", bme69x_heatr_conf->shared_heatr_dur);

    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_3;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_4;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_RAW_TEMPERATURE;
    requested_virtual_sensors[4].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[5].sensor_id = BSEC_OUTPUT_RAW_PRESSURE;
    requested_virtual_sensors[5].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[6].sensor_id = BSEC_OUTPUT_RAW_HUMIDITY;
    requested_virtual_sensors[6].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[7].sensor_id = BSEC_OUTPUT_RAW_GAS;
    requested_virtual_sensors[7].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[8].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[8].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[9].sensor_id = BSEC_OUTPUT_IAQ;
    requested_virtual_sensors[9].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[10].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE;
    requested_virtual_sensors[10].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[11].sensor_id = BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY;
    requested_virtual_sensors[11].sample_rate = BSEC_SAMPLE_RATE_SCAN ;

    /*
    float SEL = ((float)1000) / ((float)bme69x_heatr_conf->shared_heatr_dur);
    printf("SEL %.6f\n", SEL);

    for (uint8_t i = 0; i < 4; i++)
    {
        if (i < num_ai_classes)
        {
            requested_virtual_sensors[i].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1 + i;
            requested_virtual_sensors[i].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
        }
    }

    float HTR = ((float)1000) / ((float)bme69x_heatr_conf->heatr_dur_prof[0]);
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[4].sample_rate = HTR;
    */
    bsec_library_return_t rslt = bsec_update_subscription((void *)bme, requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);
    printf("SET_SAMPLE_RATE_AI %d\n", rslt);
    return rslt;
}

bsec_library_return_t bsec_read_data(struct bme69x_data *data, int64_t time_stamp, bsec_input_t *inputs, uint8_t *n_bsec_inputs, int32_t bsec_process_data, uint8_t op_mode, struct bme69x_dev *bme, int8_t temp_offset)
{
    if (bsec_process_data)
    {
        /* Pressure to be processed by BSEC */
        if (bsec_process_data & BSEC_PROCESS_PRESSURE)
        {
            printf("PRESSURE %f\n", data->pressure);
            /* Place presssure sample into input struct */
            inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_PRESSURE;
            inputs[*n_bsec_inputs].signal = data->pressure;
            inputs[*n_bsec_inputs].time_stamp = time_stamp;
            (*n_bsec_inputs)++;
        }
        /* Temperature to be processed by BSEC */
        if (bsec_process_data & BSEC_PROCESS_TEMPERATURE)
        {
            printf("TEMPERATURE %f\n", data->temperature);
            /* Place temperature sample into input struct */
            inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
#ifdef BME69X_FLOAT_POINT_COMPENSATION
            inputs[*n_bsec_inputs].signal = data->temperature;
#else
            inputs[*n_bsec_inputs].signal = data->temperature / 100.0f;
#endif
            inputs[*n_bsec_inputs].time_stamp = time_stamp;
            (*n_bsec_inputs)++;

            /* Also add optional heatsource input which will be subtracted from the temperature reading to 
             * compensate for device-specific self-heating (supported in BSEC IAQ solution)*/
            inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
            inputs[*n_bsec_inputs].signal = temp_offset;
            inputs[*n_bsec_inputs].time_stamp = time_stamp;
            (*n_bsec_inputs)++;
        }
        /* Humidity to be processed by BSEC */
        if (bsec_process_data & BSEC_PROCESS_HUMIDITY)
        {
             printf("HUMIDITY %f\n",data->humidity);
            /* Place humidity sample into input struct */
            inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
#ifdef BME69X_FLOAT_POINT_COMPENSATION
            inputs[*n_bsec_inputs].signal = data->humidity;
#else
            inputs[*n_bsec_inputs].signal = data->humidity / 1000.0f;
#endif
              inputs[*n_bsec_inputs].time_stamp = time_stamp;
            (*n_bsec_inputs)++;
        }
        /* Gas to be processed by BSEC */
        if (bsec_process_data & BSEC_PROCESS_GAS)
        {
            printf("GAS_RESISTANCE %f\n", data->gas_resistance);
            /* Check whether gas_valid flag is set */
            if (data->status & BME69X_GASM_VALID_MSK)
            {
                /* Place sample into input struct */
                inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
                inputs[*n_bsec_inputs].signal = data->gas_resistance;
                inputs[*n_bsec_inputs].time_stamp = time_stamp;
                (*n_bsec_inputs)++;
            }
        }
        /* Profile part */
        if (op_mode == BME69X_PARALLEL_MODE || op_mode == BME69X_SEQUENTIAL_MODE)
        {
            printf("PROFILE_PART %d\n", data->gas_index);
            inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_PROFILE_PART;
            inputs[*n_bsec_inputs].signal = data->gas_index;
            inputs[*n_bsec_inputs].time_stamp = time_stamp;
            (*n_bsec_inputs)++;
        }
        /* Baseline tracker for TVOC (only in LP mode) */
        /* Use tolerance for floating-point comparison */
        if (fabs(current_sample_rate - BSEC_SAMPLE_RATE_LP) < 0.01f)
        {
            inputs[*n_bsec_inputs].sensor_id = BSEC_INPUT_DISABLE_BASELINE_TRACKER;
            inputs[*n_bsec_inputs].signal = baseline_tracker;
            inputs[*n_bsec_inputs].time_stamp = time_stamp;
            (*n_bsec_inputs)++;
        }
    }
    return BSEC_OK;
}

bsec_library_return_t bsec_process_data(void *bme, bsec_input_t *bsec_inputs, uint8_t num_bsec_inputs)
{
    /* Output buffer set to the maximum virtual sensor outputs supported */
    bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
    uint8_t num_bsec_outputs = 0;

    bsec_library_return_t bsec_status = BSEC_OK;

    /* Check if something should be processed by BSEC */
    if (num_bsec_inputs > 0)
    {
        num_bsec_outputs = BSEC_NUMBER_OUTPUTS;
        bsec_status = bsec_do_steps(&(bme), bsec_inputs, num_bsec_inputs, bsec_outputs, &num_bsec_outputs);
    }
    return bsec_status;
}

/**
 * @brief Function to enable or disable the baseline for TVOC equivalent in the BSEC
 *
 * @param[in] data     TVOC equivalent baseline enable or disable
 *                     TRUE  -> TVOC equivalent baseline adaption ON
 *                     FALSE -> TVOC equivalent baseline adaption OFF
 */
void set_tvoc_equivalent_baseline(bool data)
{
    if (data)
    {
        baseline_tracker = TVOC_EQUIVALENT_ENABLE;
    }
    else
    {
        baseline_tracker = TVOC_EQUIVALENT_DISABLE;
    }
}

/**
 * @brief Function to calibrate the TVOC equivalent by enabling and disabling the baseline adaptation.
 * Note: TVOC equivalent calibration is only possible in LP Mode.
 * This should be called periodically (e.g., before each get_bsec_data call).
 */
void tvoc_equivalent_calibration()
{
    /* Only calibrate in LP mode */
    float sample_rate_diff = fabs(current_sample_rate - BSEC_SAMPLE_RATE_LP);
    /** printf("[TVOC Calibration] Sample rate: %.5f, LP rate: %.5f, diff: %.5f, test result: %s\n",  
           current_sample_rate, BSEC_SAMPLE_RATE_LP, sample_rate_diff, 
           (sample_rate_diff < 0.01f) ? "PASS (LP mode)" : "FAIL (not LP mode)"); **/
    if (sample_rate_diff < 0.01f)
    {
        if (!tvoc_calibration_started)
        {
            /* First call - enable baseline adaptation */
            set_tvoc_equivalent_baseline(true);
            tvoc_disable_flag = true;
            tvoc_start_time = time(NULL);
            tvoc_calibration_started = true;
            printf("[TVOC] Calibration started at %ld - baseline adaptation enabled for 30 minutes\n", 
                   (long)tvoc_start_time);
        }
        else if (tvoc_disable_flag)
        {
            /* Check if 30 minutes have elapsed */
            time_t current_time = time(NULL);
            time_t elapsed_sec = current_time - tvoc_start_time;
            
            if (elapsed_sec >= TVOC_CALIBRATION_TIME_SEC)
            {
                /* After 30 minutes - disable baseline adaptation */
                set_tvoc_equivalent_baseline(false);
                tvoc_disable_flag = false;
                printf("[TVOC] Calibration complete at %ld - baseline adaptation disabled after %ld seconds\n",
                       (long)current_time, (long)elapsed_sec);
            }
            /** else
            {
                printf("[TVOC] Calibration in progress - elapsed: %ld/%d seconds\n", 
                       (long)elapsed_sec, TVOC_CALIBRATION_TIME_SEC);
            } **/
        }
    }
    else if (tvoc_calibration_started)
    {
        printf("[TVOC] Calibration not supported in current BSEC mode (not LP)\n");
        tvoc_calibration_started = false;
    }
}

/**
 * @brief Function to get the sample rate
 *
 * @return     Return the sample rate value
 */
float get_sample_rate_from_bsec()
{
    return current_sample_rate;
}
#endif
