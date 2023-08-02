// If you want your ESP-32 to be the access point, set it to True. Otherwise, if you want your ESP-32 to connect to your network, set it to false.
const bool is_AP = false;
const bool debug_Sensor = false;
const bool discord_Integration = true;

const char* username_OTA = "w.i.zhang";
const char* password_OTA = "Lwz195";

const char* ssid = "SKC-MAC";
const char* password = ">cuvXCXy+yPD58p?";

//Discord Integration. Only works when is_AP is not true.
const char* webhookUrl = "https://discord.com/api/webhooks/1123407102019383428/HP6gpLgDbWTuilm_--zjdMw0KInJu9cwRfkx0j7N8u4MNzK_YMQfRWwhtiZCUXH5wdeW";

int max_safe = 1000;
int max_caution = 4000;

int wait_Interval = 2000;
