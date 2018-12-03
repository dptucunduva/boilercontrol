/**
 * TODO
 */

// General constants
#define CHECK_INTERVAL 5000

// Pin layout
#define HEATER_CONTROL_PIN 40
#define TEMP_SENSOR_PIN 30
#define PUMP_CONTROL_PIN 41

// ESP8266 setup
#define ESP8266_SERIAL Serial1
#define ESP8266_SPEED 115200
#define WIFI_SERVER_PORT 80
#define WIFI_SERVER_TIMEOUT 5
const String WIFI_SSID = "asdfasd-int";
const String WIFI_PASSWORD = "*********";
ESP8266 wifi(Serial1);

// DS18B20 temperature sensor setup
OneWire oneWire(TEMP_SENSOR_PIN);
DallasTemperature sensors(&oneWire);
DeviceAddress boilerSensor = { 0x28, 0xFF, 0x9B, 0x09, 0xB3, 0x17, 0x01, 0x98 };
DeviceAddress solarPanelSensor = { 0x28, 0x9D, 0x1B, 0x77, 0x91, 0x06, 0x02, 0x52 }; 

// Temperature setup
float boilerTemp = 45;
float solarPanelTemp = 45;
const float HEATER_ON_TEMP_DEFAULT = 27;
const float HEATER_OFF_TEMP_DEFAULT = 38;
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

// Current sensor setup
const float VOLTAGE_15 = 221;
const float VOLTAGE_14 = 117;
const float VOLTAGE_13 = 221;
const float VOLTAGE_12 = 221;
const float VOLTAGE_11 = 221;
const float VOLTAGE_10 = 221;
const float VOLTAGE_09 = 221;
const float VOLTAGE_08 = 221;
const float VOLTAGE_07 = 221;
const float VOLTAGE_06 = 221;
const float VOLTAGE_05 = 221;
const float VOLTAGE_04 = 221;
const float VOLTAGE_03 = 221;
const float VOLTAGE_02 = 221;
const float VOLTAGE_01 = 221;
const float VOLTAGE_00 = 221;
ACS712 acSensor15(ACS712_20A, A15);
ACS712 acSensor14(ACS712_20A, A14);
ACS712 acSensor13(ACS712_20A, A13);
ACS712 acSensor12(ACS712_30A, A12);
ACS712 acSensor11(ACS712_30A, A11);
ACS712 acSensor10(ACS712_30A, A10);
ACS712 acSensor09(ACS712_30A, A9);
ACS712 acSensor08(ACS712_30A, A8);
ACS712 acSensor07(ACS712_30A, A7);
ACS712 acSensor06(ACS712_30A, A6);
ACS712 acSensor05(ACS712_30A, A5);
ACS712 acSensor04(ACS712_30A, A4);
ACS712 acSensor03(ACS712_30A, A3);
ACS712 acSensor02(ACS712_30A, A2);
ACS712 acSensor01(ACS712_30A, A1);
ACS712 acSensor00(ACS712_30A, A0);
float acCurrent21 = 0;
float acCurrent20 = 0;
float acCurrent19 = 0;
float acCurrent18 = 0;
float acCurrent17 = 0;
float acCurrent16 = 0;
float acCurrent15 = 0;
float acCurrent14 = 0;
float acCurrent13 = 0;
float acCurrent12 = 0;
float acCurrent11 = 0;
float acCurrent10 = 0;
float acCurrent09 = 0;
float acCurrent08 = 0;
float acCurrent07 = 0;
float acCurrent06 = 0;
float acCurrent05 = 0;
float acCurrent04 = 0;
float acCurrent03 = 0;
float acCurrent02 = 0;
float acCurrent01 = 0;
float acCurrent00 = 0;
float acPower21 = 0;
float acPower20 = 0;
float acPower19 = 0;
float acPower18 = 0;
float acPower17 = 0;
float acPower16 = 0;
float acPower15 = 0;
float acPower14 = 0;
float acPower13 = 0;
float acPower12 = 0;
float acPower11 = 0;
float acPower10 = 0;
float acPower09 = 0;
float acPower08 = 0;
float acPower07 = 0;
float acPower06 = 0;
float acPower05 = 0;
float acPower04 = 0;
float acPower03 = 0;
float acPower02 = 0;
float acPower01 = 0;
float acPower00 = 0;

// Zeros
/*
int zero00 = 0;
int zero01 = 0;
int zero02 = 0;
int zero03 = 0;
int zero04 = 0;
int zero05 = 0;
int zero06 = 0;
int zero07 = 0;
int zero08 = 0;
int zero09 = 0;
int zero10 = 0;
int zero11 = 0;
int zero12 = 0;
int zero13 = 0;
int zero14 = 0;
int zero15 = 0;
*/
