# DUPLO 10427 bonding test

A standalone Arduino sketch for the **Waveshare ESP32-S3-Zero**. Only a USB data cable and the train are needed. It tests BLE connection, encryption, bonding, bond persistence, horn playback, and light colours. It sends no motor commands.

## Install and upload with Arduino IDE

1. Install [Arduino IDE 2](https://www.arduino.cc/en/software).
2. Open Settings/Preferences. Add this to **Additional Boards Manager URLs**:

   ```text
   https://espressif.github.io/arduino-esp32/package_esp32_index.json
   ```

3. In Boards Manager, install **esp32 by Espressif Systems**, version **3.3.12**. In Library Manager, install **NimBLE-Arduino by h2zero**, version **2.5.1**. These match the locally verified build.
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
8. After upload, press **RESET** if the sketch does not start. Open **Serial Monitor** at **115200 baud**. Select the port again if it changed after flashing. Either newline setting works; commands are single characters (letters are lowercase).

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
| `1` / `2` / `3` | Set the train light to white / green / red |
| `0` | Turn the train light off |
| `b` | Play the horn (requires an encrypted, bonded connection) |
| `d` | Disconnect; retain the bond |
| `f` | Disconnect and delete **all ESP32-side BLE bonds** |
| `r` | Restart the ESP32; retain the bond |
| `h` | Print command help |

NimBLE stores ESP32 bonds in flash (NVS). Do not erase flash or change partitions during the persistence test. `f` does not delete the train's copy of the bond. If fresh pairing fails, power-cycle the train, close other BLE clients, and retry; record the printed error if it still fails.

## Next test: horn

The initial bonding-only version is saved in commit `b319775`. The user confirmed the initial connection, horn playback, and horn playback again after an ESP32 power cycle on the real train.

1. Upload the updated sketch with **Erase All Flash Before Sketch Upload disabled**, keeping the same partition scheme to retain the bond.
2. Wake the train. Open Serial Monitor at **115200 baud**. Send `c` if it did not reconnect automatically, then `s` to check encryption and bonding.
3. Send **`b`** once. You should hear the horn. The message `Horn frame written` only reports a successful BLE write, not confirmation that the train played the sound.
4. If the write succeeds but the train is silent, save the serial output. Do not clear the bond as the first troubleshooting step.

The packet uses the [10427 reference implementation's horn action](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/protocol/commands.rs). No sound is sent automatically at startup.

## Next test: lights

The working horn and power-cycle checkpoint is commit `f84ebf7`.
Upload this sketch without erasing flash, reconnect, then send `1` (white), `2` (green), `3` (red), and `0` (off), one at a time. Observe the train light after each command. `b` still plays the horn. No light or sound command runs automatically.

Light colour values come from the [10427 reference colour mapping](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/types.rs). A successful write does not prove the light changed; this test needs visual confirmation.

## Limits and troubleshooting

- This is a diagnostic sketch, not the remote firmware. It makes one startup reconnection attempt; subsequent attempts require `c`. Scanning and security negotiation temporarily block serial command processing.
- A BLE link alone is not a pass. Require encryption, bonding, and a stored peer bond. A successful reboot test shows persistence and reconnection; these status flags alone cannot prove that the stack never negotiated replacement keys.
- Stored-peer reconnection uses its saved identity address. If first pairing works but reconnecting fails, record the addresses and error codes; address privacy/resolution may need investigation rather than repeatedly clearing bonds.
- Blank monitor: check USB CDC settings, the selected port and data cable, press RESET, or send `h`. The sketch waits at most five seconds for the monitor.
- No hub found: wake the train, close the phone app, and retry `c`. Multiple hubs found: turn off the others.
- Hearing the horn verifies this LWP3 action. Motor control and what happens when a moving train loses connection remain separate hardware tests.

## References and validation

- [Espressif Arduino installation](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
- [Waveshare ESP32-S3-Zero setup](https://www.waveshare.com/wiki/ESP32-S3-Zero)
- [NimBLE-Arduino 2.3.6 client API](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEClient.h)
- [NimBLE security and bond-storage API](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEDevice.h)
- [10427 bonding background](https://github.com/micschr0/duplo-train-10427-ble2mqtt)

The updated sketch compiled successfully with Arduino IDE's bundled CLI, ESP32 core 3.3.12, and NimBLE-Arduino 2.5.1, using the S3 settings above. The user confirmed bonding, horn playback, and successful reconnection with horn playback after an ESP32 power cycle on the real train.

The user also confirmed white, green, red, and light-off controls on the train.
