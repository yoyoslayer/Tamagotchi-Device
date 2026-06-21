#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_LTR390.h>

#define Screen_WIDTH 128
#define Screen_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(Screen_WIDTH, Screen_HEIGHT, &Wire, OLED_RESET);

Adafruit_LTR390 ltr = Adafruit_LTR390();

#define BTN_LEFT   2
#define BTN_CENTER 3
#define BTN_RIGHT  4

#define SDA_PIN 6
#define SCL_PIN 7

#define BUZZER_PIN 10

#define LED_PIN 20

Adafruit_NeoPixel pixel(1, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ALS_DARK_LEVEL 30
#define UV_SUN_LEVEL   50

#define ANNOUNCE_INTERVAL 300000UL
#define ANNOUNCE_DURATION 3000UL

struct Pet {
  int hunger;
  int happiness;
  int energy;
  unsigned long age;
};

Pet pet;

enum Screen {
  SCREEN_MAIN,
  SCREEN_FEED,
  SCREEN_PLAY,
  SCREEN_SLEEP,
};

Screen currentScreen = SCREEN_MAIN;

enum LightType {
  LIGHT_DARK,
  LIGHT_INDOOR,
  LIGHT_SUN,
};

LightType lightType = LIGHT_DARK;

void setup() {
  Serial.begin(115200);

  pinMode(BTN_LEFT, INPUT_PULLUP);
  pinMode(BTN_CENTER, INPUT_PULLUP);
  pinMode(BTN_RIGHT, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);

  Wire.begin(SDA_PIN, SCL_PIN);

  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("Tamagotchi Init...");
  display.display();

  if (!ltr.begin()) {
    Serial.println("LTR390 not found - check wiring/address");
  }
  ltr.setGain(LTR390_GAIN_3);
  ltr.setResolution(LTR390_RESOLUTION_16BIT);

  pixel.begin();
  pixel.setBrightness(50);
  pixel.clear();
  pixel.show();

  delay(1000);

  pet.hunger = 80;
  pet.happiness = 80;
  pet.energy = 80;
  pet.age = 0;
}

unsigned long lastUpdate = 0;

void updatePet() {
  if (millis() - lastUpdate > 5000) {
    pet.hunger--;
    pet.happiness--;
    pet.energy--;

    if (pet.hunger < 0) pet.hunger = 0;
    if (pet.happiness < 0) pet.happiness = 0;
    if (pet.energy < 0) pet.energy = 0;

    pet.age += 5;
    lastUpdate = millis();
  }
}

unsigned long lastButtonPress = 0;

void checkButtons() {
  if (millis() - lastButtonPress < 200) return;

  if (digitalRead(BTN_LEFT) == LOW) {
    currentScreen = SCREEN_FEED;
    tone(BUZZER_PIN, 1000, 50);
    lastButtonPress = millis();
  }
  else if (digitalRead(BTN_CENTER) == LOW) {
    currentScreen = SCREEN_PLAY;
    tone(BUZZER_PIN, 1000, 50);
    lastButtonPress = millis();
  }
  else if (digitalRead(BTN_RIGHT) == LOW) {
    currentScreen = SCREEN_SLEEP;
    tone(BUZZER_PIN, 1000, 50);
    lastButtonPress = millis();
  }
}

void handleScreenLogic() {
  switch (currentScreen) {
    case SCREEN_FEED:
      pet.hunger += 10;
      if (pet.hunger > 100) pet.hunger = 100;
      currentScreen = SCREEN_MAIN;
      break;

    case SCREEN_PLAY:
      pet.happiness += 10;
      pet.energy -= 5;
      if (pet.happiness > 100) pet.happiness = 100;
      if (pet.energy < 0) pet.energy = 0;
      currentScreen = SCREEN_MAIN;
      break;

    case SCREEN_SLEEP:
      pet.energy += 15;
      if (pet.energy > 100) pet.energy = 100;
      currentScreen = SCREEN_MAIN;
      break;

    case SCREEN_MAIN:
      break;
  }
}

unsigned long lastLightCheck = 0;

void checkLight() {
  if (millis() - lastLightCheck < 2000) return;
  lastLightCheck = millis();

  ltr.setMode(LTR390_MODE_ALS);
  delay(50);
  uint32_t visible = ltr.readALS();

  ltr.setMode(LTR390_MODE_UVS);
  delay(50);
  uint32_t uv = ltr.readUVS();

  if (visible < ALS_DARK_LEVEL) {
    lightType = LIGHT_DARK;
  } else if (uv > UV_SUN_LEVEL) {
    lightType = LIGHT_SUN;
  } else {
    lightType = LIGHT_INDOOR;
  }

  Serial.print("visible(ALS)="); Serial.print(visible);
  Serial.print("  uv(UVS)=");    Serial.print(uv);
  Serial.print("  -> ");
  Serial.println(lightType == LIGHT_SUN ? "SUN"
               : lightType == LIGHT_INDOOR ? "INDOOR" : "DARK");
}

unsigned long lastAnnounce = 0;
unsigned long announceUntil = 0;

void updateAnnounce() {
  if (millis() - lastAnnounce > ANNOUNCE_INTERVAL && lightType != LIGHT_DARK) {
    lastAnnounce = millis();
    announceUntil = millis() + ANNOUNCE_DURATION;
  }
}

bool isAnnouncing() {
  return millis() < announceUntil;
}

int petMood() {
  if (pet.hunger < 30 || pet.happiness < 30 || pet.energy < 30) return 0;
  if (pet.hunger > 50 && pet.happiness > 50 && pet.energy > 50) return 2;
  return 1;
}

void updateLED() {
  uint32_t color;
  switch (petMood()) {
    case 0:  color = pixel.Color(255, 0, 0);   break;
    case 2:  color = pixel.Color(0, 255, 0);   break;
    default: color = pixel.Color(255, 170, 0);
  }
  pixel.setPixelColor(0, color);
  pixel.show();
}

const unsigned char PROGMEM petHappy[] = {
  0b00000000, 0b00000000,
  0b00001111, 0b11110000,
  0b00011000, 0b00011000,
  0b00110000, 0b00001100,
  0b01100000, 0b00000110,
  0b01001110, 0b01110010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01011000, 0b00011010,
  0b01011000, 0b00011010,
  0b01001100, 0b00110010,
  0b01100111, 0b11100110,
  0b00010000, 0b00001000,
  0b00001100, 0b00110000,
  0b00000111, 0b11100000,
  0b00000000, 0b00000000
};

const unsigned char PROGMEM petSad[] = {
  0b00000000, 0b00000000,
  0b00001111, 0b11110000,
  0b00011000, 0b00011000,
  0b00110000, 0b00001100,
  0b01100000, 0b00000110,
  0b01001110, 0b01110010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000011, 0b11000010,
  0b01000110, 0b01100010,
  0b01001100, 0b00110010,
  0b01100000, 0b00000110,
  0b00010000, 0b00001000,
  0b00001100, 0b00110000,
  0b00000111, 0b11100000,
  0b00000000, 0b00000000
};

const unsigned char PROGMEM petNeutral[] = {
  0b00000000, 0b00000000,
  0b00001111, 0b11110000,
  0b00011000, 0b00011000,
  0b00110000, 0b00001100,
  0b01100000, 0b00000110,
  0b01001110, 0b01110010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01001111, 0b11110010,
  0b01100000, 0b00000110,
  0b00010000, 0b00001000,
  0b00001100, 0b00110000,
  0b00000111, 0b11100000,
  0b00000000, 0b00000000
};

const unsigned char PROGMEM petSleepy[] = {
  0b00000000, 0b00000000,
  0b00001111, 0b11110000,
  0b00011000, 0b00011000,
  0b00110000, 0b00001100,
  0b01101010, 0b01010110,
  0b01000100, 0b00100010,
  0b01001010, 0b01010010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01000011, 0b11000010,
  0b01100000, 0b00000110,
  0b00010000, 0b00001000,
  0b00001100, 0b00110000,
  0b00000111, 0b11100000,
  0b00000000, 0b00000000
};

const unsigned char PROGMEM petSunshine[] = {
  0b00000000, 0b00000000,
  0b00011111, 0b11111000,
  0b00111000, 0b00011100,
  0b01110000, 0b00001110,
  0b01101010, 0b01010110,
  0b01001010, 0b01010010,
  0b01001110, 0b01110010,
  0b01000000, 0b00000010,
  0b01001000, 0b00010010,
  0b01001000, 0b00010010,
  0b01001100, 0b00110010,
  0b01100111, 0b11100110,
  0b00010000, 0b00001000,
  0b00001100, 0b00110000,
  0b00000111, 0b11100000,
  0b00000000, 0b00000000
};

const unsigned char PROGMEM petIndoor[] = {
  0b00000000, 0b00000000,
  0b00011111, 0b11111000,
  0b00111000, 0b00011100,
  0b01110000, 0b00001110,
  0b01101110, 0b01110110,
  0b01001010, 0b01010010,
  0b01001110, 0b01110010,
  0b01000000, 0b00000010,
  0b01000000, 0b00000010,
  0b01001000, 0b00010010,
  0b01001100, 0b00110010,
  0b01100111, 0b11100110,
  0b00010000, 0b00001000,
  0b00001100, 0b00110000,
  0b00000111, 0b11100000,
  0b00000000, 0b00000000
};

void drawBar(int x, int y, int value) {
  int barWidth = 100;
  int barHeight = 6;
  int fillWidth = map(value, 0, 100, 0, barWidth);
  display.drawRect(x, y, barWidth, barHeight, SSD1306_WHITE);
  display.fillRect(x, y, fillWidth, barHeight, SSD1306_WHITE);
}

void render() {
  display.clearDisplay();

  const unsigned char* sprite;
  if (isAnnouncing()) {
    if (lightType == LIGHT_SUN) {
      sprite = petSunshine;
      display.setCursor(0, 4);
      display.print("Sun!");
    } else {
      sprite = petIndoor;
      display.setCursor(0, 4);
      display.print("Indoor");
    }
  } else {
    switch (petMood()) {
      case 0:  sprite = petSad;     break;
      case 2:  sprite = petHappy;   break;
      default: sprite = petNeutral;
    }
  }

  display.drawBitmap(56, 2, sprite, 16, 16, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 24);
  display.print("HUN ");
  drawBar(24, 24, pet.hunger);
  display.setCursor(0, 34);
  display.print("HAP ");
  drawBar(24, 34, pet.happiness);
  display.setCursor(0, 44);
  display.print("ENG ");
  drawBar(24, 44, pet.energy);

  display.setCursor(0, 56);
  display.println("[Feed] [Play] [Sleep]");

  display.display();
}

void loop() {
  checkButtons();
  updatePet();
  handleScreenLogic();
  checkLight();
  updateAnnounce();
  updateLED();
  render();
  delay(100);
}
