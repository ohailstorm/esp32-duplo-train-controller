# Duplo 10427 – ESP32 Train Remote

## Language for Codex

All work in Codex should be in English: prompts, plans, responses, code, file and folder names, identifiers, comments, serial commands and status messages, tests, technical documentation, and commit messages. Conversation in this ChatGPT project may remain in Swedish.

## Tests and guides

- [Physical button horn test](experiments/button_horn_test/README.md) — B1 triggers the train horn over BLE.

- [Physical button test: wiring, upload, and checks](experiments/button_input_test/README.md) — start here for your breadboard; supports S3-Zero and XIAO ESP32-C3.
- [BLE bonding, horn, lights, and movement test](experiments/bonding_test/README.md).
- [Purple action brick: preset commands and research](docs/purple-action-brick-research.md).
- [Button assignments and next-stage plan](docs/physical-button-plan.md).

## Goals and decisions

A handheld, standalone remote based on the Printables model below, adapted for LEGO Duplo 10427. The ESP32 board, potentiometer, and buttons fit in the same enclosure and control the train directly over BLE. No Raspberry Pi, Zigbee, Home Assistant, or MQTT is required for operation. Start with USB-C connected to an external 5 V supply, such as an existing power bank or USB adapter, while choosing a board and reserving space for a possible internal battery later. Use a Bambu Lab P2S Combo for printing.

**Agreed version 1 design:** Place the lever on the right, with forward – stop – reverse, four function buttons, and a noticeable mechanical click at the lever's centre position. The potentiometer does not have its own centre detent. Use the XIAO ESP32-C3 as the battery-ready board, with USB-C and integrated LiPo charging. Power it through USB-C initially; install a battery later. A separate power switch is not a priority before the battery is installed.

## Reference designs and protocol

- 3D model: https://www.printables.com/model/878210-remote-compatible-with-lego-duplo-train-using-esp3
- Hag3D's remix with a right-hand lever and revised top: https://www.printables.com/model/1179027-modifications-for-remote-compatible-with-lego-dupl/files
- Original instructions: https://files.printables.com/media/prints/878210/pdfs/878210-remote-compatible-with-lego-duplo-train-using-esp32-426e9a69-59ed-4e20-8777-6a7eb186573c.pdf
- Original firmware: https://github.com/mav00/LDTrainRemote
- Existing Raspberry Pi BLE control implementation for 10427, to inform the ESP32 port: https://github.com/micschr0/duplo-train-10427-ble2mqtt

The original project targets older Duplo hubs and uses Legoino. The 10427 requires Bluetooth LE bonding and the correct LWP3 commands. Develop separate ESP32 firmware; the original code cannot be used unchanged. The ESP32 should persist its bond with the train so that normal startup reconnects without manual pairing. Provide an intentional action to clear the old bond and pair again when needed: try holding an existing function button during startup, and add a dedicated pairing button only if necessary. Provide visible connection and pairing feedback, for example using the board's LED if it is visible through the enclosure. The 10427 test project uses motor values from −100 to +100. Map the potentiometer around a calibrated centre position: one side controls forward motion and the other reverse. Apply a small dead zone around the centre and send stop before changing direction. Determine the actual minimum usable speed with the train.

## Hardware

- Prototype board: Waveshare ESP32-S3-Zero, powered over USB.
- Final controller board: Seeed Studio XIAO ESP32-C3, with USB-C and LiPo charging.
- Four tactile push buttons, 12 × 12 × 7.3 mm.
- Linear 10 kΩ potentiometer with a 6 mm shaft, 18 splines, and a 20 mm shaft length.
- Soldered pin headers on the XIAO, with short female jumper connectors at the board and soldered connections at the buttons and potentiometer. Share ground between buttons and secure connectors with strain relief for final assembly.
- M2 × 6 mm countersunk screws and Bossard BN 1054 M2 threaded inserts, 4.1 mm long.
- External 5 V power over USB-C initially. An internal 3.7 V LiPo battery is a later option.

