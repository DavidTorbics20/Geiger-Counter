
#include <GyverOLED.h>
#include <Wire.h>
#include <SparkFun_SCD4x_Arduino_Library.h>
// #include <Adafruit_GFX.h>
// #include <Adafruit_SSD1306.h>

#define OLED_SPI_SPEED 1000000ul // why 8000000ul not work? 
#define WIDTH 128 // OLED display width, in pixels
#define HEIGHT 64 // OLED display height, in pixels

// Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, -1);
GyverOLED<SSH1106_128x64> oled;
// GyverOLED<SSH1106_128x64, OLED_NO_BUFFER> oled(0x3C);

// ## PINs 

const int LEDpin = D5; // on esp32 23 // on D1 mini D5
// const int fakeRod = 19; // on esp32 19 // on D1 mini undef.
const int capacitorPIN = D7; // on esp32 17 // on D1 mini D7

// ## Geiger Counter Stats

volatile unsigned long clicks = 0;
unsigned long lastClicks = 0;
float cpm = 0.0;
float usv_per_hour = 0.0;
const float CONVERSION_FACTOR = 0.00332; // 0.0057; // change to the tubes factor
const float DEAD_TIME_SEC = 0.000180; // 180 microseconds for the J305 geiger tube (could be adjusted)

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
const unsigned int interval = 1000; // interval in ms KEEP AT 1000 !!!

// interrupt for the geiger counter rod 

void IRAM_ATTR radiationDetected(){
  clicks++;

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

  // pinMode(fakeRod, OUTPUT);
  // digitalWrite(fakeRod, HIGH);
  
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

    // is it neccessary to detach and then reattach the PIN? YES
    // detachInterrupt(digitalPinToInterrupt(capacitorPIN));
    noInterrupts();

    unsigned long currentTotalClicks = clicks;

    //attachInterrupt(digitalPinToInterrupt(capacitorPIN), radiationDetected, RISING);
    interrupts();

    float elapsedTime = (currentTime - lastTime) / interval; // interval has to be 1000 for seconds to work
    unsigned long clicksInWindow = currentTotalClicks - lastClicks;
    float cps = clicksInWindow / elapsedTime;

    // for highly radioactive areas the dead time correction has to be applied
    float trueCPS = cps / (1.0 - (cps * DEAD_TIME_SEC));

    // Calculate clicks per minute and uSv/h
    float cpm = trueCPS * 60.0;
    float usv_per_hour = cpm * CONVERSION_FACTOR;

    lastClicks = currentTotalClicks;
    lastTime = currentTime;

    // print to LCD
    
    oled.clear();
    oled.setCursorXY(0,1); 
    oled.print("CPS: ");    oled.println(trueCPS);
    oled.print("CPM: ");    oled.println(cpm);
    oled.print("Dose: ");      oled.print(usv_per_hour, 2);    oled.println(" uSv/h"); // Dose rate
    oled.print("Clicks: ");   oled.println(currentTotalClicks); // total clicks 
    oled.print("CO2: ");    oled.print(co2Sensor.getCO2());          oled.println(" ppm");
    oled.print(co2Sensor.getTemperature(), 1);    oled.print("C ");   oled.print(co2Sensor.getHumidity(), 1);   oled.println("%");
    oled.update();

    // temporary serial output 

    Serial.print("CPS: "); Serial.print(trueCPS);
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
