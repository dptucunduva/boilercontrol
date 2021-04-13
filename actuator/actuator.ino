#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include <PubSubClient.h>
#include <Dns.h>
#include <Ethernet.h>
#include "actuator.h"

void setup() {
  Serial.begin(9600);

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
  nextTempCheck = millis();
  sensors.requestTemperatures();
  boilerTemp = sensors.getTempC(boilerSensor);
  solarPanelTemp = sensors.getTempC(solarPanelSensor);

  // Ethernet
  startEthernet();

  // MQTT communication
  startMQTT();
}

void loop() {
  // Check if we are connected to the network and to MQTT server.
  checkEthernetAndMqtt();

  // Update temperatures
  updateTemp();
  
  // Handle heater on/off
  checkHeaterStatus();

  // Handle pump on/off
  checkPumpStatus();
}

/**
 *  TEMPERATURE FUNCTIONS
 */
// Get temperature readings from sensors
void updateTemp() {
  if (nextTempCheck <= millis()) {
    nextTempCheck = millis() + TEMP_CHECK_INTERVAL;
    sensors.requestTemperatures();
    float readBoilerTemp = sensors.getTempC(boilerSensor);
    float readSolarPanelTemp = sensors.getTempC(solarPanelSensor);

    Serial.print("Boiler sensor temperature read: ");
    Serial.print(readBoilerTemp);
    Serial.print("°C\n");
    Serial.print("Solar panel sensor temperature read: ");
    Serial.print(readSolarPanelTemp);
    Serial.print("°C\n");

    // COMMENTED: Workaround for eventual innacurate readings. 
    // 85 (boiler) or 45 (panels) are temperaturees that the sensor returns when there is an error reading, so we ignore it.
    if ( readBoilerTemp > 5 && readBoilerTemp < 99 ) { // && readBoilerTemp != 85 ) {
      if (readBoilerTemp != boilerTemp) {
        boilerTemp = readBoilerTemp;
        publishBoilerTemp();
      }
      boilerReadError = 0;
    } else {
      boilerReadError++;
      Serial.print("Boiler read error count increased to ");
      Serial.println(boilerReadError);
    }
    if ( readSolarPanelTemp > 5 && readSolarPanelTemp < 99) { // && readSolarPanelTemp != 45 ) {
      if (readSolarPanelTemp != solarPanelTemp) {
        solarPanelTemp = readSolarPanelTemp;
        publishSolarPanelTemp();
      }
      solarPanelReadError = 0;
    } else {
      solarPanelReadError++;
      Serial.print("Solar panel read error count increased to ");
      Serial.println(boilerReadError);
    }

    // Check if we have reached the error limit.
    if (boilerReadError > 150) {
      boilerReadError = 0;
      // PUBLISH ERROR
      publishBoilerReadError();
    }
    if (solarPanelReadError > 150) {
      solarPanelReadError = 0;
      // PUBLISH ERROR
      publishSolarPanelReadError();
    }
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
  boolean publish = heaterEnabled;
  digitalWrite(HEATER_CONTROL_PIN, HIGH);
  heaterEnabled = false;
  if (publish) {
    publishHeaterStatus();
  }
}

// Enable heater
void enableHeater() {
  boolean publish = !heaterEnabled;
  digitalWrite(HEATER_CONTROL_PIN, LOW);
  heaterEnabled = true;
  if (publish) {
    publishHeaterStatus();
  }
}

// Get Heater ON temperature
float getHeaterOnTemp() {
  return heaterOnTemp;
}

// Set Heater ON temperature
void setHeaterOnTemp(float newTemp) {
  heaterOnTemp = newTemp;
  EEPROM.put(heaterOnEepromAddr, heaterOnTemp);
  publishHeaterOnTemp();
}

// Get Heater OFF temperature
float getHeaterOffTemp() {
  return heaterOffTemp;
}

// Set Heater ON temperature
void setHeaterOffTemp(float newTemp) {
  heaterOffTemp = newTemp;
  EEPROM.put(heaterOffEepromAddr, heaterOffTemp);
  publishHeaterOffTemp();
}

// Get heater override flag
boolean getHeaterOverride() {
  return heaterOverride;
}

// Enable heater override. Arduino won't check for heater on/off temperatures during provided duration
void enableHeaterOverride(unsigned long heaterOverrideDuration) {
  heaterOverride = true;
  if (heaterOverrideDuration > 0) {
    heaterOverrideUntil = millis() + (heaterOverrideDuration * 1000 * 60);
  } else {
    heaterOverrideUntil = millis() + 8640000L;
  }
  publishHeaterOverride();
}

// Disable heater override
void disableHeaterOverride() {
  heaterOverride = false;
  publishHeaterOverride();
}

/**
 * PUMP CONTROL FUNCTIONS
 */
// Disable pump
void disablePump() {
  boolean publish = pumpEnabled;
  if (pumpEnabled) {
    lastTimePumpEnabled = millis();
  }
  digitalWrite(PUMP_CONTROL_PIN, HIGH);
  pumpEnabled = false;
  if (publish) {
    publishPumpStatus();
  }
}

// Enable pump
void enablePump() {
  boolean publish = !pumpEnabled;
  digitalWrite(PUMP_CONTROL_PIN, LOW);
  pumpEnabled = true;
  if (publish) {
    publishPumpStatus();
  }
}

// Enable Cycle
void enableCycle() {
  boolean publish = !cycleEnabled;
  cycleEnabled = true;
  if (publish) {
    publishCycleStatus();
  }
}

// Disable Cycle
void disableCycle() {
  boolean publish = cycleEnabled;
  cycleEnabled = false;
  if (publish) {
    publishCycleStatus();
  }
}

// Get cycle status
boolean getCycleEnabled() {
  return cycleEnabled;
}

// Disable pump override
void disablePumpOverride() {
  pumpOverride = false;
  publishPumpOverride();
}

// Enable pump override
void enablePumpOverride(unsigned long pumpOverrideDuration, unsigned int unit) {
  pumpOverride = true;
  if (pumpOverrideDuration > 0) {
    if (unit == OVERRIDE_UNIT_MINUTES) {
      pumpOverrideUntil = millis() + (pumpOverrideDuration * 1000 * 60);
    } else if (unit == OVERRIDE_UNIT_SECONDS) {
      pumpOverrideUntil = millis() + (pumpOverrideDuration * 1000);
    } else if (unit == OVERRIDE_UNIT_MILISECONDS) {
      pumpOverrideUntil = millis() + pumpOverrideDuration;
    } else {
      // Unsupported unit, default to 24h
      pumpOverrideUntil = millis() + 8640000L;
    }
  } else {
    // No value informed, defult to 24h
    pumpOverrideUntil = millis() + 8640000L;
  }
  publishPumpOverride();
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
  
  // If the pump was not enabled for more than 3 minutes and panel temp is near boiler temp, enable it for 5 seconds for the panel sensor to get an accurate reading.
  if (getCycleEnabled() && !getPumpOverride() && getSolarPanelTemp() >= (getBoilerTemp()-10) && lastTimePumpEnabled + (3L*60L*1000L) < millis()) {
    enablePump();
    enablePumpOverride(5, OVERRIDE_UNIT_SECONDS);
  }
}

/** 
 *  Communication setup functions
 */
// Reset arduino
void(* resetFunc) (void) = 0; 

// Setup and start ethernet
void startEthernet() {
  Serial.println("(Re)Starting ethernet conectivity");
  nextConectivityCheck = millis() + CONECTIVITY_CHECK_INTERVAL;
  Ethernet.begin(mac, 10000L, 3000L);
  dnsClient.begin(Ethernet.dnsServerIP());
  myAddr = Ethernet.localIP();
  dnsClient.getHostByName(MQTT_SERVER_HOSTNAME,mqttAddr);

  if (!sanitycheckClient.connect(MQTT_SERVER_HOSTNAME, 80)) {
    Serial.print("Error connecting to [");
    Serial.print(MQTT_SERVER_HOSTNAME);
    Serial.println(":80]. Will assume ethernet did not start successfuly");
  } else {
    Serial.println("Ethernet (re)started successfuly. IPs that will be used:");
    Serial.print("DHCP assigned IP addres: ");
    Serial.print(myAddr);
    Serial.print("\n");
    Serial.print("MQTT server [");
    Serial.print(MQTT_SERVER_HOSTNAME);
    Serial.print("] resolved IP addres: ");
    Serial.println(mqttAddr);
    sanitycheckClient.stop();
  }
}

// Setup mqtt server connection
void startMQTT() {
  Serial.print("(Re)Connecting to MQTT server at [");
  Serial.print(MQTT_SERVER_HOSTNAME);
  Serial.print("] with ID [");
  Serial.print(BOILERCONTROL_CLIENT_ID);
  Serial.println("]");
  mqttClient.setServer(mqttAddr, 1883);
  mqttClient.setKeepAlive(3000L);
  mqttClient.setCallback(handleCommand);
  mqttClient.connect(BOILERCONTROL_CLIENT_ID);
  if (!mqttClient.connected()) {
    Serial.print("MQTT server connection not OK. Error code ");
    Serial.println(mqttClient.state());
  } else {
    Serial.println("MQTT server connection OK");
    // Subscribe to all command topics - These are the topics that the webapp will use to trigger actions
    mqttClient.subscribe(HEATER_SET_OVERRIDE_TOPIC);
    mqttClient.subscribe(PUMP_SET_OVERRIDE_TOPIC);
    mqttClient.subscribe(CYCLE_SET_STATUS_TOPIC);
    mqttClient.subscribe(HEATER_SET_ON_TEMP);
    mqttClient.subscribe(HEATER_SET_OFF_TEMP);
    mqttClient.subscribe(REQUEST_FULL_REFRESH_TOPIC);
    mqttClient.subscribe(RESET_TOPIC);
    // Now publish a full status update.
    publishAll();
  }
}

//Check if we are connected
void checkEthernetAndMqtt() {
  mqttClient.loop();

  if (nextConectivityCheck < millis()) {
    nextConectivityCheck = millis() + CONECTIVITY_CHECK_INTERVAL;
    // First ethernet. Simply connect to mqttserver por 80 to see if everything is ok.
    if (!sanitycheckClient.connect(MQTT_SERVER_HOSTNAME, 80)) {
      // Restart  Ethernet
      Serial.println("Ethernet check not OK");
      startEthernet();
    } else {
      Serial.println("Ethernet check OK");
      sanitycheckClient.stop();
    }
    
    // Now MQTT
    if (!mqttClient.connected()) {
      Serial.print("MQTT server connection not OK. Error code ");
      Serial.println(mqttClient.state());
      startMQTT();
    } else {
      Serial.println("MQTT server connection OK");
    }
  }
}

/**
 * MQTT topic message handling functions
 */
// Publish a complete status update
void publishAll() {
  Serial.println("Starting a full publish");
  publishBoilerTemp();
  publishSolarPanelTemp();
  publishHeaterStatus();
  publishPumpStatus();
  publishCycleStatus();
  publishHeaterOnTemp();
  publishHeaterOffTemp();
  publishHeaterOverride();
  publishPumpOverride();
  Serial.println("Full publish finished");
}

// Publish Solar Panel read error
void publishSolarPanelReadError() {
  String message = "solarPanelTempSensor";
  if (mqttClient.publish(EMERGENCY_TOPIC,message.c_str(),false)) {
    Serial.println("Published solar panel read error alert");
  }else {
    Serial.println("Error publishing solar panel read error alert");
  }
}

// Publish Boiler read error
void publishBoilerReadError() {
  String message = "boilerTempSensor";
  if (mqttClient.publish(EMERGENCY_TOPIC,message.c_str(),false)) {
    Serial.println("Published boiler read error alert");
  }else {
    Serial.println("Error publishing boiler read error alert");
  }
}

// Publish boiler temperature
void publishBoilerTemp() {
  String message = "";
  message += getBoilerTemp();
  if (mqttClient.publish(BOILER_TEMP_TOPIC,message.c_str(),true)) {
    Serial.print("Published boiler temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }else {
    Serial.print("Error publishing boiler temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }
}

// Publish solar panel temperature
void publishSolarPanelTemp() {
  String message = "";
  message += getSolarPanelTemp();
  if(mqttClient.publish(SOLAR_PANEL_TEMP_TOPIC,message.c_str(),true)) {
    Serial.print("Published solar panel temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }else {
    Serial.print("Error publishing solar panel temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }
}

// Publish heater status update
void publishHeaterStatus() {
  String message = heaterEnabled ? "enabled" : "disabled";
  if(mqttClient.publish(HEATER_STATUS_TOPIC,message.c_str(),true)) {
    Serial.print("Published heater status update to ");
    Serial.println(message);
  }else {
    Serial.print("Error publishing heater status update to ");
    Serial.println(message);
  }
}

// Publish pump status update
void publishPumpStatus() {
  String message = pumpEnabled ? "enabled" : "disabled";
  if(mqttClient.publish(PUMP_STATUS_TOPIC,message.c_str(),true)) {
    Serial.print("Published pump status update to ");
    Serial.println(message);
  }else {
    Serial.print("Error publishing pump status update to ");
    Serial.println(message);
  }
}

// Publish pump cycle status update
void publishCycleStatus() {
  String message = cycleEnabled ? "enabled" : "disabled";
  if(mqttClient.publish(CYCLE_STATUS_TOPIC,message.c_str(),true)) {
    Serial.print("Published pump cycle status update to ");
    Serial.println(message);
  }else {
    Serial.print("Error publishing pump cycle status update to ");
    Serial.println(message);
  }
}

// Publish heater on temperature (heater is enabled if water temperature is below this value)
void publishHeaterOnTemp() {
  String message = "";
  message += getHeaterOnTemp();
  if (mqttClient.publish(HEATER_ON_TEMP_TOPIC,message.c_str(),true)) {
    Serial.print("Published heater on temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }else {
    Serial.print("Error publishing heater on temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }
}

// Publish heater off temperature (heater is disabled if water temperature is over this value)
void publishHeaterOffTemp() {
  String message = "";
  message += getHeaterOffTemp();
  if (mqttClient.publish(HEATER_OFF_TEMP_TOPIC,message.c_str(),true)) {
    Serial.print("Published heater off temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }else {
    Serial.print("Error publishing heater off temperature update to ");
    Serial.print(message);
    Serial.println(" °C");
  }
}

// Publish heater override status - publish "disabled" or for how long, in minutes, override will take place
void publishHeaterOverride() {
  String message = "";
  if (getHeaterOverride()) {
    message += ((heaterOverrideUntil - millis())/1000);
  } else {
    message += "disabled";
  }

  if (mqttClient.publish(HEATER_OVERRIDE_TOPIC,message.c_str(),true)) {
    Serial.print("Published heater override update. ");
    if (getHeaterOverride()) {
      Serial.print("It will remain ");
      Serial.print(heaterEnabled ? "ON for " : "OFF for ");
      Serial.print(message);
      Serial.println(" seconds");
    }else {
      Serial.println("There is no override in place");
    }
  }else {
    Serial.println("Error publishing heater override update");
  }
}

// Publish pump override status - publish "disabled" or for how long, in minutes, override will take place
void publishPumpOverride() {
  String message = "";
  if (getPumpOverride()) {
    message += ((pumpOverrideUntil - millis())/1000);
  } else {
    message += "disabled";
  }

  if (mqttClient.publish(PUMP_OVERRIDE_TOPIC,message.c_str(),true)) {
    Serial.print("Published pump override update. ");
    if (getPumpOverride()) {
      Serial.print("It will remain ");
      Serial.print(pumpEnabled ? "ON for " : "OFF for ");
      Serial.print(message);
      Serial.println(" seconds");
    }else {
      Serial.println("There is no override in place");
    }
  }else {
    Serial.println("Error publishing pump override update");
  }
}

// Callback function - handle mqtt events
void handleCommand(char* topic, byte* payload, unsigned int length) {
  // Convert payload to String
  String payloadMessage = "";
  for (int i = 0; i < length; i++)
  {
    payloadMessage += (char)payload[i];
  }

  // Log it
  Serial.print("Received message [");
  Serial.print(payloadMessage);
  Serial.print("] in topic [");
  Serial.print(topic);
  Serial.println("]");

  // Check which topic we got the message from and dispatch it.
  if(strcmp(topic, HEATER_SET_OVERRIDE_TOPIC) == 0) {
    handleHeaterOverrideMessage(payloadMessage);
  } else if (strcmp(topic, PUMP_SET_OVERRIDE_TOPIC) == 0) {
    handlePumpOverrideMessage(payloadMessage);
  } else if (strcmp(topic, CYCLE_SET_STATUS_TOPIC) == 0) {
    handleCycleMessage(payloadMessage);
  } else if (strcmp(topic, HEATER_SET_ON_TEMP) == 0) {
    handleHeaterRangeMessage(true, payloadMessage);
  } else if (strcmp(topic, HEATER_SET_OFF_TEMP) == 0) {
    handleHeaterRangeMessage(false, payloadMessage);
  } else if (strcmp(topic, REQUEST_FULL_REFRESH_TOPIC) == 0) {
    publishAll();
  } else if (strcmp(topic, RESET_TOPIC) == 0) {
    resetFunc();
  } else {
    Serial.println("Topic not recognized, discarding message");
  }
}

// Heater override message handler
void handleHeaterOverrideMessage(String payloadMessage) {
  unsigned long heaterOverrideDuration = 0;
  if (payloadMessage.equals("auto")) {
    disableHeaterOverride();
  } else if (payloadMessage.startsWith("enable")) {
    if (payloadMessage.charAt(6) == ',') {
      heaterOverrideDuration = atol(payloadMessage.substring(7, 11).c_str());
    }
    enableHeater();
    enableHeaterOverride(heaterOverrideDuration);
  } else if (payloadMessage.startsWith("disable")) {
    if (payloadMessage.charAt(7) == ',') {
      heaterOverrideDuration = atol(payloadMessage.substring(8, 12).c_str());
    }
    disableHeater();
    enableHeaterOverride(heaterOverrideDuration);
  } else {
    Serial.println("Message not recognized. Possible values are \"auto\", \"enable,9999\" or \"disable,9999\", where 9999 is the override duration in minutes");
  }
}

// Pump override message handler
void handlePumpOverrideMessage(String payloadMessage) {
  unsigned long pumpOverrideDuration = 0;
  if (payloadMessage.equals("auto")) {
    disablePumpOverride();
  } else if (payloadMessage.startsWith("enable")) {
    if (payloadMessage.charAt(6) == ',') {
      pumpOverrideDuration = atol(payloadMessage.substring(7, 11).c_str());
    }
    enablePump();
    enablePumpOverride(pumpOverrideDuration, OVERRIDE_UNIT_MINUTES);
  } else if (payloadMessage.startsWith("disable")) {
    if (payloadMessage.charAt(7) == ',') {
      pumpOverrideDuration = atol(payloadMessage.substring(8, 12).c_str());
    }
    disablePump();
    enablePumpOverride(pumpOverrideDuration, OVERRIDE_UNIT_MINUTES);
  } else {
    Serial.println("Message not recognized. Possible values are \"auto\", \"enable,9999\" or \"disable,9999\", where 9999 is the override duration in minutes");
  }
}

// Cycle message handler
void handleCycleMessage(String payloadMessage) {
  if (payloadMessage.equals("enable")) {
    enableCycle();
  } else if (payloadMessage.equals("disable")) {
    disableCycle();
  } else {
    Serial.println("Message not recognized. Possible values are \"enable\" or \"disable\"");
  }
}

// Heater temperature range message handler
void handleHeaterRangeMessage(boolean onTemp, String payloadMessage) {
  long temperature = atol(payloadMessage.c_str());
  if (temperature >= 0 && temperature <= 99) {
    onTemp ? setHeaterOnTemp(temperature): setHeaterOffTemp(temperature);
  } else {
    Serial.println("Invalid temperature. It must be a valid number between 0 and 99");
  }
}

/**
 * EEPROM functions
 */
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
