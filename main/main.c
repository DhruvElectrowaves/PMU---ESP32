#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <math.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"


#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"

static const char *TAG = "MQTT_CHARGER";
esp_mqtt_client_handle_t mqtt_client;

const char *wifi_ssid = "ApnaBanda";
const char *wifi_password = "12345abc";

// MQTT broker details
const char *mqtt_broker_uri = "mqtt://broker.hivemq.com:1883"; // Update with your broker address

// Example charger data (replace these with actual sensor readings in a real application)
double ac_meter_reading = 150.5;
double dc_meter_reading = 120.7;
double grid_voltage = 230.0;
double grid_current = 5.31;
double dc_voltage = 480.45;
double dc_current = 10.0;
const char *cms_name = "CMS_001";
int connector_status = 1;
double power_module1_data = 95.8;
double power_module2_data = 100.6;

char charger_Id[20];  // Variable to store the value of charger_iD
int flag1_Check=0;  // This is the flag when rises when charger iD is received.
char Serial_Number[] = "EWE01" ;  // This variable would hold the value of serial Number received . 

// Example charger Connector Status Data (replace these with actual sensor readings in a real application)
typedef enum {
    CONNECTOR_CHARGING = 1,
    CONNECTOR_FINISHED = 2,
    CONNECTOR_AVAILABLE = 3,
    CONNECTOR_UNAVAILABLE = 4,
    CONNECTOR_PREPARING = 5,
    CONNECTOR_OCCUPIED = 6
} ConnectorStatus;

typedef enum {
	WEBSOCKET_CONNECTED = 1,
	WEBSOCKET_DISCONNECTED = 2
}WebSocketStatus;

typedef enum {
	FAULT_NOERROR = 0,
	FAULT_EMERGENCYBUTTON = 1,
	FAULT_CANA_FAILED = 2,
	FAULT_CANB_FAILED = 3,
	FAULT_EARTH_FAULT = 4,
	FAULT_SMOKE_FAULT = 5,
	FAULT_DOOR_FAULT = 6,
	FAULT_GRID_NOT_AVAILABLE = 7,
	FAULT_BATTERY_OVER_VOLT = 8,
	FAULT_DCLINK_OVER_VOLT = 9,
	FAULT_DCLINK_IMBALANCE = 10,
	FAULT_BATTERY_OVER_CURRENT = 11,
	FAULT_GRID_OVER_CURRENT = 12,
	FAULT_HARDWARE_TRIP = 13,
	FAULT_SOFTWARE_CHARGE_FAIL = 14,
	FAULT_HV_LINK_LOW = 15,
	FAULT_THERMAL_FAULT = 16,
	FAULT_POWERED_UP_IN_DEFAULT = 17,
	FAULT_EV_BATTERY_VOLTAGE_EXCEED = 18,
	FAULT_IMD_FAULT_OCCUR = 19,
	FAULT_PCM_EARTH_FAULT = 20,
	FAULT_SPURIOUS_FAULT = 21
} FaultStatus;

typedef enum {
	CHARGER = 0,
	CONNECTOR_1 = 1,
	CONNECTOR_2 = 2
} Connector;

typedef enum {
	INITIATED = 0,
	STARTED = 1,
	FINISHED = 2,
	TERMINATED = 3
} Event ;

