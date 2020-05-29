/**
 * TODO
 */

// General constants
#define CONECTIVITY_CHECK_INTERVAL 15000;
#define TEMP_CHECK_INTERVAL 2000;
unsigned long nextConectivityCheck = millis() + CONECTIVITY_CHECK_INTERVAL;
unsigned long nextTempCheck = millis() + TEMP_CHECK_INTERVAL;

// Pin layout
#define HEATER_CONTROL_PIN 40
#define PUMP_CONTROL_PIN 41
#define TEMP_SENSOR_PIN 30

// DS18B20 temperature sensor setup
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
//DeviceAddress solarPanelSensor = { 0x28, 0x31, 0xCC, 0x77, 0x91, 0x09, 0x02, 0x64 }; //Spare
//DeviceAddress solarPanelSensor = { 0x28, 0x9D, 0x1B, 0x77, 0x91, 0x06, 0x02, 0x52 }; 
//DeviceAddress boilerSensor = { 0x28, 0xFF, 0x9B, 0x09, 0xB3, 0x17, 0x01, 0x98 };

// REMOVE
DeviceAddress solarPanelSensor = { 0x28, 0x31, 0xCC, 0x77, 0x91, 0x09, 0x02, 0x64 }; //Spare
DeviceAddress boilerSensor = { 0x28, 0x31, 0xCC, 0x77, 0x91, 0x09, 0x02, 0x64 }; //Spare
// REMOVE

// Temperature setup
float boilerTemp = 44;
float solarPanelTemp = 44;
const float HEATER_ON_TEMP_DEFAULT = 44;
const float HEATER_OFF_TEMP_DEFAULT = 46;
const int heaterOnEepromAddr = 10;
const int heaterOffEepromAddr = 20;
float heaterOnTemp = HEATER_ON_TEMP_DEFAULT;
float heaterOffTemp = HEATER_OFF_TEMP_DEFAULT;
boolean heaterEnabled;
boolean heaterOverride;
unsigned long heaterOverrideUntil;

// Pump setup
boolean pumpEnabled;
boolean pumpOverride;
unsigned long pumpOverrideUntil;
unsigned long lastTimePumpEnabled = 0L;
boolean cycleEnabled = false;

// Ethernet setup
byte mac[] = {0xB0,0xCD,0xAE,0x0F,0xDE,0x10};
EthernetClient ethClient;
DNSClient dnsClient;
IPAddress myAddr;
IPAddress mqttAddr;
EthernetClient sanitycheckClient;

// MQTT setup
PubSubClient mqttClient(ethClient);
const char* MQTT_SERVER_HOSTNAME = "mqtt.home";
const char* BOILERCONTROL_CLIENT_ID = "boiler-sensors-and-actuators";

// Channels that we will publish to
const char* BOILER_TEMP_TOPIC = "/boiler/sensor/reservoir/temperature";
const char* SOLAR_PANEL_TEMP_TOPIC = "/boiler/sensor/solarpanel/temperature";
const char* HEATER_STATUS_TOPIC = "/boiler/sensor/heater/status";
const char* HEATER_ON_TEMP_TOPIC = "/boiler/sensor/heater/ontemp";
const char* HEATER_OFF_TEMP_TOPIC = "/boiler/sensor/heater/offtemp";
const char* HEATER_OVERRIDE_TOPIC = "/boiler/sensor/heater/override";
const char* PUMP_STATUS_TOPIC = "/boiler/sensor/pump/status";
const char* PUMP_OVERRIDE_TOPIC = "/boiler/sensor/pump/override";
const char* CYCLE_STATUS_TOPIC = "/boiler/sensor/cycle/status";

// Channels that we will listen to
const char* REQUEST_FULL_REFRESH_TOPIC = "/boiler/requestfullrefresh";
const char* HEATER_SET_OVERRIDE_TOPIC = "/boiler/actuator/heater/setoverride";
const char* HEATER_SET_ON_TEMP = "/boiler/actuator/heater/setontemp";
const char* HEATER_SET_OFF_TEMP = "/boiler/actuator/heater/setofftemp";
const char* PUMP_SET_OVERRIDE_TOPIC = "/boiler/actuator/pump/setoverride";
const char* CYCLE_SET_STATUS_TOPIC = "/boiler/actuator/cycle/setstatus";
