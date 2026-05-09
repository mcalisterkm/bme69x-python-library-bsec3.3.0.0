# BME69X I2C addresses
BME69X_I2C_ADDR_LOW = 0x76
BME69X_I2C_ADDR_HIGH = 0x77

# BME69X chip identifier
BME69X_CHIP_ID = 0x61

# BME69X return code definitions
# SUCCESS
BME69X_OK = 0

# ERRORS
# Null pointer passed
BME69X_E_NULL_PTR = -1
# Communication failure
BME69X_E_COM_FAIL = -2
# Sensor not found
BME69X_E_DEV_NOT_FOUND = -3
# Incorrect length parameter
BME69X_E_INVALID_LENGTH = -4
# Self test fail error
BME69X_E_SELF_TEST = -5

# WARNINGS
# Define a valid operation mode
BME69X_W_DEFINE_OP_MODE = 1
# No new data was found
BME69X_W_NO_NEW_DATA = 2
# Define the shared heating duration
BME69X_W_DEFINE_SHD_HEATR_DUR = 3

# Chip ID register
BME69X_REG_CHIP_ID = 0xd0
# Variant ID register
BME69X_REG_VARIANT_ID = 0xF0

# Enable
BME69X_ENABLE = 0x01
# Disable
BME69X_DISABLE = 0x00

# Low gas variant - 680
BME69X_VARIANT_GAS_LOW = 0x00
# High gas variant - 688
BME69X_VARIANT_GAS_HIGH = 0x01
# New High gas variant - 690
BME69X_VARIANT_GAS_MAX =0x02

# OVERSAMPLING SETTING MACROS
# Switch off measurement
BME69X_OS_NONE = 0
# Perform 1 measurement
BME69X_OS_1X = 1
# Perform 2 measurements
BME69X_OS_2X = 2
# Perform 4 measurements
BME69X_OS_4X = 3
# Perform 8 measurements
BME69X_OS_8X = 4
# Perform 16 measurements
BME69X_OS_16X = 5

# IRR FILTER SETTINGS
# Switch off the filter
BME69X_FILTER_OFF = 0
# Filter coefficient of 2
BME69X_FILTER_SIZE_1 = 1
# Filter coefficient of 4
BME69X_FILTER_SIZE = 2
# Filter coefficient of 8
BME69X_FILTER_SIZE_7 = 3
# Filter coefficient of 16
BME69X_FILTER_SIZE_15 = 4
# Filter coefficient of 32
BME69X_FILTER_SIZE_31 = 5
# Filter coefficient of 64
BME69X_FILTER_SIZE_63 = 6
# Filter coefficient of 128
BME69X_FILTER_SIZE_127 = 7

# ODR / STANDBY TIME MACROS
# Standby time of 0.59ms
BME69X_ODR_0_59_MS = 0
# Standby time of 62.5ms
BME69X_ODR_62_5_MS = 1
# Standby time of 125ms
BME69X_ODR_125_MS = 2
# Standby time of 250ms
BME69X_ODR_250_MS = 3
# Standby time of 500ms
BME69X_ODR_500_MS = 4
# Standby time of 1s
BME69X_ODR_1000_MS = 5
# Standby time of 10ms
BME69X_ODR_10_MS = 6
# Standby time of 20ms
BME69X_ODR_20_MS = 7
# No standby time
BME69X_ODR_NONE = 8

# OPERATING MODE _MACROS
# Sleep mode
BME69X_SLEEP_MODE = 0
# Forced mode
BME69X_FORCED_MODE = 1
# Parallel mode
BME69X_PARALLEL_MODE = 2
# Sequential mode
BME69X_SEQUENTIAL_MODE = 3

# GAS MEASUREMENT MACROS
# Disable gas measurement
BME69X_DISABLE_GAS_MEAS = 0x00
# Enable gas measurement low
BME69X_ENABLE_GAS_MEAS_L = 0x01
# Enable gas measurement high
BME69X_ENABLE_GAS_MEAS_H = 0x02

# HEATER CONTROL MACROS
# Enable heater
BME69X_ENABLE_HEATER = 0x00
# Disable heater
BME69X_DISABLE_HEATER = 0x01

# MEASUREMENT VALUE RANGE MACROS
# Temperature min degC
BME69X_MIN_TEMPERATURE = 0
# Temperature max degC
BME69X_MAX_TEMPERATURE = 60
# Pressure min Pa
BME69X_MIN_PRESSURE = 90000
# Pressure max Pa
BME69X_MAX_PRESSURE = 110000
# Humidity min %rH
BME69X_HUMIDITY_MIN = 20
# Humidity max %rh
BME69X_HUMIDITY_MAX = 80
