bme69x-python-library
=====================
This module supports the BME689 sensor and BSEC3 as a Python class. BME688 and BME680 are supported by BSEC2 and bme68x-python-library (2.6.1.0).  The BME69x module is a Python Extension implemented as "C" program containing functions that are invoked from within Python. If you are coming to this from the BME68x python wrapper then the changes to support multiple sensors have necessitated changing the initialisation signature.


## Architecture Changes

In addition to supporting the BME690 the BME69X Python wrapper has been enhanced to support multiple independent sensor instances. Each instance maintains:
- Its own BSEC state and configuration
- Separate I2C file descriptors
- Sensor-specific config/state files

<br>Import the module via `<import bme69x>` or import the class via `<from bme69x import BME68X>`
- To use the BME69X API constants, import bme69xConstants.py via `<import bme69xConstants as cnst>` 
- To use the BSEC constants, import bsecConstants.py via `<import bsecConstants as bsec>`

Key features:

- Per-sensor BSEC context and independent I2C file descriptors
- Per-sensor config/state files so each sensor can restore its own calibration/state
- Sequential multi-sensor reads by default (no per-sensor threads)

Recommended imports:

```python
import bme69x
from bme69x import BME69X
import bme69xConstants as cnst
import bsecConstants as bsec
```

The `BME69X` constructor (Python API) accepts the following arguments:

```python
sensor = bme69x.BME69X(
    i2c_addr,        # Required: I2C address (0x76 or 0x77)
    i2c_bus=1,       # Optional: I2C bus number (default: 1 -> /dev/i2c-1)
    debug_mode=0,    # Optional: Enable debug output (0/1)
    sensor_name=None # Optional: Custom sensor identifier used for file names
)
```

- If `sensor_name` is provided it is used as-is for file naming.
- Otherwise the code auto-generates a sensor id such as `sensor_0x77`.

Internals (C extension): `BMEObject` now contains per-instance fields such as:

```c
uint8_t i2c_addr;    // I2C address of this sensor
char sensor_id[64];  // Unique identifier for config/state filenames
```

Because each instance keeps its own BSEC context and I2C FD, saving/loading config or state operates on the specific instance.

This wrapper performs sequential reads for multiple sensors (read sensor1 → read sensor2 → sleep). If you need independent, precise duty cycles per sensor, prefer running separate scripts and using a system scheduler (cron/systemd) or implement per-sensor threads or an async scheduler.

Useful resources:

- Building Python C extensions: https://realpython.com/build-python-c-extension-module/
- Python C API: https://docs.python.org/3/c-api/index.html

## Quick Start

Minimal example that initializes a sensor, restores state/config, sets a heater profile and reads one BSEC sample:

```python
from time import sleep
from bme69x import BME69X
import bme69xConstants as cnst
import bsecConstants as bsec

sensor = BME69X(i2c_addr=0x76, sensor_name='sensor_0x76')
sensor.load_bsec_conf()
sensor.load_bsec_state()

# Simple forced-mode heater (single step)
sensor.set_heatr_conf(cnst.BME69X_ENABLE,320,5,cnst.BME69X_FORCED_MODE)
sensor.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)

sleep(2)
data = sensor.get_bsec_data()
print(data)
```
When the print step returns {} (null) re-run the get_bsec_data() line again muntil it prints valuees.
See `examples/` for the more examples and other usage patterns.

## Public API (summary)

Constructor:

```python
sensor = BME69X(i2c_addr, i2c_bus=1, debug_mode=0, sensor_name=None)
```

Common methods:

- `get_chip_id()` → int: Returns device chip id
- `get_variant()` → str: Returns sensor variant (e.g., `BME690`)
- `get_bsec_version()` → str: Returns BSEC version
- `set_heatr_conf(enable, temp_profile, dur_profile, operation_mode)` → int
  - `temp_profile`: list of up to 10 temperatures (°C)
  - `dur_profile`: list of up to 10 durations (units of 140 ms)
     Example: `[5,2,10,30,5,5,5,5,5,5]` → sum = 77 units → 77*140ms = 10.78s of heater time
- `set_sample_rate(rate)` → sets BSEC virtual sensor sampling rate (`bsecConstants`)
- `get_data()` → raw physical sensor readings (without BSEC processing)
- `get_bsec_data()` → physical + virtual (IAQ, VOC estimates, etc.) — may return `None` if no new data is available

See API.md for more detail. 

Notes on sampling and cycle time:

- The BME/BSEC workflow is: configure heater → sleep for heater duration → read sensor → run BSEC → sleep until next control signal. The effective cycle time = heater profile duration + BSEC sample rate latency.
- Polling too frequently will often return `Null` from `get_bsec_data()` because the sensor/BSEC has no new processed output. Adjust your duty cycle accordingly.

