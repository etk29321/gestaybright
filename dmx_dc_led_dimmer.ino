#include "GEstaybright.h"
#include <WiFi.h>
#include <ESPTelnet.h>
#include <ESPAsyncE131.h>
#include <EEPROM.h>
#include <esp_dmx.h>
#define EEPROM_SIZE 168
#define TIMER_SPEED 2
#define UNIVERSE_COUNT 1

int UNIVERSE;  // First DMX Universe to listen for
int START_CHANNEL;
int ISMULTICAST;
ESPAsyncE131 e131(UNIVERSE_COUNT);
String WIFI_SSID;
String WIFI_PASSWORD;
WiFiServer server(80);
IPAddress ip;
int WIFI_MODE;
int STATIC_IP;
IPAddress WIFI_IP;
IPAddress WIFI_SUBNET;
IPAddress WIFI_GW;
IPAddress WIFI_DNSServer;
int its_effect_time;
//gestring lights(16, 17); // the led light string is connected to GPIO16 and GPIO17
gestring lights(12, 14);  // the led light string is connected to GPIO16 and GPIO17
COLOR_MODE_typedef color;
EFFECT_MODE_typedef mode;
int raw_color;
int brightness;
int factory_reset_called = 0;
int boot_complete = 0;
hw_timer_t *effect_timer;

//local DMX settings
#define DMXtx 17
#define DMXrx 16
#define DMXrts 21
#define DMXport 1
byte data[DMX_PACKET_SIZE];
bool dmxIsConnected = false;
unsigned long lastUpdate = millis();


void IRAM_ATTR isr_reset() {
  factory_reset_called = 1;
}

void IRAM_ATTR runEffectsTimer() {
  its_effect_time = 1;
}

void factory_reset() {
  factory_reset_called = 0;
  Serial.print("Factory reset...");
  digitalWrite(2, LOW);
  int i = 0;
  while (digitalRead(0) == LOW && i < 5) {
    Serial.print("*");
    i++;
    delay(1000);
  }
  digitalWrite(2, LOW);
  if (digitalRead(0) == LOW) {
    Serial.println("resetting.");
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(100);
    digitalWrite(2, HIGH);
    delay(100);
    digitalWrite(2, LOW);
    delay(100);
    digitalWrite(2, HIGH);
    UNIVERSE = 1;
    START_CHANNEL = 1;
    ISMULTICAST = 0;  // 0 for multicast mode
    uint8_t mac[6];
    WiFi.macAddress(mac);
    String macLast4 = String(mac[4], HEX) + String(mac[5], HEX);
    WIFI_SSID = "GE-DMX-" + macLast4;
    WIFI_PASSWORD = "dmx";
    WIFI_MODE = 0;  // 0 for AP mode
    STATIC_IP = 1;
    WIFI_IP.fromString("192.168.2.1");
    WIFI_SUBNET.fromString("255.255.255.0");
    WIFI_GW.fromString("192.168.2.1");
    WIFI_DNSServer.fromString("8.8.8.8");
    mode = SLO_GLO;
    Serial.println(WIFI_SSID);
    writeEEPROM();
    ESP.restart();  //make settings changes effective
  } else {
    Serial.println("reset canceled.");
  }
}


void writeStringToEEPROM(int addrOffset, const String &strToWrite) {
  byte len = strToWrite.length();
  EEPROM.write(addrOffset, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(addrOffset + 1 + i, strToWrite[i]);
  }
}

String readStringFromEEPROM(int addrOffset) {
  int newStrLen = EEPROM.read(addrOffset);
  char data[newStrLen + 1];
  for (int i = 0; i < newStrLen; i++) {
    data[i] = EEPROM.read(addrOffset + 1 + i);
  }
  data[newStrLen] = '\0';
  return String(data);
}

