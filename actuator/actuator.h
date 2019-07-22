/**
 * TODO
 */

// General constants
#define CHECK_INTERVAL 5000

// Pin layout
#define HEATER_CONTROL_PIN 40
#define PUMP_CONTROL_PIN 41
#define TEMP_SENSOR_PIN 30

// DS18B20 temperature sensor setup
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
//DeviceAddress solarPanelSensor = { 0x28, 0x31, 0xCC, 0x77, 0x91, 0x09, 0x02, 0x64 }; //Spare
DeviceAddress solarPanelSensor = { 0x28, 0x9D, 0x1B, 0x77, 0x91, 0x06, 0x02, 0x52 }; 
DeviceAddress boilerSensor = { 0x28, 0xFF, 0x9B, 0x09, 0xB3, 0x17, 0x01, 0x98 };

// Temperature setup
float boilerTemp = 45;
float solarPanelTemp = 45;
const float HEATER_ON_TEMP_DEFAULT = 41;
const float HEATER_OFF_TEMP_DEFAULT = 42;
const int heaterOnEepromAddr = 10;
const int heaterOffEepromAddr = 20;
float heaterOnTemp = HEATER_ON_TEMP_DEFAULT;
float heaterOffTemp = HEATER_OFF_TEMP_DEFAULT;
boolean heaterEnabled;
boolean tempOverride;
unsigned long heaterOverrideUntil;

// Pump setup
boolean pumpEnabled;
boolean pumpOverride;
unsigned long pumpOverrideUntil;

