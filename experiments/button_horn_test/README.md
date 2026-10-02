# Physical button → train horn

Open **button_horn_test.ino** in this folder. B1 plays the same horn already verified through serial `b`. This is a separate BLE experiment; the input-only and serial-command sketches remain available. No motor, light, or purple-preset commands are sent by this sketch.

## Wiring and upload

Keep the working B1 wiring: **S3-Zero GPIO4 → normally-open button → GND**. On **XIAO ESP32-C3**, use **D3 (GPIO5)** instead. Other buttons may stay wired but are unused. See the [breadboard guide](../button_input_test/README.md#your-kjell-breadboard-step-by-step) for terminal pairing and rows.

1. Install **esp32 by Espressif Systems 3.3.12** and **NimBLE-Arduino by h2zero 2.5.1**.
2. Open this folder's `button_horn_test.ino` in Arduino IDE.
3. For S3-Zero select **ESP32S3 Dev Module**, not Ozobot DRVKit. Set **USB CDC On Boot: Enabled**, **USB Mode: Hardware CDC and JTAG**, **Upload Mode: UART0 / Hardware CDC**, **Flash Size: 4MB**, **Partition Scheme: Default 4MB with spiffs**, **PSRAM: Disabled**.
4. For XIAO select **XIAO_ESP32C3**, enable **USB CDC On Boot**, and use the board defaults.
5. Keep **Erase All Flash Before Sketch Upload: Disabled** and retain the partition scheme used previously on that board. The sketch reuses stored BLE bonds and never deletes them. Switching to the other board requires pairing on that board.
6. Select the USB port and upload. Reselect the port if it changes. Open Serial Monitor at **115200 baud** and press RESET. The sketch rejects disabled USB CDC and unsupported board selections during compilation.

## Run

1. Close the LEGO app and wake the train. Keep other LEGO hubs off for initial pairing.
2. With one stored bond, the sketch makes one automatic reconnection attempt at boot. Otherwise send **`c`** to connect/pair. If the train was asleep or connection fails, wake it and send `c` again.
3. Wait for **`PASS: encrypted and bonded connection`**, release B1, and look for **`B1 ready: press for horn.`**
4. Press B1 normally: expect `B1 pressed`, a horn-write message, and the horn. Release and press again to repeat. A held button does not repeat; debounce is 25 ms.
5. Hold B1 while rebooting or reconnecting: no horn should sound until you release it and press again. Presses made while disconnected are discarded.
6. Power-cycle the ESP32 with the train awake. Confirm reconnection, the stored bond, and horn operation again.

| Serial command | Action |
| --- | --- |
| `c` | Connect/pair or show status if already connected |
| `s` | Print bonds and encryption status |
| `b` | Play horn directly, to distinguish BLE issues from button issues |
| `d` | Disconnect, retaining bond |
| `h` | Help |

If `b` works but B1 does not, use the [input-only sketch](../button_input_test/README.md) to check wiring. If neither works, inspect connection/security output. A successful BLE write is not confirmation of audible playback. BLE calls can block; this is a horn test, not a continuously responsive motor controller. It does not stop a train that is already moving, so begin with the train stationary.

Purple presets remain in the [serial virtual-button test](../bonding_test/README.md#next-test-purple-action-brick-presets-virtual-button) until verified. To intentionally clear a bond, use the bonding experiment's `f` command; ordinary button presses never clear bonds.

## Validation

Compiled with ESP32 core 3.3.12 and NimBLE-Arduino 2.5.1 for both supported boards with USB CDC enabled: S3 563,809 bytes flash / 32,868 bytes global RAM; C3 578,843 / 22,644. A temporary host harness exercised the actual button state functions: startup hold, bounce, one horn per press, no hold repeat, disconnected presses, reconnect hold, security gating, and timer rollover passed. Audible playback from a physical button still needs testing on your train.

For CLI builds, the USB CDC option identifiers differ: S3 uses `CDCOnBoot=cdc` for Enabled; XIAO C3 uses `CDCOnBoot=default` for Enabled. In Arduino IDE simply select the displayed **Enabled** value.
