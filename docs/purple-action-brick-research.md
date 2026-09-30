# Purple action brick: research notes

Research date: 2026-09-30. Target: DUPLO 10427, standalone ESP32 controller.

## Conclusion

The deeper GitHub API search found published purple-action configuration packets. The earlier conclusion that no mapping was available was incomplete. Preset selection now has a concrete implementation to test; immediate playback and an on/off toggle remain unverified on our train.

The user confirmed lullaby and Happy Birthday in the iPhone app. LEGO's Data Act notice also says clips up to five seconds can be transferred to the train. That establishes transfer capability, not its BLE encoding. [LEGO product information](https://www.lego.com/en-za/product/interactive-adventure-train-10427)

## Published purple-action configuration

[drndos' project](https://github.com/drndos/duplo-train-controller) explicitly describes reverse-engineering captured BLE traffic and controlling the purple brick. [JanHouwers/LDTrainRemote](https://github.com/JanHouwers/LDTrainRemote) credits that work and reports testing its action-sound cycle on 10427.

The original [packet table](https://github.com/drndos/duplo-train-controller/blob/main/duplo_nimble.ino) labels these as violet action-block configurations:

```text
0B 00 81 34 11 51 01 06 01 ID 00
```

| ID (hex) | Source label |
| --- | --- |
| 00 | Nothing |
| 01 | Beach |
| 02 | Cat |
| 03 | Night |
| 04 | Birthday |
| 05 | Rain |
| 06 | Recorded sound |

These target port `0x34`, mode `1`, opcode `0x0106`. Night is a plausible match for lullaby, but that identification needs listening. Nothing is a candidate for disabling the assigned action, not proof that it cancels active playback. Recorded sound selects a slot; this is not an audio-upload packet. Do not substitute these IDs into the separate speaker or horn commands.

The existing 10427 reference has an [open issue about this bank](https://github.com/micschr0/duplo-train-10427-ble2mqtt/issues/1), explaining why it was absent from its implemented command list. Its uncertainty about model support is supplemented by JanHouwers' explicit 10427 report; our hardware verification is still needed.

## Search scope and limitations

In addition to web searches, GitHub repository searches for `duplo 10427` and `duplo 10428`, plus issue search for `duplo purple`, surfaced the links above. YellowLemon1/DuploTrain2025 and stasfeelin/duplo-train were also inspected as leads; neither supplied the purple mapping used here. JanHouwers has two different repositories: the older `duplo-train-controller` targets 10874/10875, while `LDTrainRemote` explicitly targets 10427. Confusing these would miss the useful implementation.

This search found a practical configuration route, not an exhaustive proof about all possible train commands. No iPhone capture is required before testing these published packets.

## Useful new lead: observing action bricks

A separate project's first-hand 10428 research identifies port `0x34`, mode `1`, as EVENTS. After subscribing to the LEGO characteristic's BLE notifications, it sends:

```text
0A 00 41 34 01 01 00 00 00 01
```

This requests event updates, not a purple action. Its reported notification format is:

```text
08 00 45 34 <opcode low> <opcode high> <parameter low> <parameter high>
```

The reported horn-brick inbound opcode is `0xA001`; outbound horn uses `0x0107`. Incoming events must not be blindly replayed as output commands. That source leaves other brick mappings unfinished. Its port `0x33` TAG writes reportedly failed, so this is not an established route for injecting purple detections. All of these observations concern **10428**, not our tested 10427. [Protocol notes](https://github.com/romanlamsal/duplo-train-10428-arduino-remote/blob/main/duplo-train-10428.md)

The project's [source code](https://github.com/romanlamsal/duplo-train-10428-arduino-remote/blob/main/Train.cpp) implements the event subscription and raw notification printing. Its claim that subscribing is necessary for sound/light writes must not override our own successful 10427 tests without that subscription.

The author also reports that repeated throttle writes interfere with action-brick movement sequences; identifying completion events and handling configurable purple actions remain open work. For our lever, avoid continuously resending unchanged motor power and investigate how manual override should interact with brick sequences. [First-hand issue report](https://github.com/romanlamsal/duplo-train-10428-arduino-remote/issues/1)

## Other command candidates already documented for 10427

The reference encodes direct speaker sounds as `08 00 81 01 11 51 01 <id>`, with brake=3, departure=5, refill=7, horn=9, steam=10. Our verified horn uses a different multi-port route; these speaker commands remain untested here. Its purple light packet is `0B 00 81 34 11 51 01 04 01 0A 00`, which only requests a colour. [Encoders](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/protocol/commands.rs), [IDs](https://github.com/micschr0/duplo-train-10427-ble2mqtt/blob/main/src/types.rs)

## Next investigation

1. Add a separate, explicit serial test for IDs `03`, `04`, and `00`. Keep existing motion controls unchanged and drive wheels clear. Send one configuration packet and first observe whether it plays immediately.
2. Present the real purple brick after each selection. Compare night with the app's lullaby, birthday with Happy Birthday, and nothing with the baseline. Record whether the setting survives reconnect and train restart.
3. During an effect, test whether ID `00` cancels it or only changes future brick encounters. Do not label this command Stop until verified. Motor stop and effect cancellation must remain separate.
4. If configuration works but direct playback does not, inspect notifications and use the iPhone/Mac PacketLogger capture route to investigate the app's trigger/stop operations. Do not replay received sensor events as commands.
5. Choose a button behaviour based on results: cycle presets, toggle the brick's configured action, or toggle immediate playback if supported. Keep recorded audio out of the first test.

No experimental packets were added to firmware or sent to the train during this research.
