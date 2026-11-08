#include <Arduino.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "fruit_model.h"
#define LED_GREEN 25
#define LED_RED   26

// ======================================
// DISPLAY CONFIGURATION
// ======================================

#define LCD_ADDRESS 0x27
#define OLED_ADDRESS 0x3C

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64

LiquidCrystal_I2C lcd(LCD_ADDRESS, 16, 2);

Adafruit_SSD1306 oled(
    SCREEN_WIDTH,
    SCREEN_HEIGHT,
    &Wire,
    -1
);

// ======================================
// SENSOR CONFIGURATION
// ======================================

#define MQ3_PIN  34
#define MQ5_PIN  35
#define MQ8_PIN  32
#define MQ135_PIN 33

// Dataset sensor ranges
const float SENSOR_MIN[4] = {
    74.0f, 6.0f, 64.0f, 531.0f
};

const float SENSOR_MAX[4] = {
    892.0f, 804.0f, 1103.0f, 2909.0f
};

// ======================================
// SENSOR MAPPING
// ======================================

float mapSensor(int adcValue, int sensorIndex) {
    float ratio = adcValue / 4095.0f;

    return SENSOR_MIN[sensorIndex] +
           ratio * (
               SENSOR_MAX[sensorIndex] -
               SENSOR_MIN[sensorIndex]
           );
}

// ======================================
// DISPLAY FUNCTIONS
// ======================================

void updateLCD(uint8_t prediction, float probability) {
    lcd.setCursor(0, 0);

    if (prediction == 0) {
        lcd.print("STATUS: FRESH  ");
    } else {
        lcd.print("STATUS: SPOILED");
    }

    lcd.setCursor(0, 1);
    lcd.print("SPOIL: ");
    lcd.print(probability * 100.0f, 2);
    lcd.print("%    ");
}

void updateOLED(
    const float input[4],
    uint8_t prediction,
    float probability
) {
    oled.clearDisplay();

    oled.setTextColor(SSD1306_WHITE);

    oled.setTextSize(1);

    oled.setCursor(0, 0);
    oled.println("FRUIT MONITOR");

    oled.drawLine(0, 11, 127, 11, SSD1306_WHITE);

    oled.setCursor(0, 16);
    oled.print("MQ3   : ");
    oled.println(input[0], 1);

    oled.setCursor(0, 27);
    oled.print("MQ5   : ");
    oled.println(input[1], 1);

    oled.setCursor(0, 38);
    oled.print("MQ8   : ");
    oled.println(input[2], 1);

    oled.setCursor(0, 49);
    oled.print("MQ135 : ");
    oled.println(input[3], 1);

    oled.display();
}

// ======================================
// SETUP
// ======================================

void setup() {
    Serial.begin(115200);

    analogReadResolution(12);

    Wire.begin(21, 22);

    // Initialize LCD
    lcd.init();
    lcd.backlight();

    lcd.setCursor(0, 0);
    lcd.print("FRUIT MONITOR");

    lcd.setCursor(0, 1);
    lcd.print("Initializing...");

    pinMode(LED_GREEN, OUTPUT);
    pinMode(LED_RED, OUTPUT);

    digitalWrite(LED_GREEN, LOW);
    digitalWrite(LED_RED, LOW);

    // Initialize OLED
    if (!oled.begin(
        SSD1306_SWITCHCAPVCC,
        OLED_ADDRESS
    )) {
        Serial.println("OLED initialization failed!");
        while (true) {
            delay(100);
        }
    }

    oled.clearDisplay();
    oled.setTextSize(1);
    oled.setTextColor(SSD1306_WHITE);
    oled.setCursor(0, 0);
    oled.println("FRUIT MONITOR");
    oled.println("Initializing...");
    oled.display();

    delay(1500);

    Serial.println("==============================");
    Serial.println(" FRUIT FRESHNESS DETECTION");
    Serial.println(" RANDOM FOREST - ESP32");
    Serial.println("==============================");
}

// ======================================
// MAIN LOOP
// ======================================

void loop() {

    // Read potentiometers
    int adcMQ3 = analogRead(MQ3_PIN);
    int adcMQ5 = analogRead(MQ5_PIN);
    int adcMQ8 = analogRead(MQ8_PIN);
    int adcMQ135 = analogRead(MQ135_PIN);

    // Convert ADC to dataset sensor ranges
    float input[4] = {
        mapSensor(adcMQ3, 0),
        mapSensor(adcMQ5, 1),
        mapSensor(adcMQ8, 2),
        mapSensor(adcMQ135, 3)
    };

    // Random Forest inference
    float spoiledProbability = 0.0f;

    uint8_t prediction = predictFruit(
        input,
        &spoiledProbability
    );

    // LED Indicator
    if (prediction == 0) {
        digitalWrite(LED_GREEN, HIGH);
        digitalWrite(LED_RED, LOW);
    } else {
        digitalWrite(LED_GREEN, LOW);
        digitalWrite(LED_RED, HIGH);
    }

    // Update displays
    updateLCD(prediction, spoiledProbability);

    updateOLED(
        input,
        prediction,
        spoiledProbability
    );

    // Serial Monitor
    Serial.println("\n------------------------------");

    Serial.printf(
        "MQ3: %.2f | MQ5: %.2f\n",
        input[0], input[1]
    );

    Serial.printf(
        "MQ8: %.2f | MQ135: %.2f\n",
        input[2], input[3]
    );

    Serial.print("Prediction: ");

    Serial.println(
        prediction == 0 ? "FRESH" : "SPOILED"
    );

    Serial.printf(
        "Spoiled Probability: %.2f%%\n",
        spoiledProbability * 100.0f
    );

    delay(1000);
}