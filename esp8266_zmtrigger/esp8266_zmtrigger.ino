/*
* ESP8266_zmtrigger 
*
* Date of Last Revision: Jan 14, 2015
*
* Upon activation of GPIO2, this sketch sends a user defined command to a ZoneMinder 
* server running zmtrigger on port 6802.
* 
*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266WebServer.h>

#define GPIO0 0
#define GPIO2 2

ESP8266WiFiMulti WiFiMulti;
WiFiClient client;
ESP8266WebServer server(80);

// Define global variables
const char zmserver[] = "192.168.1.122";
const int port = 6802;
bool sensorActive = false;
unsigned long previousMillis = 0;

/* 
*  zmTrigger expects commands to be in the following format:
*  
*<id>|<action>|<score>|<cause>|<text>|<showtext>
*
*<id>
*  is the id number or name of the ZM monitor.
*  
*<action>
*  Valid actions are 'on', 'off', 'cancel' or 'show' where
*  'on' forces an alarm condition on;
*  'off' forces an alarm condition off;
*  'cancel' negates the previous 'on' or 'off'.
*  The 'show' action merely updates some auxiliary text which can optionally
*  be displayed in the images captured by the monitor. Ordinarily you would
*  use 'on' and 'cancel', 'off' would tend to be used to suppress motion
*  based events. Additionally 'on' and 'off' can take an additional time
*  offset, e.g. on+20 which automatically 'cancel's the previous action
*  after that number of seconds.
*  
*<score>
*  is the score given to the alarm, usually to indicate it's
*  importance. For 'on' triggers it should be non-zero, otherwise it should
*  be zero.
*  
*<cause>
*  is a 32 char max string indicating the reason for, or source of
*  the alarm e.g. 'Relay 1 open'. This is saved in the 'Cause' field of the
*  event. Ignored for 'off' or 'cancel' messages.
*  
*<text>
*  is a 256 char max additional info field, which is saved in the
*  'Description' field of an event. Ignored for 'off' or 'cancel' messages.
*  
*<showtext>
*  is up to 32 characters of text that can be displayed in the
*  timestamp that is added to images. The 'show' action is designed to
*  update this text without affecting alarms but the text is updated, if
*  present, for any of the actions. This is designed to allow external input
*  to appear on the images captured, for instance temperature or personnel
*  identity etc.
*/

String zmtriggerStartCmd = "2|on|200|Woofing Detected|The dogs are barking. Check who is at the door.|Alarm";
String zmtriggerStopCmd = "2|off+10|0|Woofing Has Stopped|The dogs are no longer barking.|Normal";

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
    Serial.print("Signal Strength (%): ");
    Serial.println(2 * (WiFi.RSSI() + 100));
}

void updateZMTrigger(String Cmd) {
  if (client.connect(zmserver, port)) {
    client.println(Cmd);

    if (client.connected()) {
      Serial.println();
      Serial.println("Event Trigger Sent to ZoneMinder");
      Serial.println(Cmd);
    }
  } else {
    Serial.println();
    Serial.println("Error Connecting to ZoneMinder server!");
    Serial.println(client.readString());
  }
}

void setup() {
    // Configure our GPIO's as inputs
    pinMode(GPIO0, INPUT_PULLUP);
    pinMode(GPIO2, INPUT_PULLUP);  
    
    Serial.begin(115200);
   // Serial.setDebugOutput(true);

    Serial.println("");

    for(uint8_t t = 4; t > 0; t--) {
        Serial.printf("[SETUP] WAIT %d...\n", t);
        Serial.flush();
        delay(1000);
    }

    // WiFIMulti class allows you to add as many wifi networks are you want
    WiFiMulti.addAP("ssid1", "password1");
    WiFiMulti.addAP("ssid2", "password2");

    // Wait for connection
    Serial.print("Waiting for WiFi to Connect...");
    while (WiFiMulti.run() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("Connected!");
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
}

void loop() {
  
    server.handleClient();
    
    // Reset previousMillis when millis overflows back to zero
    if (millis() - previousMillis < 0 ) {
      previousMillis = 0;
    }

    if ( millis() - previousMillis > 10000 ) {
        WiFiStatus();
        Serial.println("GPIO0 state: "+String(digitalRead(GPIO0)));
        Serial.println("GPIO2 state: "+String(digitalRead(GPIO2)));
        previousMillis = millis();
    }

    if ( !digitalRead(GPIO2) && !sensorActive ) { //Sensor attached to this input detected something
        updateZMTrigger(zmtriggerStartCmd);
        sensorActive = true;
    }

    if ( digitalRead(GPIO2) && sensorActive ) {
        updateZMTrigger(zmtriggerStopCmd);
        sensorActive = false;
    }

}
