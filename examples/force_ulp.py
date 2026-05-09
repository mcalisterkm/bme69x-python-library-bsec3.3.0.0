# This example demonstrates the FORCED MODE

from bme69x import BME69X
import bme69xConstants as cst
import bsecConstants as bsec
from time import sleep

print('\nTESTING FORCED MODE WITH BSEC')
bme = BME69X(cst.BME69X_I2C_ADDR_HIGH, 1, debug_mode=0)
# Temp and Dur single values 320C for 100 units (unit is 140ms - see API.md)
bme.set_heatr_conf(cst.BME69X_ENABLE, 320, 100, cst.BME69X_FORCED_MODE)
bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_ULP)


def get_data(sensor):
    data = {}
    try:
        data = bme.get_bsec_data()
    except Exception as e:
        print(e)
        sleep(1)
        return None
    if data == None or data == {}:
        sleep(300)
        return None
    else:
        return data


while True:
        bsec_data = get_data(bme)
        while bsec_data == None:
                print("Looping zero data")
                bsec_data = get_data(bme)
        print(bsec_data)
