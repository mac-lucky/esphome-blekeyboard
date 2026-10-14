#ifdef USE_ESP32

#include "ble_keyboard.h"
#include "esphome/core/log.h"
#include <NimBLEServer.h>
#include <NimBLEDevice.h>
#include <NimBLEService.h>
#include <NimBLECharacteristic.h>
#include <NimBLEAdvertising.h>
#include <string>
#include <list>

#ifdef USE_ARDUINO
// arduino-esp32 frees the Bluetooth controller memory at startup unless a library
// claims it, and NimBLE-Arduino 1.x does not. esp_bt_controller_init() then fails
// with ESP_ERR_INVALID_STATE and NimBLEDevice::init() aborts, so the device boot
// loops into safe mode. ESPHome's own esp32_ble component does the same.
extern "C" bool btInUse() { return true; }  // NOLINT(readability-identifier-naming)
#endif

namespace esphome {
namespace ble_keyboard {
static const char *const TAG = "ble_keyboard";

void Esp32BleKeyboard::setup() {
  ESP_LOGI(TAG, "Setting up...");

  bleKeyboard.begin();

  pServer = BLEDevice::getServer();

  pServer->advertiseOnDisconnect(this->reconnect_);

  bleKeyboard.releaseAll();
}

void Esp32BleKeyboard::stop() {
  if (this->reconnect_) {
    pServer->advertiseOnDisconnect(false);
  }

  std::vector<uint16_t> ids = pServer->getPeerDevices();

  if (ids.size() > 0) {
    for (uint16_t &id : ids) {
      pServer->disconnect(id);
    }
  } else {
    pServer->stopAdvertising();
  }
}

void Esp32BleKeyboard::start() {
  if (this->reconnect_) {
    pServer->advertiseOnDisconnect(true);
  }

  pServer->startAdvertising();
}

void Esp32BleKeyboard::update() { state_sensor_->publish_state(bleKeyboard.isConnected()); }

bool Esp32BleKeyboard::is_connected() {
  if (!bleKeyboard.isConnected()) {
    ESP_LOGI(TAG, "Disconnected");

    return false;
  }

  return true;
}

void Esp32BleKeyboard::update_timer() {
  this->cancel_timeout(TAG);
  this->set_timeout(TAG, release_delay_, [this]() { this->release(); });
}

void Esp32BleKeyboard::press(std::string message) {
  if (this->is_connected()) {
    if (message.length() >= 5) {
      for (unsigned i = 0; i < message.length(); i += 5) {
        bleKeyboard.print(message.substr(i, 5).c_str());

        delay(default_delay_);
      }

      return;
    }

    bleKeyboard.print(message.c_str());
  }
}

void Esp32BleKeyboard::press(uint8_t key, bool with_timer) {
  if (this->is_connected()) {
    if (with_timer) {
      this->update_timer();
    }

    bleKeyboard.press(key);
  }
}

void Esp32BleKeyboard::press(MediaKeyReport key, bool with_timer) {
  if (this->is_connected()) {
    if (with_timer) {
      this->update_timer();
    }
    bleKeyboard.press(key);
  }
}

void Esp32BleKeyboard::release() {
  if (this->is_connected()) {
    this->cancel_timeout(TAG);
    bleKeyboard.releaseAll();
  }
}
}  // namespace ble_keyboard
}  // namespace esphome

#endif