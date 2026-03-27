# Peleng Hydroacoustic Direction Finder

Embedded project for a 4-channel hydroacoustic direction finder on STM32G473.
The firmware acquires synchronous ADC data from four hydrophone channels, estimates envelope fronts,
computes relative delays (`1-2`, `1-3`, `1-4`), and sends telemetry over UART.

## 1. Device Logic Overview

The device implements a deterministic streaming DSP pipeline:

1. ADC sampling (`Fs = 250 kHz`) is triggered by TIM6 TRGO.
2. Four ADC peripherals run in DMA circular mode with half/full transfer events.
3. Raw 12-bit ADC codes are converted to centered/scaled `float32` samples.
4. Each channel is squared and passed through FIR low-pass filtering to get an envelope.
5. First threshold crossing index is detected per channel.
6. Relative delays are computed vs channel 1:
   - `d12 = t2 - t1`
   - `d13 = t3 - t1`
   - `d14 = t4 - t1`
7. Delays are converted to microseconds and sent through non-blocking UART telemetry queue.

## 2. Data Path and Timing

### Sampling and buffering

- Sample rate: `250000 Hz`
- Half-buffer size per channel: `1024` samples
- Full DMA buffer size per channel: `2048` samples
- Half-buffer time window: `1024 / 250000 = 4.096 ms`

This means DSP processing for one half-buffer must complete within ~4.096 ms to maintain margin.

### Main processing stages

- `ConvertAdcToF32`: remove DC midpoint and keep legacy scaling (`Q31-like`, `<<19`)
- Envelope estimation:
  - square: `arm_mult_f32`
  - FIR: CMSIS-DSP `arm_fir_f32`
- Delay estimation by threshold crossing on envelope buffers

## 3. Project Structure

- `Core/Inc/Peleng.hpp`, `Core/Src/Peleng.cpp`
  - hardware-facing orchestrator: DMA callbacks, conversion, invoking DSP pipeline
- `Core/Inc/dsp/PelengDsp.hpp`, `Core/Src/dsp/PelengDsp.cpp`
  - DSP logic independent from HAL
- `Core/Inc/Filter.hpp`, `Core/Src/Filter.cpp`
  - FIR wrapper over CMSIS-DSP
- `Core/Inc/platform/UartTelemetry.hpp`, `Core/platforms/UartTelemetry.cpp`
  - non-blocking UART telemetry queue (`HAL_UART_Transmit_IT`)
- `Core/platforms/Hw*.{h,cpp}`
  - STM32 hardware init and IRQ glue
- `Core/Inc/CommonSettings.h`
  - DSP and pipeline constants

## 4. Current Telemetry Format

Text line over UART (ASCII):

- valid delay frame:
  - `D12=<samples>(<us>) D13=<samples>(<us>) D14=<samples>(<us>)`
- no detections:
  - `NO_SIGNAL`

UART output is queued and transmitted asynchronously.
If queue is full, new message is dropped (`SendDelayTelemetryUart` returns `false`).

## 5. Build and Flash

### Configure and build (preset)

```bash
cmake --preset Debug
cmake --build --preset Debug -j
```

Resulting firmware binary:

- `build/Debug/Peleng.elf`

### Toolchain notes

- GCC ARM Embedded is configured via:
  - `st_gen/toolchein/gcc-arm-none-eabi.cmake`
- CMSIS-DSP is built as a subdirectory library.

## 6. Key Configuration

Main constants are in `Core/Inc/CommonSettings.h`:

- `SAMPLE_RATE_HZ`
- `BUFFER_SIZE`
- `DMA_HALF_BUFFER_SIZE`, `DMA_FULL_BUFFER_SIZE`
- `NUM_TAPS`, `BLOCK_SIZE`
- `SIGNAL_THRESHOLD`

When changing these values, verify:

1. FIR block alignment (`sample_count % BLOCK_SIZE == 0`)
2. real-time budget under new `Fs`
3. threshold robustness on real hydrophone data

## 7. Real-Time and Safety Notes

- DSP path is designed for deterministic half-buffer processing.
- UART uses non-blocking IT transmit to avoid stalling DSP loop.
- Delay estimation currently uses first threshold crossing (fast, but sensitive to noise).

## 8. Validation Checklist

After any DSP or platform change:

1. Build (`cmake --build --preset Debug`) must pass.
2. Verify ADC DMA half/full callbacks are firing.
3. Verify UART telemetry stream continuity (no long pauses under load).
4. Inject known inter-channel delay and check `d12/d13/d14` sign and magnitude.

## 9. Recommended Next Improvements

- Sub-sample delay refinement using cross-correlation around detected front.
- Binary telemetry mode (compact framed packet).
- Host-side test harness for deterministic DSP regression checks.

## 10. Geometry Calibration and Angle Estimation

This firmware currently reports inter-channel delays. To convert delays into bearing angle,
calibrate array geometry and run a geometric model on top of `d12/d13/d14`.

### 10.1 Required calibration data

- hydrophone coordinates in one rigid frame, for example:
  - `p1 = (x1, y1, z1)`
  - `p2 = (x2, y2, z2)`
  - `p3 = (x3, y3, z3)`
  - `p4 = (x4, y4, z4)`
- speed of sound in current water conditions (`c`, m/s)
- sign convention for delays (`dij = tj - ti`)

### 10.2 Delay to TDOA conversion

For each delay in samples:

- `tau12 = d12 / Fs`
- `tau13 = d13 / Fs`
- `tau14 = d14 / Fs`

Equivalent range differences:

- `Delta12 = c * tau12`
- `Delta13 = c * tau13`
- `Delta14 = c * tau14`

### 10.3 Far-field approximation (direction vector)

For far-field source, let unit direction vector be `u`.
For each baseline `bi = pi - p1`:

- `dot(u, b2) = Delta12`
- `dot(u, b3) = Delta13`
- `dot(u, b4) = Delta14`

Solve (least-squares if needed), normalize `u`, then compute azimuth/elevation in your chosen frame.

### 10.4 Practical calibration procedure

1. Measure hydrophone coordinates with mm-level accuracy.
2. Place a reference source at known bearings.
3. Record `d12/d13/d14` and fit residual bias terms per channel.
4. Validate sign, channel order, and angle wrap conventions.
5. Store geometry and bias constants in a dedicated config header.

## 11. Doxygen Documentation

Doxygen config is generated via CMake from `docs/Doxyfile.in`.

Build API docs:

```bash
cmake --preset Debug
cmake --build --preset Debug --target docs
```

Generated HTML output:

- `build/Debug/docs/doxygen/html/index.html`

If `docs` target is missing, install Doxygen in your environment.

## 12. CI/CD (GitHub Actions)

Repository includes two workflows:

- `.github/workflows/ci-release.yml`
  - builds firmware on each `push` and `pull_request`
  - publishes build artifacts (`.elf`, `.map`, `.hex`, `.bin`)
  - on tags matching `v*` creates/updates GitHub Release and attaches
    firmware artifacts plus source archives (`.zip`, `.tar.gz`)
- `.github/workflows/publish-doxygen.yml`
  - builds docs on `push` to `master` / `main`
  - deploys generated Doxygen HTML to GitHub Pages

Release flow:

```bash
git tag v1.0.0
git push origin v1.0.0
```

To publish Doxygen on Pages, set repository Pages source to **GitHub Actions**
in repository settings.
