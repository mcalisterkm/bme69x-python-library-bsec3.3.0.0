#!/usr/bin/env python3
"""Test set_bsec_conf with the C array from bsec_selectivity.c"""
import sys
sys.path.insert(0, '.')

from bme69x import BME69X

# Extract the bsec_config_selectivity array from the .c file
config_c_file = 'bsec_v3-2-1-0/examples/BSEC_Integration_Examples/src/config/bsec_selectivity.c'
print(f"Reading config from {config_c_file}...")

import re
with open(config_c_file, 'r') as f:
    content = f.read()

# Find the array definition: const uint8_t bsec_config_selectivity[2005] = { ... };
match = re.search(r'const uint8_t bsec_config_selectivity\[(\d+)\]\s*=\s*\{(.*?)\};', content, re.DOTALL)
if not match:
    print("ERROR: Could not find bsec_config_selectivity array in .c file")
    sys.exit(1)

size = int(match.group(1))
array_str = match.group(2)

# Parse the array: "0,1,2,3,189,1,0,0,..."
try:
    config_list = [int(x.strip()) for x in array_str.split(',') if x.strip()]
    print(f"✓ Parsed config array: {size} bytes, got {len(config_list)} values")
    if len(config_list) != size:
        print(f"⚠ WARNING: Expected {size} values but got {len(config_list)}")
except ValueError as e:
    print(f"ERROR: Failed to parse config array: {e}")
    sys.exit(1)

# Test set_bsec_conf with the array
print("\n--- Testing set_bsec_conf ---")
try:
    b = BME69X(0x76, 1, debug_mode=1)
    print("BME69X created")
    
    result = b.set_bsec_conf(config_list)
    print(f"set_bsec_conf returned: {result}")
    
    if result == 0:
        print("✓ Config set successfully!")
    else:
        print(f"⚠ Non-zero return code: {result}")
except Exception as e:
    import traceback
    print(f"✗ Exception: {e}")
    traceback.print_exc()
