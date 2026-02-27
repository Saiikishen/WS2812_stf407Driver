
# STM32F407 WS2812 (NeoPixel) Driver — TIM1 PWM + DMA (6 LEDs)

This project drives a WS2812/WS2812B LED strip using an STM32F407 timer (TIM1) configured for PWM output and a DMA stream that updates TIM1->CCR1 every 1.25 µs bit period.  
It includes a small API to set pixel colors and run simple patterns (Solid, Rainbow, Breathe, Blink, Chase) on a 6-LED strip (configurable).

---

## 1) Key idea (how it works)

WS2812 LEDs use a 1-wire, self-clocked protocol at **800 kHz** (1.25 µs per bit). Each LED consumes **24 bits** in **GRB** order, then forwards the remaining bits to the next LED.

To generate the precise timing without bit-banging:
- TIM1 runs PWM at 800 kHz (1.25 µs period).
- DMA writes a new duty-cycle value into **CCR1** for every bit time.
- A “reset/latch” low time (>50 µs) is created by appending a tail of 0-duty samples.

---

## 2) Hardware requirements

### 2.1 Wiring
- **STM32 PA8 (TIM1_CH1)** → WS2812 **DIN**
- **STM32 GND** ↔ WS2812 **GND** (must be common ground)
- WS2812 VCC typically **5V** (strip dependent)

### 2.2 Recommended signal integrity parts
- Series resistor on DIN: **330 Ω** (typical range 220–470 Ω)
- Decoupling capacitor at strip input: **100 nF** (and optionally a bulk cap like 100–470 µF)
- Keep the DIN wire short; avoid breadboards for high edge-rate signals.

> Note: A 1 kΩ series resistor often makes DIN edges too slow and can cause bit errors on later LEDs.

---

## 3) STM32CubeMX / `.ioc` configuration (TIM1 + DMA)

### 3.1 Clocks
A typical stable setup for F407:
- SYSCLK = 168 MHz
- APB2 prescaler = /2 → PCLK2 = 84 MHz
- TIM1 clock becomes **168 MHz** (APB2 timer clocks are doubled when prescaler != 1)

### 3.2 TIM1
Configure:
- TIM1 Channel 1: **PWM Generation CH1**
- Prescaler (PSC): `0`
- Counter Period (ARR): `209`  → 168 MHz / (209+1) = 800 kHz
- PWM mode: PWM1
- Polarity: High
- Pulse (CCR1): 0 (DMA will overwrite at runtime)

### 3.3 DMA (recommended mapping)
TIM1_CH1 commonly maps to:
- **DMA2 Stream 1, Channel 6** (recommended if available)

DMA settings:
- Direction: Memory-to-Peripheral
- Mode: Normal (not circular)
- Peripheral increment: Disabled
- Memory increment: Enabled
- Peripheral data width: Half-word (16-bit)
- Memory data width: Half-word (16-bit)
- FIFO: Disabled (Direct mode)

### 3.4 NVIC interrupts
Enable:
- DMA2 Stream1 global interrupt
- TIM1 Update interrupt (depending on HAL callback usage)

---

## 4) Timing parameters (ARR/CCR math)

Assuming TIM1 clock = 168 MHz:

- Bit frequency: 800 kHz → period = 1.25 µs  
- Timer ticks per period: 168e6 / 800e3 = 210 ticks
- ARR = 210 - 1 = **209**

Common duty targets:
- Logic ‘1’: ~2/3 high → CCR ≈ 140  
- Logic ‘0’: ~1/3 high → CCR ≈ 70

Reset:
- Append ~50 “0” samples → 50 × 1.25 µs = 62.5 µs low (latch)

---

## 5) File layout

- `ws2812.h`  
  Public API, constants (`MAX_LED`, `USE_BRIGHTNESS`), and function prototypes.

- `ws2812.c`  
  Implementation: LED buffers, brightness scaling, PWM-bit encoding into `pwmData[]`, DMA start, completion callback, and patterns.

---

## 6) Data structures & buffer sizes

