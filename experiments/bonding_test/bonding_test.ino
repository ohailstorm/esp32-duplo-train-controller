#include <Arduino.h>
#include <atomic>
#include <NimBLEDevice.h>

// Waveshare ESP32-S3-Zero; NimBLE-Arduino 2.5.1.
// BLE diagnostics with short, explicitly requested movement tests.
static const NimBLEUUID trainService("00001623-1212-efde-1623-785feabcd123");
static const NimBLEUUID trainCharacteristic("00001624-1212-efde-1623-785feabcd123");
static NimBLEClient* client = nullptr;
static std::atomic<bool> disconnected{false};
static bool motorMayBeRunning = false;
static int8_t pendingPower = 0;
static uint32_t movementStarted = 0;
static uint32_t stopWrittenAt = 0;
static uint32_t lastStopAttempt = 0;
static constexpr uint32_t movementDurationMs = 2000;
static constexpr uint32_t directionPauseMs = 300;

// Experimental purple-brick configuration, not immediate sound playback.
// Source: github.com/drndos/duplo-train-controller/blob/main/duplo_nimble.ino
static constexpr uint8_t purplePresetIds[] = {0x03, 0x04, 0x01, 0x05, 0x02, 0x00};
static const char* const purplePresetNames[] = {
  "Night (possible lullaby)", "Birthday", "Beach", "Rain", "Cat", "Nothing"
};
static constexpr size_t purplePresetCount = sizeof(purplePresetIds) / sizeof(purplePresetIds[0]);
static size_t nextPurplePreset = 0;

bool stopMotor();

void printStatus() {
  Serial.printf("Stored bonds: %d\n", NimBLEDevice::getNumBonds());
  for (int i = 0; i < NimBLEDevice::getNumBonds(); ++i) {
    Serial.printf("  %s\n", NimBLEDevice::getBondedAddress(i).toString().c_str());
  }
  if (!client->isConnected()) {
    Serial.println("Disconnected.");
    return;
  }
  const auto info = client->getConnInfo();
  Serial.printf("Peer identity: %s; encrypted=%d; bonded=%d; authenticated=%d\n",
                info.getIdAddress().toString().c_str(), info.isEncrypted(),
                info.isBonded(), info.isAuthenticated());
  Serial.printf("Peer bond in storage: %d\n", NimBLEDevice::isBonded(info.getIdAddress()));
}

class ConnectionCallbacks : public NimBLEClientCallbacks {
  void onDisconnect(NimBLEClient*, int reason) override {
    disconnected.store(true);
    Serial.printf("Disconnected; BLE reason=%d. Send c to reconnect.\n", reason);
  }
};
static ConnectionCallbacks callbacks;

void connectTrain() {
  if (client->isConnected()) {
    printStatus();
    return;
  }

  // No preset readback: start the local cycle at Night on each connection attempt.
  // This does not change the train's preset until p is explicitly requested.
  nextPurplePreset = 0;
  bool connected = false;
  if (NimBLEDevice::getNumBonds() == 1) {
    // Use the stored identity rather than silently pairing with another hub.
    const auto address = NimBLEDevice::getBondedAddress(0);
    Serial.printf("Connecting to stored peer %s...\n", address.toString().c_str());
    connected = client->connect(address);
  } else if (NimBLEDevice::getNumBonds() > 1) {
    Serial.println("Multiple stored bonds. This test expects one. Use f to clear them.");
    return;
  } else {
    Serial.println("Scanning for LEGO hubs for 5 seconds. Keep only the target hub on.");
    auto* scan = NimBLEDevice::getScan();
    scan->setActiveScan(true);
    auto results = scan->getResults(5000);
    const NimBLEAdvertisedDevice* target = nullptr;
    int matches = 0;
    for (int i = 0; i < results.getCount(); ++i) {
      const auto* device = results.getDevice(i);
      if (device->isAdvertisingService(trainService)) {
        Serial.printf("LEGO hub: %s\n", device->toString().c_str());
        target = device;
        ++matches;
      }
    }
    if (matches == 1) {
      connected = client->connect(target);
    } else {
      Serial.printf("Found %d LEGO hubs; exactly one is required. Wake the train and retry c.\n", matches);
    }
    scan->clearResults();
    if (matches != 1) return;
  }

  if (!connected) {
    Serial.printf("Connection failed; NimBLE error=%d. Wake the train and retry c.\n", client->getLastError());
    return;
  }
  Serial.println("BLE connected. Requesting encryption and bonding...");
  if (!client->secureConnection()) {
    Serial.printf("Security failed; NimBLE error=%d.\n", client->getLastError());
    client->disconnect();
    return;
  }
  const auto info = client->getConnInfo();
  if (!info.isEncrypted() || !info.isBonded()) {
    Serial.println("FAIL: connection is not both encrypted and bonded.");
    printStatus();
    client->disconnect();
    return;
  }
  Serial.println("PASS: encrypted and bonded connection. Check persistence after a power cycle.");
  stopMotor(); // Never resume previous movement on reconnect.
  printStatus();
}

