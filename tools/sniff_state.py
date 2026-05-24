#!/usr/bin/python3
# Sniff air / coffe beans
# Keith McAlister updated May2026 based on the PI3G Meat and Cheese code.

from bme69x import BME69X
import bme69xConstants as cst
import bsecConstants as bsec
from time import sleep, monotonic, time
from pathlib import Path
import json
from collections import deque

# The export from AI Studio is for BSEC3.3.0.0 and the ai models are located here: tools/690-tools.bmeproject/algorithms
# Binary .config files from BSEC have a 4-byte header (little-endian size field) that must be stripped.
# This function reads the file and removes the first 4 bytes via [4:] slicing.


# This is the config file output by AI_Studio
config_file = 'may26-bsec3-example.bmeproject/algorithms/AIR-COFFEE_354_10.config'
config_path = str(Path(__file__).resolve().parent.joinpath(config_file))

SENSOR_NAME = 'sniff690'
STATE_DIR = Path('conf')
STATE_FILE = STATE_DIR / f'state_data_{SENSOR_NAME}.txt'
STARTUP_CALIBRATION_SECONDS = 30 * 60
STATE_SAVE_INTERVAL_SECONDS = 60 * 60


def read_conf(path: str):
    with open(path, 'rb') as ai_conf:
        conf = [int.from_bytes(bytes([b]), 'little') for b in ai_conf.read()]
        conf = conf[4:]
    return conf


def classify(air_score, coffee_score, air_acc, coffee_acc):
    # Confidence gate for single-sensor mode to avoid latching one class.
    min_top_score = 0.70
    min_margin = 0.20
    min_acc = 2

    if air_acc < min_acc or coffee_acc < min_acc:
        return 'UNKNOWN', 0.0

    top = max(air_score, coffee_score)
    margin = abs(air_score - coffee_score)

    if top < min_top_score or margin < min_margin:
        return 'UNKNOWN', top

    if coffee_score > air_score:
        return 'COFFEE', coffee_score
    return 'AIR', air_score


def show_key_fields(entry):
    """Print all available sensor fields for debugging."""
    fields_to_show = [
        'sample_nr', 'timestamp',
        'temperature', 'humidity', 'raw_pressure', 'raw_gas', 'raw_gas_index',
        'iaq', 'iaq_accuracy', 'static_iaq', 'static_iaq_accuracy',
        'co2_equivalent', 'co2_accuracy',
        'tvoc_equivalent', 'tvoc_equivalent_accuracy',
        'gas_estimate_1', 'gas_estimate_1_accuracy',
        'gas_estimate_2', 'gas_estimate_2_accuracy',
        'gas_percentage', 'gas_percentage_accuracy',
    ]
    for field in fields_to_show:
        if field in entry:
            val = entry[field]
            if isinstance(val, float):
                print(f'{field:30s}: {val:12.6f}')
            else:
                print(f'{field:30s}: {val}')


def wait_for_first_sample(bme, timeout_s=6.0):
    # Avoid long fixed startup sleeps; proceed as soon as data is ready.
    waited = 0.0
    step = 0.2
    while waited < timeout_s:
        try:
            data = bme.get_digital_nose_data()
            if data:
                return True
        except Exception:
            pass
        sleep(step)
        waited += step
    return False


