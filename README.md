# RemoteControl

An Arduino sketch for remote home automation.

## Hardware

* An Ethernet shield, or an Arduino with onboard Ethernet. I'm using
  a Freetronics Etherten.
* Relays connected to pins 8 and 9. These drive external lighting;
  I've got mine switching some LED ropelight.
* A shift register on pins 5, 6 and 7. This in turn drives the 4
  pairs of buttons on a [433MHz remote power control](http://hardy.dropbear.id.au/blog/2012/08/jackson-pt9723-remote-power-control-and-arduino).
  I'm using a Freetronics Expand module, with the outputs wired as:
  * A: 4off
  * B: 4on
  * C: 3off
  * D: 3on
  * E: 2off
  * F: 2on
  * G: 1off
  * H: 1on

  If you connect your switches differently, you'll need to edit
  `powerOnVals` and `powerOffVals` in the sketch. See below.
* A momentary push button connected to pin 3, that will drive the pin high
  when pressed. This is a manual switch for the external lights. I used
  a funky 12V illuminated button, in series with one of the relays, to
  give some feedback for when the lights are on.

## Dependencies

* [Webduino](http://github.com/sirleech/Webduino/) web serving library.
* [Timer](https://github.com/JChristensen/Timer/) timing library.
* [OneButton](http://www.mathertel.de/Arduino/OneButtonLibrary.aspx)
  button management library.

## Setup

There's a couple of things you need to be aware of before attempting to
use this sketch.

### Authentication

This sketch uses the HTTP basic auth feature of the Webduino library,
and currently the credentials are hard-coded. You'll need to edit the
sketch to include your own before use.

Credentials are Base64 encoded, and should be the username and password,
separated by a colon, eg "`admin:pass123`". Base64-encode your credentials
(online encoders are easy to find), and paste it in to the credentials[]
variable in the sketch.

### Network configuration

The Ethernet library in Arduino 1.0 and up can handle DHCP, but to
keep the sketch size down I'm not using it. Set the ip[] variable to
an unused IP address on your local network before uploading the sketch.

## Usage

The sketch implements a web server. Load it up, connect your arduino
and browse to it. After authenticating, `http://your.arduino.ip/` will
give you a jQuery Mobile page, with toggle switches indicating the
current status of the outputs, and letting you switch them on and off.

`/status.json` returns the status of all outputs in JSON format.

`/cmd` is used to send commands. Currently, only GET requests are
accepted, with the following parameters. `eleid` is required, as well
as either `timer` or `cmd`.

* `eleid`: An integer ID representing the outlet to change:
  * 0 controls both relays.
  * 1-4 controls a single outlet via the shift register.
* `cmd`: Either "on" or "off".
* `timer` (optional): A `long`, representing seconds until timer expires.
  Activates a timer that will turn off the given `eleid`. The `eleid` will
  be turned on if it currently off.
  There's currently no way to disable a timer after it's been activated.

Successful calls to /cmd will return the current status in JSON format.

Eg:
* `GET /cmd?eleid=3&cmd=off` will turn off outlet 3.
* `GET /cmd?eleid=0&cmd=on&timer=300` will turn on the relay pins, with
them automatically turning off after 5 minutes.

### Button presses

Pressing the pushbutton once will turn the external lights on with a
five minute timer. A quick double-press turns the external lights on
permanently. Pressing the button at any time while the lights are on
will turn them off.

## Hacking

### Shift register outputs

The way I've connected my register outputs to the outlet controls could
be considered a little idiosyncratic. If you wire yours differently you'll
need to update the `powerOnVals` and `powerOffVals` arrays.

Each power outlet on the remote is represented by its matching index in
each array; the first element of powerOnVals switches outlet 1 on, the
second element outlet 2, and so on. The integers in the array are the
values that get written to the register to turn on a single pin. So to
use the pin A of the register to turn on outlet 1, `powerOnVals[0] = 1`.

### HTML

Due to size constraints in my Etherten's firmware, I'm not able to
include the SD library to read files from an SD card. This means that
as well as hard-coding the HTTP credentials, all of the HTML is also
inlined in the sketch. To change the HTML, edit `html/index.htm`, and
use prep.sh to massage it in to a form that can be easily clagged in
to the `P(index_htm)` macro.
