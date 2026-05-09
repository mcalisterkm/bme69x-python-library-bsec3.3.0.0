#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"
#include "internal_functions.h"
#include <stddef.h>

#define I2C_PORT_0 "/dev/i2c-0"
#define I2C_PORT_1 "/dev/i2c-1"

#define BME69X_VALID_DATA UINT8_C(0xB0) // DEZ 176  BIN 1011000 -> BITWISE & WITH BME69X_NEW_DATA_MSK 0x80, BME69X_GASM_VALID_MASK 0x20 AND BME69X_HEAT_STAB_MSK 0x10 YIELDS 1
#define BME69X_FLOAT_POINT_COMPENSATION
#ifndef BSEC
#define BSEC
#endif

/* Helper macro to prevent memory leaks when setting dict items */
#define DICT_SET_ITEM(dict, key, value) \
    do { \
        PyObject *_tmp = (value); \
        if (_tmp) { \
            PyDict_SetItemString((dict), (key), _tmp); \
            Py_DECREF(_tmp); \
        } \
    } while(0)

uint64_t time_stamp_interval_us = 0;
uint32_t n_samples = 0;

#ifdef BSEC
uint8_t bsec_state[BSEC_MAX_STATE_BLOB_SIZE];
uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE];
/* Workaround: bsec_get_instance_size() is absent from the Bosch Raspberry Pi
   archives in BSEC v3.3.0.0.  Following Bosch integration guidance, a fixed
   instance size is used instead.  When Bosch supply a corrected archive that
   exports bsec_get_instance_size(), revert by:
     1. Remove (or comment out) the #define BSEC_INSTANCE_SIZE_WORKAROUND line below.
     2. In setup.py add 'bsec_get_instance_size' back to REQUIRED_SYMBOLS. */
#define BSEC_INSTANCE_SIZE_WORKAROUND
#ifdef BSEC_INSTANCE_SIZE_WORKAROUND
#define BSEC_INSTANCE_SIZE UINT16_C(3272)
#endif
uint32_t bsec_state_len = 0;
bsec_library_return_t bsec_status = BSEC_OK;
const char *bsec_conf_path = "bsec_v3-3-0-0/release_bin/Sel_IAQ/config/bme690/bme690_sel_33v_3s_4d/bsec_selectivity.config";
FILE *bsec_conf;
#endif

static PyObject *bmeError;

/* Helper functions for per-sensor config/state file paths */
static void get_config_filename(const char *sensor_id, char *out_path, size_t max_len)
{
    snprintf(out_path, max_len, "conf/bsec_config_%s.txt", sensor_id);
}

static void get_state_filename(const char *sensor_id, char *out_path, size_t max_len)
{
    snprintf(out_path, max_len, "conf/state_data_%s.txt", sensor_id);
}

typedef struct
{
    PyObject_HEAD
        int linux_device;
    int8_t temp_offset;
    void *bsec_inst;
    struct bme69x_dev bme;
    struct bme69x_conf conf;
    struct bme69x_heatr_conf heatr_conf;
    struct bme69x_data *data;
    struct bme69x_data data_backup;
    int64_t timestamp_backup;
    uint32_t del_period;
    uint32_t time_ms;
    uint8_t n_fields;
    uint64_t next_call;
    uint8_t last_meas_index;
    int8_t rslt;
    uint8_t op_mode;
    uint16_t sample_count;
    uint8_t debug_mode;
    uint8_t i2c_addr;
    char sensor_id[64];
} BMEObject;

static void
bme69x_dealloc(BMEObject *self)
{
    Py_TYPE(self)->tp_free((PyObject *)self);
}

static PyObject *
bme69x_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    BMEObject *self;
    self = (BMEObject *)type->tp_alloc(type, 0);
    if (self != NULL)
    {
        self->linux_device = 0;
        self->temp_offset = 5;

/*! 690 API has replace gh1 - 3 with h1 - 3, and removed H7 - no idea why!  */
        self->bme.amb_temp = 0;
        self->bme.calib.par_g1 = 0;
        self->bme.calib.par_g2 = 0;
        self->bme.calib.par_g3 = 0;
        self->bme.calib.par_h1 = 0;
        self->bme.calib.par_h2 = 0;
        self->bme.calib.par_h3 = 0;
        self->bme.calib.par_h4 = 0;
        self->bme.calib.par_h5 = 0;
        self->bme.calib.par_h6 = 0;
/*!        self->bme.calib.par_h7 = 0;  */
        self->bme.calib.par_p10 = 0;
        self->bme.calib.par_p1 = 0;
        self->bme.calib.par_p2 = 0;
        self->bme.calib.par_p3 = 0;
        self->bme.calib.par_p4 = 0;
        self->bme.calib.par_p5 = 0;
        self->bme.calib.par_p6 = 0;
        self->bme.calib.par_p7 = 0;
        self->bme.calib.par_p8 = 0;
        self->bme.calib.par_p9 = 0;
        self->bme.calib.par_t1 = 0;
        self->bme.calib.par_t2 = 0;
        self->bme.calib.par_t3 = 0;
        self->bme.calib.range_sw_err = 0;
        self->bme.calib.res_heat_range = 0;
        self->bme.calib.res_heat_val = 0;
        self->bme.calib.t_fine = 0.0;
        self->bme.chip_id = 0;
        self->bme.delay_us = pi3g_delay_us;
        self->bme.info_msg = 0;
        self->bme.intf = BME69X_I2C_INTF;
        self->bme.intf_ptr = &(self->linux_device);
        self->bme.intf_rslt = 0;
        self->bme.mem_page = 0;
        self->bme.read = pi3g_read;
        self->bme.variant_id = 0;
        self->bme.write = pi3g_write;

        self->conf.os_hum = 0;
        self->conf.os_temp = 0;
        self->conf.os_pres = 0;
        self->conf.filter = 0;
        self->conf.odr = 0;

        self->heatr_conf.enable = 0;
        self->heatr_conf.heatr_dur = 0;
        self->heatr_conf.heatr_dur_prof = malloc(sizeof(uint16_t) * 10);
        if (self->heatr_conf.heatr_dur_prof) {
            for (int i = 0; i < 10; ++i) self->heatr_conf.heatr_dur_prof[i] = 0;
        }
        self->heatr_conf.heatr_temp = 0;
        self->heatr_conf.heatr_temp_prof = malloc(sizeof(uint16_t) * 10);
        if (self->heatr_conf.heatr_temp_prof) {
            for (int i = 0; i < 10; ++i) self->heatr_conf.heatr_temp_prof[i] = 0;
        }
        self->heatr_conf.profile_len = 0;
        self->heatr_conf.shared_heatr_dur = 0;

        self->data = malloc(sizeof(struct bme69x_data) * 3);

        self->data_backup.status = 0;
        self->data_backup.gas_index = 0;
        self->data_backup.meas_index = 0;
        self->data_backup.res_heat = 0;
        self->data_backup.idac = 0;
        self->data_backup.gas_wait = 0;
#ifndef BME69X_USE_FPU
        self->data_backup.temperature = 0;
        self->data_backup.pressure = 0;
        self->data_backup.humidity = 0;
        self->data_backup.gas_resistance = 0;
#else
        self->data_backup.temperature = 0.0;
        self->data_backup.pressure = 0.0;
        self->data_backup.humidity = 0.0;
        self->data_backup.gas_resistance = 0.0;
#endif

        self->timestamp_backup = 0;

        self->del_period = 0;
        self->time_ms = 0;
        self->n_fields = 0;
        self->next_call = 0;
        self->last_meas_index = 0;
        self->rslt = BME69X_OK;
        self->op_mode = BME69X_SLEEP_MODE;
        self->sample_count = 0;
        self->debug_mode = 0;
    }
    return (PyObject *)self;
}

static int
bme69x_init_type(BMEObject *self, PyObject *args, PyObject *kwds)
{
    static char *kwlist[] = {"i2c_addr", "i2c_bus", "debug_mode", "sensor_name", NULL};
    
    uint8_t i2c_addr = 0;
    uint8_t i2c_bus = 1;
    uint8_t debug_mode = 0;
    const char *sensor_name = NULL;
    
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "b|bbs", kwlist, &i2c_addr, &i2c_bus, &debug_mode, &sensor_name))
    {
        perror("Failed to parse arguments");
        return -1;
    }
    
    self->i2c_addr = i2c_addr;
    self->debug_mode = debug_mode;
    
    /* Generate sensor_id: use provided name or auto-generate from address */
    if (sensor_name)
    {
        snprintf(self->sensor_id, sizeof(self->sensor_id), "%s", sensor_name);
    }
    else
    {
        snprintf(self->sensor_id, sizeof(self->sensor_id), "sensor_0x%02x", i2c_addr);
    }
    
    /* Open I2C device */
    char i2c_path[32];
    snprintf(i2c_path, sizeof(i2c_path), "/dev/i2c-%d", i2c_bus);
    
    self->linux_device = open(i2c_path, O_RDWR);
    if (self->linux_device < 0)
    {
        perror("Failed to open I2C port");
        PyErr_SetString(bmeError, "Failed to open I2C device port");
        return -1;
    }

    /* Set I2C slave address */
    if (ioctl(self->linux_device, I2C_SLAVE, i2c_addr) < 0)
    {
        printf("WARNING: I2C_SLAVE ioctl failed for address 0x%02x (errno=%d)\n", i2c_addr, errno);
        perror("I2C_SLAVE");
        /* Continue anyway - BME69X might still work via I2C_RDWR */
    }
    
    /* Initialize BME69X sensor */
    self->bme.intf = BME69X_I2C_INTF;
    self->bme.amb_temp = 0;
    self->bme.read = pi3g_read;
    self->bme.write = pi3g_write;
    self->bme.delay_us = pi3g_delay_us;

    self->rslt = BME69X_OK;
    self->rslt = bme69x_init(&(self->bme));
    if (self->rslt == BME69X_OK)
    {
        if (self->debug_mode == 1)
        {
            printf("INITIALIZED BME69X\n");
            if (self->bme.variant_id == BME69X_VARIANT_GAS_LOW)
            {
                printf("VARIANT BME680\n");
            }   else if (self->bme.variant_id == BME69X_VARIANT_GAS_HIGH)  {
                printf("VARIANT BME688\n");
            } else {
                printf("VARIANT BME690\n");
            }
        }
    }
    else
    {
        perror("initialize BME69X");
        close(self->linux_device);
        PyErr_SetString(bmeError, "Could not initialize BME69X");
        return -1;
    }
