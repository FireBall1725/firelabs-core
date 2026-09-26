# firelabs-core

Shared onboarding for FireLabs devices: a captive-portal setup wizard, wifi
provisioning, config storage in LittleFS, OTA recovery, and a hold-to-reset
button. One library so every FireLabs device sets up the same way, on both
ESP8266 and ESP32.

Setup mode and a device's runtime are mutually exclusive, so the core owns the
whole setup experience with its own sync web server. A device sets its branding,
then calls `begin()`, `connect()`, and `startSetupPortal()`.

## Use

PlatformIO, via `lib_deps`:

```ini
lib_deps = https://github.com/FireBall1725/firelabs-core.git
```

Minimal sketch:

```cpp
#include <FirelabsCore.h>
FirelabsCore core;

void setup() {
  core.apPrefix = "FireLabs WX";
  core.hostPrefix = "fl-wx";
  core.begin();
  core.enableButton(0);
  core.onHold([]() { core.factoryReset(); });

  if (!core.hasWifi())            core.startSetupPortal(30UL * 60 * 1000);
  else if (!core.connect(20000)) core.startSetupPortal(5UL * 60 * 1000);
}

void loop() { core.loop(); }
```

`startSetupPortal(timeoutMs)` arms a watchdog: a provisioned device that could
not reach its wifi reboots after the window to retry instead of camping in setup
forever. Pass `0` for fresh first-boot setup, where a human is present.

## Support

Questions, updates, and works in progress: [FireBall Codes on Discord](https://discord.gg/QpV82CFfVD).

If this saved you some time, you can [buy me a sushi roll](https://ko-fi.com/fireball1725).

## License

AGPL-3.0. Author: FireBall1725.
