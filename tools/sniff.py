#!/usr/bin/python3
# Sniff air / coffe beans
# Keith McAlister updated Dec 2025 based on the PI3G Meat and Cheese code.

from bme69x import BME69X
import bme69xConstants as cst
import bsecConstants as bsec
from time import sleep
from pathlib import Path
import json

# The export from AI Studio is for BSEC3.3.0.0 and the ai models are located here: tools/690-tools.bmeproject/algorithms
# Binary .config files from BSEC have a 4-byte header (little-endian size field) that must be stripped.
# This function reads the file and removes the first 4 bytes via [4:] slicing.


# This is the config file output by AI_Studio
config_file = '690-tools.bmeproject/algorithms/alg-coffee-air/CoffeeAir_324_26.config'
config_path = str(Path(__file__).resolve().parent.joinpath(config_file))
def read_conf(path: str):
    with open(path, 'rb') as ai_conf:
        conf = [int.from_bytes(bytes([b]), 'little') for b in ai_conf.read()]
        conf = conf[4:]
    return conf

def main():
    # Open the I2C communications and set the operating mode
    bme = BME69X(cst.BME69X_I2C_ADDR_LOW,1,0)
    # report on the BME690 and BSEC version
    print(f'SENSOR: {bme.get_variant()} BSEC: {bme.get_bsec_version()}')
    sleep(1)
    # Now set sample rate
    bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
    sleep(1)
    # Load AI Studio config FIRST (before sample rate)
    # This is a new extension to load an absolute path.
    print(f'SET BSEC CONF {bme.load_bsec_conf_from_file(config_path)}')
    
    sleep(10)

    # Air and Coffee - two subscriptions (0,1)
    print(f'SUBSCRIBE GAS ESTIMATES {bme.subscribe_gas_estimates(2)}')

    sleep(10)

    # initialise the sensor
    print(f'INIT BME69X {bme.init_bme69x()}')

    print('\n\nSTARTING MEASUREMENT\n')

    while(True):
        # print(bme.get_bsec_data())
        try:
            data = bme.get_digital_nose_data()
        except Exception as e:
            print(e)
            main()
        if data:
            # for entry in bme.get_digital_nose_data():
            entry = data[-1]
            # print(f'{entry}')
            print(f'NORMAL AIR {entry["gas_estimate_1"]:.1%}\nCoffee {entry["gas_estimate_2"]:.1%}')
            print()

            NormalAir = "{:.1%}".format(entry["gas_estimate_1"])
            Coffee = "{:.1%}".format(entry["gas_estimate_2"])

            # This bit is from the Meat and Cheese PI3G demo - ist writes out the data to file as JASON
            d = {
                'NormalAir': NormalAir,
                'Coffee': Coffee,
            }

            with open('/tmp//sniff-data.json', 'w') as file:
                json.dump(d, file)



if __name__ == '__main__':
    main()