### 6.1 Pixel buffers
- `LED_Data[MAX_LED][4]` holds the “raw” values per LED in GRB format.
- `LED_Mod[MAX_LED][4]` holds the brightness-adjusted values when brightness mode is enabled.

### 6.2 DMA/PWM buffer
- `pwmData[(24 * MAX_LED) + 50]`
  - 24 bits × MAX_LED entries
  - + 50 reset entries

For `MAX_LED = 6`:
- 24×6 = 144 entries for LED bits
- +50 reset entries
- total 194 half-words (388 bytes)

---

## 7) Public API (summary)

### Initialization
```c
WS2812_Init(&htim1, TIM_CHANNEL_1);
```

### Set a single LED
```c
WS2812_SetLED(index, red, green, blue);
```
Internally stored as GRB.

### Brightness
```c
WS2812_SetBrightness(brightness);
```
Your current implementation clamps brightness to a fixed max scale (example: 0–45) and uses integer scaling to fill `LED_Mod[][]`.

### Send frame
```c
WS2812_Send();
```
Encodes the LED data into `pwmData[]`, starts TIM PWM with DMA, then blocks until DMA completion flag is set by the callback.

### Clear (turn off)
```c
WS2812_Clear();
```
Typically sets LEDs off and sends a blank frame; may also clear buffers.

---

## 8) Pattern functions

### Solid color
Shows the same color on all LEDs.
```c
WS2812_Pattern_SolidColor(255, 0, 0);   // red
WS2812_Pattern_SolidColor(0, 255, 0);   // green
WS2812_Pattern_SolidColor(255, 140, 0); // orange
```

### Chase
One LED moves along the strip.
```c
WS2812_Pattern_Chase(0, 255, 0, 100);   // green chase, 100ms step
```

### Blink
All LEDs blink simultaneously.
```c
WS2812_Pattern_Blink(0, 0, 255, 3, 300, 300); // blue blink 3x
```

### Rainbow
Generates a simple HSV-based gradient across LEDs.

### Breathe
Fades brightness up/down for a given color (ensure brightness scale matches your SetBrightness implementation).

---

## 9) Example `main.c` usage

### Minimal setup
```c
MX_GPIO_Init();
MX_DMA_Init();
MX_TIM1_Init();

WS2812_Init(&htim1, TIM_CHANNEL_1);

while (1)
{
    WS2812_Pattern_SolidColor(255, 0, 0);
    HAL_Delay(2000);

    WS2812_Pattern_Chase(0, 255, 0, 100);

    WS2812_Pattern_Blink(0, 0, 255, 1, 500, 500);
}
```

---

## 10) Notes on debug vs release behavior

WS2812 timing is tight. When debugging (single stepping, live register views, halts), the debugger can disturb timing and peripheral/DMA behavior.  
If you see “random” LED behavior only in debug sessions but not in standalone flash/run, prefer testing in release mode or avoid halting the CPU during active LED DMA transfers.

---

## 11) Troubleshooting

### Symptom: Last LEDs show old colors / partial update
Possible causes:
- Not clearing both “raw” and “modified” buffers consistently between patterns
- Partial frame latch due to insufficient reset time
- Data line integrity issues mid-strip (bad connection around LED3/LED4)

### Symptom: Specific LED index always wrong/off
Likely hardware:
- A bad solder joint between two LEDs
- A damaged LED that fails to forward data properly
- DIN edge too slow (too large series resistor, long wires, weak logic-high level)

### Symptom: Works sometimes, fails sometimes
Likely timing/config:
- ARR/CCR computed for wrong actual TIM clock
- Wrong PWM mode/polarity
- Wrong DMA stream/channel mapping
- DMA interrupts not enabled (callback never fires)

---

## 12) Extending

### More LEDs
Change:
```c
#define MAX_LED  6
```
Rebuild; buffers and patterns scale automatically.

### Two-zone patterns (first 3 LEDs vs last 3 LEDs)
Implement helpers like:
- `WS2812_SetZone(start, end, r,g,b)`
- Zone-specific chase/blink loops
Then call `WS2812_Send()` once per frame so both zones update together.