typedef enum {
	NA = 0,
	REASON_STOP_BUTTON = 1,
	REASON_EARTH_FAULT = 2,
	REASON_SMOKE_DETECTION = 3,
	REASON_DOOR_OPEN = 4,
	REASON_IMD_FAULT = 5,
	REASON_GRID_FAILURE = 6,
	REASON_BATTERY_OVER_VOLTAGE = 10,
	REASON_DC_LINK_OVER_VOLTAGE = 11,
	REASON_DC_LINK_IMBALANCE = 12,
	REASON_BATT_OVER_CURRENT = 13,
	REASON_HARDWARE_TRIP = 15,
	REASON_PCM_EARTH_FAULT = 16,
	REASON_SOFT_CHARGE_FAIL = 17,
	REASON_HV_LINK_LOW = 18,
	REASON_THERMAL_FAULT = 19,
	REASON_SPURIOUS_FAULT = 20,
	REASON_CANA_FAILED = 21,
	REASON_CANB_FAILED = 22,
	REASON_EV = 23,
	REASON_MOBILE_APP = 26,
	REASON_RFID = 27,
	REASON_CME_ERROR_STATE = 29,
	REASON_CHARGING_COMPLETE = 30,
	REASON_EM_BUTTON = 31,
	REASON_ID_TAG_DEAUTHORISED = 32,
	REASON_CHARGING_AMOUNT_LIMIT = 33,
	REASON_CFE101 = 34,
	REASON_CFE102 = 35,
	REASON_CFE103 = 36,
	REASON_CFE104 = 37,
	REASON_CFE105 = 38,
	REASON_CFE106 = 39,
	REASON_CFE107 = 40,
	REASON_CFE108 = 41,
	REASON_CFE109 = 42,
	REASON_CFE110 = 43,
	REASON_CFE111 = 44,
	REASON_CFE112 = 45,
	REASON_CFE113 = 46,
	REASON_CFE114 = 47,
	REASON_CFE115 = 48,
	REASON_CFE116 = 49,
	REASON_CFE117 = 50
} FinishReason;

typedef enum{
	POWER_MODULE_DATA=0,
	POWER_MODULE_DATA_1 = 1,
	POWER_MODULE_DATA_2 = 2,
	POWER_MODULE_DATA_3 = 3,
	POWER_MODULE_DATA_4 = 4,
	POWER_MODULE_DATA_5 = 5,
	POWER_MODULE_DATA_6 = 6,
	POWER_MODULE_DATA_7 = 7,
	POWER_MODULE_DATA_8 = 8,
	POWER_MODULE_DATA_9 = 9,
	POWER_MODULE_DATA_10 = 10,
	EV_EVSE_COMM_CONTROLLER = 11,
	EV_EVSE_COMM_CONTROLLER_1 = 12,
	EV_EVSE_COMM_CONTROLLER_2 = 13,
	OCPP_CONTROLLER = 14,
	WEBSOCKET_CONNECTION = 15,
	GRID_SUPPLY = 16
} componentName;

Connector connector = CONNECTOR_1;

Event event = STARTED ;

FinishReason finishReason = REASON_BATTERY_OVER_VOLTAGE;

ConnectorStatus ConnectorStatus0 = CONNECTOR_CHARGING;
ConnectorStatus ConnectorStatus1 = CONNECTOR_FINISHED;
ConnectorStatus ConnectorStatus2 = CONNECTOR_AVAILABLE;
ConnectorStatus ConnectorStatus3 = CONNECTOR_UNAVAILABLE;
WebSocketStatus Web_Socket_Status = WEBSOCKET_CONNECTED;

FaultStatus faultStatus0 = FAULT_NOERROR;
FaultStatus faultStatus1 = FAULT_BATTERY_OVER_CURRENT;
FaultStatus faultStatus2 = FAULT_CANA_FAILED;
FaultStatus faultStatus3 = FAULT_SMOKE_FAULT;

void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) 
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();  // Automatically start connecting when WiFi starts
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from WiFi, attempting to reconnect...");
        esp_wifi_connect();  // Reconnect if disconnected
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Connected to WiFi with IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

// Function to Initialize WiFi Connection in esp32
void wifi_init(void) 
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) 
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Setting up this loop allows the ESP32 to track and respond to changes in WiFi status automatically.

    esp_netif_create_default_wifi_sta(); // Sets up the ESP32 in STATION Mode 

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "ApnaBanda",
            .password = "12345abc",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
 //   ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL));
    
    
    ESP_ERROR_CHECK(esp_wifi_start()); // Start The WiFi

    ESP_LOGI(TAG, "Connecting to WiFi...");
