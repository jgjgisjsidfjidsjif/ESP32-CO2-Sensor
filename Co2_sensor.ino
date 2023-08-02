#include <WiFi.h>
#include <Wire.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SparkFunCCS811.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <AsyncElegantOTA.h>
#include "Secrets.h"
#include "FS.h"
#include "SD.h"

AsyncWebServer server(80);
CCS811 mySensor(0X5A);

unsigned long lastExecutedTime = 0;

int read_CO2() {
  mySensor.readAlgorithmResults();
  float c = mySensor.getCO2();
  if (isnan(c)) {
    Serial.println("Failed to read from co2 sensor!");
    return -1;
  }
  else {
    Serial.println(c);
    return int(c);
  }
}

int read_VOC() {
  mySensor.readAlgorithmResults();
  float v = mySensor.getTVOC();
  if (isnan(v)) {
    Serial.println("Failed to read VOC concentration from CO2 Sensor");
    return -1;
  }
  else {
    Serial.println(v);
    return int(v);
  }

}



String status_Msg() {
  int c = read_CO2();
  if (c == -1)
  {
    return "ERROR!";
  }

  if (c < max_safe) {
    return "Safe levels. You do not need to worry";
  }
  else if (c > max_caution) {
    return "Danger! Please do not stay for an extended amount of time";
  }
  else if (c > max_safe) {
    return "Caution. Ventilate your rooms.";
  }

}

String processor(const String& var) {
  //Serial.println(var);
  if (var == "STATUS") {
    return status_Msg().c_str();
  }

  if (var == "CO2") {
    return String(read_CO2());
  }

  if (var == "TVOC") {
    return String(read_VOC());
  }

  return String();
}

void setup() {
  // Initialize Serial monitor
  Serial.begin(115200);
  pinMode(14, OUTPUT);
  tone(14, 440);
  Wire.begin(); //Inialize I2C Hardware


  if (mySensor.begin() == false)
  {
    Serial.print("CCS811 error. Please check your wiring and restart the ESP32");
    while (1);
  }

  if (!SD.begin()) {
    Serial.println("Card Mount Failed");
    return;
  }
  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return;
  }
  noTone(14);


  // Connect to Wi-Fi
  if (is_AP == true) {
    WiFi.softAP(ssid, password);
    IPAddress ip = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(ip);
  }
  else {

    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
      delay(900);
      Serial.println("Connecting to WiFi..");
      tone(14, 220);
      delay(100);
      noTone(14);
    }
    //Print the IP address
    Serial.print("Connected to your Wifi network! Your ip address is: ");
    Serial.println(WiFi.localIP());

    if (discord_Integration == true) {
      // Get local IP address
      IPAddress localIP = WiFi.localIP();

      // Construct the message
      String message = "@everyone ESP32 IP Address: " + localIP.toString();

      // Send message to Discord webhook

      sendDiscordMessage(message);

    }

  }

  Serial.print("Your SSID is: ");
  Serial.println(ssid);
  if (password == NULL) {
    Serial.print("You don't have a password set. You can connect to ");
    Serial.println(ssid);
    Serial.println(" Without a password");
  }
  else {
    Serial.print("Your password is: ");
    Serial.println(password);
  }





  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/index.html", String(), false, processor);
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/index.html", String(), false, processor);
  });

  server.on("/stats.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/stats.html", String(), false, processor);
  });

  server.on("/settings.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/settings.html", String(), false, processor);
  });


  // Route to load style.css file
  server.on("/styles.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    int c = read_CO2();
    if (c < max_safe) {
      request->send(SD, "/styles-safe.css", "text/css");
    }
    else if (c > max_caution) {
      request->send(SD, "/styles-danger.css", "text/css");
    }
    else if (c > max_safe) {
      request->send(SD, "/styles-caution.css", "text/css");
    }
  });

  // This happens when you want CO2 measurements
  server.on("/co2", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(read_CO2()));
  });

  // This happens when you want VOC measurements
  server.on("/voc", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200, "text/plain", String(read_VOC()));
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/favicon.ico");
  });

  server.begin();

  AsyncElegantOTA.begin(&server, username_OTA, password_OTA);

  Serial.print("Your OTA Username is: ");
  Serial.println(username_OTA);
  Serial.print("Your OTA password is: ");
  Serial.println(password_OTA);
}

void loop() {
  unsigned long current_Millisecond = millis();
  //Is debug settings enabled?
  if (debug_Sensor == true) {
    //If so, have the sensor read and calculate the results.
    //Get them later
    mySensor.readAlgorithmResults();

    Serial.print("CO2[");
    //Returns calculated CO2 reading
    Serial.print(mySensor.getCO2());
    Serial.print("] tVOC[");
    //Returns calculated TVOC reading
    Serial.print(mySensor.getTVOC());
    Serial.print("] millis[");
    //Display the time since program start
    Serial.print(millis());
    Serial.print("]");
    Serial.println();

    delay(1000); //Don't spam the I2C bus

  }


  if (current_Millisecond - lastExecutedTime >= wait_Interval) {
    read_CO2();
    if (read_CO2() >= max_caution) {
      tone(14, 800);
    }

    else {
      noTone(14);
    }
    lastExecutedTime = current_Millisecond;
  }

}

void sendDiscordMessage(const String& message) {
  delay(1000);
  HTTPClient http;

  // Prepare the payload
  String payload = "{\"content\":\"" + message + "\"}";

  // Send POST request to Discord webhook
  http.begin(webhookUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(payload);

  // Check if the request was successful
  if (httpResponseCode == HTTP_CODE_OK) {
    Serial.println("Message sent to Discord!");
  }
  else if (httpResponseCode == 204) {
    Serial.println("Message sent to Discord!");
  }
  else {
    Serial.print("Error sending message to Discord. Error code: ");
    Serial.println(httpResponseCode);
  }

  // Free resources
  http.end();
}