#ifdef BSEC
    {
#ifdef BSEC_INSTANCE_SIZE_WORKAROUND
        size_t bsec_inst_size = (size_t)BSEC_INSTANCE_SIZE;
#else
        size_t bsec_inst_size = bsec_get_instance_size();
#endif
        if (bsec_inst_size == 0) {
            close(self->linux_device);
            PyErr_SetString(bmeError, "BSEC instance size is zero");
            return -1;
        }
        self->bsec_inst = malloc(bsec_inst_size);
        if (!self->bsec_inst) {
            close(self->linux_device);
            PyErr_SetString(bmeError, "Failed to allocate BSEC instance");
            return -1;
        }
        memset(self->bsec_inst, 0, bsec_inst_size);
        self->rslt = bsec_init(self->bsec_inst);
        if (self->rslt != BSEC_OK)
        {
            free(self->bsec_inst);
            close(self->linux_device);
            PyErr_SetString(bmeError, "Failed to initialize BSEC");
            return -1;
        }
    }
    bsec_version_t version;
    bsec_get_version(self->bsec_inst, &version);
    if (self->debug_mode == 1)
    {
        printf("INITIALIZED BSEC\nBSEC VERSION: %d.%d.%d.%d\n", version.major, version.minor, version.major_bugfix, version.minor_bugfix);
    }
#endif
    return 0;
}

static PyMemberDef bme69x_members[] = {
    {"linux_device_handle", T_INT, offsetof(BMEObject, linux_device), 0, "Linux file descriptor of the sensor device"},
    {"temp_offset", T_BYTE, offsetof(BMEObject, temp_offset), 0, "temperature offset to be subtracted from 25 degC"},
     /* Expose only simple fields to Python. Embedded C structs are not exposed as PyObject*
         because that would let Python treat raw struct memory as PyObject pointers and
         can corrupt memory. Use accessor functions if you need Python-level access. */
    {"del_period", T_ULONG, offsetof(BMEObject, del_period), 0, "delay period"},
    {"time_ms", T_ULONG, offsetof(BMEObject, time_ms), 0, "millisecond precision time stamp"},
    {"n_fields", T_UBYTE, offsetof(BMEObject, n_fields), 0, "number of data fields"},
    {"next_call", T_LONGLONG, offsetof(BMEObject, next_call), 0, "nanosecond timestamp of next bsec_sensore_control call"},
    {"last_meas_index", T_UBYTE, offsetof(BMEObject, last_meas_index), 0, "index to track measurement order"},
    {"rslt", T_BYTE, offsetof(BMEObject, rslt), 0, "function execution result"},
    {"op_mode", T_UBYTE, offsetof(BMEObject, op_mode), 0, "BME69X operation mode"},
    {"sample_count", T_UINT, offsetof(BMEObject, sample_count), 0, "number of data samples"},
    {"debug_mode", T_UBYTE, offsetof(BMEObject, debug_mode), 0, "enable/disable debug_mode"},
    {NULL},
};

#ifdef BSEC
static PyObject *bme_set_sample_rate(BMEObject *self, PyObject *args)
{
    float sample_rate;

    if (!PyArg_ParseTuple(args, "f", &sample_rate))
    {
        PyErr_SetString(bmeError, "Argument must be of type float");
        return NULL;
    }

    return Py_BuildValue("i", bsec_set_sample_rate(self->bsec_inst, sample_rate));
}
#endif

static PyObject *bme_init_bme69x(BMEObject *self)
{
    // Initialize BME69X sensor
    self->bme.intf = BME69X_I2C_INTF;
    self->bme.amb_temp = 25;
    self->bme.read = pi3g_read;
    self->bme.write = pi3g_write;
    self->bme.delay_us = pi3g_delay_us;

    self->rslt = BME69X_OK;
    self->rslt = bme69x_init(&(self->bme));
    if (self->rslt == BME69X_OK)
    {
        if (self->debug_mode == 1)
        {
            printf("INITIALIZED BME6XX\n");
            if (self->bme.variant_id == BME69X_VARIANT_GAS_LOW)
            {
                printf("VARIANT BME680\n");
            }
            else if (self->bme.variant_id == BME69X_VARIANT_GAS_HIGH)  {
                printf("VARIANT BME688\n");
            } else {
                printf("VARIANT BME690\n");
            }
        }
    }

    return Py_BuildValue("i", self->rslt);
}

static PyObject *bme_print_dur_prof(BMEObject *self)
{
    for (uint8_t i = 0; i < self->heatr_conf.profile_len; i++)
    {
        printf("%d ", self->heatr_conf.heatr_dur_prof[i]);
    }
    printf("\n");

    return Py_BuildValue("s", "None");
}

static PyObject *bme_enable_debug_mode(BMEObject *self)
{
    self->debug_mode = 1;
    return Py_BuildValue("s", "Disabled debug mode");
}

static PyObject *bme_disable_debug_mode(BMEObject *self)
{
    self->debug_mode = 0;
    return Py_BuildValue("s", "Enabled debug mode");
}

static PyObject *bme_get_sensor_id(BMEObject *self)
{
    uint8_t id_regs[4];
    uint32_t len = 4;
    uint32_t uid;
    int8_t rslt;
    rslt = bme69x_get_regs(BME69X_REG_UNIQUE_ID, (uint8_t *) &id_regs, len, &(self->bme));
    if (rslt < BME69X_OK)
    {
        PyErr_SetString(bmeError, "Failed to read sensor id register");
        return NULL;
    }
    // Not mentioned in bme688 datasheet but 4 byte sensor id is stored in register 0x83 in msb
    uid = (id_regs[0] << 24) | (id_regs[1] << 16) | (id_regs[2] << 8) | id_regs[3];

    return Py_BuildValue("i", uid);
}

static PyObject *bme_set_temp_offset(BMEObject *self, PyObject *args)
{
    int t_offs;
    if (!PyArg_ParseTuple(args, "i", &t_offs))
    {
        PyErr_SetString(bmeError, "Invalid arguments in set_temp_offset(double t_offs)");
        return NULL;
    }

    self->temp_offset = t_offs;
    self->bme.amb_temp = 25 - self->temp_offset;

    if (self->debug_mode == 1)
    {
        printf("SET TEMP OFFSET\n");
    }

    return Py_BuildValue("i", 0);
}

#ifdef BSEC
static PyObject *bme_subscribe_gas_estimates(BMEObject *self, PyObject *args)
{
    uint8_t n_requested_virtual_sensors;
    if (!PyArg_ParseTuple(args, "b", &n_requested_virtual_sensors))
    {
        PyErr_SetString(bmeError, "Argument must be int number of gas estimates (0 - 4)");
        return (PyObject *)NULL;
    }
    uint8_t n_req_sensors = n_requested_virtual_sensors + 1;
    bsec_sensor_configuration_t requested_virtual_sensors[n_req_sensors];

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    for (uint8_t i = 0; i < n_requested_virtual_sensors; i++)
    {
        requested_virtual_sensors[i].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1 + i;
        requested_virtual_sensors[i].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    }
    requested_virtual_sensors[n_requested_virtual_sensors].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[n_requested_virtual_sensors].sample_rate = BSEC_SAMPLE_RATE_SCAN ;

    self->rslt = bsec_update_subscription(self->bsec_inst, requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);

    return Py_BuildValue("i", self->rslt);
}

static PyObject *bme_subscribe_ai_classes(BMEObject *self)
{
    uint8_t n_requested_virtual_sensors;
    n_requested_virtual_sensors = 5;
    bsec_sensor_configuration_t requested_virtual_sensors[n_requested_virtual_sensors];

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_3;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_4;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[4].sensor_id = BSEC_OUTPUT_RAW_GAS_INDEX;
    requested_virtual_sensors[4].sample_rate = BSEC_SAMPLE_RATE_SCAN ;

    self->rslt = bsec_update_subscription(self->bsec_inst, requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);

    for (uint8_t i = 0; i < n_required_sensor_settings; i++)
    {
        switch (required_sensor_settings[i].sensor_id)
        {
        case BSEC_INPUT_PRESSURE:
            printf("BSEC_INPUT_PRESSURE ");
            break;
        case BSEC_INPUT_HUMIDITY:
            printf("BSEC_INPUT_HUMIDITY ");
            break;
        case BSEC_INPUT_TEMPERATURE:
            printf("BSEC_INPUT_TEMPERATURE ");
            break;
        case BSEC_INPUT_GASRESISTOR:
            printf("BSEC_INPUT_GASRESISTOR ");
            break;
        case BSEC_INPUT_HEATSOURCE:
            printf("BSEC_INPUT_HEATSOURCE ");
            break;
        case BSEC_INPUT_DISABLE_BASELINE_TRACKER:
            printf("BSEC_INPUT_DISABLE_BASELINE_TRACKER ");
            break;
        case BSEC_INPUT_PROFILE_PART:
            printf("BSEC_INPUT_PROFILE_PART ");
            break;
        case 22:
            printf("ADDITIONAL_INPUT_FOR_DEVICE_HEAT_COMPENSATION_8 ");
            break;
        default:
            printf("UNKOWN CASE FOR SENSOR ID IN SUBSCRIBE_AI_CLASSES %d ", required_sensor_settings[i].sensor_id);
            break;
        }

        printf("%.6f\n", required_sensor_settings[i].sample_rate);
    }

    return Py_BuildValue("i", self->rslt);
}
#endif

static PyObject *bme_get_chip_id(BMEObject *self)
{
    return Py_BuildValue("i", self->bme.chip_id);
}

static PyObject *bme_close_i2c(BMEObject *self)
{
    return Py_BuildValue("i", close(*((int *)self->bme.intf_ptr)));
}

