![Python Version](https://img.shields.io/badge/python-3.7+-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Version](https://img.shields.io/badge/version-3.3.0-blue.svg)
![Status](https://img.shields.io/badge/status-active-success.svg)

# BME69X and BSEC3.3.0.0 for Python

The bme69x-python-library is a Python 3 wrapper for the BSEC3 library and BME690 environment sensor available from BoschSensortec. 
The main use case for the Raspberry PI is with (1 or 2) single sensor BME690 modules, in IAQ (Air Quality and env data) mode  or SEL mode (Selectivity) sniffing using an AI Studio  model. 

Bosch Sensortec released BSEC v3.3.0.0 in March 2026, and this update to the Pyhon3 wrapper adds support for multiple BME690 sensors with isolated configuration and state data. BSEC3.3 adds   BSEC_OUTPUT_TVOC_EQUIVALENT, which replaces  BSEC_OUTPUT_BREATH_VOC_EQUIVALENT.

Also in this release the Python 'build' package is used, rather than calling setup.py directly to build the extension. 

BME690 Sensor modules from Pimoroni are used in development and testing, connected using I2C to Rapberry Pi Zero, Zero2, PI 4, and PI 5. Bosch Sensortec BSEC3.3 supports PI3 ARM v6,  PI3 ARM v8, PI4 ARM v8 (32 and 64bit support). 

The tools folder contains an AI Studio model with classes for AIR and COFFEE that you can try out.

If you have a BME680 or BME688 please use BSEC2 v2.6.1.0 and its Python wrapper which is stable and has 64bit and 32 bit support [here](https://github.com/mcalisterkm/bme68x-python-library-bsec2.6.1.0).  

### Pre-requisites

The BME690 uses I2C which will need to be enabled on the target PI and can be enabled using raspi-config and the "Interface Options" menu.  i2c-tools (apt install i2c-tools) is a useful utility to validate the I2C port your sensor is working on (i2cdetect -y 1). This release supports changing the I2C bus used on the PI, and running multiple sensors (0x76 & 0x77 tested).

Some Raspbian installs require the python3 development package to be installed, i2cdetect is usefull, and we use the Python 'build' package, to build and install this package. So there is no harm in running these package installs (if they exist it will not do anything).
```
$ sudo apt install python3-dev
$ sudo apt install python3-build
$ sudo apt install i2c-tools
```
Using a full 64 bit Raspbian Trixe fresh install, only python3-build was missing, however that is not the case for all versions. 

python3-dev),

### How to install the extension with BSEC
High level steps: 
- setup a Python virtual environment
- clone [this repo](https://TBA) to a desired location (virtual env) on your hard drive
- download the licensed BSEC3 library [from BOSCH](https://www.bosch-sensortec.com/software-tools/software/bme688-and-bme690-software/)<br>
- unzip it into the *bme69x-python-library-bsec3.3.0.0* folder, next to this *README.md* file
- open a terminal window inside the *bme69x-python-library-bsec3.3.0.0* folder, build a wheel with `python3 -m build`, then install the wheel with `pip`.

Note: Only bsec_v3-3-0-0.zip is supported by this release.

Recent versions of Raspbian require local Python packages to be installed in a Python virtual environment (venv)
1) Let's review how to create a virtual env called `BME690'
````
# The first step is to create the virtual environment BSEC3 in your home directory (/home/<user>)
$ python -m venv --system-site-packages ./BSEC3.3
$ cd BSEC3.3
````
To invoke the virtual env
````
$ source ./bin/activate
(BSEC3.3)<user>:~/BSEC3.3 $ 
````

To exit a virtual env
````
(BSEC3.3)<user>:~/BSEC3.3 $ deactivate
$
````
To remove a virtual environment, first deactivate, then delete it (cd ~ ; rm -rf ./BSEC3)

2) Download Software

In the BSEC3.3 virtual env directory clone this repo or download the zip using the Github Green "<> Code" button. The download is named bme69x-python-library-bsec3.3.0.0.zip, unzip it and your folder should look like this:

```
$cd ~
$cd BSEC3.3
$ source ./bin/activate
(BSEC3.3) <user>:~/BSEC3.3 $ 

## Unpack the zip file then cd into directory 

(BSEC3.3) <user>:~/BSEC3.3 $ cd bme69x-python-library-bsec3.3.0.0
(BSEC3.3) <user):~/BSEC3.3/bme69x-python-library-bsec3.3.0.0 $ ls -l

-rw-rw-r-- 1 kpi kpi 11126 May  9 12:07 API.md
drwxrwxr-x 4 kpi kpi  4096 May  9 12:06 BME690_SensorAPI
-rw-rw-r-- 1 kpi kpi  2933 May  9 12:07 bme69xConstants.py
-rw-rw-r-- 1 kpi kpi 86800 May  9 12:07 bme69xmodule.c
-rw-rw-r-- 1 kpi kpi   571 May  9 12:07 bsecConstants.py
-rw-rw-r-- 1 kpi kpi  9891 May  9 12:07 Documentation.md
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 examples
-rw-rw-r-- 1 kpi kpi 19028 May  9 12:07 internal_functions.c
-rw-rw-r-- 1 kpi kpi  2664 May  9 12:07 internal_functions.h
-rw-rw-r-- 1 kpi kpi    93 May  9 12:07 pyproject.toml
-rw-rw-r-- 1 kpi kpi 12605 May  9 12:07 README.md
-rw-rw-r-- 1 kpi kpi  4790 May  9 12:07 setup.py
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 tools


```

3) BSEC3 Library

The BSEC3 Library is unzipped into the BSEC3/bme69x-python-library-bsec3.3.0.0 directory which is where you are from the previous step and it should look like this:
```
(BSEC3.3) <user>:~/BSEC3.3/bme69x-python-library-bsec3.3.0.0 $ ls -l
total 188
-rw-rw-r-- 1 kpi kpi 11126 May  9 12:07 API.md
drwxrwxr-x 4 kpi kpi  4096 May  9 12:06 BME690_SensorAPI
-rw-rw-r-- 1 kpi kpi  2933 May  9 12:07 bme69xConstants.py
-rw-rw-r-- 1 kpi kpi 86800 May  9 12:07 bme69xmodule.c
-rw-rw-r-- 1 kpi kpi   571 May  9 12:07 bsecConstants.py
drwxrwxr-x 6 kpi kpi  4096 May  9 12:08 bsec_v3-3-0-0
-rw-rw-r-- 1 kpi kpi  9891 May  9 12:07 Documentation.md
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 examples
-rw-rw-r-- 1 kpi kpi 19028 May  9 12:07 internal_functions.c
-rw-rw-r-- 1 kpi kpi  2664 May  9 12:07 internal_functions.h
-rw-rw-r-- 1 kpi kpi    93 May  9 12:07 pyproject.toml
-rw-rw-r-- 1 kpi kpi 12605 May  9 12:07 README.md
-rw-rw-r-- 1 kpi kpi  4790 May  9 12:07 setup.py
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 tools
```

4) Build
Your present working directory ($ pwd) should be /home/< user >/BSEC3.3/bme69x-python-library-bsec3.3.0.0

