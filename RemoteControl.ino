/* RemoteControl
   Home automation sketch.

   Hardware:
   * A Freetronics Etherten. But any other Arduino-compatible with
   Ethernet and an SD card reader should work.
   * Relays connected to pins 8 and 9. These control some 3-wire
   ropelights.
   * A shift register connected to pins 5, 6 and 7. This in turn
   drives the 4 pairs of buttons on a 433MHz remote power control.
   I'm using a Freetronics Expand module, with the outputs wired as:
     A: 4off
     B: 4on
     C: 3off
     D: 3on
     E: 2off
     F: 2on
     G: 1off
     H: 1on

   Dependencies:
   * Webduino library. ( http://github.com/sirleech/Webduino/ )
*/

#include <SPI.h>
#include <Ethernet.h>
#include <WebServer.h>
#include <SD.h>
#include <stdlib.h>

// IO pins
// Select pin for the SD card
const int SDchipSelect = 4;
// Pins for the expand module
const int xpDataPin = 7;
const int xpLatchPin = 6;
const int xpClockPin = 5;
// Pins for the relays
const int relayAPin = 8;
const int relayBPin = 9;

// Ethernet constants
static uint8_t mac[] = { 0x06, 0x17, 0x17, 0x17, 0x17, 0x17 };
static uint8_t ip[] = { 10, 17, 17, 5 };
WebServer webserver ("", 80);

// HTTP variables
char credentials[] = "YXJuaWU6Z2V0dG9kYWNob3BwYQ==";
const int NAMELEN = 8;
const int VALUELEN = 8;

// External lights
int externLightState = LOW;
// Power points
bool powerSwitches[] = { false, false, false, false };
// Values to write to turn power points on
int powerOnVals[] = { 128, 32, 8, 2 };
// Values to write to turn power points off
int powerOffVals[] = { 64, 16, 4, 1 };

// Timer variables
bool timerActive = false;
long timerTime;

boolean authorise(WebServer &server) {
  if (server.checkCredentials(credentials)) {
    server.httpSuccess();
    return true;
  } else {
    server.httpUnauthorized();
    return false;
  }
}

void sendFile(WebServer &server, char *page) {
  if (SD.exists(page)) {
    File fd = SD.open(page);
    while (fd.available()) {
      server.print(char(fd.read()));
    }
  }
}

void statusCmd(WebServer &server, WebServer::ConnectionType type,
	       char *, bool) {
  if (authorise(server)) {
    server.print("{\"extern\":");
    if (externLightState) {
      server.print("1");
    } else {
      server.print("0");
    }
    // Beware hard coded output size
    for (int i = 1; i < 5; i++) {
      server.print(",\"outlet");
      server.print(i);
      server.print("\":");
      if (powerSwitches[i-1]) {
    	server.print("1");
      } else {
    	server.print("0");
      }
    }
    server.println("}");
  }
}

void defaultPage(WebServer &server, WebServer::ConnectionType type,
		 char *, bool) {
  if (authorise(server)) {
    sendFile(server, "web/index.htm");
  }
}
 
void cmdParser(WebServer &server, WebServer::ConnectionType type,
	       char *url_tail, bool tail_complete) {
  if (authorise(server)) {
    if (type != WebServer::GET) {
      return;
    }

    URLPARAM_RESULT rc;
    char name[NAMELEN];
    char value[VALUELEN];
    char ele[NAMELEN];
    int eleid;
    bool cmd;

    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc != URLPARAM_EOS) {
	if (strcmp(name, "ele") == 0) {
	  strcpy(value, ele);
	} else if (strcmp(name, "eleid") == 0) {
	  eleid = atoi(value);
	} else if (strcmp(name, "cmd") == 0) {
	  if (strcmp(value, "on") == 0) {
	    cmd=true;
	  } else {
	    cmd=false;
	  }
	} else if (strcmp(name, "timer") == 0) {
	  long timeDelay = atol(value);
	  timerActive = true;
	  timerTime = millis() + timeDelay*1000;
	}
      }
    }

    if (eleid == 0) {
      if (cmd) {
	externlights(HIGH);
      } else {
	externlights(LOW);
      }
    } else {
      powerswitch(eleid, cmd);
    }
  }
}

void externlights(int state) {
  digitalWrite(relayAPin, state);
  digitalWrite(relayBPin, state);
  externLightState = state;
}

void powerswitch(int outlet, bool state) {
  int cmd;
  if (state) {
    cmd = powerOnVals[outlet-1];
  } else {
    cmd = powerOffVals[outlet-1];
  }

  timerActive = true;
  timerTime = millis() + 500;
  powerSwitches[outlet-1] = state;
  sendShiftCmd(cmd);
}

void sendShiftCmd(int cmd) {
  digitalWrite(xpLatchPin, LOW);
  shiftOut(xpDataPin, xpClockPin, MSBFIRST, cmd);
  digitalWrite(xpLatchPin, HIGH);
}

void updateTimer() {
  long curTime = millis();
  if (curTime > timerTime) {
    timerActive = false;
    externlights(LOW);
    sendShiftCmd(0);
  }
}

void setup() {
  pinMode(SDchipSelect, OUTPUT);
  pinMode(xpDataPin, OUTPUT);
  pinMode(xpLatchPin, OUTPUT);
  pinMode(xpClockPin, OUTPUT);
  pinMode(relayAPin, OUTPUT);
  pinMode(relayBPin, OUTPUT);

  // Flush the expand module
  sendShiftCmd(0);

  // Initialise the SD card
  SD.begin(SDchipSelect);

  // Initialise the Ethernet adapter
  Ethernet.begin(mac, ip);

  // Initialise the web server
  webserver.setDefaultCommand(&defaultPage);
  webserver.addCommand("cmd", &cmdParser);
  webserver.addCommand("status.json", &statusCmd);
  webserver.begin();
}

void loop() {
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);
  if (timerActive) {
    updateTimer();
  }
}
