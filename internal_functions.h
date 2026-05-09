#ifndef INTERNAL_FUNCTIONS_H_
#define INTERNAL_FUNCTIONS_H_

#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/i2c-dev.h>
#include "BME690_SensorAPI/bme69x.h"
#include "BME690_SensorAPI/bme69x_defs.h"

#ifndef BSEC
#define BSEC
#endif

#ifdef BSEC
#include "bsec_v3-3-0-0/release_bin/Sel_IAQ/inc/bsec_interface.h"
#include "bsec_v3-3-0-0/release_bin/Sel_IAQ/inc/bsec_datatypes.h"
#endif

/* CPP guard */
#ifdef __cplusplus
extern "C"
{
#endif

    uint16_t get_max(uint16_t array[], int8_t len);

    void pi3g_delay_us(uint32_t duration_us, void *intf_ptr);

    int8_t pi3g_read(uint8_t regAddr, uint8_t *regData, uint32_t len, void *intf_ptr);

    int8_t pi3g_write(uint8_t regAddr, const uint8_t *regData, uint32_t len, void *intf_ptr);

    int8_t pi3g_set_conf(uint8_t os_hum, uint8_t os_pres, uint8_t os_temp, uint8_t filter, uint8_t odr, struct bme69x_conf *conf, struct bme69x_dev *bme, uint8_t debug_mode);

    int8_t pi3g_set_heater_conf_fm(uint8_t enable, uint16_t heatr_temp, uint16_t heatr_dur, struct bme69x_heatr_conf *heatr_conf, struct bme69x_dev *bme, uint8_t debug_mode);

    int8_t pi3g_set_heater_conf_pm(uint8_t enable, uint16_t temp_prof[], uint16_t dur_prof[], uint8_t profile_len, struct bme69x_conf *conf, struct bme69x_heatr_conf *heatr_conf, struct bme69x_dev *bme, uint8_t debug_mode);

    int8_t pi3g_set_heater_conf_sm(uint8_t enable, uint16_t temp_prof[], uint16_t dur_prof[], uint8_t profile_len, struct bme69x_heatr_conf *heatr_conf, struct bme69x_dev *bme, uint8_t debug_mode);

    int64_t pi3g_timestamp_ns();

    uint32_t pi3g_timestamp_us();

    uint32_t pi3g_timestamp_ms();

#ifdef BSEC
    bsec_library_return_t bsec_set_sample_rate(void *inst, float sample_rate);

    bsec_library_return_t bsec_set_sample_rate_ai(void *inst, uint8_t variant_id, struct bme69x_heatr_conf *bme69x_heatr_conf, uint8_t num_ai_classes);

    bsec_library_return_t bsec_read_data(struct bme69x_data *data, int64_t time_stamp, bsec_input_t *inputs, uint8_t *n_bsec_inputs, int32_t bsec_process_data, uint8_t op_mode, struct bme69x_dev *bme, int8_t temp_offset);

    bsec_library_return_t bsec_process_data(void *inst, bsec_input_t *bsec_inputs, uint8_t num_bsec_inputs);

    void set_tvoc_equivalent_baseline(bool data);

    float get_sample_rate_from_bsec();

    void tvoc_equivalent_calibration();
#endif

#ifdef __cplusplus
}
#endif /* End of CPP guard */
#endif /* INTERNAL_FUNCTIONS_H_ */
