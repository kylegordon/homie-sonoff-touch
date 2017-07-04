[![Build Status](https://travis-ci.org/kylegordon/homie-sonoff-touch.svg?branch=master)](https://travis-ci.org/kylegordon/homie-sonoff-touch)

# homie-sonoff-touch

A simple Homie based Arduino sketch for the Sonoff Touch switch. This uses the ESP8285 platform instead of the ESP8266, so be sure to set the board correctly. platoformio.ini is configured appropriately.

Uses the [Homie](https://github.com/marvinroger/homie-esp8266/releases) framework, so you don't need to worry about wireless connectivity, wireless configuration persistence, and all that. Simply compile and upload, and configure using the Homie configuration tool.
All future flashes will not overwrite the wireless configuration.

Best used with PlatformIO. Simply git clone, edit ROM IDs as appropriate, pio run -t upload, watch the dependencies download and compile, and then if required do the initial Homie configuration with the tool for Homie 1.5 at http://marvinroger.github.io/homie-esp8266/
