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
const float HEATER_ON_TEMP_DEFAULT = 41;
const float HEATER_OFF_TEMP_DEFAULT = 42;
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
const char* BOILER_TEMP_TOPIC = "/boilercontrol/boiler/temperature";
const char* SOLAR_PANEL_TEMP_TOPIC = "/boilercontrol/solarpanel/temperature";
const char* HEATER_STATUS_TOPIC = "/boilercontrol/heater/status";
const char* HEATER_ON_TEMP_TOPIC = "/boilercontrol/heater/ontemp";
const char* HEATER_OFF_TEMP_TOPIC = "/boilercontrol/heater/offtemp";
const char* HEATER_OVERRIDE_TOPIC = "/boilercontrol/heater/override";
const char* HEATER_SET_OVERRIDE_TOPIC = "/boilercontrol/heater/setoverride";
const char* HEATER_SET_ON_TEMP = "/boilercontrol/heater/setontemp";
const char* HEATER_SET_OFF_TEMP = "/boilercontrol/heater/setofftemp";
const char* PUMP_STATUS_TOPIC = "/boilercontrol/pump/status";
const char* PUMP_SET_OVERRIDE_TOPIC = "/boilercontrol/pump/setoverride";
const char* PUMP_OVERRIDE_TOPIC = "/boilercontrol/pump/override";
const char* CYCLE_STATUS_TOPIC = "/boilercontrol/cycle/status";
const char* CYCLE_SET_STATUS_TOPIC = "/boilercontrol/cycle/setstatus";
const char* REQUEST_FULL_REFRESH_TOPIC = "/boilercontrol/requestfullrefresh";