//  ESP_ERROR_CHECK(esp_wifi_connect());
}

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t) event_data;
    
    switch(event->event_id) {
		
		case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected");
            
            // Subscribe to the CMS topic (replace "setChargerId/SerialNumber" with the actual topic name from the CMS)
       		char sub_topic1[50];
            snprintf(sub_topic1, sizeof(sub_topic1), "setChargerId/%s", Serial_Number);
            int msg_subscribe_id = esp_mqtt_client_subscribe(mqtt_client, sub_topic1, 1);
         	ESP_LOGI(TAG, "Subscribed to CMS topic with msg_id=%d", msg_subscribe_id);
         	
         	// Subscribe to the CMS topic "getComponentStatus/chargerId" 
         	// This topic send the name of component whose status is required.
         	char sub_topic2[50];
         	snprintf(sub_topic2, sizeof(sub_topic2), "getComponentStatus/%s" ,charger_Id);
         	int msg_subscribe_id2 = esp_mqtt_client_subscribe(mqtt_client, sub_topic2, 1);
         	ESP_LOGI(TAG, "Subscribed to CMS topic with msg_id=%d", msg_subscribe_id2);
         	
         	//Subscribe to the CMS topic "OTA/chargeId"
         	//This topic sends the OTA for a particular charger
         	char sub_topic3[50];
         	snprintf(sub_topic3, sizeof(sub_topic3), "OTA/%s", charger_Id);
         	int msg_subscribe_id3 = esp_mqtt_client_subscribe(mqtt_client, sub_topic3, 1);
         	ESP_LOGI(TAG, "Subscribed to CMS topic with msg_id=%d", msg_subscribe_id3);
         	
         	break;
			
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            break;
            
     	case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Received data from topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "Charger Id: %.*s", event->data_len, event->data);
        
            strncpy(charger_Id, event->data, event->data_len);
            charger_Id[event->data_len] = '\0';  // Null-terminate the string
        	flag1_Check=1;
        
  /*          cJSON *json = cJSON_ParseWithLength(event->data, event->data_len);
	    if (json == NULL) {
    	    ESP_LOGE(TAG, "Error parsing JSON data from CMS");
    	    return;
	   }

		    cJSON *serial_json = cJSON_GetObjectItem(json, "serialNumber");
        if (cJSON_IsString(serial_json) && (serial_json->valuestring != NULL)) {
    	    ESP_LOGI(TAG, "Charger Id: %s", serial_json->valuestring);
    	    // Use the serial number for further actions
	   }*/
		    break;
		    
		default:
            break;   
	  }
}

void init_mqtt() 
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = mqtt_broker_uri,
        .session.keepalive = 120
    };
    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(mqtt_client);
}

void prepare_mqtt_data(char *unique_charger_id, char *topic, char *payload) {
    // Construct the MQTT topic
    snprintf(topic, 50, "chargerPeriodicData/%s", unique_charger_id);
//    char dc_meter_reading_str[10];
//    snprintf(dc_meter_reading_str, sizeof(dc_meter_reading_str), "%.2f", dc_meter_reading);

//      int scaled_value = (int)(dc_meter_reading * 100 + 0.5);  // Scale and round

    // Construct the JSON payload using cJSON library
    cJSON *root = cJSON_CreateObject();
    cJSON_AddNumberToObject(root, "ACMeterReading", ac_meter_reading);
    cJSON_AddNumberToObject(root, "DCMeterReading", dc_meter_reading);
    cJSON_AddNumberToObject(root, "GridVoltage", grid_voltage);
    cJSON_AddNumberToObject(root, "GridCurrent", grid_current);
    cJSON_AddNumberToObject(root, "DCVoltage", dc_voltage);
    cJSON_AddNumberToObject(root, "DCCurrent", dc_current);
    cJSON_AddStringToObject(root, "CMS", cms_name);
    cJSON_AddNumberToObject(root, "ConnectorStatus", connector_status);

    cJSON *power_module_data = cJSON_CreateObject();
    cJSON_AddNumberToObject(power_module_data, "Module1", power_module1_data);
    cJSON_AddNumberToObject(power_module_data, "Module2", power_module2_data);
    cJSON_AddItemToObject(root, "PowerModuleData", power_module_data);

    // Convert JSON object to string
    strcpy(payload, cJSON_Print(root));
    cJSON_Delete(root);
}