If you do not have the Python build package installed you will need to run
```
(BSEC3) <user>:~/BSEC3$ python3 -m pip install build
```
There is no harm in running this it will simply report the installed version of build if it already exists. 

Make sure your Python virtual env is still enabled, and now build a wheel and install it into this venv.
````agsl

$ cd /home/<user>/BSEC3
$ source /home/<user>/BSEC3/bin/activate
$(BSEC3) 
````

a. For 32 bit PI3 or above (Inc PI Zero 2)
```bash
$(BSEC3) BSEC=32 python3 -m build --wheel
$(BSEC3) python3 -m pip install dist/*.whl
```
b. For PI4 or PI5 running Raspbian 64 bit
```bash
$(BSEC3) BSEC=64 python3 -m build --wheel
$(BSEC3) python3 -m pip install dist/*.whl
```
c. For PI Zero and early Arm V6 PI's, no environment variable is set
```bash
$(BSEC3) python3 -m build --wheel
$(BSEC3) python3 -m pip install dist/*.whl
```
`BSEC3` is still accepted as a compatibility alias, but `BSEC` is now the preferred selector.

Target selector values are:
- `BSEC=64` -> `PiFour_Armv8` (64-bit OS)
- `BSEC=32` -> `PiThree_ArmV8` (32-bit OS)
- unset -> `PiThree_ArmV6` (legacy Pi Zero / ArmV6)
Build on a matching target architecture/OS. For example, trying to link the 32-bit archive on a 64-bit-only build target can fail with "file in wrong format".

The move to the build package rather that calling setup.py directly has removed a deprecation warning while still using the same setuptools-based extension build.
Note: For scripts (cron, bash etc) to run a virtual environment python3 all you have to do is use the full path: /home/<user_name>/BSEC3/bin/python3 

