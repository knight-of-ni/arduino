/*
* ESP8266_zmtrigger 
*
* Date of Last Revision: Jan 16, 2015
* Andrew Bauer knnniggett@users.sourceforge.net
*
* Upon activation of GPIO2, this sketch sends a user defined command to a ZoneMinder 
* server running zmtrigger on port 6802.
* 
* Credit goes to Alex a.k.a. tzapu for his awesome WiFiManager class!
* 
* All ZoneMinder parameters are configured from the WiFiManager.
* See https://github.com/tzapu/WiFiManager for general WiFiManager instructions.
* See https://github.com/ZoneMinder/ZoneMinder/blob/master/scripts/zmtrigger.pl.in
* for zmtrigger source and help text.
* 
*/

#include <FS.h>                   //this needs to be first, or it all crashes and burns...

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#define GPIO0 0
#define GPIO2 2

WiFiClient client;
ESP8266WebServer server(80);

// Set these all to NULL. Placing values here will hide the help text in each field.
char zm_server[40];
char zmtrigger_port[5];
char monitor_id[3];
char alarm_delay[6];
char alarm_score[5];
char alarm_cause[32];
char alarm_desc[40];

bool sensorActive = false;
unsigned long previousMillis = 0;

//flag for saving data
bool shouldSaveConfig = false;

//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}

void handle_root()
{
  server.send(200, "text/plain", "Hello from the esp8266, read from /gpio0 or /gpio2");
  delay(100);
}

void WiFiStatus() {
    Serial.println("");
    Serial.println("WiFi Status");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    Serial.print("MAC address: ");
    Serial.println(WiFi.macAddress());
    Serial.print("SSID: ");
    Serial.println(WiFi.SSID());
    Serial.print("Signal Strength (dBm): ");
    Serial.println(WiFi.RSSI());
}

