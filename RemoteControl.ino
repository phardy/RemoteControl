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
bool extlights = false;
// Power points
bool powerswitches[] = { false, false, false, false };
char* switchnames[] = { "outlet1", "outlet2", "outlet3", "outlet4" };

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
  } else {
    // erm, dunno
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
    char cmd[VALUELEN];
    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, NAMELEN, value, VALUELEN);
      if (rc != URLPARAM_EOS) {
	if (strcmp(name, "ele") == 0) {
	  
	} else if (strcmp(name, "cmd") == 0) {
	  // do nothing
	}
      }
    }
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
  digitalWrite(xpLatchPin, LOW);
  shiftOut(xpDataPin, xpClockPin, MSBFIRST, 0);
  digitalWrite(xpLatchPin, HIGH);

  // Initialise the SD card
  SD.begin(SDchipSelect);

  // Initialise the Ethernet adapter
  Ethernet.begin(mac, ip);

  // Initialise the web server
  webserver.setDefaultCommand(&defaultPage);
  webserver.addCommand("cmd", &cmdParser);
  webserver.begin();
}

void loop() {
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);
}