static PyObject *bme_open_i2c(BMEObject *self, PyObject *args)
{
    close(*((int *)self->bme.intf_ptr));
    self->linux_device = open(I2C_PORT_1, O_RDWR);
    if (self->linux_device < 0)
    {
        perror("Failed to open I2C port");
        PyErr_SetString(bmeError, "Failed to open I2C device port");
        return (PyObject *)NULL;
    }
    self->bme.intf_ptr = &(self->linux_device);
    Py_ssize_t size = PyTuple_Size(args);
    uint8_t i2c_addr;

    if ((uint8_t)size == 1)
    {
        if (!PyArg_ParseTuple(args, "b", &i2c_addr))
        {
            PyErr_SetString(bmeError, "Failed to parse I2C address");
            return (PyObject *)NULL;
        }
        else if (ioctl(*((int *)self->bme.intf_ptr), I2C_SLAVE, i2c_addr) < 0)
        {
            PyErr_SetString(bmeError, "Failed to open I2C address");
            return (PyObject *)NULL;
        }
    }
    else
    {
        PyErr_SetString(bmeError, "Argument must be i2c_addr: int");
        close(*((int *)self->bme.intf_ptr));
        return (PyObject *)NULL;
    }

    return Py_BuildValue("i", 0);
}

static PyObject *bme_get_variant(BMEObject *self)
{
    char *variant = "";
    if (self->bme.variant_id == BME69X_VARIANT_GAS_LOW)
    {
        variant = "BME680";
    }
    else if (self->bme.variant_id == BME69X_VARIANT_GAS_HIGH)
    {
        variant = "BME688";
    }
    else if (self->bme.variant_id == BME690_VARIANT_GAS_HIGH)
        {
            variant = "BME690";
        }
    else
    {
        variant = "UNKNOWN";
    }
    return Py_BuildValue("s", variant);
}

static PyObject *bme_set_conf(BMEObject *self, PyObject *args)
{
    Py_ssize_t size = PyTuple_Size(args);
    uint8_t c[size];

    switch (size)
    {
    case 1:
        PyArg_ParseTuple(args, "b", &c[0]);
        self->rslt = pi3g_set_conf(c[0], self->conf.os_pres, self->conf.os_temp, self->conf.filter, self->conf.odr, &(self->conf), &(self->bme), self->debug_mode);
        break;
    case 2:
        PyArg_ParseTuple(args, "bb", &c[0], &c[1]);
        self->rslt = pi3g_set_conf(c[0], c[1], self->conf.os_temp, self->conf.filter, self->conf.odr, &(self->conf), &(self->bme), self->debug_mode);
        break;
    case 3:
        PyArg_ParseTuple(args, "bbb", &c[0], &c[1], &c[2]);
        self->rslt = pi3g_set_conf(c[0], c[1], c[2], self->conf.filter, self->conf.odr, &(self->conf), &(self->bme), self->debug_mode);
        break;
    case 4:
        PyArg_ParseTuple(args, "bbbb", &c[0], &c[1], &c[2], &c[3]);
        self->rslt = pi3g_set_conf(c[0], c[1], c[2], c[3], self->conf.odr, &(self->conf), &(self->bme), self->debug_mode);
        break;
    case 5:
        PyArg_ParseTuple(args, "bbbbb", &c[0], &c[1], &c[2], &c[3], &c[4]);
        self->rslt = pi3g_set_conf(c[0], c[1], c[2], c[3], c[4], &(self->conf), &(self->bme), self->debug_mode);
        break;
    default:
        self->rslt = pi3g_set_conf(BME69X_OS_16X, BME69X_OS_1X, BME69X_OS_2X, BME69X_FILTER_OFF, BME69X_ODR_NONE, &(self->conf), &(self->bme), self->debug_mode);
        break;
    }

    return Py_BuildValue("b", self->rslt);
}

static PyObject *bme_set_heatr_conf(BMEObject *self, PyObject *args)
{
    uint8_t enable;
    PyObject *temp_prof_obj;
    PyObject *dur_prof_obj;
    if (!PyArg_ParseTuple(args, "bOOb", &enable, &temp_prof_obj, &dur_prof_obj, &(self->op_mode)))
    {
        PyErr_SetString(bmeError, "Function takes 4 arguments: enable, temp_prof, dur_prof, op_mode");
        return (PyObject *)NULL;
    }
    if (self->op_mode == BME69X_FORCED_MODE)
    {
        uint16_t heatr_temp, heatr_dur;
        PyArg_Parse(temp_prof_obj, "H", &heatr_temp);
        PyArg_Parse(dur_prof_obj, "H", &heatr_dur);
        if (heatr_temp == 0 || heatr_dur == 0)
        {
            PyErr_SetString(bmeError, "heatr_temp and heatr_dur need to be of type uint16_t (unsigned short)");
            return (PyObject *)NULL;
        }
        struct bme69x_dev t_dev;
        memset(&t_dev, 0, sizeof(t_dev));
        t_dev.chip_id = self->bme.chip_id;
        t_dev.intf = self->bme.intf;
        t_dev.amb_temp = self->bme.amb_temp;
        t_dev.read = self->bme.read;
        t_dev.write = self->bme.write;
        t_dev.delay_us = self->bme.delay_us;
        t_dev.intf_ptr = &(self->linux_device);
        t_dev.variant_id = self->bme.variant_id;
        t_dev.calib = self->bme.calib;
        t_dev.info_msg = self->bme.info_msg;
        t_dev.intf_rslt = 0;
        t_dev.mem_page = self->bme.mem_page;
        self->rslt = pi3g_set_heater_conf_fm(enable, heatr_temp, heatr_dur, &(self->heatr_conf), &t_dev, self->debug_mode);
    }
    else
    {
        if (self->bme.variant_id == BME69X_VARIANT_GAS_LOW)
        {
            printf("ONLY FORCED MODE IS AVAILABLE FOR BME680 SENSOR\n");
            return Py_BuildValue("i", -1);
        }

        if (!PyList_Check(temp_prof_obj) || !PyList_Check(dur_prof_obj))
        {
            PyErr_SetString(bmeError, "temp_prof and dur_prof must be of type list\n");
            return (PyObject *)NULL;
        }

        int temp_size = PyList_Size(temp_prof_obj);
        int dur_size = PyList_Size(dur_prof_obj);
        if (temp_size != dur_size)
        {
            PyErr_SetString(bmeError, "temp_prof and dur_prof must have the same size");
            return (PyObject *)NULL;
        }
        if (temp_size > 10)
        {
            PyErr_SetString(bmeError, "length of heater profile must not exceed 10");
            return (PyObject *)NULL;
        }

        uint16_t temp_prof[temp_size], dur_prof[temp_size];
        PyObject *val;

        for (int i = 0; i < temp_size; i++)
        {
            val = PyList_GetItem(temp_prof_obj, i);
            temp_prof[i] = (uint16_t)PyLong_AsLong(val);
            val = PyList_GetItem(dur_prof_obj, i);
            dur_prof[i] = (uint16_t)PyLong_AsLong(val);
        }

        for (int i = 0; i < temp_size; i++)
        {
            printf("%d ", temp_prof[i]);
        }
        printf("\n");
        for (int i = 0; i < temp_size; i++)
        {
            printf("%d ", dur_prof[i]);
        }
        printf("\n");

        if (self->op_mode == BME69X_PARALLEL_MODE)
        {
            self->rslt = pi3g_set_heater_conf_pm(enable, temp_prof, dur_prof, (uint8_t)temp_size, &(self->conf), &(self->heatr_conf), &(self->bme), self->debug_mode);
            printf("DUR PROF AFTER PI3G\n");
            for (uint8_t i = 0; i < self->heatr_conf.profile_len; i++)
            {
                printf("%d ", self->heatr_conf.heatr_dur_prof[i]);
            }
            printf("\n");
        }
        else if (self->op_mode == BME69X_SEQUENTIAL_MODE)
        {
            self->rslt = pi3g_set_heater_conf_sm(enable, temp_prof, dur_prof, (uint8_t)temp_size, &(self->heatr_conf), &(self->bme), self->debug_mode);
        }
        else
        {
            perror("set_heatr_conf");
        }
    }

    return Py_BuildValue("i", self->rslt);
}

