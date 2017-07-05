/* WARNING: untested */

/* Some functional ideas
  Single, double and triple tap.
  Single and double tap and hold
  Single tap = regular on/off
  Single hold = dim up
  Double tap = Unknown
  Double tap and hold = dim down
  Triple tap = toggle state of onboard relay
*/

#include <ClickButton.h>
#include <Homie.h>

#define FW_NAME "homie-sonoff-touch"
#define FW_VERSION "0.0.8"

/* Magic sequence for Autodetectable Binary Upload */
const char *__FLAGGED_FW_NAME = "\xbf\x84\xe4\x13\x54" FW_NAME "\x93\x44\x6b\xa7\x75";
const char *__FLAGGED_FW_VERSION = "\x6a\x3f\x3e\x0e\xe1" FW_VERSION "\xb0\x30\x48\xd4\x1a";
/* End of magic sequence for Autodetectable Binary Upload */

// LED_PIN controls the LED under the wifi symbol on the front plate.
// RELAY_PIN controls the relay, *AND* the LED under the touchpad.
// Scheme is similar to https://github.com/enc-X/sonoff-homie
// ie, mosquitto_pub -h homeauto.vpn.glasgownet.com -t 'devices/85376d51/relay/relayState/set' -m 'ON'

// Nr. of buttons in the array
const int buttons = 1;

const int PIN_RELAY = 12;
const int PIN_LED = 13;
const int PIN_BUTTON = 0;

volatile bool published = false;
volatile unsigned long timestamp = 0;

ClickButton button1(PIN_BUTTON, LOW, CLICKBTN_PULLUP);

int lastbuttonState = -1;
int ledState = 0;
int LEDfunction = 0;
int function = 0;
int lastmillis = millis();

Bounce debouncer = Bounce(); // Bounce is built into Homie, so you can use it without including it first

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

void shortPress() {
  Serial.println("Short press");
  published = true;
}

void longPress() {
  Serial.println("Long press");
  published = true;
}

void loopHandler() {
  // Update button state
  button1.Update();

  // Save click codes in function, as click codes are reset at next Update()
  if(button1.clicks != 0) function = button1.clicks;


  // Once done, put all this into a switch/case
  if(function == 1) {
    Serial.println("SINGLE click");
    Homie.setNodeProperty(buttonNode, "event", "SINGLE");
  }

  if(function == 2) {
    Homie.setNodeProperty(buttonNode, "event", "DOUBLE");
    Serial.println("DOUBLE click");
  }

  if(function == 3) {
    Homie.setNodeProperty(buttonNode, "event", "TRIPLE");
    Serial.println("TRIPLE click");
  }

  if(function == -1) {
    Homie.setNodeProperty(buttonNode, "event", "SINGLEHELD");
    Serial.println("SINGLE LONG click");
  }

  if(function == -2) {
    Homie.setNodeProperty(buttonNode, "event", "DOUBLEHELD");
    Serial.println("DOUBLE LONG click");
  }

  if(function == -3) {
    Homie.setNodeProperty(buttonNode, "event", "TRIPLEHELD");
    Serial.println("TRIPLE LONG click");
  }


  // Combine the above into the below
  if ( function > 0 ) {
    // One shot message
    Serial.println("One-shot");
    function = 0;
  }

  if ( function < 0 ) {
    if ( button1.depressed == 1 ) {
      // This will need rate limited
      Serial.println("Held");
    }
    if ( button1.depressed == 0 ) {
      Serial.println("Released");
      function = 0;
    }
  }

  delay(5);

}

void setup() {
  pinMode(PIN_RELAY, OUTPUT);
  digitalWrite(PIN_RELAY, LOW);

  // Setup button timers (all in milliseconds / ms)
  // (These are default if not set, but changeable for convenience)
  button1.debounceTime   = 20;   // Debounce timer in ms
  button1.multiclickTime = 250;  // Time limit for multi clicks
  button1.longClickTime  = 1000; // time until "held-down clicks" register

  Homie.setFirmware(FW_NAME, FW_VERSION);
  Homie.setLedPin(PIN_LED, HIGH); // Status LED
  // This is a full reset, and will wipe the config
  Homie.setResetTrigger(PIN_BUTTON, LOW, 10000);
  Homie.setLoopFunction(loopHandler);
  relayNode.subscribe("relayState", RelayHandler);
  Homie.registerNode(relayNode);
  Homie.registerNode(buttonNode);
  Homie.setup();
}

void loop() {
  Homie.loop();
}
