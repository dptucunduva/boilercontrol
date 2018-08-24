#include <ESP8266.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "actuator.h"

void setup() {
  Serial.begin(115200);

  // EEPROM check and setup
  eepromCheck();

  // Setup pins
  pinMode(HEATER_CONTROL_PIN, OUTPUT);
  pinMode(PUMP_CONTROL_PIN, OUTPUT);

  // Startup system
  disableHeater();
  disableHeaterOverride();
  disablePump();
  disablePumpOverride();

  // Setup temp sensor
  sensors.begin();
  sensors.setResolution(boilerSensor, 10);
  sensors.setResolution(solarPanelSensor, 10);

  // Startup WiFi
  setupWiFi();
}

void loop() {
  // Handle http request
  httpRequest();

  // Handle heater on/off
  checkHeaterStatus();

  // Handle pump on/off
  checkPumpStatus();
}

/**
 *  TEMPERATURE FUNCTIONS
 */
// Get current temperature
float getBoilerTemp() {
  sensors.requestTemperatures();
  return sensors.getTempC(boilerSensor);
}

// Get current temperature
float getSolarPanelTemp() {
  sensors.requestTemperatures();
  return sensors.getTempC(solarPanelSensor);
}

// Get Heater ON temperature
float getHeaterOnTemp() {
  return heaterOnTemp;
}

// Set Heater ON temperature
void setHeaterOnTemp(float newTemp) {
  heaterOnTemp = newTemp;
  EEPROM.put(heaterOnEepromAddr, heaterOnTemp);
}

// Get Heater OFF temperature
float getHeaterOffTemp() {
  return heaterOffTemp;
}

// Set Heater ON temperature
void setHeaterOffTemp(float newTemp) {
  heaterOffTemp = newTemp;
  EEPROM.put(heaterOffEepromAddr, heaterOffTemp);
}

// Get temp override flag
boolean getHeaterOverride() {
  return tempOverride;
}

// Get temp override flag
void setHeaterOverride(boolean newOverride, unsigned long heaterOverrideDuration) {
  tempOverride = newOverride;
  if (heaterOverrideDuration > 0) {
    heaterOverrideUntil = millis() + (heaterOverrideDuration * 1000 * 60);
  } else {
    heaterOverrideUntil = millis() + 8640000L;
  }
}

// Disable temp override
void disableHeaterOverride() {
  tempOverride = false;
}

/**
 * HEATER CONTROL FUNCTIONS
 */
// Check if heater should be turned on or off
void checkHeaterStatus() {
  // First check temp override
  if (getHeaterOverride() && heaterOverrideUntil < millis() && heaterOverrideUntil > 0) {
    disableHeaterOverride();
    heaterOverrideUntil = 0;
  }

  // Higher than max temp - disable
  if (getBoilerTemp() > getHeaterOffTemp() && !getHeaterOverride()) {
    disableHeater();
  }

  // Lower than min temp - enable
  if (getBoilerTemp() < getHeaterOnTemp() && !getHeaterOverride()) {
    enableHeater();
  }
}

// Disable heater
void disableHeater() {
  digitalWrite(HEATER_CONTROL_PIN, HIGH);
  heaterEnabled = false;
}

// Enable heater
void enableHeater() {
  digitalWrite(HEATER_CONTROL_PIN, LOW);
  heaterEnabled = true;
}

/**
 * PUMP CONTROL FUNCTIONS
 */
// Disable pump
void disablePump() {
  digitalWrite(PUMP_CONTROL_PIN, LOW);
  pumpEnabled = false;
}

// Enable pump
void enablePump() {
  digitalWrite(PUMP_CONTROL_PIN, HIGH);
  pumpEnabled = true;
}

// Disable pump override
void disablePumpOverride() {
  pumpOverride = false;
}

// Get pump override flag
void setPumpOverride(boolean newOverride, unsigned long pumpOverrideDuration) {
  pumpOverride = newOverride;
  if (pumpOverrideDuration > 0) {
    pumpOverrideUntil = millis() + (pumpOverrideDuration * 1000 * 60);
  } else {
    pumpOverrideUntil = millis() + 8640000L;
  }
}

// Get pump override flag
boolean getPumpOverride() {
  return pumpOverride;
}

// Check if pump should be turned on or off
void checkPumpStatus() {
  // Check pump override
  if (getPumpOverride() && pumpOverrideUntil < millis() && pumpOverrideUntil > 0) {
    disablePumpOverride();
    pumpOverrideUntil = 0;
  } 
    
  // Check if pump shoud be enabled or disabled - Enabled only if solarPanelTemp is 5C higher or more, and disabled if it is 2C higher or less
  if (getSolarPanelTemp() >= (getBoilerTemp()+5) && !getPumpOverride()) {
    enablePump();
  } else if (getSolarPanelTemp() <= (getBoilerTemp()+2) && !getPumpOverride()) {
    disablePump();
  }
}

/**
 * WIFI and HTTP CONTROL/STATUS FUNCTIONS
 */
// Main http request listener
void httpRequest() {
  uint8_t buffer[256] = {0};
  uint8_t mux_id;
  uint32_t len = wifi.recv(&mux_id, buffer, sizeof(buffer), 100);
  if (len > 0) {
    // Handle command
    String command = "";
    for (int i = 0; i < len; i++) {
      Serial.print((char)buffer[i]);
      command += (char)buffer[i];
    }
    handleHttpCommand(mux_id, command);
  }
}

