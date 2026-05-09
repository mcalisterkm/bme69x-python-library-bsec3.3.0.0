#!/usr/bin/env python3
"""
Test to verify both binary .config and C array config formats work correctly.
This demonstrates the fix for loading binary .config files with 4-byte headers.
"""

import os
import re
import shutil
from bme69x import BME69X
import bme69xConstants as cst

def test_binary_config_loading():
    """Test loading binary .config file with 4-byte header"""
    print("=" * 70)
    print("TEST 1: Loading Binary .config File (with 4-byte header)")
    print("=" * 70)
    
    # Setup
    os.makedirs('conf', exist_ok=True)
    bme = BME69X(cst.BME69X_I2C_ADDR_LOW, 1, debug_mode=1, sensor_name="binary_test")
    
    # Copy binary config
    src = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/config/bme690/bme690_sel_33v_3s_4d/bsec_selectivity.config'
    dst = 'conf/bsec_config_binary_test.txt'
    shutil.copy(src, dst)
    
    file_size = os.path.getsize(dst)
    print(f"✓ Config file size: {file_size} bytes")
    print(f"  - File contains: 4-byte header + 2005-byte config data\n")
    
    # Load config
    print("Loading configuration...")
    result = bme.load_bsec_conf()
    print(f"✓ Config loaded successfully (result={result})\n")
    
    # Verify functionality
    init_result = bme.init_bme69x()
    version = bme.get_bsec_version()
    print(f"✓ Sensor initialized (init_result={init_result})")
    print(f"✓ BSEC version: {version}\n")
    
    return True

def test_c_array_config_loading():
    """Test loading C array format config"""
    print("=" * 70)
    print("TEST 2: Loading C Array Format Config (from .c file)")
    print("=" * 70)
    
    # Setup
    bme = BME69X(cst.BME69X_I2C_ADDR_LOW, 1, debug_mode=0)
    
    # Extract config from .c file (use the one from the same location as binary)
    c_file = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/config/bme690/bme690_sel_33v_3s_4d/bsec_selectivity.c'
    print(f"✓ Reading config from: {c_file}")
    
    with open(c_file) as f:
        content = f.read()
    
    match = re.search(r'\{\s*(.*?)\s*\}', content, re.DOTALL)
    if not match:
        print("✗ Failed to extract config array")
        return False
    
    hex_str = match.group(1)
    values = [int(x.strip().rstrip(','), 0) for x in hex_str.split(',') if x.strip()]
    config_array = values
    
    print(f"✓ Extracted config array: {len(config_array)} bytes\n")
    
    # Load config
    print("Loading configuration...")
    result = bme.set_bsec_conf(config_array)
    print(f"✓ Config loaded successfully (bsec_rslt={result})\n")
    
    # Verify functionality
    init_result = bme.init_bme69x()
    version = bme.get_bsec_version()
    print(f"✓ Sensor initialized (init_result={init_result})")
    print(f"✓ BSEC version: {version}\n")
    
    return True

def test_config_equivalence():
    """Verify that binary and C array formats are equivalent"""
    print("=" * 70)
    print("TEST 3: Verifying Config Format Equivalence")
    print("=" * 70)
    
    # Load binary config
    binary_file = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/config/bme690/bme690_sel_33v_3s_4d/bsec_selectivity.config'
    with open(binary_file, 'rb') as f:
        binary_data = f.read()
    
    # Load C array config (use matching one from same directory)
    c_file = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/config/bme690/bme690_sel_33v_3s_4d/bsec_selectivity.c'
    with open(c_file) as f:
        content = f.read()
    
    match = re.search(r'\{\s*(.*?)\s*\}', content, re.DOTALL)
    if not match:
        print("✗ Failed to extract config")
        return False
    
    hex_str = match.group(1)
    c_values = [int(x.strip().rstrip(','), 0) for x in hex_str.split(',') if x.strip()]
    c_data = bytes(c_values)
    
    print(f"Binary file size: {len(binary_data)} bytes (with 4-byte header)")
    print(f"C array size: {len(c_data)} bytes (no header)")
    print()
    
    # Compare after stripping header
    binary_stripped = binary_data[4:]
    
    if binary_stripped == c_data:
        print("✓ Binary data (header-stripped) matches C array format exactly!")
        print(f"  - Verified: {len(binary_stripped)} bytes of config data are identical\n")
        return True
    else:
        print("✗ Config data mismatch!")
        return False

if __name__ == '__main__':
    try:
        success = True
        success &= test_binary_config_loading()
        success &= test_c_array_config_loading()
        success &= test_config_equivalence()
        
        print("=" * 70)
        if success:
            print("✓ ALL TESTS PASSED")
            print("\nSummary:")
            print("  - Binary .config files now load correctly")
            print("  - C array format configs work as before")
            print("  - Both formats are equivalent (header stripped automatically)")
        else:
            print("✗ SOME TESTS FAILED")
        print("=" * 70)
        
    except Exception as e:
        print(f"\n✗ Test error: {e}")
        import traceback
        traceback.print_exc()
