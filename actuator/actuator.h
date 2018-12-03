/**
 * TODO
 */

// General constants
#define CHECK_INTERVAL 5000

// Pin layout
#define HEATER_CONTROL_PIN 40
#define PUMP_CONTROL_PIN 41
#define TEMP_SENSOR_PIN 30

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

// Current sensor setup
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
ACS712 acSensor[] = {acSensor00,acSensor01,acSensor02,acSensor03,acSensor04,acSensor05,acSensor06,acSensor07,acSensor08,acSensor09,acSensor10,acSensor11,acSensor12,acSensor13,acSensor14,acSensor15};
float acVoltage[] = {221,221,221,221,221,221,221,221,221,221,221,221,221,221,117,221};
float acCurrent[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float acPower[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};
float acZero[] = {506,500,503,508,518,98,71,110,103,101,116,133,96,500,509,512};
int acZeroEEPROMAddr[] = {110,120,130,140,150,160,170,180,190,200,210,220,230,240,250,260};
String acSensorName[] = {"16","15","14","13","12","11","10","09","08","07","06","05","04","03","02","01","17","18","19","20","21","22"};