## Configuration and State files

To support multiple sensors, config and state files are written with the sensor id embedded in their filenames:

- Config: `conf/bsec_config_{sensor_id}.txt`
- State:  `conf/state_data_{sensor_id}.txt`

New API helpers on the instance have been added:

```python
sensor.load_bsec_conf()
sensor.save_bsec_conf()
sensor.load_bsec_state()
sensor.save_bsec_state()
```

Data formats:
The underlying getters and setters remain in place, but are not  multi-instance aware.
- `get_bsec_state()` returns an array of integers representing the BSEC state (commonly 197 integers for the BSEC v3 state).
- `get_bsec_conf()` returns the BSEC configuration as a list of integers (length can be larger; example exports may be ~2277 integers depending on the config produced by Bosch AI Studio).

When saving/restoring state from a file, read the file contents and convert the textual list back to integers, for example:

```python
with open(state_path, 'r') as f:
    contents = f.read().strip()
# Remove surrounding brackets and split
ints = [int(x) for x in contents.strip()[1:-1].split(',') if x.strip()]
sensor.set_bsec_state(ints)
```

Why save/load state:  Restoring the saved state shortens the time to re-acquire high accuracy after power cycles. When you burn-in a sensor it reaches a state of high accuracy (state 3) at this time it is useful to save state. Similarly saveing a config tailors the settings, rather than choosing the defaults which do not fit all cases.  

### Loading configs from arbitrary file paths

You can now load BSEC configs directly from any file path (e.g., Bosch AI Studio exports) without copying into `./conf/`:

```python
from bme69x import BME69X
import bme69xConstants as cnst
import bsecConstants as bsec

sensor = BME69X(0x76, sensor_name='sensor_76')
sensor.load_bsec_conf_from_file('/path/to/AI_Studio_export.config')
sensor.set_sample_rate(bsec.BSEC_SAMPLE_RATE_LP)
print(sensor.get_bsec_data())
```

Notes:
- Binary `.config` files typically include a 4-byte header (little‑endian size). The loader auto‑detects and strips this, passing the correct 2005‑byte blob to BSEC.
- `.c` array configs (2005 bytes) also work via `set_bsec_conf(conf_list)` if you prefer embedding directly in Python.

### Recommended initialization order

1. Create `BME69X`
2. Load config first: `load_bsec_conf()` or `load_bsec_conf_from_file(path)`
3. Optionally `load_bsec_state()` after burn‑in
4. Set sample rate via `set_sample_rate(...)`
5. Only configure heater (`set_heatr_conf`) if required by your workflow; avoid overriding AI Studio heater profiles embedded in the `.config`
6. Read processed results: `get_bsec_data()` (or gas‑estimate helpers)

## Multi-sensor considerations and scheduling

This wrapper reads sensors sequentially: read sensor A, read sensor B, then sleep. That makes it simple but means each sensor's effective duty cycle depends on the total work per loop (heater durations + BSEC latency + global sleep).

Options if you need independent timing per sensor:

- Run separate scripts per sensor and schedule them with `cron` or `systemd` timers (recommended for simplicity and reliability).
- Implement per-sensor threads or an async scheduler inside a single process (more complex).

Example `cron` entry to run a per-sensor script every 5 minutes:

```
*/5 * * * * /usr/bin/python3 /path/to/sensor1_collector.py >> /var/log/sensor1.log 2>&1
```

## Examples and troubleshooting

- See `examples/multi_sensor_test.py` for a tested multi-sensor sequence and `examples/forced_mode.py` for single-sensor forced-mode usage.
- If `get_bsec_data()` returns `Null` frequently, increase your sleep/duty-cycle or adjust the heater profile so the sensor has time to recover between measurements.

### Digital Nose best practices

- Prefer `PARALLEL_MODE` with AI Studio-trained configs that embed heater profiles; avoid overriding them with `set_heatr_conf()` unless necessary.
- Load config first (`load_bsec_conf()` or `load_bsec_conf_from_file(path)`), then set the sample rate, then poll `get_bsec_data()`.
- Allow adequate sleeps to respect long heater cycles used by trained models; start with 1–3 seconds and adjust based on cycle length.
- If you see `-16` timing warnings, double-check the order above and reduce polling frequency.

### CLI example: load config from path

Use `examples/load_config_from_path.py` to pass a path to an AI Studio `.config` file:

```bash
python3 examples/load_config_from_path.py --config /path/to/export.config --i2c-addr 0x76 --rate LP
```

## Links

- Bosch BSEC integration guide: refer to `bsec_v3-3-0-0/integration_guide` which is part of the BSEC3 package from Bosch Sensortec: [here](https://www.bosch-sensortec.com/software-tools/software/bme688-and-bme690-software/#Library)
- Python C API: https://docs.python.org/3/c-api/
