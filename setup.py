from setuptools import setup, Extension, find_packages
from pathlib import Path
import os
import subprocess
import sys

# Build-time environment variable to select the BSEC3 binary.
# Accept both BSEC and BSEC3 for compatibility.
# If neither is set a 32bit ArmV6 build is carried out.
# For 64 bit PI4 or PI5:
# BSEC=64 python -m build --wheel
# For 32 bit PI3 or above (inc PI Zero 2):
# BSEC=32 python -m build --wheel
# For PI Zero and early Arm V6 PI's:
# python -m build --wheel
# Configs for bme680, bme688 and bme690 are included in BSEC: the 690 33v_3s_4d config is selected by default.

BSEC3 = os.environ.get("BSEC", os.environ.get("BSEC3", None))
# Three PI Architectures are supported by BSEC2.6: PiThree_ArmV6 32 bit,  PiThree_ArmV8 32 bit,  PiFour_ArmV8 64 bit (also works for PI5) 

LIBDIR = Path(__file__).parent

if BSEC3 == '64':
    algo = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/bin/RaspberryPi/PiFour_Armv8'
    # 64 bit Raspbian OS - PI 4 and PI5, ARM V8A, ARM V8.2-A  Must be 64 bit OS
elif BSEC3 == '32':
    algo = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/bin/RaspberryPi/PiThree_ArmV8'
    # 32bit Raspbian OS - PI 5 / 4 / 3 /  Zero 2, ARM V8A  Must be 32bit  OS
else:
    algo = 'bsec_v3-3-0-0/release_bin/Sel_IAQ/bin/RaspberryPi/PiThree_ArmV6'
    # 32bit Raspbian OS - Pi Zero, Arm V6 Must be 32 bit OS

BSEC = True

if BSEC:
    ext_comp_args = ['-DBSEC', '-fPIC', '-g']
    libs = ['pthread', 'm', 'rt']
    lib_dirs = ['/usr/local/lib']
    lib_path = str(LIBDIR / algo / 'libalgobsec.a')
    extra_objects = [lib_path]

    # Verify required symbols are present in the selected BSEC archive.
    # bsec_get_instance_size() is absent from Bosch Raspberry Pi archives in
    # BSEC v3.3.0.0 (packaging defect); the fixed BSEC_INSTANCE_SIZE_WORKAROUND
    # macro is used instead.  When Bosch supply a corrected archive, remove
    # BSEC_INSTANCE_SIZE_WORKAROUND from bme69xmodule.c and restore
    # 'bsec_get_instance_size' to REQUIRED_SYMBOLS below.
    REQUIRED_SYMBOLS = ['bsec_init', 'bsec_get_version']  # add 'bsec_get_instance_size' when workaround is removed
    try:
        nm_out = subprocess.check_output(['nm', '--defined-only', lib_path],
                                         stderr=subprocess.DEVNULL).decode()
        missing = [s for s in REQUIRED_SYMBOLS if s not in nm_out]
        if missing:
            print(
                f"\nERROR: The BSEC archive\n  {lib_path}\n"
                f"is missing required symbol(s): {missing}\n"
                "Build will proceed but linking will fail until a compatible "
                "BSEC archive is provided.\n",
                file=sys.stderr,
            )
    except (FileNotFoundError, subprocess.CalledProcessError):
        pass  # nm not available; skip check
else:
    ext_comp_args = []
    libs = ['pthread', 'm', 'rt']
    lib_dirs = ['/usr/local/lib']
    extra_objects = []

README = (LIBDIR / "README.md").read_text()

include_dirs=['/usr/local/include', 'bsec_v3-3-0-0/release_bin/Sel_IAQ/inc']
bme69x = Extension('bme69x',
                   extra_compile_args=ext_comp_args,
                   extra_objects=extra_objects,
                   include_dirs=include_dirs,
                   libraries=libs,
                   library_dirs=lib_dirs,
                   depends=['BME690_SensorAPI/bme69x.h', 'BME690_SensorAPI/bme69x.c',
                            'BME690_SensorAPI/bme69x_defs.h', 'internal_functions.h', 'internal_functions.c'],
                   sources=['bme69xmodule.c', 'BME690_SensorAPI/bme69x.c', 'internal_functions.c'])

setup(name='bme69x',
      version='3.3.0.0',
      description='pi3g Python interface for BME69X sensor and BSEC',
      long_description=README,
      long_description_content_type='text/markdown',
      url='https://github.com/mcalisterkm/bme68x-python-library-bsec3.3.0.0',
      author='Multiple',
      author_email='',
      license='MIT',
      classifiers=[
           'Development Status :: 3 - Prod§',
           'Intended Audience :: Developers',
          'Natural Language :: English',
          'Operating System :: POSIX :: Linux',
          'Programming Language :: Python :: Implementation :: CPython',
          'Topic :: Scientific/Engineering :: Atmospheric Science',
      ],
      keywords='bme69x bme690 bme680 bme688 BME69X BME68X BME690 BME680 BME688 bsec BSEC Bosch Sensortec environment sensor',
      packages=find_packages(),
      py_modules=['bme69xConstants', 'bsecConstants'],
      package_data={
          'bme69x': [
               'bsec_v3-3-0-0/release_bin/Sel_IAQ/config/bme690/bme690_sel_33v_3s_4d/bsec_selectivity.config',
          ]
      },
      headers=['BME690_SensorAPI/bme69x.h',
               'BME690_SensorAPI/bme69x_defs.h', 'internal_functions.h'],
      ext_modules=[bme69x])
