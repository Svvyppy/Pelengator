# Peleng Firmware API

## Overview

This documentation contains only the public firmware API used by the Peleng processing pipeline:

- `Peleng` orchestration class
- FIR envelope `Filter`
- Delay estimation primitives
- UART telemetry API
- Shared DSP constants
- Platform hardware interface (`Hw`)

Internal generated STM32 sources, HAL internals, and build artifacts are intentionally excluded.

## Modules

- Signal pipeline: `Peleng`, `Filter`, `DelayEstimator`
- Telemetry: `UartTelemetry`
- Platform interface: `Hw`
- Configuration constants: `CommonSettings`

## Runtime flow

1. ADC DMA half/full callback marks ready data.
2. Samples are converted from ADC format to float.
3. Per-channel envelope is computed by squaring + FIR.
4. Delay estimator extracts threshold crossings.
5. Latest delays are sent via UART telemetry.
