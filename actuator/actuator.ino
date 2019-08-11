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
}

void loop() {
  // Update temperatures
  updateTemp();
  
  // Handle heater on/off
  checkHeaterStatus();

  // Handle pump on/off
  checkPumpStatus();

  // Read commands from angent
  readCommand();
}

/**
 *  TEMPERATURE FUNCTIONS
 */
// Get temperature readings from sensors
void updateTemp() {
  sensors.requestTemperatures();
  float readBoilerTemp = sensors.getTempC(boilerSensor);
  float readSolarPanelTemp = sensors.getTempC(solarPanelSensor);

  // Workaround for eventual innacurate readings. 
  // 85 is a temperature that the sensor returns when there is an error reading, so we ignore it.
  if (readBoilerTemp > 5 && readBoilerTemp < 99 && readBoilerTemp != 85) {
    boilerTemp = readBoilerTemp;
  }
  if (readSolarPanelTemp > 5 && readSolarPanelTemp < 99 && readSolarPanelTemp != 85) {
    solarPanelTemp = readSolarPanelTemp;
  }
}

// Get current temperature
float getBoilerTemp() {
  return boilerTemp;
}

// Get current temperature
float getSolarPanelTemp() {
  return solarPanelTemp;
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
  if (pumpEnabled) {
    lastTimePumpEnabled = millis();
  }
  digitalWrite(PUMP_CONTROL_PIN, LOW);
  pumpEnabled = false;
}

// Enable pump
void enablePump() {
  digitalWrite(PUMP_CONTROL_PIN, HIGH);
  pumpEnabled = true;
}

// Enable Cycle
void enableCycle() {
  cycleEnabled = true;
}

// Disable Cycle
void disableCycle() {
  cycleEnabled = false;
}

// Enable Cycle
boolean getCycleEnabled() {
  return cycleEnabled;
}

// Disable pump override
void disablePumpOverride() {
  pumpOverride = false;
}

// Get pump override flag
void enablePumpOverride(unsigned long pumpOverrideDuration) {
  pumpOverride = true;
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

  // Update pump enabled timestamp
  if (pumpEnabled) {
    lastTimePumpEnabled = millis();
  }
    
  // Check if pump shoud be enabled or disabled - Enabled only if solarPanelTemp is 5C higher or more, and disabled if it is 2C higher or less
  if (getSolarPanelTemp() >= (getBoilerTemp()+5) && !getPumpOverride()) {
    enablePump();
  } else if (getSolarPanelTemp() <= (getBoilerTemp()+2) && !getPumpOverride()) {
    disablePump();
  } 
  
  // If the pump was not enabled for more than 3 minutes and panel temp is near boiler temp, enable it for 3 seconds for the panel sensor to get an accurate reading.
  if (getCycleEnabled() && !getPumpOverride() && getSolarPanelTemp() >= (getBoilerTemp()-10) && lastTimePumpEnabled + (3L*60L*1000L) < millis()) {
    enablePumpOverride(millis() + (3 * 1000));
  }
}

/** 
 *  Communication functions
 */
void readCommand() {
  String command;

  // Read it
  while (Serial.available()) {
    delay(3); // Delay to allow the buffer to fill up
    if (Serial.available() > 0) {
      command += (char)Serial.read();
    }
  } 

  // Check it
  if (command.length() > 0) {
    handleCommand(command);
    // Write response
    Serial.print(getData());
  }
}

// Handle command
void handleCommand(String command) {
  if (command.startsWith("PUT")) {
    unsigned long heaterOverrideDuration = 0;
    unsigned long pumpOverrideDuration = 0;
    if (command.startsWith("PUT /reset")) {
      disableHeater();
      disableHeaterOverride();
      disablePumpOverride();
      disablePump();
    } else if (command.startsWith("PUT /cycle/on")) {
      enableCycle();
    } else if (command.startsWith("PUT /cycle/off")) {
      disableCycle();
    } else if (command.startsWith("PUT /temp/on/")) {
      setHeaterOnTemp(command.substring(13, 15).toFloat());
      disableHeaterOverride();
    } else if (command.startsWith("PUT /temp/off/")) {
      setHeaterOffTemp(heaterOffTemp = command.substring(14, 16).toFloat());
      disableHeaterOverride();
    } else if (command.startsWith("PUT /heater/on")) {
      if (command.charAt(14) == '/') {
        heaterOverrideDuration = atol(command.substring(15, 19).c_str());
      }
      enableHeater();
      setHeaterOverride(true, heaterOverrideDuration);
    } else if (command.startsWith("PUT /heater/auto")) {
      disableHeaterOverride();
    } else if (command.startsWith("PUT /heater/off")) {
      if (command.charAt(15) == '/') {
        heaterOverrideDuration = atol(command.substring(16, 20).c_str());
      }
      disableHeater();
      setHeaterOverride(true, heaterOverrideDuration);
    } else if (command.startsWith("PUT /pump/auto")) {
      disablePumpOverride();
    } else if (command.startsWith("PUT /pump/on")) {
      if (command.charAt(13) == '/') {
        pumpOverrideDuration = atol(command.substring(14, 19).c_str());
      }
      enablePump();
      enablePumpOverride(pumpOverrideDuration);
    } else if (command.startsWith("PUT /pump/off")) {
      if (command.charAt(13) == '/') {
        pumpOverrideDuration = atol(command.substring(14, 19).c_str());
      }
      disablePump();
      enablePumpOverride(pumpOverrideDuration);
    } 
  }
}

// Build json response
String getData() {
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
  responseBody += ", \"cycleEnabled\": ";
  responseBody += cycleEnabled ? "true" : "false";
  responseBody += ", \"pumpOverride\": ";
  responseBody += getPumpOverride() ? "true" : "false";
  if (getPumpOverride()) {
    responseBody += ", \"pumpOverrideUntil\": ";
    responseBody += (pumpOverrideUntil - millis());
  } 
  responseBody += "}\r\n\r\n";
  return responseBody;
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

