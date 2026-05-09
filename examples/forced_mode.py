# This example demonstrates the FORCED MODE
# First with BSEC disabled
# Then with BSEC enabled

from bme69x import BME69X
import bme69xConstants as cst
import bsecConstants as bsec
from time import sleep
from pprint import pprint 
import sys, os


print('TESTING FORCED MODE WITHOUT BSEC')
bme = BME69X(0x76,1, debug_mode=0)
# Configure sensor to measure at 320 degC for 100 units (unit is 140ms - see API.md)
# According to the BME690 Data Sheet p18, Forced mode reads T,P,H,G sequentially for 1 heater duration and temperature.
print("Back from init.\n")
bme.set_heatr_conf(cst.BME69X_FORCED_MODE, 320, 100, cst.BME69X_ENABLE)
print('Heater config done, now getting data.\n')
print(bme.get_data())
sleep(3)
print('\nTESTING FORCED MODE WITH BSEC3')
# Create BME object and load any stored BSEC configuration/state first
bme = BME69X(0x76,1, debug_mode=1)
sleep(2)
# Now set the desired sample rate (after config is applied)
rslt = bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
print("set_sample_rate() returned:", rslt)


def get_data(sensor):
    data = {}
    try:
        data = sensor.get_bsec_data()
    except Exception as e:
        print(e)
        return None
    if data == None or data == {}:
        sleep(0.1)
        return None
    else:
        sleep(3)
        return data


bsec_data = get_data(bme)
while bsec_data == None:
    bsec_data = get_data(bme)
print(bsec_data)
