[![Build Status](https://travis-ci.org/kylegordon/homie-sonoff-touch.svg?branch=master)](https://travis-ci.org/kylegordon/homie-sonoff-touch)

# Multiclick Sonoff Touch

Multiclick support for the Sonoff Touch switches. This will allow you to have up to 6 different actions from a single touch switch.

A simple Homie based Arduino sketch for the Sonoff Touch switch. This uses the ESP8285 platform instead of the ESP8266, so be sure to set the board correctly. platoformio.ini is configured appropriately.

This firmware decouples the relay from the switch, and allows it to be controlled separately, through MQTT messages.

The topic structure is based on what you set through the Homie configuration, with the following subtopics.

* relaystate/set/ON|OFF
* button/event

The messages emitted from the button/event node are...

* SINGLE
* DOUBLE
* TRIPLE
* SINGLEHELD
* DOUBLEHELD
* TRIPLEHELD

All of the HELD variants will repeat endlessly until the button is released.

Uses the [Homie](https://github.com/marvinroger/homie-esp8266/releases) framework, so you don't need to worry about wireless connectivity, wireless configuration persistence, and all that. Simply compile and upload, and configure using the Homie configuration tool.
All future flashes will not overwrite the wireless configuration.

Best used with PlatformIO. Simply git clone, pio run -t upload, watch the dependencies download and compile, and then if required do the initial Homie configuration with the tool for Homie #develop branch at http://marvinroger.github.io/homie-esp8266/

To reset the device back to Homie defaults, press the touchpad for over 10 seconds. This will wipe the configuration and drop it into config AP mode.

## Features:

This is heavily based on the code at https://github.com/enc-X/sonoff-homie

* ON/OFF relay
* Configurable default (on boot) relay state - (MQTT independent)
* Local button on/off
* keepalive feature - device will reboot if not receive keepalive message in given time
* All Homie built in features (OTA, configuration, reporting, etc)

## MQTT messages

<table>
<tr>
  <th>Property</th>
  <th>Message format</th>
  <th>Direction</th>
  <th>Description</th>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/relay01/relayState</td>
  <td><code>(ON|OFF)</code></td>
  <td>Device → Controller</td>
  <td>Current state of relay</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/relay01/relayState/set</td>
  <td><code>(ON|OFF)</code></td>
  <td>Controller → Device</td>
  <td>Change relay state</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/relay01/relayInitMode</td>
  <td><code>(0|1)</code></td>
  <td>Device → Controller</td>
  <td>Current boot mode <code>1</code> - relay ON, <code>0</code> - relay OFF</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/relay01/relayInitMode/set</td>
  <td><code>(0|1)</code></td>
  <td>Controller → Device</td>
  <td>Set Boot mode <code>1</code> - relay ON, <code>0</code> - relay OFF</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/keepalive/keepAliveValue</td>
  <td><code>\d+</code></td>
  <td>Device → Controller</td>
  <td>Report of keepalive value (seconds), 0 - keep alive feature is not active</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/keepalive/keepAliveValue/set</td>
  <td><code>\d+</code></td>
  <td>Controller → Device</td>
  <td>Set keepalive time in seconds. Reboot if no ticks received in this time. 0 - keep alive feature is not active</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/keepalive/tick/set</td>
  <td><code>.*</code></td>
  <td>Controller → Device</td>
  <td>Keepalive message from controller to gateway - if device will not receive during keepAliveValue time slot, it will reboot, keepalive is not active when keepAliveValue is equal 0</td>
</tr>
<tr>
  <td>_HOMIE_PREFIX_/_node-id_/$online</td>
  <td><code>(true|false)</code></td>
  <td>Device → Controller</td>
  <td><code>/true</code> when the device is online, <code>false</code> when the device is offline (through LWT)</td>
</tr>
</table>

# Updating

To use Homie built in OTA updater...
./ota_updater.py -l 172.24.32.13 -t devices/ -i 60019485376d /home/kyleg/Projects/homie-sonoff-touch/.pioenvs/esp8285/firmware.bin

# Renaming

To change the config, eg, device name, in flight...
Use the 'devices/60019485376d/$implementation/config/set topic, such as...
mosquitto_pub -h homeauto.vpn.glasgownet.com -t 'devices/60019485376d/$implementation/config/set' -m '{"wifi":{"ssid":"Glasgownet"},"mqtt":{"host":"172.24.32.13","port":1883,"base_topic":"devices/","auth":false},"name":"bedroom-wall-switch","ota":{"enabled":true}}'

# Watchdog feature

Very occasionally, the Homie stack will fail to reconnect after the resumption of wireless connectivity. The watchdog uses most of the same code from https://github.com/enc-X/sonoff-homie and simply needs to be 'set' and 'ticked' periodically.

To set it, publish to the ```devices/$device/keepalive/timeOut/set``` topic To provide a tick, publish to the ```device/keepalive/tick/set``` topic.
To disable the watchdog feature, publish 0 to ```device/keepalive/timeOut/set``` and ```device/keepalive/tick/set```

Firstly this sets the timeout period before a reboot, and the second publish causes the watchdog timer to reset. If the tick publish is not received, the device will restart.

The following script will cycle through a list of devices on the broker, and set their timeout to be 5 minutes and 10 seconds. The script can then run every 5 minutes via crontab.

```for device in 5ccf7faf5486 5ccf7faf630b 60019416854d 5ccf7fe9b845
60019485376d 60019485412a 6001949b9222 6001948e0f19 6001948e0f70
do
    mosquitto_pub -h homeauto.vpn.glasgownet.com -t devices/$device/keepalive/timeOut/set -m '610'
    mosquitto_pub -h homeauto.vpn.glasgownet.com -t devices/$device/keepalive/tick/set -m 'tick'
done
```
