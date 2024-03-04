#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// BME280 settings
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
unsigned long previousMillis = 0;
const long interval = 1000;

// UUIDs need to be unique for your application
#define SERVICE_UUID        "e583fb9a-f645-4c06-853c-597dc019909a"
#define CHARACTERISTIC_UUID "5dbfc2be-1190-46a4-bfe0-710feb96c4ab"

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
        deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
        deviceConnected = false;
    }
};

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE work!");

    // Initialize BME280
    if (!bme.begin(0x76)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1){
          Serial.print('.');
          delay(100);
        }
    }

    BLEDevice::init("XIAO_ESP32S3_Sensor");
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());
    BLEService *pService = pServer->createService(SERVICE_UUID);
    pCharacteristic = pService->createCharacteristic(
        CHARACTERISTIC_UUID,
        BLECharacteristic::PROPERTY_READ |
        BLECharacteristic::PROPERTY_WRITE |
        BLECharacteristic::PROPERTY_NOTIFY
    );
    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setValue("Initializing...");
    pService->start();

    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void loop() {
    if (deviceConnected) {
        unsigned long currentMillis = millis();
        if (currentMillis - previousMillis >= interval) {
            // Read humidity from BME280
            float humidity = bme.readHumidity();

            // Convert humidity to string to send via BLE
            char humidityStr[10];
            dtostrf(humidity, 1, 2, humidityStr); // Convert float to string
            pCharacteristic->setValue(humidityStr);
            pCharacteristic->notify();
            Serial.print("Notify humidity: ");
            Serial.println(humidityStr);
            
            previousMillis = currentMillis;
        }
    }

    // handle the connection status changes
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);  // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // advertise again
        Serial.println("Start advertising");
        oldDeviceConnected = deviceConnected;
    }
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
    }
    delay(1000);
}