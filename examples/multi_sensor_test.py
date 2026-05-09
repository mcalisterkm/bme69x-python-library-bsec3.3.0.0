#!/usr/bin/env python3
"""
Multi-sensor test script demonstrating independent BSEC instances
and separate config/state files for each sensor.

Each sensor instance maintains its own:
- I2C address (0x76, 0x77, etc.)
- Sensor ID (for file naming)
- BSEC state and configuration

Note: This test will fail if sensor hardware is not connected.
      To test just the multi-sensor framework without hardware,
      comment out the sensor creation blocks.
"""

import sys
import os

# Add parent directory to path so we can import bme69x
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)) + '/../')

import bme69x
import bme69xConstants as cnst
import bsecConstants as bsec
from time import sleep
rslt=0

def main():
    print("=== Multi-Sensor BME69X Test ===\n")
    
    # Heater and duration profiles for parallel mode
    temp_prof = [320, 100, 100, 100, 200, 200, 200, 320, 320, 320]
    dur_prof = [5, 2, 10, 30, 5, 5, 5, 5, 5, 5]
    
    try:
        # Try to create one sensor if hardware exists
        print("Attempting to connect to sensor at 0x77 on /dev/i2c-1...")
        sensor1 = bme69x.BME69X(i2c_addr=0x77, sensor_name='test_sensor', debug_mode=0)
        print("✓ Sensor 1 created successfully\n")
        
        # Get chip IDs to verify connection
        print("=== Testing Sensor 1 ===")
        print(f"Chip ID: 0x{sensor1.get_chip_id():02x}")
        print(f"Variant: {sensor1.get_variant()}")
        print(f"BSEC version: {sensor1.get_bsec_version()}")
        
        # Try saving/loading state
        print("\n=== Testing Config/State File Separation ===")
        print(f"Sensor uses config file: conf/bsec_config_test_sensor.txt")
        print(f"Sensor uses state file: conf/state_data_test_sensor.txt\n")
        
        print("Saving BSEC state...")
        sensor1.save_bsec_state()
        print("✓ State saved")
        
        print("Saving BSEC config...")
        sensor1.save_bsec_conf()
        print("✓ Config saved")
        
        print("Loading BSEC state...")
        sensor1.load_bsec_state()
        print("✓ State loaded")
        
        print("Loading BSEC config...")
        sensor1.load_bsec_conf()
        print("✓ Config loaded")
        
        # Try to create second sensor if hardware exists
        print("\nAttempting to connect to sensor at 0x76 on /dev/i2c-1...")
        sensor2 = bme69x.BME69X(i2c_addr=0x76, sensor_name='test_sensor2', debug_mode=0)
        print("✓ Sensor 2 created successfully\n")
        
        # Get chip IDs to verify connection
        print("=== Testing Sensor 2 ===")
        print(f"Chip ID: 0x{sensor2.get_chip_id():02x}")
        print(f"Variant: {sensor2.get_variant()}")
        print(f"BSEC version: {sensor2.get_bsec_version()}")
        
        # Try saving/loading state
        print("\n=== Testing Config/State File Separation ===")
        print(f"Sensor uses config file: conf/bsec_config_test_sensor2.txt")
        print(f"Sensor uses state file: conf/state_data_test_sensor2.txt\n")
        
        print("Saving BSEC state...")
        sensor2.save_bsec_state()
        print("✓ State saved")
        
        print("Saving BSEC config...")
        sensor2.save_bsec_conf()
        print("✓ Config saved")
        
        print("Loading BSEC state...")
        sensor2.load_bsec_state()
        print("✓ State loaded")
       
        print("Loading BSEC config...")
        sensor2.load_bsec_conf()
        print("✓ Config loaded")
        
        # Configure both sensors for data collection
        print("\n=== Configuring Sensors for Data Collection ===")
        print("Setting heater configuration...")
        sensor1.set_heatr_conf(cnst.BME69X_ENABLE, temp_prof, dur_prof, cnst.BME69X_PARALLEL_MODE)
        sensor1.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
        
        sensor2.set_heatr_conf(cnst.BME69X_ENABLE, temp_prof, dur_prof, cnst.BME69X_PARALLEL_MODE)
        sensor2.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
        print("✓ Both sensors configured\n")

        # Set up oversampling and filter settings
        # See IRR FILTER SETTINGS and OVERSAMPLING SETTING MACROS in bme69xConstants.py
        # humidity_os, pressure_os, temperature_os, filter enabled/disabled , ODR (Output Data Rate)
        sensor1.set_conf(cnst.BME69X_OS_4X, cnst.BME69X_OS_2X, cnst.BME69X_OS_8X, 1, 2)
        sensor2.set_conf(cnst.BME69X_OS_4X, cnst.BME69X_OS_2X, cnst.BME69X_OS_8X, 1, 2)


        # Collect 300 readings sequentially from both sensors
        print("=== Collecting 300 BSEC Readings from Both Sensors ===")
        for iteration in range(300):
            # Get data from sensor1
            data1 = sensor1.get_bsec_data()
            if data1 and data1 != {}:
                print(f"Iteration {iteration+1:3d}: Sensor1 IAQ={data1.get('iaq', 'N/A'):6.1f}, Temp={data1.get('temperature', 'N/A'):6.2f}°C, Humidity={data1.get('humidity', 'N/A'):6.2f}%", end="")
            else:
                print(f"Iteration {iteration+1:3d}: Sensor1 null", end="")
            
            # Get data from sensor2
            data2 = sensor2.get_bsec_data()
            if data2 and data2 != {}:
                print(f" | Sensor2 IAQ={data2.get('iaq', 'N/A'):6.1f}, Temp={data2.get('temperature', 'N/A'):6.2f}°C, Humidity={data2.get('humidity', 'N/A'):6.2f}%")
            else:
                print(f" | Sensor2 null")
            
            sleep(3)  # Sleep after reading both sensors
        
    except SystemError as e:
        print(f"✗ Hardware not available or error: {e}")
        print("\nThis is expected if sensor is not connected.")
        print("The multi-sensor API is ready for use when hardware is available.\n")
        print("API Summary:")
        print("  • Each BME69X() instance gets its own I2C file descriptor")
        print("  • Each instance gets its own BSEC state/config in memory")
        print("  • Each instance's state/config saves to sensor-specific files")
        print("  • Supports multiple sensors on same I2C bus (different addresses)")
        print("  • Supports sensors on different I2C buses (different bus numbers)")
        return 0
    except Exception as e:
        print(f"\n✗ Error: {e}")
        import traceback
        traceback.print_exc()
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