/*
void prepare_mqtt_Status_data(char *unique_charger_id, char *status_topic, char *status_payload){
	// Construct the MQTT topic
    snprintf(status_topic, 50, "chargerStatus/%s", unique_charger_id);
    
    // Construct the JSON payload using cJSON library
    cJSON *status_root = cJSON_CreateObject();
	if (status_root == NULL) {
        return;
    }
    
    cJSON_AddNumberToObject(status_root, "connectorStatus0", ConnectorStatus0);
    cJSON_AddNumberToObject(status_root, "connectorStatus1", ConnectorStatus1);
    cJSON_AddNumberToObject(status_root, "connectorStatus2", ConnectorStatus2);
    cJSON_AddNumberToObject(status_root, "connectorStatus3", ConnectorStatus3);
    cJSON_AddNumberToObject(status_root, "webSocketStatus", Web_Socket_Status);
    
    // Convert JSON object to string
    strcpy(status_payload, cJSON_Print(status_root));
    cJSON_Delete(status_root);
    
}*/

void prepare_mqtt_Status_data(char *unique_charger_id, char *status_topic, char *status_payload) {
    // Construct the MQTT topic
    snprintf(status_topic, 50, "chargerStatus/%s", unique_charger_id);

    // Construct the JSON payload using cJSON library
    cJSON *status_root = cJSON_CreateObject();
    if (status_root == NULL) {
        return;
    }

    // Define an array for connector statuses
    int connector_statuses[] = {
        ConnectorStatus0,
        ConnectorStatus1,
        ConnectorStatus2,
        ConnectorStatus3
    };

	int fault_statuses[] = {
		faultStatus0,
		faultStatus1,
		faultStatus2,
		faultStatus3
	};

    // Create a JSON array to hold the connector statuses
    cJSON *status_array = cJSON_CreateArray();
    // Create a JSON array to hold the fault statuses
    cJSON *fault_array = cJSON_CreateArray();
    
     for (int i = 0; i < sizeof(fault_statuses) / sizeof(fault_statuses[0]); i++) {
        cJSON_AddItemToArray(fault_array, cJSON_CreateNumber(fault_statuses[i]));
    }
    cJSON_AddItemToObject(status_root, "faultStatuses", fault_array);
    
    for (int i = 0; i < sizeof(connector_statuses) / sizeof(connector_statuses[0]); i++) {
        cJSON_AddItemToArray(status_array, cJSON_CreateNumber(connector_statuses[i]));
    }
    cJSON_AddItemToObject(status_root, "connectorStatuses", status_array);

    // Add WebSocket status to the JSON object
    cJSON_AddNumberToObject(status_root, "webSocketStatus", Web_Socket_Status);

    // Convert JSON object to string
    strcpy(status_payload, cJSON_Print(status_root));
    cJSON_Delete(status_root);
}

void prepare_mqtt_charging_session_data(char *unique_charger_id, char *charging_session_topic, char *charging_session_payload){
	// Construct the MQTT topic
    snprintf(charging_session_topic, 50, "chargingSession/%s", unique_charger_id);
	
   // Construct the JSON payload using cJSON library
    cJSON *charging_session_root = cJSON_CreateObject();
    if (charging_session_root == NULL) {
        return;
    }

	// Add Connector to the JSON Object 
	cJSON_AddNumberToObject(charging_session_root, "connector", connector);
	
	// Add Event to the JSON Object 
	cJSON_AddNumberToObject(charging_session_root, "Event", event);
	
	// Add finishReason to the JSON Object 
	cJSON_AddNumberToObject(charging_session_root, "finishReason", finishReason);
	
	// Convert JSON object to string
    strcpy(charging_session_payload, cJSON_Print(charging_session_root));
    cJSON_Delete(charging_session_root);
}

