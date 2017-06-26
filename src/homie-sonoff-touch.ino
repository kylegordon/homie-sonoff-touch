/* WARNING: untested */

#include <Homie.h>

#define FW_NAME "homie-sonoff-touch"
#define FW_VERSION "0.0.5"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

// LED_PIN controls the LED under the wifi symbol on the front plate.
// RELAY_PIN controls the relay, *AND* the LED under the touchpad.
// Scheme is similar to https://github.com/enc-X/sonoff-homie
// ie, mosquitto_pub -h homeauto.vpn.glasgownet.com -t 'devices/85376d51/relay/relayState/set' -m 'ON'

const int PIN_RELAY = 12;
const int PIN_LED = 13;
const int PIN_BUTTON = 0;

Bounce debouncer = Bounce(); // Bounce is built into Homie, so you can use it without including it first
int lastbuttonState = -1;

HomieNode relayNode("relay", "relay");
HomieNode buttonNode("button", "button");

bool RelayHandler(String value) {
  if (value == "ON") {
    digitalWrite(PIN_RELAY, HIGH);
    Homie.setNodeProperty(relayNode, "set", value);
    Serial.println("Relay is on");
  } else if (value == "OFF") {
    digitalWrite(PIN_RELAY, LOW);
    Homie.setNodeProperty(relayNode, "set", value);
    Serial.println("Relay is off");
  } else {
    Serial.print("Unknown value: ");
    Serial.println(value);
    return false;
  }
  return true;
}

void loopHandler() {
  int buttonState = debouncer.read();

  if (buttonState != lastbuttonState) {
     Serial.print("Button is now: ");
     Serial.println(buttonState ? "open" : "close");

     if (Homie.setNodeProperty(buttonNode, "open", buttonState ? "true" : "false", true)) {
       lastbuttonState = buttonState;
     } else {
       Serial.println("Sending failed");
     }
  }
}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  pinMode(PIN_BUTTON, INPUT);
  digitalWrite(PIN_BUTTON, HIGH);
  debouncer.attach(PIN_BUTTON);
  debouncer.interval(50);

  Homie.setFirmware(FW_NAME, FW_VERSION);
  Homie.setLedPin(PIN_LED, HIGH); // Status LED
  Homie.setResetTrigger(PIN_BUTTON, LOW, 5000);
  Homie.setLoopFunction(loopHandler);
  relayNode.subscribe("relayState", RelayHandler);
  Homie.registerNode(relayNode);
  Homie.registerNode(buttonNode);
  Homie.setup();
}

void loop() {
  Homie.loop();
  debouncer.update();
}
