#!/usr/bin/env python3
import argparse
import time
from bme69x import BME69X
import bsecConstants as bsec


def main():
    parser = argparse.ArgumentParser(description="Load BSEC config from an arbitrary path and read processed data.")
    parser.add_argument("--i2c-addr", type=lambda x: int(x, 0), default=0x76, help="I2C address (e.g., 0x76 or 0x77)")
    parser.add_argument("--sensor-name", type=str, default=None, help="Optional sensor name for state/config files")
    parser.add_argument("--config", type=str, required=True, help="Path to AI Studio .config file")
    parser.add_argument("--rate", type=str, default="LP", choices=["ULP","LP","CONT"], help="BSEC sample rate")
    args = parser.parse_args()

    rate_map = {
        "ULP": bsec.BSEC_SAMPLE_RATE_ULP,
        "LP": bsec.BSEC_SAMPLE_RATE_LP,
        "CONT": bsec.BSEC_SAMPLE_RATE_CONT,
    }

    sensor = BME69X(args.i2c_addr, 1, 0, args.sensor_name)
    sensor.load_bsec_conf_from_file(args.config)
    sensor.set_sample_rate(rate_map[args.rate])

    # Simple loop: poll for processed data, allow time for heater/sample cycles
    print("Reading BSEC data; press Ctrl+C to exit...")
    try:
        while True:
            data = sensor.get_bsec_data()
            if data:
                print(data)
                # Sleep to respect heater/profile cycle; adjust as needed
                time.sleep(2)
            else:
                time.sleep(1)
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
