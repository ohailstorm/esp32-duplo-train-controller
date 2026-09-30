# Purple action brick: research notes

Research date: 2026-09-30. Target: DUPLO 10427, standalone ESP32 controller.

## Conclusion

No verified BLE command for starting/stopping the purple action-brick mode, or enabling/disabling all action-brick reactions, was found in the sources inspected. These are separate features; neither is established by setting the headlight purple. Keep a purple-function button provisional.

LEGO confirms the customization brick supports sounds recorded with its app, but does not document the BLE mechanism or where audio playback occurs in that product description. This does not establish standalone playback/upload support for our ESP32. [LEGO 10427](https://www.lego.com/en-us/product/interactive-adventure-train-10427)

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

1. Create a separate event-logging experiment; retain the tested bonding/movement sketch unchanged. Subscribe to characteristic notifications and inspect attached-port announcements before trying the EVENTS subscription above. Log raw frames with timestamps and handle fragmented messages.
2. With drive wheels lifted, present the real purple brick to the sensor. Compare notifications with other bricks and record when the physical sound/light/motor sequence starts and ends. Do not assume a completion notification exists.
3. Separately connect the official app and check whether it offers a direct trigger or stop for the effect. If it does, capture that interaction using Android Bluetooth HCI logging. Capture one action at a time and distinguish configuration writes from playback controls. The app and ESP32 should be tested in separate BLE sessions.
4. Only promote an observed outbound command to a test after establishing its target, payload, effect, and stop behaviour on 10427. A received sensor event is not evidence of an equivalent writable command.
5. Decide the intended button behaviour: trigger the effect once, toggle a persistent effect, or toggle reactions to track bricks. If the hub offers no equivalent, an ESP32-managed light/sound sequence is an approximation and should be labelled as such.

No experimental packets were added to firmware or sent to the train during this research.
