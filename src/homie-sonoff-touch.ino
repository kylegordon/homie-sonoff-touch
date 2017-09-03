#include <ClickButton.h>
#include <Homie.h>

#define FW_NAME "homie-sonoff-touch"
#define FW_VERSION "1.0.1"

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

int function = 0;

// Timer related for dimming delays
unsigned long previousMillis = 0;
// Milliseconds to wait per cycle for a held command.
const int waitInterval = 100;

// Associate the PIN_BUTTON GPIO with the ClickButton library
ClickButton button1(PIN_BUTTON, LOW, CLICKBTN_PULLUP);

// Register our two HomieNode instances
HomieNode relayNode("relay", "relay");
HomieNode buttonNode("button", "button");

bool RelayHandler(String value) {
  /*
  Here we handle incoming requests to set the state of the relay
  Set the RELAY_PIN to the appropriate level, and additionally set the
  state on the relayState topic.
  */
  if (value == "ON") {
    digitalWrite(PIN_RELAY, HIGH);
    relayNode.setProperty("relayState").send("ON");
    Serial.println("Relay is on");
  } else if (value == "OFF") {
    digitalWrite(PIN_RELAY, LOW);
    relayNode.setProperty("relayState").send("OFF");
    Serial.println("Relay is off");
  } else {
    Serial.print("Unknown value: ");
    Serial.println(value);
    return false;
  }
  return true;
}

void loopHandler() {
  // Update button state
  button1.Update();

  // Save click codes in function, as click codes are reset at next Update()
  if(button1.clicks != 0) function = button1.clicks;

  // These are single event button presses.
  // Handle them and reset function back to 0
  if ( function > 0 ) {
    Serial.println("One-shot");
    if ( function == 1 ) {
      Serial.println("SINGLE click");
      buttonNode.setProperty("event").send("SINGLE");
    }

    if ( function == 2 ) {
      buttonNode.setProperty("event").send("DOUBLE");
      Serial.println("DOUBLE click");
    }

    if ( function == 3 ) {
      buttonNode.setProperty("event").send("TRIPLE");
      Serial.println("TRIPLE click");
    }
    // This has been a single event.
    function = 0;
  }

  // These are repeat events, where the button is being held down.
  // Handle them, but don't reset function back to 0 unless the button is released.
  if ( function < 0 ) {
    if ( millis() - previousMillis >= waitInterval ) {
      previousMillis = millis();
      if ( function == -1 ) {
        buttonNode.setProperty("event").send("SINGLEHELD");
        Serial.println("SINGLE LONG click");
      }

      if ( function == -2 ) {
        buttonNode.setProperty("event").send("DOUBLEHELD");
        Serial.println("DOUBLE LONG click");
      }

      if ( function == -3 ) {
        buttonNode.setProperty("event").send("TRIPLEHELD");
        Serial.println("TRIPLE LONG click");
      }
    }

    // Decide whether the button is being held or not
    if ( button1.depressed == 1 ) {
      // This will need rate limited
      Serial.println("Held");
    }
    if ( button1.depressed == 0 ) {
      Serial.println("Released");
      function = 0;
    }
  // Rate limit...

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

  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setLedPin(PIN_LED, HIGH); // Status LED
  // This is a full reset, and will wipe the config
  Homie.setResetTrigger(PIN_BUTTON, LOW, 30000);
  Homie.setLoopFunction(loopHandler);
  // FIXME relayNode.subscribe("relayState", RelayHandler);
  // FIXME Homie.registerNode(relayNode);
  // FIXME Homie.registerNode(buttonNode);
  Homie.setup();
}

void loop() {
  Homie.loop();
}