void readEEPROM() {
  /*. DMX Universe: 1 byte
    DMX Start Channel: 1 byte
    DMX Multicast/Unicast: 1 byte
    Wifi SSID: 32 bytes
    Wifi Password: 63 bytes
    = 98 total bytes */
  EEPROM.begin(EEPROM_SIZE);
  UNIVERSE = EEPROM.read(0);
  Serial.println("Read UNIVERSE from pos zero: ");
  Serial.println(UNIVERSE);

  START_CHANNEL = EEPROM.read(1);
  ISMULTICAST = EEPROM.read(2);
  WIFI_SSID = readStringFromEEPROM(3);
  WIFI_PASSWORD = readStringFromEEPROM(36);
  WIFI_MODE = EEPROM.read(100);
  STATIC_IP = EEPROM.read(101);
  WIFI_IP.fromString(readStringFromEEPROM(102));
  WIFI_SUBNET.fromString(readStringFromEEPROM(118));
  WIFI_GW.fromString(readStringFromEEPROM(134));
  WIFI_DNSServer.fromString(readStringFromEEPROM(150));
  mode = (EFFECT_MODE_typedef)EEPROM.read(167);
}

void parseToken(String token) {
  String var = token.substring(0, token.indexOf("="));
  String val = token.substring(token.indexOf("=") + 1);
  if (var.compareTo("UNIVERSE") == 0) {
    UNIVERSE = val.toInt();
  }
  if (var.compareTo("CHANNEL") == 0) {
    START_CHANNEL = val.toInt();
  }
  if (var.compareTo("MULTICAST") == 0) {
    if (val.compareTo("MULTICAST") == 0)
      ISMULTICAST = 0;
    if (val.compareTo("UNICAST") == 0)
      ISMULTICAST = 1;
  }
  if (var.compareTo("WIFI_SSID") == 0) {
    WIFI_SSID = val;
  }
  if (var.compareTo("WIFI_PASSWORD") == 0) {
    WIFI_PASSWORD = val;
  }
  if (var.compareTo("WIFI_MODE") == 0) {
    WIFI_MODE = val.toInt();
    //Serial.printf("Set WIFI_MODE to: %i\n",WIFI_MODE);
  }
  if (var.compareTo("STATIC_IP") == 0) {
    STATIC_IP = val.toInt();
  }
  if (var.compareTo("WIFI_IP") == 0) {
    WIFI_IP.fromString(val);
  }
  if (var.compareTo("WIFI_SUBNET") == 0) {
    WIFI_SUBNET.fromString(val);
  }
  if (var.compareTo("WIFI_GW") == 0) {
    WIFI_GW.fromString(val);
  }
  if (var.compareTo("WIFI_DNSServer") == 0) {
    WIFI_DNSServer.fromString(val);
  }
  if (var.compareTo("EFFECT") == 0) {
    mode = (EFFECT_MODE_typedef)(val.toInt());
    Serial.printf("Set mode to: %i\n", mode);
  }
}

void writeEEPROM() {
  /*. DMX Universe: 1 byte
    DMX Start Channel: 1 byte
    DMX Multicast/Unicast: 1 byte
    Wifi SSID: 32 bytes
    Wifi Password: 63 bytes
    255.255.255.255
    123456789012345
    = 98 total bytes */
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(0, UNIVERSE);
  EEPROM.write(1, START_CHANNEL);
  EEPROM.write(2, ISMULTICAST);
  writeStringToEEPROM(3, WIFI_SSID);
  writeStringToEEPROM(36, WIFI_PASSWORD);
  EEPROM.write(100, WIFI_MODE);
  EEPROM.write(101, STATIC_IP);
  writeStringToEEPROM(102, WIFI_IP.toString());
  writeStringToEEPROM(118, WIFI_SUBNET.toString());
  writeStringToEEPROM(134, WIFI_GW.toString());
  writeStringToEEPROM(150, WIFI_DNSServer.toString());
  EEPROM.write(167, (int)mode);

  EEPROM.commit();
}


void processDMX(e131_packet_t *packet, void *UserInfo) {

  /*Serial.printf("Universe %u / %u Channels | Packet#: %u / Errors: %u / CH1: %u\n", 
      htons(packet->universe),                 // The Universe for this packet
      htons(packet->property_value_count) - 1, // Start code is ignored, we're interested in dimmer data
      e131.stats.num_packets,                 // Packet counter
      e131.stats.packet_errors,               // Packet error counter
      packet->property_values[1]);             // Dimmer data for Channel 1
    */
  if (packet->property_values[START_CHANNEL] != brightness) {  // Set brightness based on DMX data
    brightness = packet->property_values[START_CHANNEL];
    lights.setOutput(brightness, color);
  }
  if (packet->property_values[START_CHANNEL + 1] != raw_color) {
    raw_color = packet->property_values[START_CHANNEL + 1];
    if (raw_color < 128) {  // set white or multicolor based on DMX data
      color = WHITE;
    } else {
      color = MULTICOLOR;
    }
    lights.setOutput(brightness, color);
  }
}

