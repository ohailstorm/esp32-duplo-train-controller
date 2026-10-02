# DUPLO 10427 bonding test

A standalone Arduino sketch for the **Waveshare ESP32-S3-Zero**. Only a USB data cable and the train are needed. It tests BLE connection, encryption, bonding, bond persistence, horn playback, and light colours. It now includes explicitly requested two-second motor tests.

## Physical buttons and purple-brick presets

For breadboard wiring and a separate Arduino sketch, open the [physical-button input test](../button_input_test/README.md). It supports both boards and verifies presses/releases before BLE controls are added.

The [purple action-brick guide](../../docs/purple-action-brick-research.md) documents preset selection commands and remaining hardware checks. Send `p` in this sketch to test purple selection over serial before adding physical BLE buttons. See the [button plan](../../docs/physical-button-plan.md) for proposed assignments.

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
| `w` / `v` | Forward +50 / reverse -50 power for two seconds |
| `x` | Stop motor and cancel queued movement |
| `c` | Connect to the stored peer, or scan and pair if there are no bonds |
| `s` | Print bond storage and current security status |
| `1` / `2` / `3` | Set the train light to white / green / red |
| `0` | Turn the train light off |
| `p` | Select next purple-brick preset (Night → Birthday → Beach → Rain → Cat → Nothing → repeat) |
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

## Next test: movement

Verified light controls are saved in commit `b6e9b1d`. Upload this version without erasing flash or changing partitions.

1. For the first test, hold the train with its drive wheels clear of the surface. Keep its power button accessible.
2. Connect with `c` if needed. The sketch writes a motor stop after securing each connection; it never resumes a previous run.
3. Send `w`: a stop is written first, followed by a nonblocking 300 ms pause, then +50 motor power. After two seconds the ESP32 writes stop.
4. Send `v` for the same test at -50 power. Observe that the wheels turn in the opposite direction.
5. Send `w`, then send `x` before the timer expires to check manual stop. `x` also cancels movement during the 300 ms pause.
6. Send `w`, then `v` while moving to check stop-before-reverse. Each new movement command stops the previous run before the pause and next run.
7. Once wheel-up tests pass, repeat on clear track. Confirm physical stopping; a successful BLE write is not motor feedback.

`w` and `v` use fixed power values, not measured speed. Each command starts a new two-second run. While a movement or stop retry is active, other commands are rejected so they cannot delay stop with a BLE discovery or disconnect operation. Send `x` before using lights, horn, disconnect, restart, or bond deletion. `0` still means **light off**, not motor stop.

A failed stop write is retried every 250 ms while connected. This is an ESP32-side timer, not a train-side watchdog: USB power loss, BLE loss, or a blocked BLE call can prevent or delay stop. If the train keeps moving, use its power button. Connection-loss behaviour is not yet verified; do not unplug the controller while testing on track.

Motor packets follow the [10427 command encoder](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/protocol/commands.rs), using port `0x32` and signed power. The user reported all movement tests working on the real train.

## Next test: purple action-brick presets (virtual button)

Use this existing sketch and Serial Monitor first; no button wiring is needed.

1. Upload `bonding_test.ino` using the settings above, without erasing flash or changing partitions.
2. Close the LEGO app, wake the train, and send `c` if needed. Send `x` to stop before changing presets.
3. Send **`p`** once: the first request is **Night**, ID `0x03`. Trigger the train's sensor with the physical purple action brick and listen. Confirm whether this is lullaby; the source calls it Night.
4. Send **`p`** again: **Birthday**, ID `0x04`. Trigger the purple brick again and check for Happy Birthday.
5. Further `p` commands select **Beach → Rain → Cat → Nothing**, then wrap to Night. This is our test order, not a claim about the app's ordering. Check each selection individually; Nothing is expected to disable the assigned action, not necessarily stop a sound already playing.

Each `p` is one virtual button press. It configures the purple action; it does not command immediate playback. Use a controlled area if running the train over the brick, and keep its power button accessible. The existing `w`/`v` tests remain limited to two seconds; `x` stops movement. Preset writes are rejected while a timed movement or stop retry is active.

The cycle advances only after a successful BLE write; that does not confirm the train accepted the preset. A failed/disconnected request is not queued and does not advance. Reconnecting or rebooting resets the local cycle so the next `p` requests Night; no preset is sent automatically. The sketch does not read the train's current selection or save the cycle position in flash.

Recorded sound (`0x06`) is excluded because it depends on an existing recording; this test cannot upload audio. These published commands are **not yet verified on our train**. See [the packet table and sources](../../docs/purple-action-brick-research.md). Report the printed preset name and what the train does after encountering the brick.

## Limits and troubleshooting

- This is a diagnostic sketch, not the remote firmware. It makes one startup reconnection attempt; subsequent attempts require `c`. Scanning and security negotiation temporarily block serial command processing.
- A BLE link alone is not a pass. Require encryption, bonding, and a stored peer bond. A successful reboot test shows persistence and reconnection; these status flags alone cannot prove that the stack never negotiated replacement keys.
- Stored-peer reconnection uses its saved identity address. If first pairing works but reconnecting fails, record the addresses and error codes; address privacy/resolution may need investigation rather than repeatedly clearing bonds.
- Blank monitor: check USB CDC settings, the selected port and data cable, press RESET, or send `h`. The sketch waits at most five seconds for the monitor.
- No hub found: wake the train, close the phone app, and retry `c`. Multiple hubs found: turn off the others.
- Hearing the horn verifies this LWP3 action. The user confirmed the movement tests; connection-loss behaviour remains unverified.

## References and validation

- [Espressif Arduino installation](https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html)
- [Waveshare ESP32-S3-Zero setup](https://www.waveshare.com/wiki/ESP32-S3-Zero)
- [NimBLE-Arduino 2.3.6 client API](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEClient.h)
- [NimBLE security and bond-storage API](https://github.com/h2zero/NimBLE-Arduino/blob/2.3.6/src/NimBLEDevice.h)
- [10427 bonding background](https://github.com/micschr0/duplo-train-10427-ble2mqtt)

The updated sketch compiled successfully with Arduino IDE's bundled CLI, ESP32 core 3.3.12, and NimBLE-Arduino 2.5.1, using the S3 settings above. The user confirmed bonding, horn playback, and successful reconnection with horn playback after an ESP32 power cycle on the real train.

The user also confirmed white, green, red, and light-off controls on the train.

Movement build: compilation passed with ESP32 core 3.3.12 and NimBLE-Arduino 2.5.1 (565,221 bytes flash; 32,884 bytes global RAM). A temporary host harness exercised the actual movement functions with simulated time and BLE writes: timed stop, queued-movement cancellation, stop-before-reverse, failed-stop retry, disconnect cancellation, and timer rollover passed. This does not replace testing the physical motor or BLE failure behaviour.

Hardware result: the user reported all movement tests working. Connection-loss and controller-power-loss behaviour have not been separately confirmed.