static PyObject *bme_get_data(BMEObject *self)
{
    self->rslt = bme69x_set_op_mode(self->op_mode, &(self->bme));

    if (self->rslt != BME69X_OK)
    {
        perror("set_op_mode");
    }

    if (self->op_mode == BME69X_FORCED_MODE)
    {
        self->del_period = bme69x_get_meas_dur(BME69X_FORCED_MODE, &(self->conf), &(self->bme)) + (self->heatr_conf.heatr_dur * 1000);
        self->bme.delay_us(self->del_period, self->bme.intf_ptr);
        self->time_ms = pi3g_timestamp_ms();

        self->rslt = bme69x_get_data(self->op_mode, self->data, &(self->n_fields), &(self->bme));
        if (self->rslt == BME69X_OK && self->n_fields > 0)
        {
            self->sample_count++;
            self->bme.amb_temp = self->data[0].temperature - self->temp_offset;

            PyObject *pydata = PyDict_New();
            DICT_SET_ITEM(pydata, "sample_nr", Py_BuildValue("i", self->sample_count));
            DICT_SET_ITEM(pydata, "timestamp", Py_BuildValue("i", self->time_ms));
            DICT_SET_ITEM(pydata, "raw_temperature", Py_BuildValue("d", self->data[0].temperature));
            DICT_SET_ITEM(pydata, "raw_pressure", Py_BuildValue("d", self->data[0].pressure / 100));
            DICT_SET_ITEM(pydata, "raw_humidity", Py_BuildValue("d", self->data[0].humidity));
            DICT_SET_ITEM(pydata, "raw_gas", Py_BuildValue("d", self->data[0].gas_resistance / 1000));
            DICT_SET_ITEM(pydata, "status", Py_BuildValue("i", self->data[0].status));
            return pydata;
        }
    }
    else
    {
        PyObject *pydata = PyList_New(self->heatr_conf.profile_len);
        uint8_t counter = 0;
        while (counter < self->heatr_conf.profile_len)
        {
            if (self->op_mode == BME69X_PARALLEL_MODE)
            {
                self->del_period = bme69x_get_meas_dur(BME69X_PARALLEL_MODE, &(self->conf), &(self->bme)) + (self->heatr_conf.shared_heatr_dur * 1000);
            }
            else if (self->op_mode == BME69X_SEQUENTIAL_MODE)
            {
                self->del_period = bme69x_get_meas_dur(BME69X_SEQUENTIAL_MODE, &(self->conf), &(self->bme)) + (self->heatr_conf.heatr_dur_prof[0] * 1000);
            }
            else
            {
                PyErr_SetString(bmeError, "Failed to receive data");
                return (PyObject *)NULL;
            }
            self->bme.delay_us(self->del_period, self->bme.intf_ptr);

            self->rslt = bme69x_get_data(self->op_mode, self->data, &(self->n_fields), &(self->bme));
            if (self->rslt < 0)
            {
                perror("bme69x_get_data");
            }

            /* Check if rslt == BME69X_OK, report or handle if otherwise */
            for (uint8_t i = 0; i < self->n_fields; i++)
            {
                if (self->data[i].status == BME69X_VALID_DATA)
                {
                    PyObject *field = PyDict_New();
                    self->time_ms = pi3g_timestamp_ms();
                    DICT_SET_ITEM(field, "sample_nr", Py_BuildValue("i", self->sample_count));
                    DICT_SET_ITEM(field, "timestamp", Py_BuildValue("i", self->time_ms));
                    DICT_SET_ITEM(field, "raw_temperature", Py_BuildValue("d", self->data[i].temperature));
                    DICT_SET_ITEM(field, "raw_pressure", Py_BuildValue("d", self->data[i].pressure / 100));
                    DICT_SET_ITEM(field, "raw_humidity", Py_BuildValue("d", self->data[i].humidity));
                    DICT_SET_ITEM(field, "raw_gas", Py_BuildValue("d", self->data[i].gas_resistance / 1000));
                    DICT_SET_ITEM(field, "gas_index", Py_BuildValue("i", self->data[i].gas_index));
                    DICT_SET_ITEM(field, "meas_index", Py_BuildValue("i", self->data[i].meas_index));
                    DICT_SET_ITEM(field, "status", Py_BuildValue("i", self->data[i].status));
                    PyList_SetItem(pydata, self->data[i].gas_index, field);
                    self->sample_count++;
                    counter++;
                }
            }
        }
        self->bme.amb_temp = self->data[0].temperature - self->temp_offset;
        return pydata;
    }
    return Py_BuildValue("s", "Failed to get data");
}

