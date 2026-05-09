Examples
========

All the examplea code assumes that your BME68X sensor  is I2C address 0x77 (119).<br>
To use I2C address 0x76 (118) change the BME68X constructor call to BME68X_I2C_ADDR_LOW.

The examples are from the BSEC2 BSEC Python Wrapper adapted to meet the new signature of BME69X() initialisation. In addition there are new examples to test new features.

 Multi_sensor_test.py shows the new capabilities to suppport two sensor modules. 

## Air Quality
airquality.py is intended to light up coloured LED's depending on IAQ level.
It does work without LED's printing the IAQ value. 

## Burn-In
The burn_in.py script is designed to run for 24 hours to burn in a sensor, then save state and config.
Thre config and/or state can then be loaded  as required. The files are saved in a local ./conf directory which is created if not present.

## Forced_mode & Force_ulp
forced_mode.py runs the hot plate at 320C for 100 units of time. All 13 virtual sensors data is printed out. Force mode has little sleep time so the sensor will heat up and distort environmental readings. 

ULP has a 300 sec cycle for ulra low power consumption.

## Multi_Sensor
 Multi_sensor_test.py shows the new capabilities to suppport two sensor modules on separate I2C addresses. 

## Load Config from path
ad_config_from_path.py shows how to support command line parameters for the main variables such as I2C address,I2C Bus,  Mode, and path to .config

## Parallel_model & Parallel_mode_ulp
parallel_mode.py uses the default temperature and duration profile used in AI Studio. The parallel part is the iphysical environment sensors (temp, Humidity, pressure) are read in parallel with the heater plate.  Serial mode runs the environmentatl sensors first before starting up the heater plate. 


