#include "FirelabsCore.h"
#include "FirelabsSetupPage.h"
#include "FirelabsLogo.h"

#include <DNSServer.h>
#include <LittleFS.h>
#include <Update.h>
#include <ArduinoJson.h>

#if defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESP8266WebServer.h>
  using FlWebServer = ESP8266WebServer;
#elif defined(ESP32)
  #include <WiFi.h>
  #include <WebServer.h>
  using FlWebServer = WebServer;
#endif

static const char* kPath = "/fl-core.json";

static FlWebServer server(80);
static DNSServer dns;
static FirelabsCore* self = nullptr;

// ---------- identity ----------

String FirelabsCore::macSuffix() {
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char buf[7];
  snprintf(buf, sizeof(buf), "%02X%02X%02X", mac[3], mac[4], mac[5]);
  return String(buf);
}

static String sanitize(const String& in) {
  String out;
  for (size_t i = 0; i < in.length(); i++) {
    char c = in[i];
    if (c >= 'A' && c <= 'Z') c = c - 'A' + 'a';
    if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) out += c;
    else if ((c == ' ' || c == '-' || c == '_') && out.length() && out[out.length() - 1] != '-') out += '-';
  }
  while (out.length() && out[out.length() - 1] == '-') out.remove(out.length() - 1);
  return out;
}

String FirelabsCore::hostname() const {
  String s = sanitize(deviceName);
  if (!s.length()) { s = macSuffix(); s.toLowerCase(); }
  return hostPrefix + "-" + s;
}

String FirelabsCore::apSsid() const {
  return apPrefix + " " + macSuffix();
}

// ---------- config ----------

bool FirelabsCore::load_() {
  if (!LittleFS.begin()) {
#if defined(ESP32)
    LittleFS.begin(true);  // format if needed
#endif
  }
  File f = LittleFS.open(kPath, "r");
  if (!f) return false;
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) return false;
  wifiSsid   = doc["wifi_ssid"] | "";
  wifiPass   = doc["wifi_pass"] | "";
  deviceName = doc["name"]      | "";
  return true;
}

bool FirelabsCore::save() {
  LittleFS.begin();
  JsonDocument doc;
  doc["wifi_ssid"] = wifiSsid;
  doc["wifi_pass"] = wifiPass;
  doc["name"]      = deviceName;
  File f = LittleFS.open(kPath, "w");
  if (!f) return false;
  serializeJson(doc, f);
  f.close();
  return true;
}

void FirelabsCore::clear() {
  LittleFS.begin();
  LittleFS.remove(kPath);
}

void FirelabsCore::begin() {
  self = this;
#if defined(ESP32)
  LittleFS.begin(true);
#else
  LittleFS.begin();
#endif
  load_();
}

void FirelabsCore::factoryReset() {
  clear();
  delay(200);
  ESP.restart();
}

// ---------- wifi ----------

bool FirelabsCore::connect(uint32_t timeoutMs) {
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.setHostname(hostname().c_str());
  WiFi.begin(wifiSsid.c_str(), wifiPass.c_str());
  uint32_t t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < timeoutMs) delay(150);
  return WiFi.status() == WL_CONNECTED;
}

void FirelabsCore::buildScanCache_() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  int n = WiFi.scanNetworks();
  String s = "[";
  int count = 0;
  for (int i = 0; i < n && count < 20; i++) {
    String ssid = WiFi.SSID(i);
    if (!ssid.length()) continue;
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    if (count++) s += ",";
    s += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  s += "]";
  scanCache_ = s;
  WiFi.scanDelete();
}

// ---------- setup portal ----------

void FirelabsCore::registerRoutes_() {
  auto servePage = []() { server.send_P(200, "text/html", FL_SETUP_HTML); };
  server.on("/", HTTP_GET, servePage);
  server.onNotFound(servePage);  // captive-portal catch-all

  server.on("/logo.png", HTTP_GET, []() {
    server.send_P(200, "image/png", (PGM_P)LOGO_PNG, LOGO_PNG_LEN);
  });

  server.on("/api/scan", HTTP_GET, []() {
    server.send(200, "application/json", self->scanCache_);
  });

  server.on("/api/provision", HTTP_POST, []() {
    JsonDocument d;
    if (deserializeJson(d, server.arg("plain"))) { server.send(400, "application/json", "{\"err\":1}"); return; }
    self->wifiSsid   = (const char*)(d["ssid"] | "");
    self->wifiPass   = (const char*)(d["pass"] | "");
    self->deviceName = (const char*)(d["name"] | "");
    self->save();
    server.send(200, "application/json", "{\"ok\":true}");
    self->rebootAt_ = millis() + 2000;  // reboot into station mode
  });

  server.on("/update", HTTP_POST,
    []() {
      server.send(200, "text/plain", Update.hasError() ? "FAIL" : "OK, rebooting");
      if (!Update.hasError()) self->rebootAt_ = millis() + 800;
    },
    []() {
      HTTPUpload& up = server.upload();
      if (up.status == UPLOAD_FILE_START) {
#if defined(ESP8266)
        uint32_t maxSketch = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
        Update.begin(maxSketch);
#else
        Update.begin(UPDATE_SIZE_UNKNOWN);
#endif
      } else if (up.status == UPLOAD_FILE_WRITE) {
        Update.write(up.buf, up.currentSize);
      } else if (up.status == UPLOAD_FILE_END) {
        Update.end(true);
      }
    });
}

void FirelabsCore::startSetupPortal(uint32_t timeoutMs) {
  self = this;
  buildScanCache_();            // scan before the AP exists (the S31 lesson)
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apSsid().c_str());
  dns.setErrorReplyCode(DNSReplyCode::NoError);
  dns.start(53, "*", WiFi.softAPIP());
  registerRoutes_();
  server.begin();
  portalUp_ = true;
  portalTimeoutMs_ = timeoutMs;
  portalStart_ = millis();
}

// ---------- button ----------

void FirelabsCore::enableButton(int pin, bool activeLow, uint32_t holdMs) {
  btnPin_ = pin;
  btnActiveLow_ = activeLow;
  btnHoldMs_ = holdMs;
  pinMode(pin, activeLow ? INPUT_PULLUP : INPUT);
}

void FirelabsCore::serviceButton_() {
  if (btnPin_ < 0) return;
  bool down = digitalRead(btnPin_) == (btnActiveLow_ ? LOW : HIGH);
  if (down && !btnDown_) { btnDown_ = true; btnStart_ = millis(); }
  else if (down && btnDown_ && millis() - btnStart_ >= btnHoldMs_) {
    btnDown_ = false;
    if (onHold_) onHold_();
  } else if (!down) btnDown_ = false;
}

void FirelabsCore::loop() {
  if (portalUp_) {
    dns.processNextRequest();
    server.handleClient();
  }
  serviceButton_();

  // Portal watchdog: a provisioned device that fell back to setup shouldn't camp
  // here forever. When the window elapses, reboot to retry wifi (the outage may
  // be over), or hand off to the device via onTimeout (e.g. the WX deep-sleeps).
  if (portalUp_ && portalTimeoutMs_ && !rebootAt_ &&
      millis() - portalStart_ > portalTimeoutMs_) {
    if (onTimeout_) onTimeout_();
    else ESP.restart();
    return;
  }

  if (rebootAt_ && millis() > rebootAt_) ESP.restart();
}
