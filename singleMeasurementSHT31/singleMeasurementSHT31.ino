#include <DFRobot_SHT3x.h>

/*
    * @brief Constrict the function for REPACSS MEASUREMENT
    * @param pWire I2C bus pointer object and construction device, can both pass or not pass parameters, Wire in default. 
    * @param address Chip I2C address, two optional addresse.

    * @file singleMeasurementSHT31.ino
    * @brief Read ambient temperature (C/F) and relative humidity (%RH) in single-read mode.
    * @n Experimental phenomenon: the chip defaults in this mode, we need to send instructions to enable the chip collect data, we need to set the read time of the sensor to get the data.
    * @n Single measure mode: read data as needed, power consumption is relatively low, the chip idle state only costs 0.5mA.

    * @copyright  Copyright (c) 2025 Texas Tech University
    * @licanse    The MIT License (MIT)
    * @author     [Batuhan Sencer](batuhan.sencer@ttu.edu)
    * @version    V1.0
    * @date       10/17/2025
*/
#include <DFRobot_SHT3x.h>

DFRobot_SHT3x sht3x(&Wire, 0x44, 4);
void setup() {
  Serial.begin(9600);
  while(sht3x.begin()!=0){
    Serial.println("Failed to initialize the chip, check the wire connection.");
    delay(1000);
  }
  
  Serial.print("Chip serial number");
  Serial.println(sht3x.readSerialNumber());

  if(!sht3x.softReset()){
    Serial.println("Failed to initialize the chip...");
  }

  Serial.println("--------------------------Read in Single Measurement Mode--------------------------");

}

void loop() {
  Serial.print("Ambient Temperature(C/F): ");
  
  Serial.print(sht3x.getTemperatureC());
  Serial.print(" /C");

  Serial.print(sht3x.getTemperatureF());
  Serial.print(" /F");

  Serial.print("Relative Humidity(%RH): ");
  Serial.print(sht3x.getHumidityRH());
  Serial.println(" %RH");
  
}
