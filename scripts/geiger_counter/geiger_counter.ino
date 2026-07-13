
#include <GyverOLED.h>
#include <Wire.h>
#include <SparkFun_SCD4x_Arduino_Library.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

#define OLED_SPI_SPEED 8000000ul
#define WIDTH 128 // OLED display width, in pixels
#define HEIGHT 64 // OLED display height, in pixels

// Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, -1);
GyverOLED<SSH1106_128x64> oled;
// GyverOLED<SSH1106_128x64, OLED_NO_BUFFER> oled(0x3C);

// ## PINs 

int LEDpin = 23; // on esp32 23
int fakeRod = 19; // on esp32 19
int capacitorPIN = 17; // on esp32 17

// ## Geiger Counter Stats

unsigned long totalCounts = 0;
volatile unsigned long counts = 0;
float cpm = 0.0;
float usv_per_hour = 0.0;
const float CAL_FACTOR = 1; // 0.0057; // change to the tubes factor

// ## CO2 Sensor Values
// co2 sensor address 0x62

SCD4x co2Sensor;

// ## Tone Values

// const int freq = 2000;    // Hz
const int duration = 3;   // ms

// ## Display Values
// display address 0x3C

const int sda_pin = 21;
const int sck_pin = 18;

// ## Interval Timing

unsigned long lastTime = 0;
const unsigned long interval = 1000; // interval in ms

// interrupt for the geiger counter rod 

void IRAM_ATTR radiationDetected(){
  counts++;

  digitalWrite(LEDpin, HIGH);
  int freq = 2000 + random(0, 10);
  tone(LEDpin, freq, duration);
  digitalWrite(LEDpin, LOW);
}

void setup() {
  Serial.begin(115200);
  Wire.begin();

  // displayStartUp();

  // starting co2 sensor
  
  if (co2Sensor.begin() == false) {
    Serial.println(F("Sensor not found! Check wiring: A4, D2, 21(SDA) and A5, D1, 22(SCL)"));
    while (1);
  }

  // starting display and geigen counter

  pinMode(fakeRod, OUTPUT);
  digitalWrite(fakeRod, HIGH);
  
  pinMode(LEDpin, OUTPUT);
  pinMode(capacitorPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(capacitorPIN), radiationDetected, RISING);

  // Wire.begin(sda_pin, sck_pin);
  Wire.setClock(400000);
  delay(500);
  
  oled.init();
  oled.clear();
  oled.setScale(1);
  oled.update();
}

void loop() {
  unsigned long currentTime = millis();

  if (currentTime - lastTime >= interval) {

    // is it neccessary to detach and then reattach the PIN?
    detachInterrupt(digitalPinToInterrupt(capacitorPIN));

    unsigned long countCopy = counts;
    totalCounts += counts;
    counts = 0;
    lastTime = currentTime;

    attachInterrupt(digitalPinToInterrupt(capacitorPIN), radiationDetected, RISING);

    // Calculate counts per minute and uSv/h
    cpm = countCopy * 60.0;                  // counts in 1 second * 60
    usv_per_hour = cpm * CAL_FACTOR;         // convert CPM to uSv/h

    // print to LCD
    
    oled.clear();
    oled.setCursorXY(0,1); 
    oled.print("CPS: ");    oled.println(countCopy);
    oled.print("CPM: ");    oled.println(cpm);
    oled.print("Rate: ");      oled.print(usv_per_hour, 2);    oled.println(" uSv/h"); // Dose rate
    oled.print("#Clicks: ");   oled.println(totalCounts); // total clicks 
    oled.print("CO2: ");    oled.print(co2Sensor.getCO2());          oled.println(" ppm");
    oled.print(co2Sensor.getTemperature(), 1);    oled.print("C ");   oled.print(co2Sensor.getHumidity(), 1);   oled.println("%");
    oled.update();

    // temporary serial output 

    Serial.print("CPS: "); Serial.print(countCopy);
    Serial.print("  CPM: "); Serial.print(cpm);
    Serial.print("  Dose rate: ");
    Serial.print(usv_per_hour, 3);
    Serial.println(" uSv/h");

    if (co2Sensor.readMeasurement()) {
      Serial.print(F("CO2: "));
      Serial.print(co2Sensor.getCO2());
      Serial.print(F(" ppm | "));

      float fixed_temp = co2Sensor.getTemperature() - co2Sensor.getTemperatureOffset();
      Serial.print(F("Temperature: "));
      Serial.print(fixed_temp, 1);
      Serial.print(F(" °C | "));
      
      Serial.print(F("Humidity: "));
      Serial.print(co2Sensor.getHumidity(), 1);
      Serial.println(F(" %"));
    }
  }
}