### How to use the extension
- to import in Python
```python
import bme69x
```
or as a Class
```python
from bme69x import BME69X
```
- see Documentation.md for a quick overview and API.md as a reference
- to test the installation make sure you connected your BME690 sensor via I2C
- run the following code in a Python3 interpreter
```python
from bme69x import BME69X

# Replace I2C_ADDR with the I2C address of your sensor
# Typically  I2C is  0x76  or 0x77  (Pimoroni BME690 module requires a link to be cut for 0x77)
bme69x = BME69X(I2C_ADDR,1, 0)
bme69x.set_heatr_conf(1, 320, 100, 1)
data = bme69x.get_data()
```

The examples folder has useful programs, include burning in a sensor, using ultra low power mode, using multiple sensor modules, and  saving and loading config and state data for a sensor. 
The tools folder provide a sample AI Model, data and code to use with a BME690 sensor to classify smells. Collecting data is best done with the BME690  8 sensor BOSCH Sensortec DevKit


### A walk through a 64bit PI4 installation follows.
```
$ sudu apt install i2c-tools
$ i2cdetect -y 1
0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
00:                         -- -- -- -- -- -- -- --
10: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
20: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
30: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
40: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
50: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
60: -- -- -- -- -- -- -- -- -- -- -- -- -- -- -- --
70: -- -- -- -- -- -- -- 77 
```
The BME690 module is showing up on port 0x77.
If it fails to show up check connections and the module documentation. 

Install "Python3-dev" package (If you miss this step you may see a Python.h missing error)
```
$ sudo apt install python3-dev
$ sudo apt install python3-build
$sudo apt install i2c-tools
```

As I am using Raspbian Trixie  a virtual environment is required.
```
$ python -m venv --system-site-packages ./BSEC3.3
$ cd BSEC3.3
$ source bin/activate
<user>:~/BSEC3.3 $
```
Next clone this repo into the virtual environment 
```
(BSEC3.3) <user>:~/BSEC3.3/bme69x-python-library-bsec3.3.0.0 $ ls -l
total 188
-rw-rw-r-- 1 kpi kpi 11126 May  9 12:07 API.md
drwxrwxr-x 4 kpi kpi  4096 May  9 12:06 BME690_SensorAPI
-rw-rw-r-- 1 kpi kpi  2933 May  9 12:07 bme69xConstants.py
-rw-rw-r-- 1 kpi kpi 86800 May  9 12:07 bme69xmodule.c
-rw-rw-r-- 1 kpi kpi   571 May  9 12:07 bsecConstants.py
-rw-rw-r-- 1 kpi kpi  9891 May  9 12:07 Documentation.md
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 examples
-rw-rw-r-- 1 kpi kpi 19028 May  9 12:07 internal_functions.c
-rw-rw-r-- 1 kpi kpi  2664 May  9 12:07 internal_functions.h
-rw-rw-r-- 1 kpi kpi    93 May  9 12:07 pyproject.toml
-rw-rw-r-- 1 kpi kpi 12605 May  9 12:07 README.md
-rw-rw-r-- 1 kpi kpi  4790 May  9 12:07 setup.py
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 tools
```

Now copy the BoschSensortech bsec_v3-3-0-0 into the bmx69x repo clone. 
It should look like this.

```
BSEC3.3) <user>:~/BSEC3.3/bme69x-python-library-bsec3.3.0.0 $ ls -l
total 188
-rw-rw-r-- 1 kpi kpi 11126 May  9 12:07 API.md
drwxrwxr-x 4 kpi kpi  4096 May  9 12:06 BME690_SensorAPI
-rw-rw-r-- 1 kpi kpi  2933 May  9 12:07 bme69xConstants.py
-rw-rw-r-- 1 kpi kpi 86800 May  9 12:07 bme69xmodule.c
-rw-rw-r-- 1 kpi kpi   571 May  9 12:07 bsecConstants.py
drwxrwxr-x 6 kpi kpi  4096 May  9 12:08 bsec_v3-3-0-0
-rw-rw-r-- 1 kpi kpi  9891 May  9 12:07 Documentation.md
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 examples
-rw-rw-r-- 1 kpi kpi 19028 May  9 12:07 internal_functions.c
-rw-rw-r-- 1 kpi kpi  2664 May  9 12:07 internal_functions.h
-rw-rw-r-- 1 kpi kpi    93 May  9 12:07 pyproject.toml
-rw-rw-r-- 1 kpi kpi 12605 May  9 12:07 README.md
-rw-rw-r-- 1 kpi kpi  4790 May  9 12:07 setup.py
drwxrwxr-x 3 kpi kpi  4096 May  9 12:06 tools

```
From here build the wheel with the 64bit env set for the Pi 4 board with 64 bit Raspbian Trixie , then install using pip it into the active venv.