bool writeTrainCommand(const uint8_t* packet, size_t length, const char* label) {
  if (!client->isConnected()) {
    Serial.println("Not connected. Send c first.");
    return false;
  }
  const auto info = client->getConnInfo();
  if (!info.isEncrypted() || !info.isBonded()) {
    Serial.println("Commands require an encrypted, bonded connection.");
    return false;
  }
  // Resolve on each command so reconnects cannot leave a stale cached pointer.
  auto* service = client->getService(trainService);
  auto* characteristic = service ? service->getCharacteristic(trainCharacteristic) : nullptr;
  if (!characteristic) {
    Serial.println("LEGO command characteristic not found.");
    return false;
  }
  if (!characteristic->canWrite() && !characteristic->canWriteNoResponse()) {
    Serial.println("LEGO command characteristic is not writable.");
    return false;
  }
  if (characteristic->writeValue(packet, length, characteristic->canWrite())) {
    Serial.printf("%s frame written. Confirm the effect on the train.\n", label);
    return true;
  } else {
    Serial.printf("%s write failed. Send s to check the connection.\n", label);
  }
  return false;
}

void playHorn() {
  // LWP3 multi-port horn action, from the 10427 reference implementation.
  const uint8_t horn[] = {0x0B, 0x00, 0x81, 0x34, 0x11, 0x51,
                          0x01, 0x07, 0x01, 0x00, 0x00};
  writeTrainCommand(horn, sizeof(horn), "Horn");
}

void setLight(uint8_t color) {
  // 10427 colour IDs: off=0x00, white=0x01, green=0x07, red=0x0C.
  const uint8_t light[] = {0x0B, 0x00, 0x81, 0x34, 0x11, 0x51,
                           0x01, 0x04, 0x01, color, 0x00};
  Serial.printf("Requesting light colour 0x%02X.\n", color);
  writeTrainCommand(light, sizeof(light), "Light");
}

void selectNextPurplePreset() {
  const uint8_t id = purplePresetIds[nextPurplePreset];
  const uint8_t packet[] = {0x0B, 0x00, 0x81, 0x34, 0x11, 0x51,
                            0x01, 0x06, 0x01, id, 0x00};
  Serial.printf("Requesting purple preset: %s (0x%02X).\n",
                purplePresetNames[nextPurplePreset], id);
  if (writeTrainCommand(packet, sizeof(packet), "Purple preset")) {
    Serial.println("Trigger the physical purple brick to check the selected action.");
    nextPurplePreset = (nextPurplePreset + 1) % purplePresetCount;
  }
  // Failed/disconnected writes do not advance or queue a selection.
}

bool writeMotor(int8_t power) {
  // LWP3 motor port 0x32, direct mode 0; signed power -100..100.
  const uint8_t packet[] = {0x08, 0x00, 0x81, 0x32, 0x11, 0x51,
                            0x00, static_cast<uint8_t>(power)};
  return writeTrainCommand(packet, sizeof(packet), power == 0 ? "Stop" : "Motor");
}

bool stopMotor() {
  pendingPower = 0;
  lastStopAttempt = millis();
  if (!writeMotor(0)) {
    // Keep the stop retry active: a failed write does not prove the motor stopped.
    motorMayBeRunning = true;
    movementStarted = millis() - movementDurationMs;
    Serial.println("Stop not confirmed by BLE write. Check the train; turn it off if needed.");
    return false;
  }
  motorMayBeRunning = false;
  stopWrittenAt = millis();
  return true;
}

