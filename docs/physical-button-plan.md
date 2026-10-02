# Physical-button test plan and command choices

## Starting point

Movement checkpoint: `daddd31`. The user confirmed bonding, reconnection after an ESP32 power cycle, horn, white/green/red/off lights, and the movement tests. Loss of BLE or controller power while moving remains unverified.

Continue initial breadboard experiments on the USB-powered Waveshare ESP32-S3-Zero already in use. Port to XIAO ESP32-C3 before final wiring and enclosure assembly; it needs a fresh bond and its own pin assignments.

The [standalone button-input experiment](../experiments/button_input_test/README.md) now includes breadboard wiring, both board pin maps, Arduino upload instructions, and press/release checks. The bonding sketch still only reads serial commands; BLE button integration remains a later stage.

The user has a breadboard for these tests. A purple-preset cycle button is also a candidate: each press selects the next preset and wraps at the end; see [purple action-brick research](purple-action-brick-research.md). That research now includes published purple preset configuration packets and a project reporting 10427 support. Preset selection needs testing on our train; playback/cancellation controls are not required for this button.

## Commands available to build on

These are capabilities in our sketch or the linked 10427 implementation, not an exhaustive inventory of every possible hub command.

| Capability | Current evidence | Possible button use |
| --- | --- | --- |
| Forward/reverse/stop | Tested at +50/-50 and zero; protocol uses signed power -100..100 | Timed run, hold-to-run, stop; later handled mainly by lever |
| Horn | Tested via multi-port action | One press plays horn |
| Light colour/off | White, green, red, off tested | Toggle white/off or cycle a chosen palette |
| Other colours | Reference lists yellow, light/dark blue, purple, purple-pink, light-pink, red-pink | Additional palette entries after testing |
| Speaker sounds | Reference IDs: brake 3, station departure 5, water refill 7, horn 9, steam 10 | Dedicated sound or cycle through sounds; direct speaker route still untested here |
| Purple preset selection | Published configuration IDs for night, birthday, beach, rain, cat, recorded, and nothing; untested here | Select next preset per press, wrap at end; use only verified entries |
| Battery request | Reference has request encoder; no local notification handling yet | Status display rather than a dedicated function button |
| Speedometer subscription | Reference supports speed notifications; needs port discovery/parsing | Diagnostics rather than a function button |

Sources: [command encoders](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/protocol/commands.rs), [sound and colour IDs](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/types.rs).

Brake **sound** does not stop the motor. A stop-with-brake button must send motor zero first, then optionally play the sound. Light cycling, boost, timed runs, and departure sequences are behaviours we implement by combining commands. These references do not establish arbitrary audio playback, volume control, or arbitrary RGB colours.

Pair/reconnect, clear bonds, and controller restart are controller operations. Keep bond clearing out of ordinary button presses; later use a deliberate startup hold with visible feedback.

## Prototype wiring

Proposed S3-Zero pins (GPIO numbers, not physical pin positions):

| Button | GPIO | Initial role |
| --- | --- | --- |
| B1 | 4 | Horn |
| B2 | 5 | White/off toggle |
| B3 | 6 | Forward two-second pulse |
| B4 | 7 | Stop and cancel queued movement |

Wire with USB power disconnected. Each normally-open button connects its GPIO to shared GND when pressed. Configure `INPUT_PULLUP`: released reads HIGH, pressed reads LOW. No 5 V connection or external pull-up is needed for the short breadboard wiring.

```text
GPIO4 ---- button B1 ---- GND
GPIO5 ---- button B2 ---- GND
GPIO6 ---- button B3 ---- GND
GPIO7 ---- button B4 ---- GND
```

For four-legged tactile switches, identify the two internally connected terminal pairs with a continuity meter. Use one terminal from each pair so the circuit closes only when pressed. Do not assume orientation from appearance alone.

Check labels against the [Waveshare pinout](https://docs.waveshare.com/ESP32-S3-Zero). This selection avoids the [S3 strapping pins](https://docs.espressif.com/projects/esp-idf/en/v5.0/esp32s3/api-reference/peripherals/gpio.html) GPIO0/3/45/46, native USB pins, and the board LED on GPIO21. Do not transfer these GPIO numbers to the XIAO unchanged.

## Staged work

1. **Button inputs only.** Run [the input test](../experiments/button_input_test/README.md), with no BLE. Start with B1, then add all four. Print one press and one release event per action; use approximately 25 ms of nonblocking debounce. Check repeated presses, a held button, simultaneous presses, and rebooting with a button held. A held button must not repeat events.
2. **Test purple selection over serial first.** Use `p` in the [bonding experiment](../experiments/bonding_test/README.md#next-test-purple-action-brick-presets-virtual-button): Night, Birthday, Beach, Rain, Cat, Nothing, then wrap. Confirm effects with the physical purple brick before assigning a physical button. **Audition the remaining sounds over serial.** Extend a separate BLE experiment with the documented sound IDs and additional colours. Choose favourites after hearing them. Keep motor power zero while auditioning; no wiring changes are needed to change assignments later.
3. **Connect buttons to BLE.** Create `experiments/button_remote_test/`, preserving the current bonding experiment as a known baseline. Use the initial mapping above. Start with horn/light only, then enable the timed movement pulse. Keep serial `x` available. Store button-to-action assignments in one small table so they can be changed easily.
4. **Give stop priority.** Scan inputs before optional BLE actions; stop cancels any queued movement and wins over simultaneous presses. Do not let held buttons at boot/reconnect start the train: require release and a new press. Initially retain the two-second limit and the movement-time restriction on other BLE commands. The current blocking BLE calls must be revisited before promising responsive physical controls during normal continuous driving.
5. **Check failures with drive wheels lifted.** Separately test train power loss, controller USB power loss, and BLE disconnect during a run. Record whether and how quickly the train stops. Confirm reconnect never restarts motion. An ESP32-side timer cannot stop a disconnected train; investigate hub-side behaviour before removing the timed-run limit.
6. **Move to the XIAO, then add the lever.** Recheck bonding and button tests on the C3. Select its pins separately, reserving ADC1 for the potentiometer. Calibrate centre/dead zone and minimum useful power. Stop must override a displaced lever and require returning it to centre before motion can resume.

Acceptance for the first BLE button prototype: one action per press; no action on release except a deliberately designed hold-to-run mode; reliable manual and timed stop; no motion on boot/reconnect; stop wins simultaneous input; unchanged bonds after restart.

## Four-button layouts to try

| Layout | B1 | B2 | B3 | B4 |
| --- | --- | --- | --- | --- |
| Initial bench test, no lever | Horn | Light toggle | Forward pulse | Stop |
| Suggested final layout with lever | Horn | Light toggle/cycle | Water refill | Stop, optionally brake sound |
| Purple-preset alternative with lever | Horn | Light toggle/cycle | Next purple preset | Stop |
| Sound-focused alternative with lever | Horn | Station departure | Water refill or steam | Stop |

Start with short presses only. Add long presses only for a clear need, such as cycling colours or intentional pairing at startup. Keep the physical labels provisional until the sounds and handling have been tried.
