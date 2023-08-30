#include <WiFi.h>
#include <Wire.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SparkFunCCS811.h>
#include <SPI.h>
#include <HTTPClient.h>
#include <AsyncElegantOTA.h>
#include <Preferences.h>
#include "Secrets.h"
#include "FS.h"
#include "SD.h"

Preferences preferences;
AsyncWebServer server(80);
CCS811 mySensor(0X5A);


unsigned long lastExecutedTime = 0;

int max_safe;
int max_caution;
int wait_Interval;

bool buzzer = true;

int read_CO2() {
  mySensor.readAlgorithmResults();
  float c = mySensor.getCO2();
  if (isnan(c)) {
    Serial.println("Failed to read from co2 sensor!");
    return -1;
  }
  else {
    //Serial.println(c);
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
    //Serial.println(v);
    return int(v);
  }

}



String status_Msg() {
  int c = read_CO2();
  if (c == -1)
  {
    return "ERROR!";
  }

  else if (c < preferences.getInt("max_safe", safe_threshold)) {
    return "Safe levels. You do not need to worry";
  }
  else if (c > preferences.getInt("max_caution", danger_threshold)) {
    return "Danger! Please do not stay for an extended amount of time";
  }
  else if (c > preferences.getInt("max_safe", safe_threshold)) {
    return "Caution. Ventilate your rooms.";
  }
}

String processor(const String & var) {
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


}

void buzzer_check() {
  if (buzzer == true) {
    tone(14, 800);
  }
  else if (buzzer == false) {
    noTone(14);
  }

}

void sendDiscordMessage(const String & message) {
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

void setup() {

  // Initialize Serial monitor
  Serial.begin(115200);
  pinMode(14, OUTPUT);
  tone(14, 440);
  Wire.begin(); //Inialize I2C Hardware
  preferences.begin("Co2-Sensor", false); // This creates/open the Co2-Sensor profile, which is a persistant settings profile.
  max_safe = preferences.getInt("max_safe", safe_threshold);
  max_caution = preferences.getInt("max_caution", danger_threshold);
  wait_Interval = preferences.getInt("wait_time", sensor_wait);



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
      //String message = "@everyone ESP32 IP Address: " + localIP.toString();

      // Send message to Discord webhook
      sendDiscordMessage("@everyone ESP32 IP Address: " + localIP.toString() + ". MAC-Address: " + WiFi.macAddress());
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

  server.on("/about.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SD, "/about.html", String(), false, processor);
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
    if (c < preferences.getInt("max_safe", safe_threshold)) {
      request->send(SD, "/styles-safe.css", "text/css");
    }
    else if (c > preferences.getInt("max_caution", safe_threshold)) {
      request->send(SD, "/styles-danger.css", "text/css");
    }
    else if (c > preferences.getInt("max_safe", safe_threshold)) {
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

  server.onNotFound([](AsyncWebServerRequest * request) { //handles errors with a cool, custom webpage
    request->send(SD, "/error.html", String(), false, processor);
  });

  server.on("/get-safe", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasArg("input_integer")) {
      int inputMessage = request->arg("input_integer").toInt();

      max_safe = inputMessage;
      preferences.putInt("max_safe", max_safe);
      Serial.print("Received max_safe: ");
      Serial.println(max_safe);
      if (inputMessage == 0) {
        Serial.print("Malformed input has been detected, default settings have been applied");
        max_safe = safe_threshold;
        preferences.putInt("max_safe", max_safe);
        request->send(SD, "/malformed.html", String(), false, processor);

      }
      else {
        request->send(SD, "/commited.html", String(), false, processor);

      }

    }
  });

  server.on("/get-caution", HTTP_GET, [](AsyncWebServerRequest * request) {

    if (request->hasArg("input_integer")) {
      int inputMessage = request->arg("input_integer").toInt();
      max_caution = inputMessage;
      preferences.putInt("max_caution", max_caution);
      Serial.print("Received max_caution: ");
      Serial.println(max_caution);
      if (inputMessage == 0) {
        Serial.print("Malformed input has been detected, default settings have been applied");
        max_caution = danger_threshold;
        preferences.putInt("max_caution", max_caution);
        request->send(SD, "/malformed.html", String(), false, processor);

      }

      else {
        request->send(SD, "/commited.html", String(), false, processor);
      }
    }


  });

  server.on("/get-wait", HTTP_GET, [](AsyncWebServerRequest * request) {
    if (request->hasArg("input_integer")) {
      int inputMessage = request->arg("input_integer").toInt();
      wait_Interval = inputMessage;
      preferences.putInt("wait_time", wait_Interval);
      Serial.print("Received wait_Interval: ");
      Serial.println(wait_Interval);
      if (inputMessage == 0) {
        Serial.print("Malformed input has been detected, default settings have been applied");
        wait_Interval = sensor_wait;
        preferences.putInt("wait_time", wait_Interval);
        request->send(SD, "/malformed.html", String(), false, processor);

      }
      else {
        request->send(SD, "/commited.html", String(), false, processor);
      }
    }
  });
  server.on("/buzzer-password", HTTP_GET, [](AsyncWebServerRequest * request) {

    if (request->hasArg("input_integer")) {
      const char* inputMessage = request->arg("input_integer").c_str();
      Serial.print("Received OTA password: ");
      Serial.println(inputMessage);

      if (strcmp(inputMessage, password_OTA) == 0) {
        Serial.println("Buzzer settings have been changed.");

        if (buzzer) {
          buzzer = false;
          request->send(SD, "/buzzer-off.html", String(), false, processor);
        } else {
          buzzer = true;
          request->send(SD, "/buzzer-on.html", String(), false, processor);
        }
      } else {
        Serial.println("Access Denied!");   
        request->send(SD, "/unauthorized.html", String(), false, processor);
      }
    }


  });




  /*
    server.on("/get-caution", HTTP_GET, [](AsyncWebServerRequest * request) {
      if (request->hasParam("input_integer", true)) {
        int inputMessage;
        inputMessage = request->getParam("input_integer", true)->value().toInt();
        inputMessage = max_caution;
        Serial.print(inputMessage);
        Serial.print(max_caution);


      }
      request->send(200, "text/plain", String(max_caution));
    });

    server.on("/get-wait", HTTP_GET, [](AsyncWebServerRequest * request) {
      if (request->hasParam("input_integer", true)) {
        int inputMessage;
        inputMessage = request->getParam("input_integer", true)->value().toInt();
        inputMessage = wait_Interval;
        Serial.print(inputMessage);
        Serial.print(wait_Interval);
      }
      request->send(200, "text/plain", String(wait_Interval));
    });




    }*/
  server.begin();

  AsyncElegantOTA.begin(&server, username_OTA, password_OTA);

  Serial.print("Your OTA Username is: ");
  Serial.println(username_OTA);
  Serial.print("Your OTA password is: ");
  Serial.println(password_OTA);

  Serial.print("Maximum Safe level is: ");
  Serial.println(max_safe);
  Serial.print("Max caution level is: ");
  Serial.println(max_caution);
  Serial.print("Wait interval in milliseconds: ");
  Serial.println(wait_Interval);



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
      buzzer_check();
    }

    else {
      noTone(14);
    }
    lastExecutedTime = current_Millisecond;
  }

}