void setup() {
  pinMode(2, OUTPUT);
  digitalWrite(2, LOW);

  pinMode(0, INPUT_PULLUP);
  attachInterrupt(0, isr_reset, FALLING);
  Serial.begin(115200);
  delay(500);  // wait for serial to be ready
  Serial.println("Starting DMX DC dimmer...");
  readEEPROM();
  Serial.println("WIFI_SSID: ");
  Serial.println(WIFI_SSID);
  Serial.println("WIFI_PASSWORD: ");
  Serial.println(WIFI_PASSWORD);

  Serial.println("WIFI Mode (AP or client): ");
  Serial.println(WIFI_MODE);
  Serial.println("Use Static IP: ");
  Serial.println(STATIC_IP);
  Serial.println("WIFI_IP: ");
  Serial.println(WIFI_IP);

  Serial.println("WIFI_SUBNET: ");
  Serial.println(WIFI_SUBNET);

  Serial.println("WIFI_GW: ");
  Serial.println(WIFI_GW);

  Serial.println("WIFI_DNSServer: ");
  Serial.println(WIFI_DNSServer);

  Serial.println("LED Mode: ");
  Serial.println(mode);


  if (WIFI_MODE == 1) {
    if (STATIC_IP == 1)
      if (!WiFi.config(WIFI_IP, WIFI_GW, WIFI_SUBNET, WIFI_DNSServer)) {
        Serial.println("Static IP Failed to configure");
      }
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int i = 0;
    while (WiFi.status() != WL_CONNECTED && i < 60) {
      digitalWrite(2, !digitalRead(2));  //blink LED while connecting.
      delay(500);
      Serial.print(".");
      i++;
    }
    if (i >= 60)
      Serial.println("Failed to connect to Wifi.  Please reset and check SSID and password.");
    else
      digitalWrite(2, HIGH);  //turn on LED so show we're connected.

  } else {
    if (STATIC_IP == 1)
      if (!WiFi.softAPConfig(WIFI_IP, WIFI_GW, WIFI_SUBNET)) {
        Serial.println("Static IP Failed to configure");
      }
    //if (!WiFi.softAP(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str())) {
    if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD)) {
      Serial.printf("Soft AP creation failed.  SSID=%s PW=%s\n", WiFi.softAPSSID(), WIFI_PASSWORD.c_str());
      while (1)
        ;
    }
    digitalWrite(2, HIGH);  //turn on LED so show we're connected.
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("UNIVERSE: ");
  Serial.println(UNIVERSE);

  Serial.println("START_CHANNEL: ");
  Serial.println(START_CHANNEL);

  Serial.println("MUILTICAST: ");
  Serial.println(ISMULTICAST);


  server.begin();      // start webserver
  lights.begin(mode);  // intialize PWM lights interface
  dmx_config_t config = DMX_CONFIG_DEFAULT;

  switch (mode) {
    case DMX:
      dmx_driver_install(DMXport, &config, DMX_INTR_FLAGS_DEFAULT);
      /* Now set the DMX hardware pins to the pins that we want to use and setup
          will be complete! */
      dmx_set_pin(DMXport, DMXtx, DMXrx, DMXrts);
      Serial.printf("Listening on DMX port.  DMX timeout is %i", DMX_TIMEOUT_TICK);
      break;
    case E131:
      color = WHITE;
      raw_color = 0;
      brightness = 0;
      if (ISMULTICAST == 0) {
        if (e131.begin(E131_MULTICAST, UNIVERSE, UNIVERSE_COUNT))  // Listen via Multicast
          Serial.println(F("Listening for DMX E131 data in multicast..."));
        else
          Serial.println(F("*** e131.begin failed ***"));
      } else {
        if (e131.begin(E131_UNICAST))  // Listen via Unicast
          Serial.println(F("Listening for DMX E131 data in unicast..."));
        else
          Serial.println(F("*** e131.begin failed ***"));
      }
      e131.registerCallback(NULL, (*processDMX));  // register the function to handle DMX packets
      break;
    default:
      Serial.println(F("Running local effects..."));
      Serial.printf("Effect: %i\n", mode);
      effect_timer = timerBegin(0, TIMER_SPEED, true);
      timerAttachInterrupt(effect_timer, &runEffectsTimer, true);
      timerAlarmWrite(effect_timer, 1000000, true);
      timerAlarmEnable(effect_timer);
  }


  factory_reset_called = 0;
  its_effect_time = 0;
  Serial.println("Startup is complete");
}



