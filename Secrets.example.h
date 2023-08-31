// Change values if needed, and remove the ".example".
// If you want your ESP-32 to be the access point, set it to True. Otherwise, if you want your ESP-32 to connect to your network, set it to false.
const bool is_AP = false;
const bool debug_Sensor = false;
const bool discord_Integration = true;


const char* username_OTA = "CHANGE THIS PLEASE";
const char* password_OTA = "CHANGE THIS PLEASE";

const char* ssid =  "WIFI SSID";
const char* password = "WIFI PASSWORD. YOU CAN ALSO REPLACE THIS WITH NULL IF YOU ARE CONNECTING TO A NETWORK WITHOUT A PASSWORD";
//Discord Integration. Only works when is_AP is not true.
const char* webhookUrl = "https://discord.com/api/webhooks/NUMBERS/WEBHOOK-SECRET-KEYS";

#define safe_threshold 1000
#define danger_threshold 4000
#define sensor_wait 2000
