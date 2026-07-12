#include <Wire.h>
#include <SparkFun_SCD4x_Arduino_Library.h>

SCD4x mySensor;

unsigned long previousMillis = 0;
const unsigned long interval = 1000;

void setup() {
  Serial.begin(9600);
  Wire.begin();

  if (mySensor.begin() == false) {
    Serial.println(F("Sensor not found! Check wiring: A4, D2, 21(SDA) and A5, D1, 22(SCL)"));
    while (1);
  }
}

void loop() {
  
  unsigned long currentMillis = millis();
  
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;

    if (mySensor.readMeasurement()) {
      Serial.print(F("CO2: "));
      Serial.print(mySensor.getCO2());
      Serial.print(F(" ppm | "));

      Serial.print(F("Temperature: "));
      Serial.print(mySensor.getTemperature(), 1);
      Serial.print(F(" °C | "));

      Serial.print(F("Humidity: "));
      Serial.print(mySensor.getHumidity(), 1);
      Serial.println(F(" %"));
    }
  }
}