def main():
    # Open the I2C communications and set the operating mode
    bme = BME69X(cst.BME69X_I2C_ADDR_HIGH, 1, 0, SENSOR_NAME)
    # report on the BME690 and BSEC version
    print(f'SENSOR: {bme.get_variant()} BSEC: {bme.get_bsec_version()}')

    STATE_DIR.mkdir(parents=True, exist_ok=True)

    sleep(1)
    # Load AI Studio config FIRST (before sample rate)
    # This is a new extension to load an absolute path.
    print(f'SET BSEC CONF {bme.load_bsec_conf_from_file(config_path)}')

    if STATE_FILE.exists():
        try:
            print(f'LOAD BSEC STATE {bme.load_bsec_state()} FROM {STATE_FILE}')
        except Exception as e:
            print(f'FAILED TO LOAD STATE FROM {STATE_FILE}: {e}')
    else:
        print(f'NO SAVED STATE FOUND AT {STATE_FILE}; STARTING COLD')

    sleep(1)

    # Set sample rate after loading the model config.
    # bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
    bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_SCAN)
    wait_for_first_sample(bme, timeout_s=3.0)

    # Air and Coffee - two subscriptions (0,1)
    print(f'SUBSCRIBE GAS ESTIMATES {bme.subscribe_gas_estimates(2)}')
    wait_for_first_sample(bme, timeout_s=3.0)

    # initialise the sensor
    print(f'INIT BME69X {bme.init_bme69x()}')

    print('\n\nSTARTING MEASUREMENT\n')

    # A short rolling average smooths jitter in single-sensor operation.
    est1_window = deque(maxlen=5)
    est2_window = deque(maxlen=5)
    warmup_samples = 3
    
    # Fixed class mapping from AI Studio model (AIR-COFFEE_354_10.config):
    # estimate_1 = AIR, estimate_2 = COFFEE
    class_mapping = {'est1': 'AIR', 'est2': 'COFFEE'}

    start_time = monotonic()
    next_state_save = start_time + STARTUP_CALIBRATION_SECONDS

    while(True):
        try:
            data = bme.get_digital_nose_data()
        except Exception as e:
            print(e)
            sleep(0.5)
            continue

        if data:
            entry = data[-1]
            # show_key_fields(entry)  # Debug output
            print(f'{entry}')

            est1_raw = float(entry['gas_estimate_1'])
            est2_raw = float(entry['gas_estimate_2'])
            est1_acc = int(entry.get('gas_estimate_1_accuracy', 0))
            est2_acc = int(entry.get('gas_estimate_2_accuracy', 0))
            sample_nr = int(entry.get('sample_nr', 0))

            est1_window.append(est1_raw)
            est2_window.append(est2_raw)

            est1_smoothed = sum(est1_window) / len(est1_window)
            est2_smoothed = sum(est2_window) / len(est2_window)

            # Apply fixed class mapping
            air_smoothed = est1_smoothed    # estimate_1 = AIR
            coffee_smoothed = est2_smoothed # estimate_2 = COFFEE
            air_acc = est1_acc
            coffee_acc = est2_acc

            label, confidence = classify(air_smoothed, coffee_smoothed, air_acc, coffee_acc)

            if sample_nr <= warmup_samples:
                label = 'WARMUP'

            print(f'NORMAL AIR {air_smoothed:.1%}\nCoffee {coffee_smoothed:.1%}')
            print(f'PREDICTION {label} ({confidence:.1%})')
            
            # Uncomment next line to debug all sensor fields
            # show_key_fields(entry)
            
            print()

            NormalAir = "{:.1%}".format(air_smoothed)
            Coffee = "{:.1%}".format(coffee_smoothed)

            # Write scores and class output for downstream consumers.
            d = {
                'NormalAir': NormalAir,
                'Coffee': Coffee,
                'Prediction': label,
                'Confidence': "{:.1%}".format(confidence),
                'ClassMap': 'estimate_1->AIR, estimate_2->COFFEE',
                'AirAccuracy': air_acc,
                'CoffeeAccuracy': coffee_acc,
                'SampleNr': int(entry.get('sample_nr', 0)),
            }

            target = Path('/tmp/sniff-data.json')
            try:
                tmp_target = target.with_suffix('.json.tmp')
                with open(tmp_target, 'w') as file:
                    json.dump(d, file)
                tmp_target.replace(target)
            except Exception:
                with open(target, 'w') as file:
                    json.dump(d, file)

            now = monotonic()
            if now >= next_state_save:
                try:
                    print(f'SAVE BSEC STATE {bme.save_bsec_state()} AT {int(time())}')
                    next_state_save = now + STATE_SAVE_INTERVAL_SECONDS
                except Exception as e:
                    print(f'FAILED TO SAVE BSEC STATE: {e}')
                    # Retry later instead of looping on every sample when save fails.
                    next_state_save = now + 60



if __name__ == '__main__':
    main()
