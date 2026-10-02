#include <Arduino.h>
#include <NimBLEDevice.h>

#if !ARDUINO_USB_CDC_ON_BOOT
#error "Enable Tools > USB CDC On Boot, then upload again."
#endif
// These tests use hardware GPIO numbers, not remapped Arduino pin numbers.
#if defined(BOARD_HAS_PIN_REMAP) && !defined(BOARD_USES_HW_GPIO_NUMBERS)
#error "Remapped board selected. For Waveshare S3-Zero choose ESP32S3 Dev Module, not Ozobot DRVKit."
#endif

// GPIO numbers, not physical header positions. See this folder's README.
#if defined(ARDUINO_XIAO_ESP32C3)
static constexpr uint8_t buttonPins[] = {5}; // D3
#elif defined(ARDUINO_ESP32S3_DEV) && defined(CONFIG_IDF_TARGET_ESP32S3)
static constexpr uint8_t buttonPins[] = {4}; // Waveshare S3-Zero
#else
#error "Select ESP32S3 Dev Module (S3-Zero) or XIAO_ESP32C3; check the wiring guide."
#endif

static const NimBLEUUID trainService("00001623-1212-efde-1623-785feabcd123");
static const NimBLEUUID trainCharacteristic("00001624-1212-efde-1623-785feabcd123");
static NimBLEClient* client = nullptr;
static bool rawPressed = false;
static bool stablePressed = true;
static bool buttonArmed = false;
static uint32_t changedAt = 0;
static constexpr uint32_t debounceMs = 25;

void resetButton() {
  rawPressed = digitalRead(buttonPins[0]) == LOW;
  stablePressed = true;
  buttonArmed = false;
  changedAt = millis();
}

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

void connectTrainInternal() {
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
    Serial.println("Multiple stored bonds. This test expects one. Use the bonding_test sketch to manage bonds.");
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

// BLE connection/discovery calls block; discard button activity during them.
void connectTrain() {
  resetButton();
  connectTrainInternal();
  resetButton();
}

void serviceButton() {
  if (!client->isConnected() || !client->getConnInfo().isEncrypted() ||
      !client->getConnInfo().isBonded()) {
    resetButton(); // No queued presses; release after reconnect to arm.
    return;
  }
  const uint32_t now = millis();
  const bool pressed = digitalRead(buttonPins[0]) == LOW;
  if (pressed != rawPressed) {
    rawPressed = pressed;
    changedAt = now;
  }
  if (now - changedAt < debounceMs || pressed == stablePressed) return;
  stablePressed = pressed;
  if (!buttonArmed) {
    if (!pressed) {
      buttonArmed = true;
      Serial.println("B1 ready: press for horn.");
    }
    return;
  }
  Serial.println(pressed ? "B1 pressed" : "B1 released");
  if (pressed) playHorn(); // One attempt per press; never repeat/retry automatically.
}

void printHelp() {
  Serial.println("B1=horn; serial: c=connect/pair, s=status, b=horn, d=disconnect, h=help");
}

void setup() {
  Serial.begin(115200);
  const uint32_t started = millis();
  while (!Serial && millis() - started < 5000) delay(10);
  Serial.println("\nDUPLO 10427 physical button horn test (no motor commands)");
  Serial.printf("Compiled board: %s; B1 hardware GPIO%u\n", ARDUINO_BOARD, buttonPins[0]);
  pinMode(buttonPins[0], INPUT_PULLUP);
  resetButton();
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
        resetButton();
        if (client->isConnected()) client->disconnect();
        break;
      case 'h': printHelp(); break;
      case '\r': case '\n': case ' ': break;
      default: Serial.println("Unknown command. Send h for help."); break;
    }
  }
  serviceButton();
  delay(1);
}
