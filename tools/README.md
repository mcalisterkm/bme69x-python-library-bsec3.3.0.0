Tools
=====

# may26-bsec3-example.bmeproject
This is a project folder from AI_Studio Desktop 3.2. It is a complete project, which can be opened in BME AI Studio and you can make changes and re-train. 

There are two data sets for Indoor Air, and Coffee, with a single exported model(algorithms) generated for Air-Coffee. Data was recorded using an 8x BME690 Sensor with  the BME690 developemnt kit and AI Studio Mobile. 
 
## Sniff.py
This is a nexample using the Air-Coffee exported AI Model with a single sensor BME module used to sniff (SEL mode).  The output is the model predictions for each of the classes in the model. The sensor is assumed to be 0x77 (HIGH I2C address)

## Sniff-lo.py 
This is a copy of sniff.py using the LO I2C address (0x76)
I have 2 pimoroni BME690 modules for testing, and they can be set up for 0x76 or 0x77 I2c adress. 

## Sniff_state.py 
On startup the BME690 goes through calibration, and can take 15 to 20 min before it  settles down to give good readings and becomes responsive to change. Saving state after calibration is completed, and loading that on start-up is expected to speed up the calibration. 

This modification to sniff.py checks on startup for a saved state file and loads it if found. After 30 min it saves state, and then updates the state file every hour.  My tests have shown that loading state on a cold start reduces the time to useful data to under 10 min.

See also my repository mcalisterkm/teach-your-pi-to-sniff-with-bme690, for a tutorial on data collection, conversion, preparation, and  model generation.