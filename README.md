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
  powerOnVals and powerOffVals in the sketch. See below.
  
## Dependencies

* [Webduino](http://github.com/sirleech/Webduino/) web serving library.

## Authentication

This sketch uses the HTTP basic auth feature of the Webduino library,
and currently the credentials are hard-coded. You'll need to edit the
sketch to include your own before use.

Credentials are Base64 encoded, and should be the username and password,
separated by a colon, eg "admin:pass123". Base64-encode your credentials
(online encoders are easy to find), and paste it in to the credentials[]
variable in the sketch.

## Shift register outputs

The way I've connected my register outputs to the outlet controls could
be considered a little idiosyncratic. If you wire yours differently you'll
need to update the powerOnVals and powerOffVals arrays.

Each power outlet on the remote is represented by its matching index in
each array; the first element of powerOnVals switches outlet 1 on, the
second element outlet 2, and so on. The integers in the array are the
values that get written to the register to turn on a single pin. So to
use the pin A of the register to turn on outlet 1, powerOnVals[0] = 1.

## Hacking the HTML

Due to size constraints in my Etherten's firmware, I'm not able to
include the SD library to read files from an SD card. This means that
as well as hard-coding the HTTP credentials, all of the HTML is also
inlined in the sketch. To change the HTML, edit html/index.htm, and
use prep.sh to massage it in to a form that can be easily clagged in
to the P(index_htm) macro.