```
(BSEC3.3) <user>:~/BSEC3.3/bme69x-python-library-bsec3.3.0.0 $ BSEC=64 python3 -mbuild --wheel
* Creating isolated environment: venv+pip...
* Installing packages in isolated environment:
  - setuptools>=68
  - wheel
* Getting build dependencies for wheel...
running egg_info
writing bme69x.egg-info/PKG-INFO
writing dependency_links to bme69x.egg-info/dependency_links.txt
writing top-level names to bme69x.egg-info/top_level.txt
reading manifest file 'bme69x.egg-info/SOURCES.txt'
writing manifest file 'bme69x.egg-info/SOURCES.txt'
* Building wheel...
running bdist_wheel
running build
running build_py
copying bme69xConstants.py -> build/lib.linux-aarch64-cpython-313
copying bsecConstants.py -> build/lib.linux-aarch64-cpython-313
running build_ext
building 'bme69x' extension
aarch64-linux-gnu-gcc -fno-strict-overflow -Wsign-compare -DNDEBUG -g -O2 -Wall -fPIC -I/usr/local/include -Ibsec_v3-3-0-0/release_bin/Sel_IAQ/inc -I/tmp/build-env-usptbsma/include -I/usr/include/python3.13 -c BME690_SensorAPI/bme69x.c -o build/temp.linux-aarch64-cpython-313/BME690_SensorAPI/bme69x.o -DBSEC -fPIC -g
aarch64-linux-gnu-gcc -fno-strict-overflow -Wsign-compare -DNDEBUG -g -O2 -Wall -fPIC -I/usr/local/include -Ibsec_v3-3-0-0/release_bin/Sel_IAQ/inc -I/tmp/build-env-usptbsma/include -I/usr/include/python3.13 -c bme69xmodule.c -o build/temp.linux-aarch64-cpython-313/bme69xmodule.o -DBSEC -fPIC -g
aarch64-linux-gnu-gcc -fno-strict-overflow -Wsign-compare -DNDEBUG -g -O2 -Wall -fPIC -I/usr/local/include -Ibsec_v3-3-0-0/release_bin/Sel_IAQ/inc -I/tmp/build-env-usptbsma/include -I/usr/include/python3.13 -c internal_functions.c -o build/temp.linux-aarch64-cpython-313/internal_functions.o -DBSEC -fPIC -g
aarch64-linux-gnu-gcc -shared -Wl,-O1 -Wl,-Bsymbolic-functions -Wl,-z,relro -g -fwrapv -O2 build/temp.linux-aarch64-cpython-313/BME690_SensorAPI/bme69x.o build/temp.linux-aarch64-cpython-313/bme69xmodule.o build/temp.linux-aarch64-cpython-313/internal_functions.o /home/kpi/BSEC3.3/bme69x-python-library-bsec3.3.0.0/bsec_v3-3-0-0/release_bin/Sel_IAQ/bin/RaspberryPi/PiFour_Armv8/libalgobsec.a -L/usr/local/lib -L/usr/lib/aarch64-linux-gnu -lpthread -lm -lrt -o build/lib.linux-aarch64-cpython-313/bme69x.cpython-313-aarch64-linux-gnu.so
installing to build/bdist.linux-aarch64/wheel
running install
running install_lib
creating build/bdist.linux-aarch64/wheel
copying build/lib.linux-aarch64-cpython-313/bme69xConstants.py -> build/bdist.linux-aarch64/wheel/.
copying build/lib.linux-aarch64-cpython-313/bsecConstants.py -> build/bdist.linux-aarch64/wheel/.
copying build/lib.linux-aarch64-cpython-313/bme69x.cpython-313-aarch64-linux-gnu.so -> build/bdist.linux-aarch64/wheel/.
running install_headers
creating build/bdist.linux-aarch64/wheel/bme69x-3.3.0.0.data/headers
copying BME690_SensorAPI/bme69x.h -> build/bdist.linux-aarch64/wheel/bme69x-3.3.0.0.data/headers
copying BME690_SensorAPI/bme69x_defs.h -> build/bdist.linux-aarch64/wheel/bme69x-3.3.0.0.data/headers
copying internal_functions.h -> build/bdist.linux-aarch64/wheel/bme69x-3.3.0.0.data/headers
running install_egg_info
running egg_info
writing bme69x.egg-info/PKG-INFO
writing dependency_links to bme69x.egg-info/dependency_links.txt
writing top-level names to bme69x.egg-info/top_level.txt
reading manifest file 'bme69x.egg-info/SOURCES.txt'
writing manifest file 'bme69x.egg-info/SOURCES.txt'
Copying bme69x.egg-info to build/bdist.linux-aarch64/wheel/./bme69x-3.3.0.0-py3.13.egg-info
running install_scripts
creating build/bdist.linux-aarch64/wheel/bme69x-3.3.0.0.dist-info/WHEEL
creating '/home/kpi/BSEC3.3/bme69x-python-library-bsec3.3.0.0/dist/.tmp-2on1a5zh/bme69x-3.3.0.0-cp313-cp313-linux_aarch64.whl' and adding 'build/bdist.linux-aarch64/wheel' to it
adding 'bme69x.cpython-313-aarch64-linux-gnu.so'
adding 'bme69xConstants.py'
adding 'bsecConstants.py'
adding 'bme69x-3.3.0.0.data/headers/bme69x.h'
adding 'bme69x-3.3.0.0.data/headers/bme69x_defs.h'
adding 'bme69x-3.3.0.0.data/headers/internal_functions.h'
adding 'bme69x-3.3.0.0.dist-info/METADATA'
adding 'bme69x-3.3.0.0.dist-info/WHEEL'
adding 'bme69x-3.3.0.0.dist-info/top_level.txt'
adding 'bme69x-3.3.0.0.dist-info/RECORD'
removing build/bdist.linux-aarch64/wheel
Successfully built bme69x-3.3.0.0-cp313-cp313-linux_aarch64.whl


(BSEC3) <user>:~/ $ python3 -m pip install dist/*.whl

(BSEC3.3) kpi@PI43:~/BSEC3.3/bme69x-python-library-bsec3.3.0.0 $ python3 -m pip install dist/*.whl
Looking in indexes: https://pypi.org/simple, https://www.piwheels.org/simple
Processing ./dist/bme69x-3.3.0.0-cp313-cp313-linux_aarch64.whl
Installing collected packages: bme69x
Successfully installed bme69x-3.3.0.0

```
The build should be clean with no warnings, and the wheel build should complete with output under the `dist/` directory.

