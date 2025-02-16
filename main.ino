#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <DHT.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

#define DHTPIN 2
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

#define MOISTURE_PIN A0
#define LIGHT_SENSOR A3
#define RED_LED 6
#define GREEN_LED 5
#define BLUE_LED 3
#define BUZZER 9

#define LEADERBOARD_BUTTON_PIN 8
#define WATER_BUTTON_PIN 3

#define DELTA 5

int plantHealth = 100; // current health of your plant
int xp = 0; // score

const int moistureOptimalLow  = 550;
const int moistureOptimalHigh = 780;
const int lightOptimalLow     = 300;
const int lightOptimalHigh    = 900;  // adjust based on your calibration
const float tempOptimalLow    = 15.0;
const float tempOptimalHigh   = 30.0;
const float humidityOptimalLow  = 10.0;
const float humidityOptimalHigh = 30.0;

unsigned long lastUpdateTime = 0;
unsigned long lastScoreTime = 0;
const unsigned long updateInterval = 1600;
const unsigned long scoreInterval = 10000;

int buttonClickCount = 0; 
bool currentLeaderboardButtonState = HIGH;
bool showLeaderboard = false;
bool prevButtonState = HIGH;
unsigned long prevLeaderboardDebounceTime = 0;
const unsigned long leaderboardDebounceDelay = 1;

bool waterButtonState = HIGH;
bool lastWaterButtonState = HIGH;
unsigned long lastWaterDebounceTime = 0;
const unsigned long waterDebounceDelay = 1;

// String npcNames[3] = {"Albert", "Steven", "Kevin"};
int npcScores[3] = {0, 0, 0};

void setup() {
  Serial.begin(9600);
  dht.begin();

  pinMode(MOISTURE_PIN, INPUT);
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);

  // set up buttons with internal pullup resistors
  pinMode(LEADERBOARD_BUTTON_PIN, INPUT_PULLUP);
  pinMode(WATER_BUTTON_PIN, INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("SSD1306 allocation failed");
    for (;;);
  }

  display.clearDisplay();
}

void loop() {
  int moisture = analogRead(MOISTURE_PIN);
  int light = analogRead(LIGHT_SENSOR);
  float temperature = dht.readTemperature() - 3.0; // apply offset
  float humidity = dht.readHumidity();

  int minVal = 200, maxVal = 800;

  int constrainedVal = constrain(moisture, minVal, maxVal);

  int moisturePercent = map(constrainedVal, minVal, maxVal, 0, 100);

  int lightPercent = map(light, 0, 1023, 0, 100);

  // game mechanism revised
  bool moistureOK = (moisture >= moistureOptimalLow && moisture <= moistureOptimalHigh);
  bool lightOK = (light >= lightOptimalLow && light <= lightOptimalHigh);
  bool tempOK = (temperature >= tempOptimalLow && temperature <= tempOptimalHigh);
  bool humidityOK = (humidity >= humidityOptimalLow && humidity <= humidityOptimalHigh);

  Serial.print("moisture");
  Serial.println(moistureOK);
  Serial.print("light");
  Serial.println(lightOK);
  Serial.print("temp");
  Serial.println(tempOK);
  Serial.print("humidity");
  Serial.println(humidityOK);

  bool allInRange = (moistureOK && lightOK && tempOK && humidityOK);

  if (millis() - lastUpdateTime > updateInterval) {
    if (allInRange) {
      plantHealth += 2; // gain faster
    } else {
      plantHealth--;
    }

    plantHealth = constrain(plantHealth, 0, 100);

    lastUpdateTime = millis();
  }

  if (millis() - lastScoreTime > scoreInterval) {
    if (moistureOK && lightOK && tempOK && humidityOK && plantHealth > 40) {
      xp++;

      // randomly increase each NPC's score in range [-2, 2]
      for (int i = 0; i < 3; i++) {
        npcScores[i] += random(0, 3);
        if (npcScores[i] > 100) {
          npcScores[i] = 100;
        }
      }
    }

    lastScoreTime = millis();
  }

  if (plantHealth >= 80) {
    Serial.println(plantHealth);
    setLED(0, 255, 0); // green
  } else if (plantHealth >= 40) {
    setLED(255, 255, 0);
  } else {
    setLED(255, 0, 0);
  }

  // read state of leaderboard button
  bool leaderboardReading = digitalRead(LEADERBOARD_BUTTON_PIN);
  Serial.println(leaderboardReading);
  if (leaderboardReading != prevButtonState) {
    prevLeaderboardDebounceTime = millis();
  }

  if ((millis() - prevLeaderboardDebounceTime) > leaderboardDebounceDelay) {
    if (leaderboardReading != currentLeaderboardButtonState) {
      currentLeaderboardButtonState = leaderboardReading;

      if (currentLeaderboardButtonState == LOW) {
        showLeaderboard = !showLeaderboard;
        Serial.print("showLeaderboard toggled to: ");
        Serial.println(showLeaderboard ? "true" : "false");
      }
    }
  }

  prevButtonState = leaderboardReading;

  display.clearDisplay();

  if (!showLeaderboard) {
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(0, 0);
    display.println("STATS:");

    // display.print("Health: ");
    // display.println(plantHealth);
    // display.print("XP: ");
    // display.println(xp);
    // display.display();

    // delay(2000);
    // display.println("Data");

    // display.setTextSize(1);
    // display.setCursor(0, 25);
    // display.print("Raw: ");
    // display.println(moisture);

    // display.setCursor(0, 30);
    display.print("Soil Moisture: ");
    display.print(moisturePercent);
    display.println("%");

    // print temperature
    display.print("Temperature: ");
    display.print(temperature);
    display.println(" C");

    // print humidity
    display.print("Air Humidity: ");
    display.print(humidity);
    display.println("%");

    // display light data
    display.print("Light: ");
    display.print(lightPercent);
    display.println("%");

    display.print("Health: ");
    display.print(plantHealth);
    display.print(" XP: ");
    display.println(xp);

    // display.setCursor(0, 40);
    // String adviceLine = "";

    // // Priority-based or example-based messages
    // if (!moistureOK) {
    //   adviceLine = "Please water me!";
    // } else if (!lightOK) {
    //   adviceLine = "I need more light!";
    // } else if (!tempOK) {
    //   adviceLine = "Adjust temperature!";
    // } else if (!humidityOK) {
    //   adviceLine = "Check humidity!";
    // } else {
    //   adviceLine = "All good!";
    // }

    // display.println(adviceLine);

  } else {
    // sortNPCsByScore();

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("Leaderboard:");
    
    display.print("You: ");
    display.println(xp);
    
    display.print("Albert: ");
    display.println(npcScores[0]);
    
    display.print("Steven: ");
    display.println(npcScores[1]);
    
    display.print("Kevin: ");
    display.println(npcScores[2]);
  }

  display.display();

  delay(250);
}

void setLED(int r, int g, int b) {
  analogWrite(RED_LED, r);
  analogWrite(GREEN_LED, g);
  analogWrite(BLUE_LED, b);
}

void sortLeaderboard(int arr[], int size) {
  // sort the leaderboard in non increasing order
  for (int i = 0; i < size - 1; i++) {
    for (int j = 0; j < size - i - 1; j++) {
      if (arr[j] < arr[j + 1]) {
        int temp = arr[j];
        arr[j] = arr[j + 1];
        arr[j + 1] = temp;
      }
    }
  }
}
