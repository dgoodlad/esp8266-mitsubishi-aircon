# ESP8266 / Mitsubishi AC interface

Software to control and monitor Mitsubishi air conditioning units over MQTT
using an ESP8266-based wifi module.

Designed with the Adafruit HUZZAH breakout board in mind, this repo also
includes a PCB layout to do the requisite logic-level shifting etc.

**NOTE**: this information comes with _no warranty of any kind_. I do not claim
that any of the software or designs in this repository will work for your
particular hardware or situation. **MODIFYING AIR CONDITIONING UNITS IS
DANGEROUS AND YOU DO SO AT YOUR OWN RISK.***

## Functional description

* Uses [WiFiManager](https://github.com/tzapu/WiFiManager/tree/development) for
  configuration via a captive portal access point
* GPIO12 acts as a "factory reset" button. When held low for 3 seconds, clears
  all settings configured by WiFiManager
* Uses the [HeatPump](https://github.com/SwiCago/HeatPump) library to handle
  comms with the aircon unit
* Uses [Async MQTT Client](https://github.com/marvinroger/async-mqtt-client) to
  communicate with your MQTT server. The messages are designed to line up with
  the expectations of the [Home Assistant](https://www.home-assistant.io/) [MQTT
  HVAC component](https://www.home-assistant.io/components/climate.mqtt/).
* Detects the presence of the aircon unit (rather than USB/FTDI/debug power) by
  looking for a high signal on GPIO4. The PCB implements this using a voltage
  divider to step the 5V power supply down to 3V3.
* If an aircon unit is detected:
    * Calls `Serial.swap()` to make UART0 use GPIO13/15 as RX/TX for the aircon
      serial communication. This avoids connecting the aircon to the default
      RX/TX pins that dump some debug/boot-time information from the ESP8266
      chip
    * Uses `Serial1` (UART1) to output debug logs on GPIO2 at 115200/8N1
* If _no_ aircon unit is detected
    * The heatpump code does not start, and no call to `Serial.swap()` is made.
    * Uses `Serial` (UART0) to output debug logs on GPIO1 (TX) at 115200/8N1
    * `Serial1` (UART1) is *not used*

## PCB

![PCB Schematic](docs/images/pcb-schematic.png?raw=true)
![PCB 3D Render](docs/images/pcb-render.png?raw=true)
![PCB Layout (top-side)](docs/images/pcb-layout-top.png?raw=true)
![PCB Layout (bottom-side)](docs/images/pcb-layout-bottom.png?raw=true)

I used [OSH Park](https://oshpark.com/) to fabricate these PCBs, it cost about
10 USD with free shipping to :flag-au: Australia for three boards.

Note that the HUZZAH is **revA** with _11 pin connectors_, not the newer 10-pin
version. The 11-pin version is just what I had a few of lying around, and they
have an extra no-connection (NC) pin between the power and GPIO pins on each
row. The order of the pins is _exactly the same_ and it would be straightforward
to modify the board layout to use the newer 10-pin version instead.

### Bill of Materials

You'll need the following to build the PCB:

| Reference(s) | Value       | Description                                    | Datasheet                                                           |
| ------------ | -----       | -----------                                    | ---------                                                           |
| A1           | HUZZAH      | Adafruit HUZZAH (revA) breakout                | [link](https://learn.adafruit.com/adafruit-huzzah-esp8266-breakout) |
| C1, C2       | 0.1uF       | SMD 0603 Tantalum Capacitor                    | [link]()                                                            |
| J1           | AIRCON      | JST PH 4-pin Male Connector                    | [link](http://www.jst-mfg.com/product/pdf/eng/ePH.pdf)              |
| J2           | DEBUG       | 2-pin 0.1" pin header (male)                   | [link]()                                                            |
| R1           | 3K9         | SMD 0603 Resistor                              | [link]()                                                            |
| R2           | 6K8         | SMD 0603 Resistor                              | [link]()                                                            |
| R3, R4       | 1K          | SMD 0603 Resistor                              | [link]()                                                            |
| SW1          | KMR2        | SMD tactile push-button switch                 | [link](https://www.ckswitches.com/media/1479/kmr2.pdf)              |
| U1           | TXB0102DCUT | 2-channel logic level shifter, VSSOP-8 package | [link](http://www.ti.com/lit/ds/symlink/txb0102.pdf)                |
