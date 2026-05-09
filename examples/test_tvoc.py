#!/usr/bin/env python3
"""
Test script to monitor TVOC output over time
TVOC requires LP mode and may take several minutes to stabilize
"""

from bme69x import BME69X
import bsecConstants as bsec
from time import sleep
from datetime import datetime

print('Testing TVOC in LP Mode')
print('=' * 60)

# Initialize sensor with debug mode
bme = BME69X(0x76, 1, debug_mode=0)
sleep(1)

# Set LP sample rate (required for TVOC)
rslt = bme.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
print(f"set_sample_rate(LP) returned: {rslt}")
print(f"LP sample rate value: {bsec.BSEC_SAMPLE_RATE_LP}")
print()

def get_data(sensor):
    try:
        data = sensor.get_bsec_data()
        return data if data else None
    except Exception as e:
        print(f"Error: {e}")
        return None

print("Monitoring sensor data (TVOC may take several minutes to appear)...")
print("Press Ctrl+C to stop\n")

sample_count = 0
tvoc_seen = False

try:
    while True:
        data = get_data(bme)
        if data:
            sample_count += 1
            timestamp = datetime.now().strftime("%H:%M:%S")
            
            # Check if TVOC is present
            has_tvoc = 'tvoc_equivalent' in data
            
            if has_tvoc and not tvoc_seen:
                print("\n" + "!" * 60)
                print("TVOC DATA APPEARED!")
                print("!" * 60 + "\n")
                tvoc_seen = True
            
            # Print key values
            print(f"[{timestamp}] Sample #{sample_count}")
            print(f"  IAQ: {data.get('iaq', 'N/A'):.1f} (acc: {data.get('iaq_accuracy', 'N/A')})")
            print(f"  Temp: {data.get('temperature', 'N/A'):.1f}°C, Humidity: {data.get('humidity', 'N/A'):.1f}%")
            print(f"  CO2: {data.get('co2_equivalent', 'N/A'):.1f} ppm (acc: {data.get('co2_accuracy', 'N/A')})")
            print(f"  bVOC: {data.get('breath_voc_equivalent', 'N/A'):.2f} ppm (acc: {data.get('breath_voc_accuracy', 'N/A')})")
            
            if has_tvoc:
                print(f"  ★ TVOC: {data.get('tvoc_equivalent', 'N/A'):.2f} ppb (acc: {data.get('tvoc_equivalent_accuracy', 'N/A')}) ★")
            else:
                print(f"  TVOC: Not yet available")
            
            print()
        
        sleep(3)  # LP mode samples every ~3 seconds
        
except KeyboardInterrupt:
    print("\n\nTest stopped by user")
    print(f"Total samples: {sample_count}")
    print(f"TVOC seen: {'Yes' if tvoc_seen else 'No'}")
