# STM32 Multi-GHz Digital RF Prescaler & Division Controller

A bare-metal C hardware control implementation running on an **STM32C0** microcontroller. This repository provides a deterministic, low-latency interface for configuring multi-GHz RF prescalers, integer frequency dividers, and synthesizer front-ends in benchtop and automated test environments.

> **Architecture & Clean-Room Note:**  
> This open-source repository serves as an abstracted proof-of-concept (PoC) reference driver (~40% reduced footprint compared to proprietary production builds). All internal company part numbers, proprietary RF front-end telemetry, custom register maps, and internal diagnostics have been sanitized for open-source reference and portfolio demonstration.

---

## System Architecture

In multi-gigahertz RF synthesis and characterization benches, high-frequency signals exceed the direct input capabilities of standard phase detectors, counters, and digital PLLs. High-speed prescaler ICs divide these ultra-high frequencies down to intermediate bands.

This controller bridges human/host inputs with the physical divider hardware:

```text
[ Rotary Encoder / Button ] ──> [ STM32C0 State Machine ] ──> [ Parallel Bus: PA0-PA6 ] ──> [ RF Prescaler IC ]
                                         │
                                         └──> [ Software Remap Layer ] ──> [ TM1637 / 7-Segment ]
```

### Core Subsystems

- **Deterministic 7-Bit Parallel Output Bus (PA0–PA6):**
  Drives hardware division ratios (div 0 through div 127) using direct port register bitmask operations (`GPIOA->ODR`). Eliminates serial communication latency and bus jitter during division state changes.

- **Draft-Staging State Machine:**
  Incoming quadrature encoder steps adjust a staged "draft" value rather than immediately shifting the output bus. Prevents destructive intermediate frequency transients in sensitive downstream RF instrumentation while dialing.

- **Software-Level Hardware Abstraction (Display Remapping):**
  Demonstrates a clean separation of concerns: physical pinout permutations and trace crossover anomalies are handled purely in application configuration (`main.c`), leaving core state-machine drivers (`RF_Control.c`) portable.

---

## Hardware Configuration & Pinout

| Signal | Pin | Direction | Description |
|---|---|---|---|
| DIV_0 – DIV_6 | PA0 – PA6 | Output | 7-bit parallel hardware divider control bus |
| ENC_BTN | PA7 | Input | Active-low pushbutton with hardware debounce/gestures |
| PHASE_A | PB6 | Input | Quadrature encoder channel A |
| PHASE_B | PB7 | Input | Quadrature encoder channel B |
| TM_CLK | PA11 | Output | 7-segment display serial clock |
| TM_DIO | PA12 | Output | 7-segment display serial data |

---

## Board-Level Trace Remapping in `main.c`

To accommodate board layout routing constraints (such as avoiding high-density trace crossovers near the display interface without adding extra PCB routing layers), segment bit assignments can be abstracted via a translation layer.

### Segment Map Definition

Users can define physical hardware layouts directly in `main.c`:

```c
// maps standard 7-segment indices (A, B, C, D, E, F, G, DP) to board-specific traces
static const uint8_t PCB_LAYOUT_MAP[8] = { 1, 2, 6, 5, 4, 0, 3, 7 };

static uint8_t Remap_PCB_Segments(uint8_t std_seg, const uint8_t *seg_map) {
    uint8_t pcb_seg = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if (std_seg & (1 << i)) {
            pcb_seg |= (1 << seg_map[i]);
        }
    }
    return pcb_seg;
}
```

This ensures that routing alterations or hardware revisions do not require touching the underlying RF state machine.

---

## Gesture Control & State Handling

- **Parameter Dialing:** Rotating the encoder increments/decrements `draft_div` and initiates decimal-point blinking to indicate uncommitted state.
- **Single Click (PA7):** Latches the draft value to `active_div` and immediately writes the byte to `GPIOA->ODR`.
- **Triple Click (PA7):** Safety shortcut that instantly resets the hardware divider output to div 0.
- **Idle Timeout (5 seconds):** Automatically aborts uncommitted draft values and reverts the display to the active hardware state.
- **Long Hold (5 seconds):** Asserts an atomic hardware reset (`NVIC_SystemReset()`).

---

## Repository Structure

```
RF-Prescaler-STM32/
├── README.md # System architecture and hardware documentation
├── Inc/
│ ├── RF_Control.h # Prescaler state machine interface
│ ├── tm1637.h # Display driver header
│ └── main.h # MCU peripheral defines
└── Src/
├── RF_Control.c # Low-level parallel bus & gesture engine
├── tm1637.c # Serial display communication
└── main.c # System clock, GPIO init, and board abstraction
```

---

## Dependencies & Attribution

- Built with ARM CMSIS and STM32 HAL (`stm32c0xx_hal`).
- 7-segment low-level serial routines adapted from the open-source library by [nimaltd/tm1637](https://github.com/nimaltd/tm1637).
