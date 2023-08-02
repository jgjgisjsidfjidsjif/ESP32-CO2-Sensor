// If you want your ESP-32 to be the access point, set it to True. Otherwise, if you want your ESP-32 to connect to your network, set it to false.
const bool is_AP = false;
const bool debug_Sensor = false;
const bool discord_Integration = true;

const char* username_OTA = "OTA_USERNAME";
const char* password_OTA = "OTA_PASSWORD";

const char* ssid = "SSID";
const char* password = "PASSWORD";

//Discord Integration. Only works when is_AP is not true.
const char* webhookUrl = "DISCORD WEBHOOK URL";

int max_safe = 1000;
int max_caution = 4000;

int wait_Interval = 2000;
