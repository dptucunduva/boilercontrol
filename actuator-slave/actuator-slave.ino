#include <ACS712.h>
#include "actuator-slave.h"

void setup() {
  Serial.begin(115200);

  // Setup current sensors  
  //Serial.println(acSensor05.calibrate());
  //Serial.println(acSensor04.calibrate());
  //Serial.println(acSensor03.calibrate());
  //Serial.println(acSensor02.calibrate());
  //Serial.println(acSensor01.calibrate());
  //Serial.println(acSensor00.calibrate());
  acSensor05.setZeroPoint(509);
  acSensor04.setZeroPoint(510);
  acSensor03.setZeroPoint(498);
  acSensor02.setZeroPoint(509);
  acSensor01.setZeroPoint(103);
  acSensor00.setZeroPoint(508);
}

void loop() {
  // Send data only when you receive a "query for data" byte
  if (Serial.available() > 0) {
    // If byte is the letter 'Q', that means that master arduino is querying for data
    byte incomingByte = Serial.read();
    if(incomingByte=='Q'){
      // Write response
      String dataToSend = getData();
      Serial.println(dataToSend);
    }
  }

  // Read sensors data
  readCurrent();
}

/** 
 *  Build response string
 */
String getData() {
  char buffer[10];
  char buffer2[10];
  String data = "";
  dtostrf(acCurrent00, 8, 2, buffer);
  data += buffer;
  dtostrf(acPower00, 8, 2, buffer2);
  data += buffer2;
  dtostrf(acCurrent01, 8, 2, buffer);
  data += buffer;
  dtostrf(acPower01, 8, 2, buffer);
  data += buffer;
  dtostrf(acCurrent02, 8, 2, buffer);
  data += buffer;
  dtostrf(acPower02, 8, 2, buffer);
  data += buffer;
  dtostrf(acCurrent03, 8, 2, buffer);
  data += buffer;
  dtostrf(acPower03, 8, 2, buffer);
  data += buffer;
  dtostrf(acCurrent04, 8, 2, buffer);
  data += buffer;
  dtostrf(acPower04, 8, 2, buffer);
  data += buffer;
  dtostrf(acCurrent05, 8, 2, buffer);
  data += buffer;
  dtostrf(acPower05, 8, 2, buffer);
  data += buffer;

  return data;
}

/**
 * Current monitoring functions
 */
void readCurrent() {
  acCurrent05 = acSensor05.getCurrentAC(60);
  acPower05 = acCurrent05 * VOLTAGE_05;
  acCurrent04 = acSensor04.getCurrentAC(60);
  acPower04 = acCurrent04 * VOLTAGE_04;
  acCurrent03 = acSensor03.getCurrentAC(60);
  acPower03 = acCurrent03 * VOLTAGE_03;
  acCurrent02 = acSensor02.getCurrentAC(60);
  acPower02 = acCurrent02 * VOLTAGE_02;
  acCurrent01 = acSensor01.getCurrentAC(60);
  acPower01 = acCurrent01 * VOLTAGE_01;
  acCurrent00 = acSensor00.getCurrentAC(60);
  acPower00 = acCurrent00 * VOLTAGE_00;
}