void requestMovement(int8_t power) {
  if (!client->isConnected()) {
    Serial.println("Not connected. Send c first.");
    return;
  }
  // Stop before every run, including direction changes. The pause is nonblocking.
  if (!stopMotor()) return;
  pendingPower = power;
  Serial.printf("Queued motor power %d for 2 seconds after a 300 ms stop pause.\n", power);
}

void serviceMovement() {
  if (disconnected.exchange(false) || !client->isConnected()) {
    if (motorMayBeRunning || pendingPower != 0) {
      Serial.println("Link lost: cannot send stop. Check the train and turn it off if still moving.");
    }
    motorMayBeRunning = false;
    pendingPower = 0;
    return;
  }
  if (motorMayBeRunning && millis() - movementStarted >= movementDurationMs &&
      millis() - lastStopAttempt >= 250) {
    stopMotor();
  }
  if (pendingPower != 0 && millis() - stopWrittenAt >= directionPauseMs) {
    const int8_t power = pendingPower;
    pendingPower = 0;
    motorMayBeRunning = true; // Even a failed write may have reached the hub.
    movementStarted = millis();
    if (!writeMotor(power)) stopMotor();
  }
}

void printHelp() {
  Serial.println("p=next purple preset: Night, Birthday, Beach, Rain, Cat, Nothing (wraps; recorded slot excluded)");
  Serial.println("Movement: w=forward +50, v=reverse -50 (2 seconds), x=STOP");
  Serial.println("Commands: c=connect/pair, s=status, b=horn, 1=white, 2=green, 3=red, 0=light off, d=disconnect, f=forget bonds, r=restart, h=help");
}

void setup() {
  Serial.begin(115200);
  const uint32_t started = millis();
  while (!Serial && millis() - started < 5000) delay(10);
  Serial.println("\nDUPLO 10427 BLE test (movement only on w/v)");
  if (!NimBLEDevice::init("DuploBondTest")) {
    Serial.println("BLE initialization failed. Restart the board.");
    while (true) delay(1000);
  }
  // Just Works bonding: encryption and persistent keys, no PIN or MITM check.
  NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
  NimBLEDevice::setSecurityAuth(true, false, true);
  client = NimBLEDevice::createClient();
  if (!client) {
    Serial.println("Could not create BLE client. Restart the board.");
    while (true) delay(1000);
  }
  client->setClientCallbacks(&callbacks, false);
  client->setConnectTimeout(10000);
  printHelp();
  printStatus();
  if (NimBLEDevice::getNumBonds() == 1) connectTrain();
}

void loop() {
  serviceMovement();
  if (Serial.available()) {
    const char command = Serial.read();
    // Avoid other blocking BLE operations while a timed movement/stop is active.
    if ((motorMayBeRunning || pendingPower != 0) &&
        command != 'x' && command != 'w' && command != 'v' &&
        command != '\r' && command != '\n' && command != ' ') {
      Serial.println("Movement active. Send x to stop before other commands.");
      return;
    }
    switch (command) {
      case 'w': requestMovement(50); break;
      case 'v': requestMovement(-50); break;
      case 'x': stopMotor(); break;
      case 'c': connectTrain(); break;
      case 's': printStatus(); break;
      case 'b': playHorn(); break;
      case 'p': selectNextPurplePreset(); break;
      case '1': setLight(0x01); break;
      case '2': setLight(0x07); break;
      case '3': setLight(0x0C); break;
      case '0': setLight(0x00); break;
      case 'd':
        if (client->isConnected()) client->disconnect();
        break;
      case 'f': {
        if (client->isConnected()) client->disconnect();
        const uint32_t started = millis();
        while (client->isConnected() && millis() - started < 3000) delay(10);
        if (client->isConnected()) {
          Serial.println("Still connected; bonds not cleared. Retry f.");
          break;
        }
        Serial.println(NimBLEDevice::deleteAllBonds() ? "ESP32 bonds cleared. Send c to pair again." : "Bond deletion failed.");
        printStatus();
        break;
      }
      case 'r': Serial.println("Restarting..."); delay(100); ESP.restart(); break;
      case 'h': printHelp(); break;
      case '\r': case '\n': case ' ': break;
      default: Serial.println("Unknown command. Send h for help."); break;
    }
  }
  delay(10);
}