#ifdef BSEC
static PyObject *bme_get_digital_nose_data(BMEObject *self)
{
    // Create Timestamp and wait until measurement has to be triggered
    int64_t time_stamp = pi3g_timestamp_ns();

    // Check if bsec_sensor_controll needs to be called
    if (time_stamp >= (int64_t)self->next_call)
    {
        bsec_bme_settings_t sensor_settings;

        self->rslt = bsec_sensor_control(self->bsec_inst, time_stamp, &sensor_settings);
        if (self->debug_mode == 1 && self->rslt != BSEC_OK)
        {
            printf("BSEC SENSOR CONTROL RSLT %d\n", self->rslt);
        }
        self->next_call = sensor_settings.next_call;

        /* Select the power mode */
        /* Must be set before writing the sensor configuration */
        self->op_mode = sensor_settings.op_mode;
        self->rslt = bme69x_set_op_mode(self->op_mode, &(self->bme));
        if (self->rslt != BME69X_OK)
        {
            perror("set_op_mode");
        }

        /* Configure sensor */
        /* Set sensor configuration */
        self->rslt = pi3g_set_conf(sensor_settings.humidity_oversampling, sensor_settings.pressure_oversampling, sensor_settings.pressure_oversampling, BME69X_FILTER_OFF, BME69X_ODR_NONE, &(self->conf), &(self->bme), self->debug_mode);
        if (self->rslt < 0)
        {
            PyErr_SetString(bmeError, "FAILED TO SET CONFIG");
            return NULL;
        }

        self->rslt = pi3g_set_heater_conf_pm(sensor_settings.run_gas, sensor_settings.heater_temperature_profile, sensor_settings.heater_duration_profile, sensor_settings.heater_profile_len, &(self->conf), &(self->heatr_conf), &(self->bme), self->debug_mode);
        if (self->rslt < 0)
        {
            PyErr_SetString(bmeError, "FAILED TO SET HEATER CONFIG");
            return NULL;
        }
        uint8_t check_meas_index = 1;

        // In case measurement has to be triggered
        if (sensor_settings.trigger_measurement && sensor_settings.op_mode != BME69X_SLEEP_MODE)
        {

            if (self->op_mode == BME69X_FORCED_MODE)
            {
                printf("WHY AM I IN FORCED MODE?\n");
                PyErr_SetString(bmeError, "Failed to get data\nSensor is in Forced mode but it needs to be in Parallel mode");
                return (PyObject *)NULL;
            }
            else
            {
                PyObject *pydata = PyList_New(self->heatr_conf.profile_len);
                uint8_t counter = 0;
                while (counter < self->heatr_conf.profile_len)
                {
                    self->del_period = bme69x_get_meas_dur(BME69X_PARALLEL_MODE, &(self->conf), &(self->bme)) + (self->heatr_conf.shared_heatr_dur * 1000);
                    self->bme.delay_us(self->del_period, self->bme.intf_ptr);
                    self->time_ms = pi3g_timestamp_ms();

                    self->rslt = bme69x_get_data(self->op_mode, self->data, &(self->n_fields), &(self->bme));
                    if (self->rslt < 0)
                    {
                        perror("bme69x_get_data");
                    }

                    /* Check if rslt == BME69X_OK, report or handle if otherwise */
                    for (uint8_t i = 0; i < self->n_fields; i++)
                    {
                        // if(self->data[i].status != BME69X_VALID_DATA)
                        // {
                        //     no_valid_data++;
                        // }

                        // if(no_valid_data == self->n_fields - 1 && self->data[i].status != BME69X_VALID_DATA)
                        // {
                        //     self->data[i] = self->data_backup;
                        //     uint8_t new_gas_index = (self->data_backup.gas_index + 1) % sensor_settings.heater_profile_len;
                        //     self->data[i].gas_index = new_gas_index;
                        // }

                        if (self->data[i].status & BME69X_GASM_VALID_MSK)
                        {
                            /* Measurement index check to track the first valid sample after operation mode change */
                            if (check_meas_index)
                            {
                                /* After changing the operation mode, Measurement index expected to be zero
                                * however with considering the data miss case as well, condition shall be checked less
                                * than last received measurement index */
                                if (self->last_meas_index == 0 || self->data[i].meas_index == 0 || self->data[i].meas_index < self->last_meas_index)
                                {
                                    check_meas_index = false;
                                }
                                else
                                {
                                    continue; // Skip the invalid data samples or data from last duty cycle scan
                                }
                            }

                            self->last_meas_index = self->data[i].meas_index;

                            // We got valid data, time to bsec_do_steps
                            uint8_t n_bsec_inputs = 0;
                            bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR];

                            // Read the data into bsec_input_t[]
                            if (sensor_settings.process_data)
                            {
                                /* Pressure to be processed by BSEC */
                                if (sensor_settings.process_data & BSEC_PROCESS_PRESSURE)
                                {
                                    // printf("PRESSURE %f\n", self->data[i].pressure);
                                    /* Place presssure sample into input struct */
                                    inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_PRESSURE;
                                    inputs[n_bsec_inputs].signal = self->data[i].pressure;
                                    inputs[n_bsec_inputs].time_stamp = time_stamp;
                                    n_bsec_inputs++;
                                }
                                /* Temperature to be processed by BSEC */
                                if (sensor_settings.process_data & BSEC_PROCESS_TEMPERATURE)
                                {
                                    // printf("TEMPERATURE %f\n", self->data[i].temperature);
                                    /* Place temperature sample into input struct */
                                    inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
#ifdef BME69X_FLOAT_POINT_COMPENSATION
                                    inputs[n_bsec_inputs].signal = self->data[i].temperature;
#else
                                    inputs[n_bsec_inputs].signal = self->data[i].temperature / 100.0f;
#endif
                                    inputs[n_bsec_inputs].time_stamp = time_stamp;
                                    n_bsec_inputs++;

                                    /* Also add optional heatsource input which will be subtracted from the temperature reading to 
                                    * compensate for device-specific self-heating (supported in BSEC IAQ solution)*/
                                    inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
                                    inputs[n_bsec_inputs].signal = self->temp_offset;
                                    inputs[n_bsec_inputs].time_stamp = time_stamp;
                                    n_bsec_inputs++;
                                }
                                /* Humidity to be processed by BSEC */
                                if (sensor_settings.process_data & BSEC_PROCESS_HUMIDITY)
                                {
                                    // printf("HUMIDITY %f\n",self->data[i].humidity);
                                    /* Place humidity sample into input struct */
                                    inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
#ifdef BME69X_FLOAT_POINT_COMPENSATION
                                    inputs[n_bsec_inputs].signal = self->data[i].humidity;
#else
                                    inputs[n_bsec_inputs].signal = self->data[i].humidity / 1000.0f;
#endif
                                    inputs[n_bsec_inputs].time_stamp = time_stamp;
                                    n_bsec_inputs++;
                                }
                                /* Gas to be processed by BSEC */
                                if (sensor_settings.process_data & BSEC_PROCESS_GAS)
                                {
                                    // printf("GAS_RESISTANCE %f\n", self->data[i].gas_resistance);
                                    /* Check whether gas_valid flag is set */
                                    if (self->data[i].status & BME69X_GASM_VALID_MSK)
                                    {
                                        /* Place sample into input struct */
                                        inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
                                        inputs[n_bsec_inputs].signal = self->data[i].gas_resistance;
                                        inputs[n_bsec_inputs].time_stamp = time_stamp;
                                        n_bsec_inputs++;
                                    }
                                }
                                /* Profile part */
                                if (self->op_mode == BME69X_PARALLEL_MODE || self->op_mode == BME69X_SEQUENTIAL_MODE)
                                {
                                    // printf("PROFILE_PART %d\n", self->data[i].gas_index);
                                    inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_PROFILE_PART;
                                    inputs[n_bsec_inputs].signal = self->data[i].gas_index;
                                    inputs[n_bsec_inputs].time_stamp = time_stamp;
                                    n_bsec_inputs++;
                                }
                            }
                            bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
                            uint8_t n_output = BSEC_NUMBER_OUTPUTS;

                            self->rslt = bsec_do_steps(self->bsec_inst, inputs, n_bsec_inputs, bsec_outputs, &n_output);
                            if (self->rslt == BSEC_W_DOSTEPS_GASINDEXMISS)
                            {
                                printf("MISSED GAS INDEX \n");
                            }
                            else if (self->rslt != BSEC_OK)
                            {
                                printf("BSEC DO STEPS ERROR %d\nAT PROFILE PART %d\n", self->rslt, self->data[i].gas_index);
                                PyErr_SetString(bmeError, "BSEC Failed to process data");
                                return NULL;
                            }
                            else
                            {
                                // Read processed data into python Dict pydata
                                /* Iterate through the outputs and extract the relevant ones. */
                                self->sample_count++;
                                PyObject *bsec_data = PyDict_New();
                                DICT_SET_ITEM(bsec_data, "sample_nr", Py_BuildValue("i", self->sample_count));
                                DICT_SET_ITEM(bsec_data, "timestamp", Py_BuildValue("L", time_stamp));
                                for (uint8_t index = 0; index < n_output; index++)
                                {
                                    switch (bsec_outputs[index].sensor_id)
                                    {
                                    case BSEC_OUTPUT_STABILIZATION_STATUS:
                                        DICT_SET_ITEM(bsec_data, "stabilization_status", Py_BuildValue("i", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_RUN_IN_STATUS:
                                        DICT_SET_ITEM(bsec_data, "run_in_status", Py_BuildValue("i", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_IAQ:
                                        DICT_SET_ITEM(bsec_data, "iaq", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "iaq_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_STATIC_IAQ:
                                        DICT_SET_ITEM(bsec_data, "static_iaq", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "static_iaq_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_CO2_EQUIVALENT:
                                        DICT_SET_ITEM(bsec_data, "co2_equivalent", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "co2_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                                        DICT_SET_ITEM(bsec_data, "breath_voc_equivalent", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "breath_voc_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                                        DICT_SET_ITEM(bsec_data, "temperature", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_RAW_PRESSURE:
                                        DICT_SET_ITEM(bsec_data, "raw_pressure", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                                        DICT_SET_ITEM(bsec_data, "humidity", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_RAW_GAS:
                                        DICT_SET_ITEM(bsec_data, "raw_gas", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_RAW_TEMPERATURE:
                                        DICT_SET_ITEM(bsec_data, "raw_temperature", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_RAW_HUMIDITY:
                                        DICT_SET_ITEM(bsec_data, "raw_humidity", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_GAS_PERCENTAGE:
                                        DICT_SET_ITEM(bsec_data, "gas_percentage", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "gas_percentage_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_RAW_GAS_INDEX:
                                        DICT_SET_ITEM(bsec_data, "raw_gas_index", Py_BuildValue("d", bsec_outputs[index].signal));
                                        break;
                                    case BSEC_OUTPUT_GAS_ESTIMATE_1:
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_1", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_1_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_GAS_ESTIMATE_2:
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_2", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_2_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_GAS_ESTIMATE_3:
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_3", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_3_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    case BSEC_OUTPUT_GAS_ESTIMATE_4:
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_4", Py_BuildValue("d", bsec_outputs[index].signal));
                                        DICT_SET_ITEM(bsec_data, "gas_estimate_4_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                        break;
                                    default:
                                        continue;
                                    }
                                }
                                PyList_SetItem(pydata, counter, bsec_data);
                                counter++;
                            }
                        }
                    }
                }
                // self->bme.amb_temp = self->data[0].temperature - self->temp_offset;
                return pydata;
            }
        }
    }

    Py_RETURN_NONE;
}

static PyObject *bme_get_bsec_data(BMEObject *self)
{
    /* Call TVOC calibration function to manage baseline adaptation */
    tvoc_equivalent_calibration();
    
    // Create Timestamp and wait until measurement has to be triggered
    int64_t time_stamp = pi3g_timestamp_ns();
    // Check if bsec_sensor_controll needs to be called
    if (time_stamp >= (int64_t)self->next_call)
    {
        bsec_bme_settings_t sensor_settings;

        self->rslt = bsec_sensor_control(self->bsec_inst, time_stamp, &sensor_settings);
        if (self->debug_mode == 1 && self->rslt != BSEC_OK)
        {
            printf("BSEC SENSOR CONTROL RSLT %d\n", self->rslt);
        }
        self->next_call = sensor_settings.next_call;

        /* Configure sensor */
        /* Set sensor configuration */
        self->rslt = pi3g_set_conf(sensor_settings.humidity_oversampling, sensor_settings.pressure_oversampling, sensor_settings.pressure_oversampling, BME69X_FILTER_OFF, BME69X_ODR_NONE, &(self->conf), &(self->bme), self->debug_mode);
        if (self->rslt < 0)
        {
            PyErr_SetString(bmeError, "FAILED TO SET CONFIG");
            return NULL;
        }

        self->rslt = pi3g_set_heater_conf_fm(sensor_settings.run_gas, sensor_settings.heater_temperature, sensor_settings.heater_duration, &(self->heatr_conf), &(self->bme), self->debug_mode);
        if (self->rslt < 0)
        {
            PyErr_SetString(bmeError, "FAILED TO SET HEATER CONFIG");
            return NULL;
        }
        uint8_t check_meas_index = 1;

        // In case measurement has to be triggered
        if (sensor_settings.trigger_measurement && sensor_settings.op_mode != BME69X_SLEEP_MODE)
        {
            /* Select the power mode */
            /* Must be set before writing the sensor configuration */
            self->op_mode = sensor_settings.op_mode;
            self->rslt = bme69x_set_op_mode(self->op_mode, &(self->bme));
            if (self->rslt != BME69X_OK)
            {
                perror("set_op_mode");
            }

            PyObject *bsec_data = PyDict_New();
            self->del_period = bme69x_get_meas_dur(BME69X_FORCED_MODE, &(self->conf), &(self->bme)) + (self->heatr_conf.shared_heatr_dur * 1000);
            self->bme.delay_us(self->del_period, self->bme.intf_ptr);
            self->time_ms = pi3g_timestamp_ms();

            self->rslt = bme69x_get_data(self->op_mode, self->data, &(self->n_fields), &(self->bme));
            if (self->rslt < 0)
            {
                perror("bme69x_get_data");
            }

            /* Check if rslt == BME69X_OK, report or handle if otherwise */
            for (uint8_t i = 0; i < self->n_fields; i++)
            {
                if (self->data[i].status & BME69X_GASM_VALID_MSK)
                {
                    /* Measurement index check to track the first valid sample after operation mode change */
                    if (check_meas_index)
                    {
                        /* After changing the operation mode, Measurement index expected to be zero
                        * however with considering the data miss case as well, condition shall be checked less
                        * than last received measurement index */
                        if (self->last_meas_index == 0 || self->data[i].meas_index == 0 || self->data[i].meas_index < self->last_meas_index)
                        {
                            check_meas_index = false;
                        }
                        else
                        {
                            continue; // Skip the invalid data samples or data from last duty cycle scan
                        }
                    }

                    self->last_meas_index = self->data[i].meas_index;

                    // We got valid data, time to bsec_do_steps
                    uint8_t n_bsec_inputs = 0;
                    bsec_input_t inputs[BSEC_MAX_PHYSICAL_SENSOR];

                    // Read the data into bsec_input_t[]
                    if (sensor_settings.process_data)
                    {
                        /* Pressure to be processed by BSEC */
                        if (sensor_settings.process_data & BSEC_PROCESS_PRESSURE)
                        {
                            // printf("PRESSURE %f\n", self->data[i].pressure);
                            /* Place presssure sample into input struct */
                            inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_PRESSURE;
                            inputs[n_bsec_inputs].signal = self->data[i].pressure;
                            inputs[n_bsec_inputs].time_stamp = time_stamp;
                            n_bsec_inputs++;
                        }
                        /* Temperature to be processed by BSEC */
                        if (sensor_settings.process_data & BSEC_PROCESS_TEMPERATURE)
                        {
                            // printf("TEMPERATURE %f\n", self->data[i].temperature);
                            /* Place temperature sample into input struct */
                            inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_TEMPERATURE;
#ifdef BME69X_FLOAT_POINT_COMPENSATION
                            inputs[n_bsec_inputs].signal = self->data[i].temperature;
#else
                            inputs[n_bsec_inputs].signal = self->data[i].temperature / 100.0f;
#endif
                            inputs[n_bsec_inputs].time_stamp = time_stamp;
                            n_bsec_inputs++;

                            /* Also add optional heatsource input which will be subtracted from the temperature reading to 
                            * compensate for device-specific self-heating (supported in BSEC IAQ solution)*/
                            inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_HEATSOURCE;
                            inputs[n_bsec_inputs].signal = self->temp_offset;
                            inputs[n_bsec_inputs].time_stamp = time_stamp;
                            n_bsec_inputs++;
                        }
                        /* Humidity to be processed by BSEC */
                        if (sensor_settings.process_data & BSEC_PROCESS_HUMIDITY)
                        {
                            // printf("HUMIDITY %f\n",self->data[i].humidity);
                            /* Place humidity sample into input struct */
                            inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_HUMIDITY;
#ifdef BME69X_FLOAT_POINT_COMPENSATION
                            inputs[n_bsec_inputs].signal = self->data[i].humidity;
#else
                            inputs[n_bsec_inputs].signal = self->data[i].humidity / 1000.0f;
#endif
                            inputs[n_bsec_inputs].time_stamp = time_stamp;
                            n_bsec_inputs++;
                        }
                        /* Gas to be processed by BSEC */
                        if (sensor_settings.process_data & BSEC_PROCESS_GAS)
                        {
                            // printf("GAS_RESISTANCE %f\n", self->data[i].gas_resistance);
                            /* Check whether gas_valid flag is set */
                            if (self->data[i].status & BME69X_GASM_VALID_MSK)
                            {
                                /* Place sample into input struct */
                                inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_GASRESISTOR;
                                inputs[n_bsec_inputs].signal = self->data[i].gas_resistance;
                                inputs[n_bsec_inputs].time_stamp = time_stamp;
                                n_bsec_inputs++;
                            }
                        }
                        /* Profile part */
                        if (self->op_mode == BME69X_PARALLEL_MODE || self->op_mode == BME69X_SEQUENTIAL_MODE)
                        {
                            // printf("PROFILE_PART %d\n", self->data[i].gas_index);
                            inputs[n_bsec_inputs].sensor_id = BSEC_INPUT_PROFILE_PART;
                            inputs[n_bsec_inputs].signal = self->data[i].gas_index;
                            inputs[n_bsec_inputs].time_stamp = time_stamp;
                            n_bsec_inputs++;
                        }
                    }
                    bsec_output_t bsec_outputs[BSEC_NUMBER_OUTPUTS];
                    uint8_t n_output = BSEC_NUMBER_OUTPUTS;

                    self->rslt = bsec_do_steps(self->bsec_inst, inputs, n_bsec_inputs, bsec_outputs, &n_output);
                    if (self->rslt != BSEC_OK)
                    {
                        printf("BSEC DO STEPS ERROR %d\nAT PROFILE PART %d\n", self->rslt, self->data[i].gas_index);
                        PyErr_SetString(bmeError, "BSEC Failed to process data");
                        return NULL;
                    }
                    else
                    {
                        // Read processed data into python Dict pydata
                        /* Iterate through the outputs and extract the relevant ones. */
                        self->sample_count++;
                        DICT_SET_ITEM(bsec_data, "sample_nr", Py_BuildValue("i", self->sample_count));
                        DICT_SET_ITEM(bsec_data, "timestamp", Py_BuildValue("L", time_stamp));
                        for (uint8_t index = 0; index < n_output; index++)
                        {
                            switch (bsec_outputs[index].sensor_id)
                            {
                            case BSEC_OUTPUT_STABILIZATION_STATUS:
                                DICT_SET_ITEM(bsec_data, "stabilization_status", Py_BuildValue("i", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_RUN_IN_STATUS:
                                DICT_SET_ITEM(bsec_data, "run_in_status", Py_BuildValue("i", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_IAQ:
                                DICT_SET_ITEM(bsec_data, "iaq", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "iaq_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_STATIC_IAQ:
                                DICT_SET_ITEM(bsec_data, "static_iaq", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "static_iaq_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_CO2_EQUIVALENT:
                                DICT_SET_ITEM(bsec_data, "co2_equivalent", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "co2_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_BREATH_VOC_EQUIVALENT:
                                DICT_SET_ITEM(bsec_data, "breath_voc_equivalent", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "breath_voc_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_TVOC_EQUIVALENT:
                                DICT_SET_ITEM(bsec_data, "tvoc_equivalent", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "tvoc_equivalent_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_TEMPERATURE:
                                DICT_SET_ITEM(bsec_data, "temperature", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_RAW_PRESSURE:
                                DICT_SET_ITEM(bsec_data, "raw_pressure", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_SENSOR_HEAT_COMPENSATED_HUMIDITY:
                                DICT_SET_ITEM(bsec_data, "humidity", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_RAW_GAS:
                                DICT_SET_ITEM(bsec_data, "raw_gas", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_RAW_TEMPERATURE:
                                DICT_SET_ITEM(bsec_data, "raw_temperature", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_RAW_HUMIDITY:
                                DICT_SET_ITEM(bsec_data, "raw_humidity", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_GAS_PERCENTAGE:
                                DICT_SET_ITEM(bsec_data, "gas_percentage", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "gas_percentage_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_RAW_GAS_INDEX:
                                DICT_SET_ITEM(bsec_data, "raw_gas_index", Py_BuildValue("d", bsec_outputs[index].signal));
                                break;
                            case BSEC_OUTPUT_GAS_ESTIMATE_1:
                                DICT_SET_ITEM(bsec_data, "gas_estimate_1", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "gas_estimate_1_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_GAS_ESTIMATE_2:
                                DICT_SET_ITEM(bsec_data, "gas_estimate_2", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "gas_estimate_2_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_GAS_ESTIMATE_3:
                                DICT_SET_ITEM(bsec_data, "gas_estimate_3", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "gas_estimate_3_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            case BSEC_OUTPUT_GAS_ESTIMATE_4:
                                DICT_SET_ITEM(bsec_data, "gas_estimate_4", Py_BuildValue("d", bsec_outputs[index].signal));
                                DICT_SET_ITEM(bsec_data, "gas_estimate_4_accuracy", Py_BuildValue("i", bsec_outputs[index].accuracy));
                                break;
                            default:
                                continue;
                            }
                        }
                    }
                }
            }
            // self->bme.amb_temp = self->data[0].temperature - self->temp_offset;
            return bsec_data;
        }
    }
    Py_RETURN_NONE;
    return NULL;
}

static PyObject *bme_get_bsec_version(BMEObject *self)
{
    bsec_version_t version;
    bsec_get_version(self->bsec_inst, &version);
    char buffer[13]; /* 13 bytes matches the BSEC version blob register size; truncation is acceptable */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
    snprintf(buffer, 13, "%d.%d.%d.%d", version.major, version.minor, version.major_bugfix, version.minor_bugfix);
#pragma GCC diagnostic pop
    return Py_BuildValue("s", buffer);
}

static PyObject *bme_get_bsec_conf(BMEObject *self)
{
    uint8_t conf_set_id = 0;
    uint8_t serialized_settings[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_serialized_settings_max = BSEC_MAX_PROPERTY_BLOB_SIZE;
    uint8_t work_buffer[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_PROPERTY_BLOB_SIZE;
    uint32_t n_serialized_settings = 0;

    self->rslt = bsec_get_configuration(&(self->bme),conf_set_id, serialized_settings, n_serialized_settings_max, work_buffer, n_work_buffer, &n_serialized_settings);

    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to read BSEC conf");
        return NULL;
    }

    // Create and populate Python List Object
    PyObject *conf_list = PyList_New(BSEC_MAX_PROPERTY_BLOB_SIZE);
    for (uint32_t i = 0; i < BSEC_MAX_PROPERTY_BLOB_SIZE; i++)
    {
        PyList_SetItem(conf_list, i, Py_BuildValue("i", serialized_settings[i]));
    }

    return conf_list;
}

static PyObject *bme_set_bsec_conf(BMEObject *self, PyObject *args)
{
    PyObject *conf_list_obj;

    printf("[set_bsec_conf] Called\n");
    if (!PyArg_ParseTuple(args, "O", &conf_list_obj))
    {
        PyErr_SetString(bmeError, "Argument must be list of BSEC_MAX_PROPERTY_BLOB_SIZE integers");
        return (PyObject *)NULL;
    }

    if (!PyList_Check(conf_list_obj))
    {
        PyErr_SetString(bmeError, "Argument must be list of BSEC_MAX_PROPERTY_BLOB_SIZE integers");
        return (PyObject *)NULL;
    }

    uint32_t conf_size = PyList_Size(conf_list_obj);
    if (conf_size != BSEC_MAX_PROPERTY_BLOB_SIZE)
    {
        PyErr_SetString(bmeError, "Argument must be list of BSEC_MAX_PROPERTY_BLOB_SIZE integers");
        return (PyObject *)NULL;
    }

    uint8_t serialized_settings[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint8_t work_buffer[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_PROPERTY_BLOB_SIZE;

    PyObject *val;
    for (uint32_t i = 0; i < BSEC_MAX_PROPERTY_BLOB_SIZE; i++)
    {
        val = PyList_GetItem(conf_list_obj, i);
        if (!val)
        {
            PyErr_SetString(bmeError, "Failed to get config list item");
            return (PyObject *)NULL;
        }
        serialized_settings[i] = (uint8_t)PyLong_AsLong(val);
        if (PyErr_Occurred())
        {
            PyErr_SetString(bmeError, "Config list items must be integers");
            return (PyObject *)NULL;
        }
    }

    // Apply the configuration - pass actual config size, not max size
    self->rslt = bsec_set_configuration(self->bsec_inst, serialized_settings, conf_size, work_buffer, n_work_buffer);
    printf("[set_bsec_conf] bsec_set_configuration(conf_size=%u) returned %d\n", conf_size, (int)self->rslt);
    if (self->rslt != BSEC_OK)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Could not set BSEC config (bsec_rslt=%d)", (int)self->rslt);
        PyErr_SetString(bmeError, msg);
        return (PyObject *)NULL;
    }

    if (self->debug_mode == 1)
    {
        printf("SET BSEC CONF RLST %d\n", self->rslt);
    }

    return Py_BuildValue("i", self->rslt);
}

static PyObject *bme_get_bsec_state(BMEObject *self)
{
    // state set id = 0 to retrieve all states
    uint8_t state_set_id = 0;
    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_serialized_state_max = BSEC_MAX_STATE_BLOB_SIZE;
    uint32_t n_serialized_state = BSEC_MAX_STATE_BLOB_SIZE;
    uint8_t work_buffer_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_work_buffer_state = BSEC_MAX_STATE_BLOB_SIZE;

    // Get BSEC state and read it into serialized state
    self->rslt = bsec_get_state(&(self->bme), state_set_id, serialized_state, n_serialized_state_max, work_buffer_state, n_work_buffer_state, &n_serialized_state);

    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to read BSEC state");
        return (PyObject *)NULL;
    }

    // Create and populate Python List Object
    PyObject *state_list = PyList_New(BSEC_MAX_STATE_BLOB_SIZE);
    for (uint32_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
    {
        PyList_SetItem(state_list, i, Py_BuildValue("i", serialized_state[i]));
    }

    return state_list;
}

static PyObject *bme_set_bsec_state(BMEObject *self, PyObject *args)
{
    PyObject *state_list_obj;

    if (!PyArg_ParseTuple(args, "O", &state_list_obj))
    {
        PyErr_SetString(bmeError, "Argument must be list of BSEC_MAX_STATE_BLOB_SIZE (197) integers");
        return (PyObject *)NULL;
    }

    if (!PyList_Check(state_list_obj))
    {
        PyErr_SetString(bmeError, "Argument must be list of BSEC_MAX_STATE_BLOB_SIZE (197) integers");
        return (PyObject *)NULL;
    }

    uint32_t state_size = PyList_Size(state_list_obj);
    if (state_size > BSEC_MAX_STATE_BLOB_SIZE)
    {
        PyErr_SetString(bmeError, "Argument must be list of BSEC_MAX_STATE_BLOB_SIZE (197) integers");
        return (PyObject *)NULL;
    }

    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_serialized_state_max = BSEC_MAX_STATE_BLOB_SIZE;
    uint8_t work_buffer[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_STATE_BLOB_SIZE;

    PyObject *val;
    for (uint32_t i = 0; i < BSEC_MAX_STATE_BLOB_SIZE; i++)
    {
        val = PyList_GetItem(state_list_obj, i);
        serialized_state[i] = (uint8_t)PyLong_AsLong(val);
    }

    // Apply the configuration
    self->rslt = bsec_set_state(&(self->bme), serialized_state, n_serialized_state_max, work_buffer, n_work_buffer);

    if (self->debug_mode == 1)
    {
        printf("SET BSEC STATE RSLT %d\n", self->rslt);
    }
    return Py_BuildValue("i", self->rslt);
}

// Load BSEC config from an arbitrary file path
// Python: load_bsec_conf_from_file(path: str) -> None or raises bme69x.error
static PyObject *bme_load_bsec_conf_from_file(BMEObject *self, PyObject *args)
{
    const char *path = NULL;
    if (!PyArg_ParseTuple(args, "s", &path))
    {
        PyErr_SetString(bmeError, "Argument must be a file path (str)");
        return NULL;
    }

    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Config file not found: %s", path);
        PyErr_SetString(bmeError, msg);
        return NULL;
    }

    uint8_t serialized_settings_raw[BSEC_MAX_PROPERTY_BLOB_SIZE + 4];
    size_t n_read = fread(serialized_settings_raw, 1, BSEC_MAX_PROPERTY_BLOB_SIZE + 4, fp);
    fclose(fp);

    if (n_read == 0)
    {
        PyErr_SetString(bmeError, "Failed to read config file");
        return NULL;
    }

    uint8_t *serialized_settings = serialized_settings_raw;
    size_t conf_size = n_read;

    if (n_read > BSEC_MAX_PROPERTY_BLOB_SIZE)
    {
        uint32_t header_size = (uint32_t)serialized_settings_raw[0] |
                               ((uint32_t)serialized_settings_raw[1] << 8) |
                               ((uint32_t)serialized_settings_raw[2] << 16) |
                               ((uint32_t)serialized_settings_raw[3] << 24);
        if (header_size == BSEC_MAX_PROPERTY_BLOB_SIZE && n_read == BSEC_MAX_PROPERTY_BLOB_SIZE + 4)
        {
            serialized_settings = serialized_settings_raw + 4;
            conf_size = BSEC_MAX_PROPERTY_BLOB_SIZE;
        }
    }

    uint8_t work_buffer[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_PROPERTY_BLOB_SIZE;

    self->rslt = bsec_set_configuration(self->bsec_inst, serialized_settings, conf_size, work_buffer, n_work_buffer);
    if (self->rslt != BSEC_OK)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed to apply config from file (bsec_rslt=%d)", (int)self->rslt);
        PyErr_SetString(bmeError, msg);
        return NULL;
    }

    Py_RETURN_NONE;
}

/* Load BSEC config from sensor-specific file */
static PyObject *bme_load_bsec_conf(BMEObject *self)
{
    char conf_path[256];
    get_config_filename(self->sensor_id, conf_path, sizeof(conf_path));
    
    FILE *fp = fopen(conf_path, "rb");
    if (!fp)
    {
        if (self->debug_mode == 1)
            printf("Config file not found: %s (this is OK for first run)\n", conf_path);
        Py_RETURN_NONE;
    }
    
    uint8_t serialized_settings_raw[BSEC_MAX_PROPERTY_BLOB_SIZE + 4];
    size_t n_read = fread(serialized_settings_raw, 1, BSEC_MAX_PROPERTY_BLOB_SIZE + 4, fp);
    fclose(fp);
    
    if (n_read == 0)
    {
        PyErr_SetString(bmeError, "Failed to read config file");
        return NULL;
    }
    
    printf("[load_bsec_conf] Read %zu bytes from %s\n", n_read, conf_path);
    
    // Binary .config files from BSEC have a 4-byte header (size field in little-endian)
    // Strip the header if present (file size > 2005 bytes for IAQ_Sel variant)
    uint8_t *serialized_settings = serialized_settings_raw;
    size_t conf_size = n_read;
    
    if (n_read > BSEC_MAX_PROPERTY_BLOB_SIZE)
    {
        // Has 4-byte header - strip it
        uint32_t header_size = (uint32_t)serialized_settings_raw[0] |
                               ((uint32_t)serialized_settings_raw[1] << 8) |
                               ((uint32_t)serialized_settings_raw[2] << 16) |
                               ((uint32_t)serialized_settings_raw[3] << 24);
        
        if (header_size == BSEC_MAX_PROPERTY_BLOB_SIZE && n_read == BSEC_MAX_PROPERTY_BLOB_SIZE + 4)
        {
            printf("[load_bsec_conf] Detected 4-byte header (size=%u), stripping it\n", header_size);
            serialized_settings = serialized_settings_raw + 4;
            conf_size = BSEC_MAX_PROPERTY_BLOB_SIZE;
        }
    }
    
    uint8_t work_buffer[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_PROPERTY_BLOB_SIZE;
    
    self->rslt = bsec_set_configuration(self->bsec_inst, serialized_settings, conf_size, work_buffer, n_work_buffer);
    printf("[load_bsec_conf] bsec_set_configuration returned %d\n", (int)self->rslt);
    if (self->rslt != BSEC_OK)
    {
        char msg[256];
        snprintf(msg, sizeof(msg), "Failed to apply loaded config to BSEC (bsec_rslt=%d)", (int)self->rslt);
        PyErr_SetString(bmeError, msg);
        return NULL;
    }
    
    if (self->debug_mode == 1)
        printf("Loaded config from: %s\n", conf_path);
    
    Py_RETURN_NONE;
}

/* Save BSEC config to sensor-specific file */
static PyObject *bme_save_bsec_conf(BMEObject *self)
{
    char conf_path[256];
    get_config_filename(self->sensor_id, conf_path, sizeof(conf_path));
    
    uint8_t serialized_settings[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_serialized_settings_max = BSEC_MAX_PROPERTY_BLOB_SIZE;
    uint8_t work_buffer[BSEC_MAX_PROPERTY_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_PROPERTY_BLOB_SIZE;
    uint32_t n_serialized_settings = 0;
    
    self->rslt = bsec_get_configuration(self->bsec_inst, 0, serialized_settings, n_serialized_settings_max,
                                        work_buffer, n_work_buffer, &n_serialized_settings);
    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to get BSEC configuration");
        return NULL;
    }
    
    FILE *fp = fopen(conf_path, "wb");
    if (!fp)
    {
        PyErr_SetString(bmeError, "Failed to open config file for writing");
        return NULL;
    }
    
    size_t n_written = fwrite(serialized_settings, 1, n_serialized_settings, fp);
    fclose(fp);
    
    if (n_written != n_serialized_settings)
    {
        PyErr_SetString(bmeError, "Failed to write all config data");
        return NULL;
    }
    
    if (self->debug_mode == 1)
        printf("Saved config to: %s\n", conf_path);
    
    Py_RETURN_NONE;
}

/* Load BSEC state from sensor-specific file */
static PyObject *bme_load_bsec_state(BMEObject *self)
{
    char state_path[256];
    get_state_filename(self->sensor_id, state_path, sizeof(state_path));
    
    FILE *fp = fopen(state_path, "rb");
    if (!fp)
    {
        if (self->debug_mode == 1)
            printf("State file not found: %s (this is OK for first run)\n", state_path);
        Py_RETURN_NONE;
    }
    
    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
    size_t n_read = fread(serialized_state, 1, BSEC_MAX_STATE_BLOB_SIZE, fp);
    fclose(fp);
    
    if (n_read == 0)
    {
        PyErr_SetString(bmeError, "Failed to read state file");
        return NULL;
    }
    
    uint8_t work_buffer[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_work_buffer = BSEC_MAX_STATE_BLOB_SIZE;
    
    self->rslt = bsec_set_state(self->bsec_inst, serialized_state, n_read, work_buffer, n_work_buffer);
    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to apply loaded state to BSEC");
        return NULL;
    }
    
    if (self->debug_mode == 1)
        printf("Loaded state from: %s\n", state_path);
    
    Py_RETURN_NONE;
}

/* Save BSEC state to sensor-specific file */
static PyObject *bme_save_bsec_state(BMEObject *self)
{
    char state_path[256];
    get_state_filename(self->sensor_id, state_path, sizeof(state_path));
    
    uint8_t state_set_id = 0;
    uint8_t serialized_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_serialized_state_max = BSEC_MAX_STATE_BLOB_SIZE;
    uint32_t n_serialized_state = BSEC_MAX_STATE_BLOB_SIZE;
    uint8_t work_buffer_state[BSEC_MAX_STATE_BLOB_SIZE];
    uint32_t n_work_buffer_state = BSEC_MAX_STATE_BLOB_SIZE;
    
    self->rslt = bsec_get_state(self->bsec_inst, state_set_id, serialized_state, n_serialized_state_max,
                                work_buffer_state, n_work_buffer_state, &n_serialized_state);
    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to get BSEC state");
        return NULL;
    }
    
    FILE *fp = fopen(state_path, "wb");
    if (!fp)
    {
        PyErr_SetString(bmeError, "Failed to open state file for writing");
        return NULL;
    }
    
    size_t n_written = fwrite(serialized_state, 1, n_serialized_state, fp);
    fclose(fp);
    
    if (n_written != n_serialized_state)
    {
        PyErr_SetString(bmeError, "Failed to write all state data");
        return NULL;
    }
    
    if (self->debug_mode == 1)
        printf("Saved state to: %s\n", state_path);
    
    Py_RETURN_NONE;
}

static PyObject *bme_update_bsec_subscription(BMEObject *self, PyObject *args)
{
    // Check if argument is a list
    PyObject *list_obj;

    if (!PyArg_ParseTuple(args, "O", &list_obj))
    {
        PyErr_SetString(bmeError, "Failed to parse Argument");
        return (PyObject *)NULL;
    }

    if (!PyList_Check(list_obj))
    {
        PyErr_SetString(bmeError, "Argument must be a List");
        return (PyObject *)NULL;
    }
    // Allocate required memory
    uint8_t len = (uint8_t)PyList_Size(list_obj);

    uint8_t n_requested_virtual_sensors;
    n_requested_virtual_sensors = len;
    bsec_sensor_configuration_t requested_virtual_sensors[n_requested_virtual_sensors];

    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    // Loop over list and parse into requested_virtual_sensors
    PyObject *val;
    for (uint8_t i = 0; i < len; i++)
    {
        val = PyList_GetItem(list_obj, i);
        if (!PyArg_ParseTuple(val, "bf", &(requested_virtual_sensors[i].sensor_id), &(requested_virtual_sensors[i].sample_rate)))
        {
            PyErr_SetString(bmeError, "List items must be tuples (sensor_id, sample_rate)");
            return (PyObject *)NULL;
        }
    }

    // Call bsec_update_subscription and return rslt
    self->rslt = bsec_update_subscription(self->bsec_inst, requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);

    return Py_BuildValue("i", self->rslt);
}

static PyObject *bme_enable_gas_estimates(BMEObject *self)
{
    uint8_t n_requested_virtual_sensors;
    n_requested_virtual_sensors = 4;
    bsec_sensor_configuration_t requested_virtual_sensors[n_requested_virtual_sensors];
    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_3;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_SCAN ;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_4;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_SCAN ;

    self->rslt = bsec_update_subscription(self->bsec_inst, requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);
    printf("ENABLE GAS ESTIMATES RSLT %d\n", self->rslt);
    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to subscribe gas estimates");
        return (PyObject *)NULL;
    }
    return Py_BuildValue("i", self->rslt);
}

static PyObject *bme_disable_gas_estimates(BMEObject *self)
{
    uint8_t n_requested_virtual_sensors;
    n_requested_virtual_sensors = 4;
    bsec_sensor_configuration_t requested_virtual_sensors[n_requested_virtual_sensors];
    bsec_sensor_configuration_t required_sensor_settings[BSEC_MAX_PHYSICAL_SENSOR];
    uint8_t n_required_sensor_settings = BSEC_MAX_PHYSICAL_SENSOR;

    requested_virtual_sensors[0].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_1;
    requested_virtual_sensors[0].sample_rate = BSEC_SAMPLE_RATE_DISABLED;
    requested_virtual_sensors[1].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_2;
    requested_virtual_sensors[1].sample_rate = BSEC_SAMPLE_RATE_DISABLED;
    requested_virtual_sensors[2].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_3;
    requested_virtual_sensors[2].sample_rate = BSEC_SAMPLE_RATE_DISABLED;
    requested_virtual_sensors[3].sensor_id = BSEC_OUTPUT_GAS_ESTIMATE_4;
    requested_virtual_sensors[3].sample_rate = BSEC_SAMPLE_RATE_DISABLED;

    self->rslt = bsec_update_subscription(&(self->bme),requested_virtual_sensors, n_requested_virtual_sensors, required_sensor_settings, &n_required_sensor_settings);

    if (self->rslt != BSEC_OK)
    {
        PyErr_SetString(bmeError, "Failed to unsubscribe gas estimates");
        return (PyObject *)NULL;
    }
    return Py_BuildValue("i", self->rslt);
}
#endif

static PyMethodDef bme69x_methods[] = {
    {"init_bme69x", (PyCFunction)bme_init_bme69x, METH_NOARGS, "Initialize the BME69X sensor"},
    {"print_dur_prof", (PyCFunction)bme_print_dur_prof, METH_NOARGS, "Print the current duration profile"},
    {"enable_debug_mode", (PyCFunction)bme_enable_debug_mode, METH_NOARGS, "Enable debug mode"},
    {"disable_debug_mode", (PyCFunction)bme_disable_debug_mode, METH_NOARGS, "Disable debug mode"},
    {"get_sensor_id", (PyCFunction)bme_get_sensor_id, METH_NOARGS, "Get unique sensor id"},
    {"set_temp_offset", (PyCFunction)bme_set_temp_offset, METH_VARARGS, "Set temperature offset"},
    {"get_chip_id", (PyCFunction)bme_get_chip_id, METH_NOARGS, "Get the chip ID"},
    {"close_i2c", (PyCFunction)bme_close_i2c, METH_NOARGS, "Close the I2C bus"},
    {"open_i2c", (PyCFunction)bme_open_i2c, METH_VARARGS, "Open the I2C bus and connect to I2C address"},
    {"get_variant", (PyCFunction)bme_get_variant, METH_NOARGS, "Return string representing variant (BME690 or BME698)"},
    {"set_conf", (PyCFunction)bme_set_conf, METH_VARARGS, "Configure the BME69X sensor"},
    {"set_heatr_conf", (PyCFunction)bme_set_heatr_conf, METH_VARARGS, "Configure the BME69X heater"},
    {"get_data", (PyCFunction)bme_get_data, METH_NOARGS, "Measure and read data from the BME69X sensor w/o BSEC"},
#ifdef BSEC
    {"subscribe_gas_estimates", (PyCFunction)bme_subscribe_gas_estimates, METH_VARARGS, "Subscribe to provided number of gas estimates"},
    {"subscribe_ai_classes", (PyCFunction)bme_subscribe_ai_classes, METH_VARARGS, "Subscribe to all gas estimates"},
    {"set_sample_rate", (PyCFunction)bme_set_sample_rate, METH_VARARGS, "Set the sample rate for all virtual sensors"},
    {"get_bsec_version", (PyCFunction)bme_get_bsec_version, METH_NOARGS, "Return the BSEC version as string"},
    {"get_digital_nose_data", (PyCFunction)bme_get_digital_nose_data, METH_NOARGS, "Measure Gas Estimates"},
    {"get_bsec_data", (PyCFunction)bme_get_bsec_data, METH_NOARGS, "Measure and read data from the BME69x sensor with BSEC"},
    {"get_bsec_conf", (PyCFunction)bme_get_bsec_conf, METH_NOARGS, "Get BSEC config as config integer array"},
    {"set_bsec_conf", (PyCFunction)bme_set_bsec_conf, METH_VARARGS, "Set BSEC config from config integer array"},
    {"get_bsec_state", (PyCFunction)bme_get_bsec_state, METH_NOARGS, "Get BSEC state"},
    {"set_bsec_state", (PyCFunction)bme_set_bsec_state, METH_VARARGS, "Set BSEC state"},
    {"load_bsec_conf", (PyCFunction)bme_load_bsec_conf, METH_NOARGS, "Load BSEC config from sensor-specific file"},
    {"load_bsec_conf_from_file", (PyCFunction)bme_load_bsec_conf_from_file, METH_VARARGS, "Load BSEC config from a specific file path (auto-strips 4-byte header)"},
    {"save_bsec_conf", (PyCFunction)bme_save_bsec_conf, METH_NOARGS, "Save BSEC config to sensor-specific file"},
    {"load_bsec_state", (PyCFunction)bme_load_bsec_state, METH_NOARGS, "Load BSEC state from sensor-specific file"},
    {"save_bsec_state", (PyCFunction)bme_save_bsec_state, METH_NOARGS, "Save BSEC state to sensor-specific file"},
    {"update_bsec_subscription", (PyCFunction)bme_update_bsec_subscription, METH_VARARGS, "Update susbcribed BSEC outputs"},
    {"enable_gas_estimates", (PyCFunction)bme_enable_gas_estimates, METH_NOARGS, "Enable all 4 gas estimates"},
    {"disable_gas_estimates", (PyCFunction)bme_disable_gas_estimates, METH_NOARGS, "Disable all 4 gas estimates"},
#endif
    {NULL, NULL, 0, NULL} // Sentinel
};

static PyTypeObject BMEType = {
    PyVarObject_HEAD_INIT(NULL, 0)
        .tp_name = "bme69x.BME69X",
    .tp_doc = "BME69X sensor object",
    .tp_basicsize = sizeof(BMEObject),
    .tp_itemsize = 0,
    .tp_flags = Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,
    .tp_new = bme69x_new,
    .tp_init = (initproc)bme69x_init_type,
    .tp_dealloc = (destructor)bme69x_dealloc,
    .tp_members = bme69x_members,
    .tp_methods = bme69x_methods,
};

static PyModuleDef custommodule = {
    PyModuleDef_HEAD_INIT,
    .m_name = "bme69x",
    .m_doc = "Example module that creates an extension type.",
    .m_size = -1,
};

PyMODINIT_FUNC
PyInit_bme69x(void)
{
    PyObject *m;
    if (PyType_Ready(&BMEType) < 0)
        return NULL;

    m = PyModule_Create(&custommodule);
    if (m == NULL)
        return NULL;

    /* Initialize module-level exception */
    bmeError = PyErr_NewException("bme69x.error", NULL, NULL);
    if (bmeError == NULL)
    {
        Py_DECREF(m);
        return NULL;
    }
    Py_INCREF(bmeError);
    PyModule_AddObject(m, "error", bmeError);

    Py_INCREF(&BMEType);
    if (PyModule_AddObject(m, "BME69X", (PyObject *)&BMEType) < 0)
    {
        Py_DECREF(&BMEType);
        Py_DECREF(m);
        return NULL;
    }

    PyModule_AddIntConstant(m, "BME69X_I2C_ADDR_LOW", 0x76);
    PyModule_AddIntConstant(m, "BME69X_I2C_ADDR_HIGH", 0x77);
    PyModule_AddIntConstant(m, "BME69X_CHIP_ID", 0x61);
    PyModule_AddIntConstant(m, "BME69X_OK", 0);

    return m;
}
