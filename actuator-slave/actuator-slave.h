// Current sensors setup
const float VOLTAGE_05 = 221;
const float VOLTAGE_04 = 221;
const float VOLTAGE_03 = 221;
const float VOLTAGE_02 = 221;
const float VOLTAGE_01 = 221;
const float VOLTAGE_00 = 221;
ACS712 acSensor05(ACS712_30A, A5);
ACS712 acSensor04(ACS712_30A, A4);
ACS712 acSensor03(ACS712_30A, A3);
ACS712 acSensor02(ACS712_30A, A2);
ACS712 acSensor01(ACS712_30A, A1);
ACS712 acSensor00(ACS712_30A, A0);
float acCurrent05 = 0;
float acCurrent04 = 0;
float acCurrent03 = 0;
float acCurrent02 = 0;
float acCurrent01 = 0;
float acCurrent00 = 0;
float acPower05 = 0;
float acPower04 = 0;
float acPower03 = 0;
float acPower02 = 0;
float acPower01 = 0;
float acPower00 = 0;


