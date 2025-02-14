#include <Wire.h>
#include "MAX30105.h"
#include "heartRate.h"
#include <WiFi.h>
#include <ArduinoMqttClient.h>

// WiFi credentials
const char* ssid = "BME2";
const char* password = "12345678";

// MQTT Broker settings
const char* mqtt_server = "54.226.172.192";
const int mqtt_port = 1883;

// MQTT topics
const char* topic_heartRate = "sensor/heartRate";
const char* topic_temperature = "sensor/temperature";
const char* topic_fingerStatus = "sensor/fingerStatus";
const char* topic_accX = "sensor/accX";
const char* topic_accY = "sensor/accY";
const char* topic_accZ = "sensor/accZ";

// WiFi and MQTT clients
WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

MAX30105 particleSensor;

const byte RATE_SIZE = 4;
byte rates[RATE_SIZE];
byte rateSpot = 0;
long lastBeat = 0;

float beatsPerMinute;
int beatAvg;

unsigned long lastPublishTime = 0; // Timer for publishing data every second

// Accelerometer setup
const int MPU = 0x68;  // I2C address of the MPU6050
float AccX, AccY, AccZ; // Variables for averaged accelerometer data
float accXSum = 0, accYSum = 0, accZSum = 0; // Sums for averaging
int readingCount = 0;                        // Counter for readings

// Function declarations
void connectToWiFi();
void connectToMQTT();
void publishHeartRate(int avgBPM);
void publishTemperature(float temperatureC);
void publishFingerStatus(const char* status);
void publishAccelerometerData(float accX, float accY, float accZ);
void initializeMPU6050();
void readAccelerometerData();
float readTemperature();

void setup() {
  Serial.begin(115200);
  Serial.println("Initializing...");

  // Initialize WiFi and MQTT connections
  connectToWiFi();
  connectToMQTT();

  // Initialize heart rate sensor
  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
    Serial.println("MAX30102 was not found. Please check wiring/power.");
    while (1);
  }
  Serial.println("Place your index finger on the sensor with steady pressure.");

  particleSensor.setup();
  particleSensor.setPulseAmplitudeRed(0x7F);
  particleSensor.setPulseAmplitudeGreen(0);

  // Initialize MPU6050
  initializeMPU6050();
}

void loop() {
  if (!mqttClient.connected()) connectToMQTT();
  mqttClient.poll();

  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true) {
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 120 && beatsPerMinute > 45) {
      rates[rateSpot++] = (byte)beatsPerMinute;
      rateSpot %= RATE_SIZE;

      beatAvg = 0;
      for (byte x = 0; x < RATE_SIZE; x++) {
        beatAvg += rates[x];
      }
      beatAvg /= RATE_SIZE;
    }
  }

  if (irValue < 50000) {
    publishFingerStatus("No finger detected");
  } else {
    publishFingerStatus(""); // Clear "No finger" message
  }

  // Read accelerometer data multiple times per second
  readAccelerometerData();

  // Publish heart rate, temperature, and accelerometer data every second
  if (millis() - lastPublishTime >= 1000) {
    lastPublishTime = millis();

    // Publish Avg BPM
    publishHeartRate(beatAvg);

    // Read and publish temperature
    float temperatureC = readTemperature();
    publishTemperature(temperatureC);

    // Calculate and publish averaged accelerometer data
    if (readingCount > 0) {
      AccX = accXSum / readingCount;
      AccY = accYSum / readingCount;
      AccZ = accZSum / readingCount;

      accXSum = 0;
      accYSum = 0;
      accZSum = 0;
      readingCount = 0;

      publishAccelerometerData(AccX, AccY, AccZ);
    }
  }
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi: ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi!");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void connectToMQTT() {
  Serial.print("Connecting to MQTT...");
  while (!mqttClient.connect(mqtt_server, mqtt_port)) {
    Serial.print(".");
    delay(5000);
  }
  Serial.println("\nConnected to MQTT broker!");
}

// Publish Avg BPM to MQTT
void publishHeartRate(int avgBPM) {
  mqttClient.beginMessage(topic_heartRate);
  mqttClient.print(avgBPM);
  mqttClient.endMessage();
  Serial.println("Published Avg BPM to MQTT.");
}

// Publish temperature to MQTT
void publishTemperature(float temperatureC) {
  mqttClient.beginMessage(topic_temperature);
  mqttClient.print(temperatureC, 2);
  mqttClient.endMessage();
  Serial.print("Published Temperature to MQTT: ");
  Serial.print(temperatureC, 2);
  Serial.println(" °C");
}

// Publish finger status to MQTT
void publishFingerStatus(const char* status) {
  mqttClient.beginMessage(topic_fingerStatus);
  mqttClient.print(status);
  mqttClient.endMessage();
  if (strlen(status) > 0) {
    Serial.println("Published finger status: No finger detected");
  }
}

// Publish accelerometer data to MQTT
void publishAccelerometerData(float accX, float accY, float accZ) {
  mqttClient.beginMessage(topic_accX);
  mqttClient.print(accX, 2);
  mqttClient.endMessage();

  mqttClient.beginMessage(topic_accY);
  mqttClient.print(accY, 2);
  mqttClient.endMessage();

  mqttClient.beginMessage(topic_accZ);
  mqttClient.print(accZ, 2);
  mqttClient.endMessage();

  Serial.print("Published AccX: ");
  Serial.print(accX, 2);
  Serial.print(", AccY: ");
  Serial.print(accY, 2);
  Serial.print(", AccZ: ");
  Serial.println(accZ, 2);
}

// Initialize MPU6050 accelerometer
void initializeMPU6050() {
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);         // Access the power management register
  Wire.write(0x00);         // Wake up MPU6050
  Wire.endTransmission(true);

  Wire.beginTransmission(MPU);
  Wire.write(0x1C);         // Access the Accelerometer Configuration Register
  Wire.write(0x00);         // Set full-scale range to ±2g
  Wire.endTransmission(true);
}

// Read accelerometer data from MPU6050
void readAccelerometerData() {
  static unsigned long lastAccelTime = 0;

  if (millis() - lastAccelTime >= 100) { // Take a reading every 100ms
    lastAccelTime = millis();

    Wire.beginTransmission(MPU);
    Wire.write(0x3B);         // Start with the accelerometer data register
    Wire.endTransmission(false);
    Wire.requestFrom(MPU, 6, true);

    // Convert the data to accelerometer readings
    int16_t rawX = (Wire.read() << 8) | Wire.read(); // X-axis raw data
    int16_t rawY = (Wire.read() << 8) | Wire.read(); // Y-axis raw data
    int16_t rawZ = (Wire.read() << 8) | Wire.read(); // Z-axis raw data

    // Convert raw data to acceleration in g
    float accX = rawX / 16384.0;  // X-axis acceleration in g
    float accY = rawY / 16384.0;  // Y-axis acceleration in g
    float accZ = rawZ / 16384.0;  // Z-axis acceleration in g

    // Accumulate readings for averaging
    accXSum += accX;
    accYSum += accY;
    accZSum += accZ;
    readingCount++;
  }
}

// Read temperature from MAX30105 sensor
float readTemperature() {
  return particleSensor.readTemperature();  // Temperature in Celsius
}
