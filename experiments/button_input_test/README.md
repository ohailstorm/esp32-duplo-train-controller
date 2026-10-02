# Physical buttons: breadboard input test

Open **button_input_test.ino** in this folder. This separate sketch tests four switches and their wiring, printing one press and release event per action. It does not connect to or move the train. Keep the working [BLE experiment](../bonding_test/README.md) as the train-command baseline.

## Parts and wiring

Use your breadboard, four normally-open tactile buttons, jumper wires, and a USB data cable. The board needs securely soldered headers or wires; loose wires through unsoldered holes are unreliable.

Unplug USB before wiring. Connect board **GND** to a breadboard ground rail. Connect each button between its GPIO and that rail:

| Button | Waveshare ESP32-S3-Zero label | XIAO ESP32-C3 label (GPIO) |
| --- | --- | --- |
| B1 | GPIO4 / IO4 | D3 (GPIO5) |
| B2 | GPIO5 / IO5 | D4 (GPIO6) |
| B3 | GPIO6 / IO6 | D5 (GPIO7) |
| B4 | GPIO7 / IO7 | D10 (GPIO10) |

Use only the column for your board. These are pin labels, not header positions. The XIAO mapping leaves D1/ADC1 available for the later potentiometer and avoids its boot strapping pins. Check the official [Waveshare pinout](https://docs.waveshare.com/ESP32-S3-Zero) or [Seeed XIAO pinout](https://wiki.seeedstudio.com/XIAO_ESP32C3_Getting_Started/).

```text
B1 GPIO ---- normally-open button ---- GND rail
B2 GPIO ---- normally-open button ---- GND rail
B3 GPIO ---- normally-open button ---- GND rail
B4 GPIO ---- normally-open button ---- GND rail
                                      |
                                  board GND
```

The sketch enables internal pull-ups: released = HIGH, pressed = LOW. **Do not connect the buttons to 5 V or 3.3 V.** No external resistors are required for this test.

Four-legged switches have two permanently connected terminal pairs. With USB unplugged, use a continuity meter to identify the pairs and wire one terminal from each pair. The chosen terminals should connect only when pressed. Place the switch so breadboard strips do not short those pairs together; straddling the centre gap often helps, but verify your switch layout. If a ground rail is split halfway along the breadboard, bridge it or use a single continuous section.

Start with B1 alone, then add B2–B4. Unconnected inputs remain released because of the pull-ups.

## Your Kjell breadboard: step by step

The [photo you linked](https://www.kjell.com/globalassets/productimages/565314_36283.jpg) shows standard solderless breadboards with numbered rows, columns **a–e** and **f–j**, a centre gap, and red/blue side rails. Use one board for this test. The photo shows the breadboard, not the switch's underside, so it cannot establish your button's internal terminal pairing or exact leg spacing.

### Understand which holes connect

In the main area, the five holes with the **same row number on the same side** connect internally. For example:

```text
       connected group           separate connected group
10     a -- b -- c -- d -- e     GAP     f -- g -- h -- i -- j
11     a -- b -- c -- d -- e     GAP     f -- g -- h -- i -- j
       row 10 and row 11 are NOT connected internally
```

So `a10` connects to `b10` through `e10`, but not to `a11` or `f10`. The centre gap electrically separates the two halves.

The side rails work lengthwise, unlike the five-hole groups. The blue **−** rail will be our ground rail **only after you connect it to ESP32 GND**. The red **+** rail is unused. Rails on opposite sides are not automatically joined; some rails also have a break halfway along. Use one verified continuous rail section for all four buttons, or bridge sections with a jumper. Printed coloured lines alone do not prove continuity.

### Identify the button contacts

1. Disconnect USB. If your tactile switch has four legs, label its two electrically common pairs **A** and **B** using a multimeter's continuity mode: the two A legs connect even when released, as do the two B legs. An A leg connects to a B leg only while pressed.
2. Use **one A leg and one B leg** for the two wires. The other legs need no wires, but their breadboard placement must not short A and B together.
3. Insert the switch across the centre gap if its spacing fits, with A and B landing on separate electrical groups. Do not force or sharply bend the legs. A 12 mm switch may not fit the convenient positions used for smaller breadboard buttons. If it does not fit, use secure insulated leads from its contacts into the breadboard instead.

This is the electrical circuit, **not a top-view pinout of the switch**:

```text
ESP32 GPIO ---- A terminal ---- / ---- B terminal ---- ESP32 GND
                              closes
                           when pressed
```

Without a meter, the input sketch can help check a candidate terminal pair: `B1 ready` when released, `B1 pressed` only when pressed, then `B1 released`. An always-held input suggests you chose the same internally connected pair or shorted separate contacts through a breadboard row. Unplug USB before changing wiring.

### Wire and test B1 first

1. Leave USB disconnected. Run a jumper from the ESP32 pin labelled **GND** to the breadboard's chosen blue **−** rail. Leave **5V**, **3V3**, and the red rail unconnected.
2. Place the button as above. Note the row and side occupied by your chosen A terminal. Put the GPIO jumper into a **free hole in that same five-hole group**: use **GPIO4/IO4 on S3-Zero**, or **D3 on XIAO ESP32-C3**.
3. Put a second jumper into a free hole in the group occupied by the chosen B terminal. Connect its other end to the same blue rail as ESP32 GND.
4. Check that A and B are not already connected by a breadboard strip or by the unused switch legs. Every jumper must reach the metal contact inside its hole.
5. Connect USB, upload `button_input_test.ino` as below, and open Serial Monitor at **115200 baud**. B1 should report `ready`, then one `pressed` and one `released` per tap.

For a concrete hole-number example **when using two leads from a button outside the breadboard**, put the A lead in `a10` and the B lead in `a15`. Connect `e10` to the B1 GPIO and `e15` to the blue ground rail:

```text
B1 GPIO ---- e10 == internal row strip == a10 ---- button A lead
GND rail --- e15 == internal row strip == a15 ---- button B lead
```

Rows 10 and 15 are examples for the **wire ends**, not a claim that the switch's legs fit those holes. With a directly inserted switch, use its actual contact rows and sides instead.

### Add B2, B3, and B4

Unplug USB again. Repeat the same two connections for each button, using the GPIO table above. Give each button's **signal contact its own separate breadboard group**; all four ground contacts connect to the same ground rail. Leave enough room around the button bodies to avoid overlaps. No extra power wire or resistor is needed.

If the ESP32 covers the holes you need, keep it beside the breadboard and use jumpers to its soldered headers. Do not push bare wires loosely through the board's header holes. Reconnect USB and run the checks below for all four buttons.

## Upload with Arduino IDE

1. Install **esp32 by Espressif Systems 3.3.12** in Boards Manager, as described in the [existing installation guide](../bonding_test/README.md#install-and-upload-with-arduino-ide). This input-only sketch needs no extra libraries.
2. Open `experiments/button_input_test/button_input_test.ino`. Keep it in its matching folder, separate from `bonding_test.ino`.
3. For **S3-Zero**, select **ESP32S3 Dev Module** and use the settings in that guide: USB CDC enabled, Hardware CDC and JTAG, 4 MB flash, default 4 MB partition, PSRAM disabled.
4. For **XIAO ESP32-C3**, select **XIAO_ESP32C3**, enable **USB CDC On Boot**, and keep the board's other defaults. The sketch selects its pin map automatically.
5. Keep **Erase All Flash Before Sketch Upload disabled** and retain the partition scheme previously used on that board. Uploading replaces the running sketch, but this sketch does not clear stored BLE bonds.
6. Connect USB, select **Tools → Port**, click **Verify**, then **Upload**. If upload cannot connect, hold BOOT, press/release RESET, release BOOT, and reselect the port. Reset after uploading if needed.
7. Open Serial Monitor at **115200 baud**, then press RESET to see the startup output.

## Board selection matters: Ozobot is not the S3-Zero

If Arduino suggests **Ozobot DRVKit**, manually select **ESP32S3 Dev Module** for the Waveshare board before uploading. Ozobot's default Arduino pin numbering maps numbers 4/5/6/7 to hardware GPIO45/46/39/40. The original input sketch accepted that S3 configuration but printed misleading GPIO4–7 labels. That can explain released inputs that never react when the actual GPIO4–7 pins are grounded. It is a possible explanation, not confirmation of the board setting used for an earlier upload.

The updated sketch rejects remapped pin numbering and unsupported board selections at compile time, and prints the compiled board name at startup. Reupload with the correct board selected; no wiring change is required for the documented S3 GPIO4–7 layout.

## Check the buttons

1. With all buttons released, expect `B1 ready` through `B4 ready`.
2. Press and release B1. Expect exactly `B1 pressed` followed by `B1 released`. Repeat for each button.
3. Hold a button for several seconds: there should be no repeated press events.
4. Press two buttons together: both should report independently.
5. Hold a button while resetting. It should produce no press event until you release it, see `ready`, and press again.

The sketch waits for 25 ms of stable input, so extremely short taps may be ignored. A permanently held/not-ready input usually means the switch terminals or breadboard strips are shorted. No response usually means a missing ground, incorrect pin, or poor contact. A blank monitor usually means the wrong port or USB CDC setting.

## Train controls and the purple brick

Test purple presets first with **`p` in the [virtual-button BLE sketch](../bonding_test/README.md#next-test-purple-action-brick-presets-virtual-button)**. A separate [B1 horn experiment](../button_horn_test/README.md) is now available after wiring checks pass; this sketch does not yet assign train actions to buttons. Candidate layout: **horn / light cycle / next purple preset / stop**, with motion later controlled by the lever. See the [button plan and command choices](../../docs/physical-button-plan.md).

The [purple action-brick documentation](../../docs/purple-action-brick-research.md) contains the published selection packet and preset IDs: nothing, beach, cat, night, birthday, rain, recorded sound. Each future button press should select the next preset and wrap at the end. These selections still need testing on our train; “night” is not yet confirmed as lullaby. Selection configures the purple brick's action, rather than promising immediate sound playback. Recording/uploading custom audio is not implemented.

## Validation

Compiled with ESP32 core 3.3.12 for both ESP32S3 Dev Module and XIAO_ESP32C3. A temporary host harness running the actual sketch passed startup-held-button suppression, contact bounce, simultaneous presses, no hold repeat, release events, and timer rollover checks. Physical wiring checks on your boards are still required.

## Working hardware checkpoint (2026-10-02)

The user reports the input test working after correcting the board and USB settings. For S3-Zero use **ESP32S3 Dev Module**, **USB CDC On Boot: Enabled**, and **USB Mode: Hardware CDC and JTAG**. Changing board selection can reset USB CDC to Disabled. Reupload after changing it, then reselect the port and open Serial Monitor at 115200 baud. A successful upload alone does not mean serial output is routed to USB.

A normal press needs only 25 ms of stable contact, not a one-second hold. Use GPIO numbers printed on the board, not header positions. With the input sketch running, a direct GPIO4-to-GND jumper should generate a B1 press; removing it generates release. A jumper held at boot prevents `B1 ready` until removed. Check continuity only with power disconnected; use DC volts for powered measurements.

## Next experiment

With inputs working, upload the [physical button horn test](../button_horn_test/README.md). It uses the same B1 wiring and adds bonded BLE horn control.
