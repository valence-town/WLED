#pragma once

#include "wled.h"
#include <Arduino.h>
#include <Wire.h>
#include <HX711.h>

uint16_t SENSOR_READ_DELAY_MS = 1000;

uint8_t PIN_DAT_SENSOR1 = 15; // DigQuad Q1
uint8_t PIN_CLK_SENSOR1 = 12; // DigQuad Q2

uint8_t PIN_DAT_SENSOR2 = 2;  // DigQuad Q3
uint8_t PIN_CLK_SENSOR2 = 32; // DigQuad Q4

HX711 sensor1;
HX711 sensor2;

class Usermod_HX711_MQTT : public Usermod
{
private:
  bool sensorsInitialized = false;
  bool mqttInitialized = false;

  long mqttDataSensor1Load = 0;
  long mqttDataSensor2Load = 0;

  String mqttTopicSensor1Load = "";
  String mqttTopicSensor2Load = "";

  unsigned long nextMeasure = 0;

  void _sensorsInitialize()
  {
    Serial.println("[usermod_v2_hx711] initializing load sensors...");

    sensor1.begin(PIN_DAT_SENSOR1, PIN_CLK_SENSOR1);
    sensor1.tare(10);
    sensor1.set_scale(1);
    sensor1.set_offset(0);
    sensor1.set_gain(128);

    sensor2.begin(PIN_DAT_SENSOR2, PIN_CLK_SENSOR2);
    sensor2.tare(10);
    sensor2.set_scale(1);
    sensor2.set_offset(0);
    sensor2.set_gain(128);

    sensorsInitialized = true;

    Serial.println("[usermod_v2_hx711] sensor initialization completed!");
  }

  void _mqttInitialize()
  {
    mqttTopicSensor1Load = String(mqttDeviceTopic) + "/sensor1/load";
    Serial.printf("[usermod_v2_hx711] initializing MQTT topic %s ...\n", mqttTopicSensor1Load.c_str());
    _mqttCreateSensor("sensor1load", mqttTopicSensor1Load, "load", "raw");

    mqttTopicSensor2Load = String(mqttDeviceTopic) + "/sensor2/load";
    Serial.printf("[usermod_v2_hx711] initializing MQTT topic %s ...\n", mqttTopicSensor2Load.c_str());
    _mqttCreateSensor("sensor2load", mqttTopicSensor2Load, "load", "raw");
  }

  void _mqttCreateSensor(const String &name, const String &topic, const String &deviceClass, const String &unitOfMeasurement)
  {
    String t = String("homeassistant/sensor/") + mqttClientID + "/" + name + "/config";

    StaticJsonDocument<300> doc;

    doc["name"] = name;
    doc["state_topic"] = topic;
    doc["unique_id"] = String(mqttClientID) + name;
    if (unitOfMeasurement != "")
      doc["unit_of_measurement"] = unitOfMeasurement;
    if (deviceClass != "")
      doc["device_class"] = deviceClass;
    doc["expire_after"] = 1800;

    JsonObject device = doc.createNestedObject("device"); // attach the sensor to the same device
    device["identifiers"] = String("wled-sensor-") + mqttClientID;
    device["manufacturer"] = "Aircoookie";
    device["model"] = "WLED";
    device["sw_version"] = VERSION;
    device["name"] = mqttClientID;

    String temp;
    serializeJson(doc, temp);
    Serial.printf("[usermod_v2_hx711] creating MQTT publisher with topic=%s and doc=%s\n", t.c_str(), temp.c_str());

    mqtt->publish(t.c_str(), 0, true, temp.c_str());
  }

  void _sensorsRead()
  {
    if (sensor1.wait_ready_timeout(100)) {
      mqttDataSensor1Load = sensor1.read();
      Serial.printf("[usermod_v2_hx711] received new sensor1 data: load=%ld\n", mqttDataSensor1Load);
    }
    if (sensor2.wait_ready_timeout(100)) {
      mqttDataSensor2Load = sensor2.read();
      Serial.printf("[usermod_v2_hx711] received new sensor2 data: load=%ld\n", mqttDataSensor2Load);
    }
  }

public:
  void setup()
  {
    Serial.println("[usermod_v2_hx711] setup called!");
    _sensorsInitialize();
  }

  // gets called every time WiFi is (re-)connected.
  void connected()
  {
    Serial.println("[usermod_v2_hx711] WiFi connected!");
    nextMeasure = millis() + SENSOR_READ_DELAY_MS; // Schedule next measurement
  }

  void loop()
  {
    unsigned long checkTimer = millis();

    if (checkTimer > nextMeasure)
    {
      nextMeasure = checkTimer + SENSOR_READ_DELAY_MS; // Schedule next measurement

      if (!sensorsInitialized)
      {
        Serial.println("[usermod_v2_hx711] sensors are still not initialized");
        _sensorsInitialize();
        return; // lets try again next loop
      }

      // Update sensor data
      _sensorsRead();

      if (mqtt != nullptr && mqtt->connected())
      {
        if (!mqttInitialized)
        {
          _mqttInitialize();
          mqttInitialized = true;
        }

        // Create string populated with user defined device topic from the UI,
        // and the read temperature, humidity and pressure.
        // Then publish to MQTT server.
        mqtt->publish(mqttTopicSensor1Load.c_str(), 0, true, String(mqttDataSensor1Load).c_str());
        mqtt->publish(mqttTopicSensor2Load.c_str(), 0, true, String(mqttDataSensor2Load).c_str());
      }
      else
      {
        //Serial.println("[usermod_v2_hx711] no MQTT connection found, will not publish data");
        mqttInitialized = false;
      }
    }
  }
};
