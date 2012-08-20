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

// IO pins
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
bool powerActive = false;
long powerTime;

// Yes, I put a web page here.
P(index_htm) = "<!DOCTYPE html>"
  "<html>"
  "<head>"
  "<title>Remote control</title>"
  "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
  "<link rel=\"stylesheet\" href=\"http://code.jquery.com/mobile/1.1.1/jquery.mobile-1.1.1.min.css\" />"
  "<script src=\"http://code.jquery.com/jquery-1.7.1.min.js\"></script>"
  "<script src=\"http://code.jquery.com/mobile/1.1.1/jquery.mobile-1.1.1.min.js\"></script>"
  "</head>"
  "<body>"
  "<div data-role=\"page\" id=\"page1\" data-theme=\"a\">"
  "<div data-role=\"header\">"
  "<h3>Remote Control</h3>"
  "</div>"
  "<div data-role=\"content\" style=\"padding: 15px\">"
  "<div class=\"ui-grid-b\">"
  "<div class=\"ui-block-a\">"
  "</div>"
  "<div class=\"ui-block-b\">"
  "<div data-role=\"fieldcontain\">"
  "<fieldset data-role=\"controlgroup\">"
  "<label for=\"extern\">Entry lights</label>"
  "<select name=\"extern\" id=\"extern\" data-theme=\"\" data-role=\"slider\">"
  "<option value=\"off\">Off</option>"
  "<option value=\"on\">On</option>"
  "</select>"
  "</fieldset>"
  "</div>"
  "</div>"
  "<div class=\"ui-block-c\"></div>"
  "</div>"
  "<h3>Power switches</h3>"
  "<div class=\"ui-grid-a\">"
  "<div class=\"ui-block-a\">"
  "<div data-role=\"fieldcontain\">"
  "<fieldset data-role=\"controlgroup\">"
  "<label for=\"outlet1\">Outlet 1</label>"
  "<select name=\"outlet1\" id=\"outlet1\" data-theme=\"\" data-role=\"slider\">"
  "<option value=\"off\">Off</option>"
  "<option value=\"on\">On</option>"
  "</select>"
  "</fieldset>"
  "</div>"
  "</div>"
  "<div class=\"ui-block-b\">"
  "<div data-role=\"fieldcontain\">"
  "<fieldset data-role=\"controlgroup\">"
  "<label for=\"outlet3\">Outlet 3</label>"
  "<select name=\"outlet3\" id=\"outlet3\" data-theme=\"\" data-role=\"slider\">"
  "<option value=\"off\">Off</option>"
  "<option value=\"on\">On</option>"
  "</select>"
  "</fieldset>"
  "</div>"
  "</div>"
  "<div class=\"ui-block-a\">"
  "<div data-role=\"fieldcontain\">"
  "<fieldset data-role=\"controlgroup\">"
  "<label for=\"outlet2\">Outlet 2</label>"
  "<select name=\"outlet2\" id=\"outlet2\" data-theme=\"\" data-role=\"slider\">"
  "<option value=\"off\">Off</option>"
  "<option value=\"on\">On</option>"
  "</select>"
  "</fieldset>"
  "</div>"
  "</div>"
  "<div class=\"ui-block-b\">"
  "<div data-role=\"fieldcontain\">"
  "<fieldset data-role=\"controlgroup\">"
  "<label for=\"outlet4\">Outlet 4</label>"
  "<select name=\"outlet4\" id=\"outlet4\" data-theme=\"\" data-role=\"slider\">"
  "<option value=\"off\">Off</option>"
  "<option value=\"on\">On</option>"
  "</select>"
  "</fieldset>"
  "</div>"
  "</div>"
  "</div>"
  "</div>"
  "</div>"
  "<script>"
  "function updateControls() {"
  "$.getJSON(\"/status.json\", function(json) {"
  "if (json.hasOwnProperty(\"extern\")) {"
  "$(\"#extern\").val(json.extern).slider(\"refresh\");"
  "}"
  "if (json.hasOwnProperty(\"outlet1\")) {"
  "$(\"#outlet1\").val(json.outlet1).slider(\"refresh\");"
  "}"
  "if (json.hasOwnProperty(\"outlet2\")) {"
  "$(\"#outlet2\").val(json.outlet2).slider(\"refresh\");"
  "}"
  "if (json.hasOwnProperty(\"outlet3\")) {"
  "$(\"#outlet3\").val(json.outlet3).slider(\"refresh\");"
  "}"
  "if (json.hasOwnProperty(\"outlet4\")) {"
  "$(\"#outlet4\").val(json.outlet4).slider(\"refresh\");"
  "}"
  "});"
  "}"
  "$('select').bind('change', function(event) {"
  "element = event.target.id;"
  "if (element.substr(0, 6) == \"outlet\") {"
  "eleid = element.substr(6, 1);"
  "element = \"outlet\";"
  "} else {"
  "eleid = 0;"
  "}"
  "command = event.target.value;"
  "$.get('/cmd', { 'ele' : element, 'eleid' : eleid, 'cmd' : command });"
  "});"
  "$('#page1').bind('pageinit', updateControls);"
  "</script>"
  "</body>"
  "</html>";

boolean authorise(WebServer &server) {
  if (server.checkCredentials(credentials)) {
    server.httpSuccess();
    return true;
  } else {
    server.httpUnauthorized();
    return false;
  }
}

void sendStatus(WebServer &server) {
  server.print("{ \"extern\" : ");
  if (externLightState) {
    server.print("\"on\"");
  } else {
    server.print("\"off\"");
  }
  // Beware hard coded output size
  for (int i = 1; i < 5; i++) {
    server.print(", \"outlet");
    server.print(i);
    server.print("\" : ");
    if (powerSwitches[i-1]) {
      server.print("\"on\"");
    } else {
      server.print("\"off\"");
    }
  }
  server.println(" }");
}

void statusCmd(WebServer &server, WebServer::ConnectionType type,
	       char *, bool) {
  if (authorise(server)) {
    sendStatus(server);
  }
}

void defaultPage(WebServer &server, WebServer::ConnectionType type,
		 char *, bool) {
  if (authorise(server)) {
    server.printP(index_htm);
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
  sendStatus(server);
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

  powerActive = true;
  powerTime = millis() + 500;
  powerSwitches[outlet-1] = state;
  sendShiftCmd(cmd);
}

void sendShiftCmd(int cmd) {
  digitalWrite(xpLatchPin, LOW);
  shiftOut(xpDataPin, xpClockPin, MSBFIRST, cmd);
  digitalWrite(xpLatchPin, HIGH);
}

void updateTimers() {
  long curTime = millis();
  if (curTime > timerTime) {
    timerActive = false;
    externlights(LOW);
  }
  if (curTime > powerTime) {
    powerActive = false;
    sendShiftCmd(0);
  }
}

void setup() {
  pinMode(xpDataPin, OUTPUT);
  pinMode(xpLatchPin, OUTPUT);
  pinMode(xpClockPin, OUTPUT);
  pinMode(relayAPin, OUTPUT);
  pinMode(relayBPin, OUTPUT);

  // Flush the expand module
  sendShiftCmd(0);

  // Initialise the Ethernet adapter
  Ethernet.begin(mac, ip);

  // Initialise the web server
  webserver.setDefaultCommand(&defaultPage);
  webserver.addCommand("index.html", &defaultPage);
  webserver.addCommand("cmd", &cmdParser);
  webserver.addCommand("status.json", &statusCmd);
  webserver.begin();
}

void loop() {
  char buff[64];
  int len = 64;
  webserver.processConnection(buff, &len);
  if (timerActive || powerActive) {
    updateTimers();
  }
}
