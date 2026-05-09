# This example demonstrates burn in of a BME690 with BSEC enabled
# It runs for 24 hr and waits for quality to reach 3
# Then it writes config and status to file.
# Status is useful when a sensor is restarted - faster to achieve quality data.
#
# Run me like this and I will run for around 24 hrs as a background task then finish
# $ nohup python3 burn_in.py &
# It is essential to provide bad and clean air during the 24 hours run-in.
# One successful approach is to put hand sanitiser (~60% ethyl alcohol) on a paper towel by the sensor for at least 30 min.
#

from bme69x import BME69X
import bme69xConstants as cnst
import bsecConstants as bsec
from time import sleep, time
from pathlib import Path
import os, errno

# Set up patameters.
temp_prof = [320, 100, 100, 100, 200, 200, 200, 320, 320, 320]
dur_prof =[5, 2, 10, 30, 5, 5, 5, 5, 5, 5]

# Check for conf directory and create if not present
try: os.makedirs("conf", mode=0o755, exist_ok = True)
except OSError as err:
# Reraise the error unless it's about an already existing directory
    if err.errno != errno.EEXIST or not os.path.isdir(newdir):
       raise

# Initialise the bme690 sensor
#  BME69X_I2C_ADDR_LOW is the pimoroni BME690 I2C default address
#  BME69X_I2C_ADDR_HIGH is the PI3G BME690 I2C default address
#  BUS -1 is default I2C bus on Raspberry Pi
bme = BME69X(cnst.BME69X_I2C_ADDR_LOW,1, debug_mode=0)
print(bme.set_heatr_conf(cnst.BME69X_ENABLE, temp_prof, dur_prof, cnst.BME69X_PARALLEL_MODE))
sleep(0.1)
# This sets the rate for all virtual sensors
print(bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP))
# time.time() returns seconds since epoch
start_time = time()

# Function to get data from the sensor
def get_data(bme):
    data = {}
    try:
        # Read BSEC data
        data = bme.get_bsec_data()
    except Exception as e:
        print(e)
        return None
    if data == None or data == {}:
        sleep(1)
        return None
    else:
        sleep(3)
        return data

# Main loop
while True:
    # Read data  If null returned, read again, loop until data is returned
    bsec_data = get_data(bme)
    while bsec_data == None:
        bsec_data = get_data(bme)
    #
    print(bsec_data)
    sleep(1)
    # Run the sensor for 24 hours and check the IAQ qulity is 3 (Best Quality)
    # 24 hours in seconds is 86400
    # If the test succeeds write out the state and config data to files in the conf subdirectory.
    # The config files are written as print(<state>, <file_handle>) - readable strings.
    # The files are closed.
    # The break command ends the loop and the script terminates
    if  ((time() - start_time > 86400) and (bsec_data['iaq_accuracy'] == 3)):
        # config and state are written to conf subdirectory
        print(f'Saving BSEC config... {bme.save_bsec_conf()}')
        print(f'Saving BSEC state... {bme.save_bsec_state()}')
        break
    else:
        # If we get here the test for accuracy or time failed so loop again.
        continue