void prepare_mqtt_get_Charger_Id_data(char *unique_serial_number, char *get_Charger_Id_topic, char *get_Charger_Id_payload){
	// Construct the MQTT topic
    snprintf(get_Charger_Id_topic, 50, "getChargerId/%s", unique_serial_number);
	
   // Construct the JSON payload using cJSON library
    cJSON *get_chargerId_root = cJSON_CreateObject();
    if (get_chargerId_root == NULL) {
        return;
    }

	// Convert empty JSON object to string
    strcpy(get_Charger_Id_payload, cJSON_PrintUnformatted(get_chargerId_root)); //converts this empty JSON object to a JSON string which represent an empty message
    cJSON_Delete(get_chargerId_root);
}



void mqtt_publish_task(void *param) {
    const TickType_t xDelay = pdMS_TO_TICKS(10000); // 10-second delay
    
    char topic[50];
    char payload[256];
    
    char status_topic[50];
	char status_payload[1000];
	
	char charging_session_topic[50];
	char charging_session_payload[256];
	
	char get_Charger_Id_topic[50];
	char get_Charger_Id_payload[256];
	
    while (1) {
		
		prepare_mqtt_get_Charger_Id_data(Serial_Number, get_Charger_Id_topic, get_Charger_Id_payload);
		int msg_id0 = esp_mqtt_client_publish(mqtt_client, get_Charger_Id_topic, get_Charger_Id_payload, 0, 1, 0);
		ESP_LOGI(TAG, "Published to topic %s with payload %s, msg_id=%d", get_Charger_Id_topic, get_Charger_Id_payload, msg_id0);
		 
		if(charger_Id[0] != '\0' && flag1_Check == 1){
			
        prepare_mqtt_data(charger_Id, topic, payload); // Use actual charger ID
		int msg_id = esp_mqtt_client_publish(mqtt_client, topic, payload, 0, 1, 0);
        ESP_LOGI(TAG, "Published to topic %s with payload %s, msg_id=%d", topic, payload, msg_id);
        
        prepare_mqtt_Status_data(charger_Id, status_topic, status_payload);
        int msg_id2 = esp_mqtt_client_publish(mqtt_client, status_topic, status_payload, 0, 1, 0);
		ESP_LOGI(TAG, "Published to topic %s with payload %s, msg_id=%d", status_topic, status_payload, msg_id2);
		
		prepare_mqtt_charging_session_data(charger_Id, charging_session_topic , charging_session_payload);
        int msg_id3 = esp_mqtt_client_publish(mqtt_client, charging_session_topic, charging_session_payload, 0, 1, 0);
		ESP_LOGI(TAG, "Published to topic %s with payload %s, msg_id=%d", charging_session_topic, charging_session_payload, msg_id3);
		
		}
        vTaskDelay(xDelay); // Delay for periodic publish
        
    }
}

// Main Function Here : 
void app_main() {
    ESP_LOGI(TAG, "Starting MQTT Charger Example - By Dhruv");

    // Initialize WiFi
    wifi_init();

    // Wait for WiFi connection
    vTaskDelay(pdMS_TO_TICKS(5000));

    // Initialize MQTT client
    init_mqtt();

    // Start periodic publishing task
    xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
    /*while(1){
		if(chargerId != NULL && Mqtt_task_creation==0)
		{
			 Mqtt_task_creation = 1;
			 xTaskCreate(mqtt_publish_task, "mqtt_publish_task", 4096, NULL, 5, NULL);
			 
		}
		 vTaskDelay(pdMS_TO_TICKS(500));
	}*/
}