// Handle http request
void handleHttpCommand(uint8_t mux_id, String command) {
  // This is the response that will be built.
  String response;

  Serial.println(command);
  if (command.startsWith("GET")) {
    response = getStatus();
  } else if (command.startsWith("PUT")) {
    unsigned long heaterOverrideDuration = 0;
    unsigned long pumpOverrideDuration = 0;
    if (command.startsWith("PUT /reset")) {
      disableHeater();
      disableHeaterOverride();
      disablePumpOverride();
      disablePump();
      response = getStatus();
    } else if (command.startsWith("PUT /temp/on/")) {
      setHeaterOnTemp(command.substring(13, 15).toFloat());
      disableHeaterOverride();
      response = getStatus();
    } else if (command.startsWith("PUT /temp/off/")) {
      setHeaterOffTemp(heaterOffTemp = command.substring(14, 16).toFloat());
      disableHeaterOverride();
      response = getStatus();
    } else if (command.startsWith("PUT /heater/on")) {
      if (command.charAt(14) == '/') {
        heaterOverrideDuration = atol(command.substring(15, 19).c_str());
      }
      enableHeater();
      setHeaterOverride(true, heaterOverrideDuration);
      response = getStatus();
    } else if (command.startsWith("PUT /heater/auto")) {
      disableHeaterOverride();
      response = getStatus();
    } else if (command.startsWith("PUT /heater/off")) {
      if (command.charAt(15) == '/') {
        heaterOverrideDuration = atol(command.substring(16, 20).c_str());
      }
      disableHeater();
      setHeaterOverride(true, heaterOverrideDuration);
      response = getStatus();
    } else if (command.startsWith("PUT /pump/auto")) {
      disablePumpOverride();
      response = getStatus();
    } else if (command.startsWith("PUT /pump/off")) {
      if (command.charAt(13) == '/') {
        pumpOverrideDuration = atol(command.substring(14, 19).c_str());
      }
      disablePump();
      setPumpOverride(true, pumpOverrideDuration);
      response = getStatus();
    } else {
      response = getBadRequestResponse();
    }
  } else {
    response = getBadRequestResponse();
  }

  // Send response
  wifi.send(mux_id, (const uint8_t*)response.c_str(), response.length());
  wifi.releaseTCP(mux_id);
}

// Get Bad request response - command not understood
String getBadRequestResponse() {
  return "HTTP/1.1 400 Bad Request\r\nServer: Arduino/ESP8266\r\nContent-Type: application/json\r\nContent-Length: 33\r\nConnection: Closed\r\n\r\n{\"error\":\"Command not supported\"}";
}

// Build json response
String getStatus() {
  String responseHeader = "HTTP/1.1 200 OK\r\n";
  responseHeader += "Server: Arduino/ESP8266\r\n";
  responseHeader += "Content-Type: application/json\r\n";
  responseHeader += "Connection: Closed\r\n";
  responseHeader += "Content-Length: ";
  String responseBody = "{\"boilerTemperature\":";
  responseBody += getBoilerTemp();
  responseBody += ", \"solarPanelTemperature\": ";
  responseBody += getSolarPanelTemp();
  responseBody += ", \"heaterOnTemperature\": ";
  responseBody += getHeaterOnTemp();
  responseBody += ", \"heaterOffTemperature\": ";
  responseBody += getHeaterOffTemp();
  responseBody += ", \"heaterEnabled\": ";
  responseBody += heaterEnabled ? "true" : "false";
  responseBody += ", \"heaterOverride\": ";
  responseBody += getHeaterOverride() ? "true" : "false";
  if (getHeaterOverride()) {
    responseBody += ", \"heaterOverrideUntil\": ";
    responseBody += (heaterOverrideUntil - millis());
  }
  responseBody += ", \"pumpEnabled\": ";
  responseBody += pumpEnabled ? "true" : "false";
  responseBody += ", \"pumpOverride\": ";
  responseBody += getPumpOverride() ? "true" : "false";
  if (getPumpOverride()) {
    responseBody += ", \"pumpOverrideUntil\": ";
    responseBody += (pumpOverrideUntil - millis());
  } 
  responseBody += "}";
  String response = responseHeader;
  response += responseBody.length();
  response += "\r\n\r\n";
  response += responseBody;
  return response;
}

/**
 * SETUP FUNCTIONS
 */
// Connect to wifi network and setup listen port
void setupWiFi() {
  ESP8266_SERIAL.begin(ESP8266_SPEED);
  wifi.setOprToStation();
  wifi.joinAP(WIFI_SSID, WIFI_PASSWORD);
  wifi.enableMUX();
  wifi.startTCPServer(WIFI_SERVER_PORT);
  wifi.setTCPServerTimeout(WIFI_SERVER_TIMEOUT);
  ESP8266_SERIAL.println("AT+CIPSTA=\"192.168.1.211\"");
}

// Check EEPROM and reset if needed.
void eepromCheck() {
  float hasData = 0;
  EEPROM.get(0, hasData);
  if (hasData > 0) {
    EEPROM.get(heaterOnEepromAddr, heaterOnTemp);
    EEPROM.get(heaterOffEepromAddr, heaterOffTemp);
  } else {
    for (int i = 0 ; i < EEPROM.length() ; i++) {
      EEPROM.write(i, 0);
    }
    EEPROM.put(0, (float)1);
    EEPROM.put(heaterOnEepromAddr, heaterOnTemp);
    EEPROM.put(heaterOffEepromAddr, heaterOffTemp);
  }
}

