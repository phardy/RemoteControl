/* RemoteControl
   Home automation sketch.

   Refer to https://github.com/phardy/RemoteControl
   for details and usage.
*/

#include <SPI.h>
#include <Ethernet.h>

#include <OneButton.h>
#include <Timer.h>
#include <WebServer.h>

// IO pins
// Pins for the expand module
const int xpDataPin = 7;
const int xpLatchPin = 6;
const int xpClockPin = 5;
// Pins for the relays
const int relayAPin = 8;
const int relayBPin = 9;

// Set up the button. I'm connecting pin to +5V when button pressed.
// I *think* that means the active flag needs to be false.
OneButton lightButton(3, false);

// Ethernet constants
static uint8_t mac[] = { 0x06, 0x17, 0x17, 0x17, 0x17, 0x17 };
static uint8_t ip[] = { 10, 17, 17, 5 };
WebServer webserver ("", 80);

// HTTP variables
char credentials[] = "YXJuaWU6Z2V0dG9kYWNob3BwYQ==";
const int attribLen = 8; // Buffer length for URL attributes.
const int valueLen = 8; // Buffer length for URL values.

int externLightState = LOW; // State of external lights
bool powerSwitches[] = { false, false, false, false }; // State of power switches
// Values to write to shift register to turn power points on
int powerOnVals[] = { 128, 32, 8, 2 }; 
// Values to write to shift register to turn power points off
int powerOffVals[] = { 64, 16, 4, 1 };

// Timer variables
Timer timer;

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
  "$.each(json, function(key, value) {"
  "$(\"#\"+key).val(value).slider(\"refresh\");"
  "});"
  "});"
  "}"
  "$('select').bind('change', function(event) {"
  "element = event.target.id;"
  "if (element.substr(0, 6) == \"outlet\") {"
  "eleid = element.substr(6, 1);"
  "} else {"
  "eleid = 0;"
  "}"
  "command = event.target.value;"
  "$.get('/cmd', { 'eleid' : eleid, 'cmd' : command });"
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
    char name[attribLen];
    char value[valueLen];
    int eleid;
    bool cmd;

    while (strlen(url_tail)) {
      rc = server.nextURLparam(&url_tail, name, attribLen, value, valueLen);
      if (rc != URLPARAM_EOS) {
	if (strcmp(name, "eleid") == 0) {
	  eleid = atoi(value);
	} else if (strcmp(name, "cmd") == 0) {
	  if (strcmp(value, "on") == 0) {
	    cmd=true;
	  } else {
	    cmd=false;
	  }
	} else if (strcmp(name, "timer") == 0) {
	  long timeDelay = atol(value) * 1000;
	  timer.after(timeDelay, externTimerCallBack);
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

// Timer callback that will
// turn off extern outputs.
void externTimerCallBack() {
  externlights(LOW);
}

void externlights(int state) {
  digitalWrite(relayAPin, state);
  digitalWrite(relayBPin, state);
  externLightState = state;
}

// Timer callback that will send 0 to shift register,
// "releasing" the button we pushed.
void powerTimerCallBack() {
  sendShiftCmd(0);
}

void powerswitch(int outlet, bool state) {
  int cmd;
  if (state) {
    cmd = powerOnVals[outlet-1];
  } else {
    cmd = powerOffVals[outlet-1];
  }

  // Timer objects can have a maximum of 10 events
  // attached. Just going to assume nobody mashes
  // the button. *stare*
  timer.after(500, powerTimerCallBack);

  powerSwitches[outlet-1] = state;
  sendShiftCmd(cmd);
}

void sendShiftCmd(int cmd) {
  digitalWrite(xpLatchPin, LOW);
  shiftOut(xpDataPin, xpClockPin, MSBFIRST, cmd);
  digitalWrite(xpLatchPin, HIGH);
}

// On single click, turn the lights on
// with a five-minute timer.
void singleClickCallback() {
  externlights(HIGH);
  timer.after(300000, externTimerCallBack);
}

// On double click, turn the lights on
// indefinitely.
void doubleClickCallback() {
  externlights(HIGH);
}

void setup() {
  pinMode(xpDataPin, OUTPUT);
  pinMode(xpLatchPin, OUTPUT);
  pinMode(xpClockPin, OUTPUT);
  pinMode(relayAPin, OUTPUT);
  pinMode(relayBPin, OUTPUT);

  // Flush the expand module
  sendShiftCmd(0);

  // Set up button callbacks
  lightButton.attachClick(&singleClickCallback);
  lightButton.attachDoubleClick(&doubleClickCallback);

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
  timer.update();
  lightButton.tick();
}
