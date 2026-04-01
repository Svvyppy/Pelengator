# Renode System Test

This directory contains the Renode harness for the Peleng system test.

What it does:

- loads the firmware ELF into Renode
- loads the STM32G4 platform description
- prepares ADC input frames from either per-channel `txt/csv` files or one
  multi-column CSV table
- supports input streams longer than one 1024-sample half-buffer
- starts the shared Renode ADC runtime feeder used by both tests and debug runs
- runs the DSP pipeline and checks resulting delays

Files:

- `run_renode_system.sh` - wrapper used by CTest
- `peleng_system_test.resc` - Renode monitor script
- `stm32g4.repl` - Renode platform template for the STM32G473 board
- `rcc_g4.py` - RCC helper used by the platform
- `adc_cluster_g4.py` - ADC register-cluster stub for STM32G4
- `data/` - ADC sample files, one per input channel

The launcher renders the platform template and monitor script into temporary
files before starting Renode, so the checkout stays clean. ADC preprocessing is
done by [`prepare_adc_data.py`](/workspaces/Peleng/renode/prepare_adc_data.py)
and runtime feeding is handled by
[`adc_runtime.py`](/workspaces/Peleng/renode/adc_runtime.py).

Input formats:

- directory with `adc1_ch13`, `adc2_ch18`, `adc3_ch13`, `adc5_ch3` files in
  `.txt` or `.csv`
- a single `.csv` file with columns named like `adc1_ch13..adc5_ch3` or
  `ch1..ch4`

Frame behavior:

- the feeder splits input into 1024-sample half-buffer frames
- if the final frame is shorter than 1024 samples, it is padded with `2048`
- for finite datasets, the last reported delay corresponds to the last frame
  that reached the firmware, including a padded tail if present

Local run example:

```bash
cmake -S tests -B build/tests -DCMAKE_BUILD_TYPE=Debug \
  -DPELENG_FIRMWARE_ELF="$(pwd)/build/Debug/Peleng.elf" \
  -DPELENG_RENODE_PLATFORM="$(pwd)/tests/renode/stm32g4.repl"
cmake --build build/tests -j
ctest --test-dir build/tests -R peleng_renode_system --output-on-failure
```

Expected result defaults for the bundled ADC data:

- `d12 = -4`
- `d13 = -4`
- `d14 = -3`
- `valid = 1`

You can override expected values via environment variables:
`PELENG_EXPECT_D12`, `PELENG_EXPECT_D13`, `PELENG_EXPECT_D14`,
`PELENG_EXPECT_VALID`.

You can point the test at another source via `PELENG_ADC_SOURCE`, for example:

```bash
PELENG_ADC_SOURCE="$(pwd)/my_capture.csv" \
ctest --test-dir build/tests -R peleng_renode_system --output-on-failure
```

If Renode, the ELF, or the platform file is missing, the test exits with a skip
code instead of failing the whole suite.