After installing the wheel, verify it with:
```
(BSEC3.3) <user>:~/ $ python3.3 -m pip show bme69x
```

Change to the examples directory and run the forced mode example:
```
(BSEC3.3) <user>:~/BSEC3.3/bme69x-python-library-bsec3.3.0.0/examples $ ls
airquality.py  build       conf            force_ulp.py          parallel_mode.py      read_conf.py
bme_ptrs.log   burn_in.py  forced_mode.py  multi_sensor_test.py  parallel_mode_ulp.py  README.md
(BME690x) kpi@dev2:~/BME690x/bme69x-python-library-bsec3.3.0.0/examples $ python3 forced_mode.py
TESTING FORCED MODE WITHOUT BSEC
{'sample_nr': 1, 'timestamp': 3956208, 'raw_temperature': 54.29930877685547, 'raw_pressure': 931.8998413085938, 'raw_humidity': 108.98556518554688, 'raw_gas': 109.448486328125, 'status': 160}

TESTING FORCED MODE WITH BSEC
{'sample_nr': 1, 'timestamp': 626732524746608, 'iaq': 50.0, 'iaq_accuracy': 0, 'static_iaq': 50.0, 'static_iaq_accuracy': 0, 'co2_equivalent': 500.0, 'co2_accuracy': 0, 'raw_temperature': 20.232421875, 'raw_pressure': 100879.46875, 'raw_humidity': 44.31648254394531, 'raw_gas': 25241.5703125, 'stabilization_status': 128, 'run_in_status': 144, 'temperature': 15.232421875, 'humidity': 60.71345901489258, 'gas_percentage': 0.0, 'gas_percentage_accuracy': 0, 'tvoc_equivalent': 0.0, 'tvoc_equivalent_accuracy': 0}
```
As the status and accuracy are all zero it is time to burn in this sensor for 24 hours. 

The original PI3G repository is available [here] (https://github.com/pi3g/bme68x-python-library) which works with BSEC 2.0.6.1/BME68x (32bit) from 2022.

> **Note: BSEC instance size workaround**
> The Bosch Raspberry Pi archives in BSEC v3.3.0.0 do not export `bsec_get_instance_size()` despite it being declared in `bsec_interface.h`.
> Following Bosch integration guidance, a fixed instance size (`UINT16_C(3272)`) is used instead, controlled by `#define BSEC_INSTANCE_SIZE_WORKAROUND` in `bme69xmodule.c`.
> When Bosch supply a corrected archive, revert by:
> 1. In `bme69xmodule.c` — comment out or delete the `#define BSEC_INSTANCE_SIZE_WORKAROUND` line.
> 2. In `setup.py` — add `'bsec_get_instance_size'` back to `REQUIRED_SYMBOLS`.
> Then rebuild and reinstall the wheel.
