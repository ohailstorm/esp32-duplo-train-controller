#include <Arduino.h>
#include <NimBLEDevice.h>

// Waveshare ESP32-S3-Zero; NimBLE-Arduino 2.5.1.
// Bonding and horn test: this sketch never sends motor commands.
static const NimBLEUUID trainService("00001623-1212-efde-1623-785feabcd123");
static const NimBLEUUID trainCharacteristic("00001624-1212-efde-1623-785feabcd123");
static NimBLEClient* client = nullptr;

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
    Serial.printf("Disconnected; BLE reason=%d. Send c to reconnect.\n", reason);
  }
};
static ConnectionCallbacks callbacks;

void connectTrain() {
  if (client->isConnected()) {
    printStatus();
    return;
  }

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
  printStatus();
}

void playHorn() {
  if (!client->isConnected()) {
    Serial.println("Not connected. Send c first.");
    return;
  }
  const auto info = client->getConnInfo();
  if (!info.isEncrypted() || !info.isBonded()) {
    Serial.println("Horn requires an encrypted, bonded connection.");
    return;
  }
  // Resolve on each command so reconnects cannot leave a stale cached pointer.
  auto* service = client->getService(trainService);
  auto* characteristic = service ? service->getCharacteristic(trainCharacteristic) : nullptr;
  if (!characteristic) {
    Serial.println("LEGO command characteristic not found.");
    return;
  }
  if (!characteristic->canWrite() && !characteristic->canWriteNoResponse()) {
    Serial.println("LEGO command characteristic is not writable.");
    return;
  }
  // LWP3 multi-port horn action, from the 10427 reference implementation.
  const uint8_t horn[] = {0x0B, 0x00, 0x81, 0x34, 0x11, 0x51,
                          0x01, 0x07, 0x01, 0x00, 0x00};
  if (characteristic->writeValue(horn, sizeof(horn), characteristic->canWrite())) {
    Serial.println("Horn frame written. Confirm that you hear the horn.");
  } else {
    Serial.println("Horn write failed. Send s to check the connection.");
  }
}

void printHelp() {
  Serial.println("Commands: c=connect/pair, s=status, b=horn, d=disconnect, f=forget bonds, r=restart, h=help");
}

void setup() {
  Serial.begin(115200);
  const uint32_t started = millis();
  while (!Serial && millis() - started < 5000) delay(10);
  Serial.println("\nDUPLO 10427 bonding test (no motor commands)");
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
  if (Serial.available()) {
    switch (Serial.read()) {
      case 'c': connectTrain(); break;
      case 's': printStatus(); break;
      case 'b': playHorn(); break;
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
