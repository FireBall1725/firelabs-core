#pragma once
#include <Arduino.h>
#include <functional>

// Shared FireLabs onboarding: captive-portal setup wizard, wifi provisioning,
// config storage, OTA recovery, and the hold-to-reset button. Setup mode and a
// device's runtime are mutually exclusive, so the core owns the entire setup
// experience with its own sync web server; this works the same on ESP8266 and
// ESP32. A device sets its branding, then calls begin()/connect()/startSetupPortal().
class FirelabsCore {
public:
  using VoidCb = std::function<void()>;

  // Set before begin(). The S31 uses "FireLabs S31"/"fl"; the WX "FireLabs WX"/"fl-wx".
  String apPrefix = "FireLabs";
  String hostPrefix = "fl";

  // Persisted identity (wifi + friendly name), stored in LittleFS.
  String wifiSsid;
  String wifiPass;
  String deviceName;

  void begin();                       // mount LittleFS + load saved wifi/name
  bool hasWifi() const { return wifiSsid.length() > 0; }
  bool connect(uint32_t timeoutMs);   // join wifi; true once connected
  // Scan, then open AP + captive portal + web wizard. timeoutMs > 0 arms a
  // watchdog: if nobody provisions within that window the device reboots (or
  // runs onTimeout if set). Use it on the FALLBACK path -- a provisioned device
  // that couldn't reach its wifi shouldn't camp in setup forever; the outage may
  // have cleared. Pass 0 (default) for fresh first-boot setup, where a human is
  // present and may take their time.
  void startSetupPortal(uint32_t timeoutMs = 0);
  void onTimeout(VoidCb cb) { onTimeout_ = cb; }  // override reboot-on-timeout
  void loop();                        // pump the portal + button; reboots after provisioning
  void factoryReset();                // wipe config and reboot into setup

  String hostname() const;            // <hostPrefix>-<name>, sanitized
  String apSsid() const;              // "<apPrefix> <macSuffix>"
  static String macSuffix();

  // GPIO0-style button. Held >= holdMs fires onHold (a device usually wires it
  // to factoryReset). Call enableButton() once; loop() services it.
  void enableButton(int pin, bool activeLow = true, uint32_t holdMs = 5000);
  void onHold(VoidCb cb) { onHold_ = cb; }

  bool save();   // persist wifi/name
  void clear();  // delete the config file

private:
  bool load_();
  void buildScanCache_();
  void registerRoutes_();
  void serviceButton_();

  String scanCache_ = "[]";
  bool portalUp_ = false;
  uint32_t rebootAt_ = 0;
  uint32_t portalTimeoutMs_ = 0;   // 0 = no watchdog
  uint32_t portalStart_ = 0;
  VoidCb onTimeout_;

  int btnPin_ = -1;
  bool btnActiveLow_ = true;
  uint32_t btnHoldMs_ = 5000;
  bool btnDown_ = false;
  uint32_t btnStart_ = 0;
  VoidCb onHold_;
};