No motor driver is required: the ESP32 communicates directly with the train hub over BLE.

## CAD and printing

- Start from Hag3D's right-hand lever design and check which remix parts fit our enclosure. Keep the lever on the right even if the body and top need to be redesigned for the selected ESP32 board.
- Modify the enclosure's ESP32 mount to suit the selected board's dimensions and leave clearance for a connected USB-C plug. All components should fit inside the handheld controller, with USB-C accessible from outside.
- Reserve an accessible compartment for a possible future 3.7 V LiPo battery. The current candidate measures 63 × 36 × 4.7 mm. Allow room for its protection circuit, wire bends, and tolerances. Use a smooth, rounded holder that secures the battery without squeezing or puncturing the pouch. Keep the compartment accessible for replacement. Investigate a power switch or sleep mode when the battery is installed.
- Retain the model's mechanism for the four 12 × 12 × 7.3 mm buttons if their fit is verified. Printables specifies two copies of `throttle_stick` and one of each other part.
- Adapt the potentiometer's lever attachment to the selected 6 mm splined shaft (18 splines, 20 mm shaft length). Test-print the lever coupling before printing the whole enclosure.
- Add a tactile mechanical centre detent: a flexible 3D-printed tab that engages a shallow recess at the stop position. Make the click noticeable without locking the lever. Test tolerances and wear resistance with a small test piece.
- According to its datasheet, the Bossard insert has an approximately 3.3 mm body, a wider 4.8 mm upper section, and a length of 4.1 mm. Redesign the screw boss holes for this insert and test-fit one before printing the whole enclosure. The original PDF specifies approximately 3.5 mm outer diameter for a different type of M2 insert.
- The PDF's GPIO 25/26/27/14/15 assignments are for the original board. Choose actual pins for the selected board. Connect each button to its own GPIO and shared ground, and the potentiometer to 3.3 V, ground, and an ADC input. On the XIAO ESP32-C3, use ADC1 for reliable readings and account for boot strapping pins. Plan cable routing and strain relief around the supplied XIAO pin headers and the Dupont female connectors at the board.

## Next steps

The first standalone experiment is [the BLE bonding test](experiments/bonding_test/README.md), with a single Arduino sketch and ESP32-S3-Zero setup/upload instructions. Keep small hardware experiments under `experiments/`; the eventual controller firmware can live separately under `firmware/`.

Bonding, power-cycle reconnection, horn, basic lights, and timed movement tests have now been reported working. See the [physical-button test plan and command choices](docs/physical-button-plan.md) for the next breadboard experiment and possible four-button layouts.

1. Start with a USB-connected Waveshare ESP32-S3-Zero as the test board and virtual buttons through a serial terminal. No potentiometer, physical buttons, or enclosure are needed for the first test. Provide commands for pairing and re-pairing, forward and reverse at a selected speed, stop, and the four functions. Print clear connection status and errors. Make stop a simple, immediate command, and send stop before changing direction.
2. Implement and test BLE bonding with the 10427, persistent reconnection after a restart, and LWP3 commands using the real train. Check what the train does if the connection drops while moving. If needed, add recurring safety stops or another suitable mechanism before using the physical remote.
3. Build the same source code for the XIAO ESP32-C3 and pair the train again with that board. Adapt the board's pin assignments and LED, and confirm bonding, reconnection, stop, and direction changes on the XIAO too. BLE bonds stored on the S3 do not transfer to the C3.
4. Only then add physical buttons and the potentiometer. Calibrate the lever's centre and dead zone with the real train and verify the minimum usable speed. Check whether an existing button is sufficient for re-pairing.
5. Check whether the STL or source files can be modified, and measure the mounts, lever travel, and USB opening. Test-print the XIAO mount, potentiometer coupling, and one screw boss with the selected insert. Measure space for a future battery compartment and complete final assembly.
