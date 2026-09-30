# DUPLO 10427 bonding test

A standalone Arduino sketch for the **Waveshare ESP32-S3-Zero**. Only a USB data cable and the train are needed. It tests BLE connection, encryption, bonding, and bond persistence. It sends no motor, light, or sound commands.

## Install and upload with Arduino IDE

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software).
2. Open Settings/Preferences. Add this to **Additional Boards Manager URLs**:

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. In Boards Manager, install **esp32 by Espressif Systems**, version **3.3.0**. In Library Manager, install **NimBLE-Arduino by h2zero**, version **2.3.6**. These are the intended dependency versions, not a claim of a verified build.
4. Open `bonding_test.ino` from this folder. Keep the sketch inside the matching `bonding_test` folder.
5. Select **Tools → Board → esp32 → ESP32S3 Dev Module**. Set:

   | Setting | Value |
   | --- | --- |
   | USB CDC On Boot | Enabled |
   | USB Mode | Hardware CDC and JTAG |
   | Upload Mode | UART0 / Hardware CDC |
   | Flash Size | 4MB (32Mb) |
   | Partition Scheme | Default 4MB with spiffs |
   | PSRAM | Disabled (unused by this test) |
   | Erase All Flash Before Sketch Upload | Disabled |

   Leave other settings at their defaults. These settings target the S3-Zero, not the later XIAO ESP32-C3.
6. Connect the board with a **USB data cable**. Select its port under **Tools → Port**; on macOS it normally looks like `/dev/cu.usbmodem...`.
7. Click **Verify**, then **Upload**. If the board is not detected or upload stalls: hold **BOOT**, press and release **RESET**, then release **BOOT**. Select the newly appearing port and upload again. If needed, unplug the board and reconnect it while holding BOOT, then release BOOT.
8. After upload, press **RESET** if the sketch does not start. Open **Serial Monitor** at **115200 baud**. Select the port again if it changed after flashing. Either newline setting works; commands are single lowercase characters.

## Run the test

1. Close the LEGO app and disconnect any other controller from the train. Turn off other nearby LEGO BLE hubs: the first pairing scan requires exactly one hub advertising the LEGO service, which is not unique to model 10427.
2. Turn on/wake the train and keep it near the ESP32.
3. In Serial Monitor, send `s`. A fresh ESP32 should show `Stored bonds: 0`.
4. Send `c`. The initial scan takes five seconds, followed by connection and security negotiation. Look for:

   ```text
   PASS: encrypted and bonded connection. Check persistence after a power cycle.
   Stored bonds: 1
   ```

   Status should include `encrypted=1`, `bonded=1`, and `Peer bond in storage: 1`. `authenticated=0` is normal for Just Works pairing without a PIN/MITM check.
5. Send `d`, then `c` to test reconnection.
6. **Unplug and reconnect ESP32 USB power without uploading again.** Keep the train awake. The sketch should load one stored bond and attempt to reconnect automatically. Reopen Serial Monitor and send `s` if you missed the startup output. If the train was asleep, wake it and send `c`.
7. Confirm the stored peer identity is unchanged and the connection is encrypted and bonded. Repeat after turning the train off and on, then send `c`.
8. To intentionally test fresh pairing, send `f`, confirm `Stored bonds: 0`, then send `c`.

| Command | Action |
| --- | --- |
| `c` | Connect to the stored peer, or scan and pair if there are no bonds |
| `s` | Print bond storage and current security status |
| `d` | Disconnect; retain the bond |
| `f` | Disconnect and delete **all ESP32-side BLE bonds** |
| `r` | Restart the ESP32; retain the bond |
| `h` | Print command help |

NimBLE stores ESP32 bonds in flash (NVS). Do not erase flash or change partitions during the persistence test. `f` does not delete the train's copy of the bond. If fresh pairing fails, power-cycle the train, close other BLE clients, and retry; record the printed error if it still fails.

## Limits and troubleshooting

- This is a diagnostic sketch, not the remote firmware. It makes one startup reconnection attempt; subsequent attempts require `c`. Scanning and security negotiation temporarily block serial command processing.
- A BLE link alone is not a pass. Require encryption, bonding, and a stored peer bond. A successful reboot test shows persistence and reconnection; these status flags alone cannot prove that the stack never negotiated replacement keys.
- Stored-peer reconnection uses its saved identity address. If first pairing works but reconnecting fails, record the addresses and error codes; address privacy/resolution may need investigation rather than repeatedly clearing bonds.
- Blank monitor: check USB CDC settings, the selected port and data cable, press RESET, or send `h`. The sketch waits at most five seconds for the monitor.
- No hub found: wake the train, close the phone app, and retry `c`. Multiple hubs found: turn off the others.
- This test does not verify LWP3 command acceptance or what happens when a moving train loses connection. Those are separate hardware tests.

## References and validation

- [Espressif Arduino installation](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
- [Waveshare ESP32-S3-Zero setup](https://www.waveshare.com/wiki/ESP32-S3-Zero)
- [NimBLE-Arduino 2.3.6 client API](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEClient.h)
- [NimBLE security and bond-storage API](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEDevice.h)
- [10427 bonding background](https://github.com/micschr0/duplo-train-10427-ble2mqtt)

The sketch's API calls were checked against NimBLE-Arduino 2.3.6 headers. Arduino IDE and its bundled CLI are available on this Mac, but the ESP32 board package and Arduino libraries were not found in their default locations. Compilation and physical testing have not been performed; install the dependencies above and click Verify before uploading.