void loop() {
  int reboot_requested = 0;
  if (factory_reset_called == 1) factory_reset();
  if (mode != DMX && mode != E131 && its_effect_time == 1) {
    //Serial.println(lights.getBrightness());
    lights.runEffects();
    its_effect_time = 0;
  }
  if (mode == DMX) {
    dmx_packet_t packet;

    /* And now we wait! The DMX standard defines the amount of time until DMX
    officially times out. That amount of time is converted into ESP32 clock
    ticks using the constant `DMX_TIMEOUT_TICK`. If it takes longer than that
    amount of time to receive data, this if statement will evaluate to false. */
    if (dmx_receive(DMXport, &packet, DMX_TIMEOUT_TICK)) {
      /* If this code gets called, it means we've received DMX data! */

      /* Get the current time since boot in milliseconds so that we can find out
      how long it has been since we last updated data and printed to the Serial
      Monitor. */
      unsigned long now = millis();

      /* We should check to make sure that there weren't any DMX errors. */
      if (!packet.err) {
        /* If this is the first DMX data we've received, lets log it! */
        if (!dmxIsConnected) {
          Serial.println("DMX is connected!");
          dmxIsConnected = true;
        }

        /* Don't forget we need to actually read the DMX data into our buffer so
        that we can print it out. */
        dmx_read(DMXport, data, packet.size);

        /* Print the received start code - it's usually 0. */
        //Serial.printf("Start code is 0x%02X and slot 1 is 0x%02X\n", data[0],data[1]);
        lastUpdate = now;
        if(data[START_CHANNEL+1]<128){
          color=WHITE;
        } else {
          color=MULTICOLOR;
        }
        lights.setOutput(data[START_CHANNEL], color);

      } else {
        /* Oops! A DMX error occurred! Don't worry, this can happen when you first
        connect or disconnect your DMX devices. If you are consistently getting
        DMX errors, then something may have gone wrong with your code or
        something is seriously wrong with your DMX transmitter. */
        Serial.println("A DMX error occurred.");
      }
    } else if (dmxIsConnected) {
      Serial.println("DMX was disconnected.");
    }
  }



  //if(effect_mode==0)
  // lights.runffects(); //run the configured on-board effect.  This is active //if DMX/E131 is disabled.

  // listen for incoming web clients
  WiFiClient client = server.available();
  if (client) {
    Serial.println("Web User Connected.");

    // read the HTTP request header line by line
    String HTTP_req = "";
    while (client.connected()) {
      if (client.available()) {
        HTTP_req = client.readStringUntil('\n');  // read the first line of HTTP request
        break;
      }
    }
    Serial.printf("HTTP_req: %s\n", HTTP_req.c_str());

    // read the remaining lines of HTTP request header
    while (client.connected()) {
      if (client.available()) {
        String HTTP_header = client.readStringUntil('\n');  // read the header line of HTTP request

        if (HTTP_header.equals("\r"))  // the end of HTTP request
          break;
      }
    }

    if (HTTP_req.indexOf("GET") == 0) {  // check if request method is GET
      // expected header is one of the following:
      // - GET door/unlock
      // - GET door/lock
      if (HTTP_req.indexOf("update") > -1 && HTTP_req.indexOf("?") > -1) {  // check the path
        HTTP_req = HTTP_req.substring((HTTP_req.indexOf("?") + 1));
        while (HTTP_req.indexOf("&") > -1) {
          String token = HTTP_req.substring(0, HTTP_req.indexOf("&"));
          HTTP_req = HTTP_req.substring((HTTP_req.indexOf("&") + 1));
          Serial.printf("Token is: %s HTTP_req is %s\n", token.c_str(), HTTP_req.c_str());
          parseToken(token);
        }
        String last_token = HTTP_req.substring(0, HTTP_req.length() - 10);
        Serial.printf("Token is: |%s|\n", last_token.c_str());
        parseToken(last_token);

        Serial.println("UNIVERSE: ");
        Serial.println(UNIVERSE);

        Serial.println("START_CHANNEL: ");
        Serial.println(START_CHANNEL);

        Serial.println("MUILTICAST: ");
        Serial.println(ISMULTICAST);
        Serial.println("WIFI_SSID: ");
        Serial.println(WIFI_SSID);
        Serial.println("WIFI_PASSWORD: ");
        Serial.println(WIFI_PASSWORD);
        writeEEPROM();
        reboot_requested = 1;
      }
    }


    // send the HTTP response
    // send the HTTP response header
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: text/html");
    client.println("Connection: close");  // the connection will be closed after completion of the response
    client.println();                     // the separator between HTTP header and body
    // send the HTTP response body
    client.println("<!DOCTYPE HTML>");
    client.println("<html>");
    client.println("<head>");
    client.println("<link rel=\"icon\" href=\"data:,\">");
    client.println("</head>");
    client.println("<center><h1>GE Staybright DMX E131 Configuration</h1>");

    client.println("<form action=\"update\" method=\"get\"><p>");
    client.println("<br><table border=1><tr><td><b>DMX/E131 Settings</b></td></tr><tr><td>");

    client.print("Universe: <span style=\"color: red;\"><input type=\"text\" name=\"UNIVERSE\" value=\"");
    client.print(UNIVERSE);
    client.println("\"></span><br>");

    client.print("Start Channel: <span style=\"color: red;\"><input type=\"text\" name=\"CHANNEL\" value=\"");
    client.print("1");
    client.println("\"></span><br>");

    if (ISMULTICAST == 0) {
      client.print("Multicast<input type=\"radio\" name=\"MULTICAST\" value=\"MULTICAST\" checked=\"checked\">");
      client.print("Unicast <input type=\"radio\" name=\"MULTICAST\" value=\"UNICAST\"><br>");

    } else {
      client.print("Multicast<input type=\"radio\" name=\"MULTICAST\" value=\"MULTICAST\" >");
      client.print("Unicast <input type=\"radio\" name=\"MULTICAST\" value=\"UNICAST\" checked=\"checked\"><br>");
    }
    client.println("</td></tr></table><br>");

    client.println("<br><table border=1><tr><td><b>Wifi Settings</b></td></tr><tr><td>");


    if (WIFI_MODE == 0) {
      client.print("Wifi AP<input type=\"radio\" name=\"WIFI_MODE\" value=\"0\" checked=\"checked\">");
      client.print("Wifi Client<input type=\"radio\" name=\"WIFI_MODE\" value=\"1\"><br>");

    } else {
      client.print("Wifi AP<input type=\"radio\" name=\"WIFI_MODE\" value=\"0\" >");
      client.print("Wifi Client<input type=\"radio\" name=\"WIFI_MODE\" value=\"1\" checked=\"checked\"><br>");
    }


    client.print("Wifi SSID: <span style=\"color: red;\"><input type=\"text\" name=\"WIFI_SSID\" value=\"");
    client.print(WIFI_SSID);
    client.println("\"></span><br>");

    client.print("Wifi Password: <span style=\"color: red;\"><input type=\"text\" name=\"WIFI_PASSWORD\" value=\"");
    client.print(WIFI_PASSWORD);
    client.println("\"></span><br>");

    if (STATIC_IP == 0) {
      client.print("Use DHCP <input type=\"radio\" name=\"STATIC_IP\" value=\"0\" checked=\"checked\"><br>");
      client.print("Use Static IP <input type=\"radio\" name=\"STATIC_IP\" value=\"1\"><br>");

    } else {
      client.print("Use DHCP <input type=\"radio\" name=\"STATIC_IP\" value=\"0\" ><br>");
      client.print("Use Static IP <input type=\"radio\" name=\"STATIC_IP\" value=\"1\" checked=\"checked\"><br>");
    }


    client.print("Static IP Address: <span style=\"color: red;\"><input type=\"text\" name=\"WIFI_IP\" value=\"");
    client.print(WIFI_IP);
    client.println("\"></span><br>");

    client.print("Subnet Mask: <span style=\"color: red;\"><input type=\"text\" name=\"WIFI_SUBNET\" value=\"");
    client.print(WIFI_SUBNET);
    client.println("\"></span><br>");

    client.print("Gateway: <span style=\"color: red;\"><input type=\"text\" name=\"WIFI_GW\" value=\"");
    client.print(WIFI_GW);
    client.println("\"></span><br>");

    client.print("DNS Server: <span style=\"color: red;\"><input type=\"text\" name=\"WIFI_DNSServer\" value=\"");
    client.print(WIFI_DNSServer);
    client.println("\"></span><br>");
    client.println("</td></tr></table><br>");

    client.println("<br><table border=1><tr><td><b>Effect Mode</b></td></tr><tr><td>");

    if (mode == STEADY_WHITE) {
      client.print("Steady White<input type=\"radio\" name=\"EFFECT\" value=\"0\" checked=\"checked\"><br>");
    } else {
      client.print("Steady White<input type=\"radio\" name=\"EFFECT\" value=\"0\"><br>");
    }
    if (mode == STEADY_MULTICOLOR) {
      client.print("Steady Multicolor<input type=\"radio\" name=\"EFFECT\" value=\"1\" checked=\"checked\"><br>");
    } else {
      client.print("Steady Multicolor<input type=\"radio\" name=\"EFFECT\" value=\"1\"><br>");
    }

    if (mode == SLO_GLO) {
      client.print("Slo Glo<input type=\"radio\" name=\"EFFECT\" value=\"2\" checked=\"checked\"><br>");
    } else {
      client.print("Slo Glo<input type=\"radio\" name=\"EFFECT\" value=\"2\" ><br>");
    }

    if (mode == WHITE_SLO_GLO) {
      client.print("White Slo Glo<input type=\"radio\" name=\"EFFECT\" value=\"3\" checked=\"checked\"><br>");
    } else {
      client.print("White Slo Glo<input type=\"radio\" name=\"EFFECT\" value=\"3\" ><br>");
    }

    if (mode == MULTI_SLO_GLO) {
      client.print("Multicolor Slo Glo<input type=\"radio\" name=\"EFFECT\" value=\"4\" checked=\"checked\"><br>");
    } else {
      client.print("Multicolor Slo Glo<input type=\"radio\" name=\"EFFECT\" value=\"4\"><br>");
    }

    if (mode == WHITE_FLASHING) {
      client.print("White Flashing<input type=\"radio\" name=\"EFFECT\" value=\"5\" checked=\"checked\"><br>");
    } else {
      client.print("White Flashing<input type=\"radio\" name=\"EFFECT\" value=\"5\" ><br>");
    }

    if (mode == MULTI_FLASHING) {
      client.print("Multicolor Flashing<input type=\"radio\" name=\"EFFECT\" value=\"6\" checked=\"checked\"><br>");
    } else {
      client.print("Multicolor Flashing<input type=\"radio\" name=\"EFFECT\" value=\"6\"><br>");
    }

    if (mode == CHANGE_FLASHING) {
      client.print("Chaning Color Flashing<input type=\"radio\" name=\"EFFECT\" value=\"7\" checked=\"checked\"><br>");
    } else {
      client.print("Chaning Color Flashing<input type=\"radio\" name=\"EFFECT\" value=\"7\" ><br>");
    }

    if (mode == DMX) {
      client.print("DMX Controlled<input type=\"radio\" name=\"EFFECT\" value=\"8\" checked=\"checked\"><br>");
    } else {
      client.print("DMX Controlled<input type=\"radio\" name=\"EFFECT\" value=\"8\"><br>");
    }

    if (mode == E131) {
      client.print("E131 Controlled<input type=\"radio\" name=\"EFFECT\" value=\"9\" checked=\"checked\"><br>");
    } else {
      client.print("E131 Controlled<input type=\"radio\" name=\"EFFECT\" value=\"9\"><br>");
    }




    client.println("</td></tr></table><br><input type=\"submit\" value=\"Update\"></p></form></center>");
    client.println("</html>");
    client.flush();


    // give the web browser time to receive the data
    delay(100);

    // close the connection:
    client.stop();
  }

  if (reboot_requested == 1) {
    delay(1000);    // give any web clients enough time to close their session
    ESP.restart();  //make settings changes effective
  }
}