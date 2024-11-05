ESP-IDF template app
====================
Author - Niharika Singh & Dhruv Joshi

The Project is the DEMO implementation of the PMU Functionality , i.e , Performance Monitoring unit .
Here , this is implemented using ESP32 functionality of MQTT , BLE .

Broker Used - HiveMQTT (The link would be defined in the macro itself).

There are various function created in this which are explained below and are understandable after ceratin level of knowledge.

1) void app_main() - The main fucntion where the code will start its execution .
2) void wifi_init(void) - Function to Initialize WiFi Connection in esp32.
3) void mqtt_event_handler - Function to handle MQTT event.
4) vTaskDelay - Wait for the 5 seconds to connect to the Wifi .
5) init_mqtt()  - This is to initialize the MQTT Broker.
6) xTaskCreate - Periodic Task to send Data to MQTT Broker .
7) void mqtt_publish_task(void *param) - This function publishes the MQTT message in 10 seconds Delay .   
8) prepare_mqtt_data() - To prepare JSON payload of Periodic Data 
9) prepare_mqtt_Status_data() - To prepare JSON payload of Status of Charger.
10) prepare_mqtt_charging_session_data - To prepare JSON charging session payload .
11) esp_mqtt_client_publish - This would publish the Data to MQTT Broker .
12) wifi_event_handler - This handles the WiFi Events.