void updateZMTrigger(String Cmd) {
  if (client.connect(zm_server, atoi(zmtrigger_port))) {
    client.println(Cmd);

    if (client.connected()) {
      Serial.println();
      Serial.println("Event Trigger Successfully Sent to ZoneMinder Server");
      Serial.println(Cmd);
    }
  } else {
    Serial.println();
    Serial.println("Error Connecting to ZoneMinder server at " + String(zm_server) + ":" + String(zmtrigger_port));
    Serial.println(client.readString());
  }
}
void setup() {
  // Configure our GPIO's as inputs
  pinMode(GPIO0, INPUT_PULLUP);
  pinMode(GPIO2, INPUT_PULLUP);  

  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();

  //clean FS, for testing
  //SPIFFS.format();

  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        StaticJsonBuffer<1024> jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");

          if (json.containsKey("zm_server")) {
              strcpy(zm_server,json["zm_server"]);
          }
          if (json.containsKey("zmtrigger_port")) {
              strcpy(zmtrigger_port,json["zmtrigger_port"]);
          }
          if (json.containsKey("monitor_id")) {
              strcpy(monitor_id,json["monitor_id"]);
          }
          if (json.containsKey("alarm_delay")) {
              strcpy(alarm_delay,json["alarm_delay"]);
          }
          if (json.containsKey("alarm_score")) {
              strcpy(alarm_score,json["alarm_score"]);
          }
          if (json.containsKey("alarm_cause")) {
              strcpy(alarm_cause,json["alarm_cause"]);
          }
          if (json.containsKey("alarm_desc")) {
              strcpy(alarm_desc, json["alarm_desc"]);
          }
        } else {
          Serial.println("failed to load json config");
        }
      }
    }
  } else {
    Serial.println("failed to mount FS");
  }
  //end read

  Serial.print("alarm_desc: ");
  Serial.println(alarm_desc);

  // The extra parameters to be configured (can be either global or just in the setup)
  // After connecting, parameter.getValue() will get you the configured value
  // id/name placeholder/prompt default length
  WiFiManagerParameter custom_zm_server("zmserver", "ZoneMinder Server IP", zm_server, 40);
  WiFiManagerParameter custom_zmtrigger_port("zmtport", "zmTrigger Port (6802)", zmtrigger_port, 5);
  WiFiManagerParameter custom_monitor_id("id", "Monitor ID", monitor_id, 3);
  WiFiManagerParameter custom_alarm_delay("delay", "Alarm Delay Seconds", alarm_delay, 6);
  WiFiManagerParameter custom_alarm_score("score", "Alarm Score", alarm_score, 5);
  WiFiManagerParameter custom_alarm_cause("cause", "Cause - Max 32 Characters", alarm_cause, 32);
  WiFiManagerParameter custom_alarm_desc("description", "Desc - Max 256 Characters", alarm_desc, 40);

  Serial.print("custom_alarm_desc: ");
  Serial.println(custom_alarm_desc.getValue());
  
  //WiFiManager
  //Local intialization. Once its business is done, there is no need to keep it around
  WiFiManager wifiManager;

  // Turn off debug output
  //wifiManager.setDebugOutput(false);
  
  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);

  //add all your parameters here
  wifiManager.addParameter(&custom_zm_server);
  wifiManager.addParameter(&custom_zmtrigger_port);
  wifiManager.addParameter(&custom_monitor_id);
  wifiManager.addParameter(&custom_alarm_delay);
  wifiManager.addParameter(&custom_alarm_score);
  wifiManager.addParameter(&custom_alarm_cause);
  wifiManager.addParameter(&custom_alarm_desc);

  //reset settings - for testing
  wifiManager.resetSettings();

  //set minimu quality of signal so it ignores AP's under that quality
  //defaults to 8%
  wifiManager.setMinimumSignalQuality();
  
  //sets timeout until configuration portal gets turned off
  //useful to make it all retry or go to sleep
  //in seconds
  wifiManager.setTimeout(300);

  //fetches ssid and pass and tries to connect
  //if it does not connect it starts an access point with the specified name
  //here  "AutoConnectAP"
  //and goes into a blocking loop awaiting configuration
  if (!wifiManager.autoConnect("ESP8266AutoConnectAP", "security")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println();
  Serial.println("WiFi Connected!");

  //read updated parameters
  strcpy(zm_server,custom_zm_server.getValue());
  strcpy(zmtrigger_port, custom_zmtrigger_port.getValue());
  strcpy(monitor_id, custom_monitor_id.getValue());
  strcpy(alarm_delay, custom_alarm_delay.getValue());
  strcpy(alarm_score, custom_alarm_score.getValue());
  strcpy(alarm_cause, custom_alarm_cause.getValue());
  strcpy(alarm_desc, custom_alarm_desc.getValue());

  //save the custom parameters to FS
  if (shouldSaveConfig) {
    Serial.println("saving config");
    StaticJsonBuffer<1024> jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    json["zm_server"] = zm_server;
    json["zmtrigger_port"] = zmtrigger_port;
    json["monitor_id"] = monitor_id;
    json["alarm_delay"] = alarm_delay;
    json["alarm_score"] = alarm_score;
    json["alarm_cause"] = alarm_cause;
    json["alarm_desc"] = alarm_desc;

    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
  }
  server.on("/", handle_root);
  
  server.on("/gpio0", [](){  // if you add this subdirectory to your webserver call, you get text below :)
      server.send(200, "text/plain", "Current value of GPIO0: "+String(digitalRead(GPIO0)));            // send to someones browser when asked
  });

  server.on("/gpio2", [](){  // if you add this subdirectory to your webserver call, you get text below :)
      server.send(200, "text/plain", "Current value of GPIO2: "+String(digitalRead(GPIO2)));               // send to someones browser when asked
  });

  server.begin();
  Serial.println("");
  Serial.println("HTTP server started");
  WiFiStatus();
}

void loop() {
    server.handleClient();  
  
    // Reset previousMillis when millis overflows back to zero
    if (millis() - previousMillis < 0 ) {
      previousMillis = 0;
    }

//    if ( millis() - previousMillis > 10000 ) {
//        WiFiStatus();
//        Serial.println("GPIO0 state: "+String(digitalRead(GPIO0)));
//        Serial.println("GPIO2 state: "+String(digitalRead(GPIO2)));
//        previousMillis = millis();
//    }

    if ( !digitalRead(GPIO2) && !sensorActive ) { //Sensor attached to this input detected something
        Serial.println(alarm_desc);
        updateZMTrigger(String(monitor_id) + "|on|" + String(alarm_score) + "|" + String(alarm_cause) + "|" + String(alarm_desc) + "|Alarm");
        sensorActive = true;
    }

    if ( digitalRead(GPIO2) && sensorActive ) {
        updateZMTrigger(String(monitor_id) + "|off+" + String(alarm_delay) + "|0|Alarm Event Restored|Sensor returned to normal state|Restored");
        sensorActive = false;
    }
}
