
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define WIDTH 128 // OLED display width, in pixels
#define HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(WIDTH, HEIGHT, &Wire, -1);

// ## PINs 

int LEDpin = 23; // on esp32 23
int fakeRod = 19; // on esp32 19
int capacitorPIN = 17; // on esp32 17

// ## Geiger Counter Stats

unsigned long totalCounts = 0;
volatile unsigned long counts = 0;
unsigned long lastTime = 0;
unsigned long interval = 1000; // 1 second interval
float cpm = 0.0;
float usv_per_hour = 0.0;

const float CAL_FACTOR = 1; // 0.0057; // change to the tubes factor

// ## Tone Values

// const int freq = 2000;    // Hz
const int duration = 3;   // ms

void IRAM_ATTR radiationDetected(){
  counts++;

  digitalWrite(LEDpin, HIGH);
  int freq = 2000 + random(0, 10);
  tone(LEDpin, freq, duration);
  digitalWrite(LEDpin, LOW);
}

void displayStartUp() {
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3D)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }

  delay(2000);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 0);
  display.println("Hello World!"); // Display static text
  display.display(); 
  delay(2000);
  display.clearDisplay();
}

void setup() {
  Serial.begin(115200);

  displayStartUp();

  pinMode(fakeRod, OUTPUT);
  digitalWrite(fakeRod, HIGH);
  
  pinMode(LEDpin, OUTPUT);
  pinMode(capacitorPIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(capacitorPIN), radiationDetected, RISING);

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
    
    display.clearDisplay();
    display.setCursor(0,0); 
    display.println("CPS: ");    display.println(countCopy);
    display.println("CPM: ");    display.println(cpm);
    display.println("Dose rate: ");    display.println(usv_per_hour, 3);    display.println(" uSv/h");
    display.println("Total clicks: ");    display.println(totalCounts);
    display.display();

    Serial.print("CPS: "); Serial.print(countCopy);
    Serial.print("  CPM: "); Serial.print(cpm);
    Serial.print("  Dose rate: ");
    Serial.print(usv_per_hour, 3);
    Serial.println(" uSv/h");
  }
}
