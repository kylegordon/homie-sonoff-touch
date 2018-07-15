#include <ClickButton.h>
#include <Homie.h>
#include <EEPROM.h>

#define FW_NAME "homie-sonoff-touch"
#define FW_VERSION "2.0.7-persistent"

// Disable this if you don't want the relay to turn on with any single tap event

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
unsigned long connectedMillis = 0;
unsigned long watchDogCounterStart=0;
unsigned long keepAliveReceived=0;

// Timer related for dimming delays
unsigned long previousMillis = 0;
// Milliseconds to wait per cycle for a held command.
const int waitInterval = 100;

struct EEpromDataStruct {
  // EEPROM structure
  int keepAliveTimeOut; // 0 - disabled, keepalive time - seconds
  bool initialState;  // Initial state (just after boot - homie independet)
  int watchDogTimeOut; // 0 - disabled, watchdog time limit - seconds
};

EEpromDataStruct EEpromData;

// Associate the PIN_BUTTON GPIO with the ClickButton library
ClickButton button1(PIN_BUTTON, LOW, CLICKBTN_PULLUP);

// Register our two HomieNode instances
HomieNode relayNode("relay", "relay");
HomieNode buttonNode("button", "button");
HomieNode keepAliveNode("keepalive", "keepalive");
HomieNode watchDogNode("watchdog", "Watchdog mode");

bool watchdogTickHandler(const HomieRange& range, const String& value) {
  // Recevied tick message for watchdog
  if (value == "0")
  {
    watchDogCounterStart = 0;
  } else {
    watchDogCounterStart = millis();
  }
  return true;
}

bool watchdogTimeOutHandler(const HomieRange& range, const String& value) {
  // Received watchdog timeout value
  int oldValue = EEpromData.watchDogTimeOut;
  if (value.toInt() > 15)
  {
    EEpromData.watchDogTimeOut = value.toInt();
  }
  if (value=="0")
  {
    EEpromData.watchDogTimeOut = 0;
  }

  if (oldValue!=EEpromData.watchDogTimeOut)
  {
    String outMsg = String(EEpromData.watchDogTimeOut);
    watchDogNode.setProperty("timeOut").send(outMsg);
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
}

bool relayInitModeHandler(HomieRange range, String value) {
  // Initial mode handler
  int oldValue = EEpromData.initialState;
  if (value.toInt() == 1 or value=="ON")
  {
    relayNode.setProperty("relayInitMode").send("1");
    EEpromData.initialState=1;
  } else {
    relayNode.setProperty("relayInitMode").send( "0");
    EEpromData.initialState=0;
  }
  if (oldValue!=EEpromData.initialState)
  {
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
  return true;
}

void setupHandler() {
  // Homie setup handler
  HomieRange emptyRange;
  if (EEpromData.initialState) {
    RelayHandler(emptyRange, "ON");
    relayNode.setProperty("relayInitMode").send("1");
  } else {
    RelayHandler(emptyRange, "OFF");
    relayNode.setProperty("relayInitMode").send("0");
  }
  String outMsg = String(EEpromData.keepAliveTimeOut);
  keepAliveNode.setProperty("timeOut").send(outMsg);
  outMsg = EEpromData.watchDogTimeOut;
  watchDogNode.setProperty("timeOut").send(outMsg);
  keepAliveReceived=millis();
  #ifdef SONOFFS20
  digitalWrite(PIN_LED, LOW);
  ledNode.setProperty("state").send("off");
  #endif
}

bool RelayHandler(const HomieRange& range, const String& value) {
  // Here we handle incoming requests to set the state of the relay
  // Set the RELAY_PIN to the appropriate level, and additionally set the
  // state on the relayState topic.
  if (value == "ON") {
    digitalWrite(PIN_RELAY, HIGH);
    if ( Homie.isConnected() ) { relayNode.setProperty("relayState").send("ON"); }
    Serial.println("Relay is on");
  } else if (value == "OFF") {
    digitalWrite(PIN_RELAY, LOW);
    if ( Homie.isConnected() ) { relayNode.setProperty("relayState").send("OFF"); }
    Serial.println("Relay is off");
  } else {
    Serial.print("Unknown value: ");
    Serial.println(value);
    return false;
  }
  return true;
}

bool keepAliveTickHandler(HomieRange range, String value) {
  // Keepalive tick handler
  keepAliveReceived=millis();
  return true;
}

bool keepAliveTimeOutHandler(HomieRange range, String value) {
  // Keepalive message handler
  int oldValue = EEpromData.keepAliveTimeOut;
  if (value.toInt() > 0)
  {
    EEpromData.keepAliveTimeOut = value.toInt();
  }
  if (value=="0")
  {
    EEpromData.keepAliveTimeOut = 0;
  }
  if (oldValue!=EEpromData.keepAliveTimeOut)
  {
    String outMsg = String(EEpromData.keepAliveTimeOut);
    keepAliveNode.setProperty("timeOut").send(outMsg);
    EEPROM.put(0, EEpromData);
    EEPROM.commit();
  }
}

// void loopHandler() {
//   //
// }

void onHomieEvent(const HomieEvent& event) {
  // Homie event handler
  switch(event.type) {
    case HomieEventType::CONFIGURATION_MODE: // Default eeprom data in configuration mode
      digitalWrite(PIN_RELAY, LOW);
      EEpromData.keepAliveTimeOut = 0;
      EEpromData.initialState = false;
      EEpromData.watchDogTimeOut = 0;
      EEPROM.put(0, EEpromData);
      EEPROM.commit();
      break;
    case HomieEventType::NORMAL_MODE:
      // Do whatever you want when normal mode is started
      break;
    case HomieEventType::OTA_STARTED:
      // Do whatever you want when OTA mode is started
      digitalWrite(PIN_RELAY, LOW);
      break;
    case HomieEventType::ABOUT_TO_RESET:
      // Do whatever you want when the device is about to reset
      break;
    case HomieEventType::WIFI_CONNECTED:
      // Do whatever you want when Wi-Fi is connected in normal mode
      break;
    case HomieEventType::WIFI_DISCONNECTED:
      // Do whatever you want when Wi-Fi is disconnected in normal mode
      break;
    case HomieEventType::MQTT_DISCONNECTED:
      // Do whatever you want when MQTT is disconnected in normal mode
      break;
  }
}

void setup() {

  Serial.begin(115200);

  EEPROM.begin(sizeof(EEpromData));
  EEPROM.get(0,EEpromData);
  pinMode(PIN_RELAY, OUTPUT);
  //digitalWrite(PIN_RELAY, LOW);
  if (EEpromData.initialState)
  {
    Serial.println("Startup: Relay On");
    digitalWrite(PIN_RELAY,HIGH);
  } else {
    Serial.println("Startup: Relay Off");
    digitalWrite(PIN_RELAY,LOW);
    EEpromData.initialState=0;
  }

  // Setup button timers (all in milliseconds / ms)
  // (These are default if not set, but changeable for convenience)
  button1.debounceTime   = 20;   // Debounce timer in ms
  button1.multiclickTime = 250;  // Time limit for multi clicks
  button1.longClickTime  = 1000; // time until "held-down clicks" register

  Homie_setFirmware(FW_NAME, FW_VERSION);
  Homie.setLedPin(PIN_LED, HIGH); // Status LED
  // This is a full reset, and will wipe the config
  Homie.setResetTrigger(PIN_BUTTON, LOW, 30000);
  Homie.setSetupFunction(setupHandler);
  //Homie.setLoopFunction(loopHandler);
  relayNode.advertise("relayState").settable(RelayHandler);
  //relayNode.advertise("relayTimer").settable(relayTimerHandler);
  relayNode.advertise("relayInitMode").settable(relayInitModeHandler);
  watchDogNode.advertise("tick").settable(watchdogTickHandler);
  watchDogNode.advertise("timeOut").settable(watchdogTimeOutHandler);
  keepAliveNode.advertise("tick").settable(keepAliveTickHandler);
  keepAliveNode.advertise("timeOut").settable(keepAliveTimeOutHandler);

  Homie.onEvent(onHomieEvent);
  Homie.setup();
}

void loop() {

  if ( Homie.isConnected() ) {
    // This counter will stop when disconnected
    connectedMillis = millis();
  } else {
    // If disconnected for over 5 minutes
    if ( millis() - connectedMillis >= 300000 ) {
      Serial.println("Restarting in ten seconds");
      delay(10000);
      // The chances of being in OTA whilst disconnected from MQTT are slim.
      ESP.restart();
    };
  }

  // New section
  // Check if keepalive is supported and expired
  if (EEpromData.keepAliveTimeOut != 0 && (millis() - keepAliveReceived) > EEpromData.keepAliveTimeOut*1000 )
  {
    ESP.restart();
  }
  if (watchDogCounterStart!=0 && EEpromData.watchDogTimeOut!=0 && (millis() - watchDogCounterStart) > EEpromData.watchDogTimeOut * 1000 )
  {
    HomieRange emptyRange;
    //relayTimerHandler(emptyRange, "10"); // Disable relay for 10 sec
    watchDogCounterStart = millis();
  }

  // Update button state
  button1.Update();

  // Save click codes in function, as click codes are reset at next Update()
  if(button1.clicks != 0) function = button1.clicks;

  // These are single event button presses.
  // Handle them and reset function back to 0
  if ( function > 0 ) {
    Serial.println("One-shot");
    if ( function == 1 ) {
      // If not connected, toggle the relay like a normal switch.
      // Calls to relayNode don't work whilst offline.
      if ( !Homie.isConnected() ) {
        Serial.println("Offline toggle of relay");
        digitalWrite(PIN_RELAY, !digitalRead(PIN_RELAY));
      }
      if ( Homie.isConnected() ) {
        relayNode.setProperty("relayState").send("ON");
        buttonNode.setProperty("event").setRetained(false).send("SINGLE");
      }
      // Read the new state, and use it to set the initial startup state
      String PIN_STATE = String(digitalRead(PIN_RELAY));
      relayNode.setProperty("relayInitMode").send(PIN_STATE);
      Serial.println("SINGLE click");
    }

    if ( function == 2 ) {
      if ( Homie.isConnected() ) { buttonNode.setProperty("event").setRetained(false).send("DOUBLE"); }
      Serial.println("DOUBLE click");
    }

    if ( function == 3 ) {
      if ( Homie.isConnected() ) { buttonNode.setProperty("event").setRetained(false).send("TRIPLE"); }
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
        if ( Homie.isConnected() ) { buttonNode.setProperty("event").setRetained(false).send("SINGLEHELD"); }
        Serial.println("SINGLE LONG click");
      }

      if ( function == -2 ) {
        if ( Homie.isConnected() ) { buttonNode.setProperty("event").setRetained(false).send("DOUBLEHELD"); }
        Serial.println("DOUBLE LONG click");
      }

      if ( function == -3 ) {
        if ( Homie.isConnected() ) { buttonNode.setProperty("event").setRetained(false).send("TRIPLEHELD"); }
        Serial.println("TRIPLE LONG click");
      }
    }

    // Decide whether the button is being held or not
    // I much prefer the idea at https://www.home-assistant.io/cookbook/dim_and_brighten_lights/
    // Where a pair of scripts are triggered by one message, and then loop until another message halts them.
    if ( button1.depressed == 1 ) {
      // This will need rate limited
      Serial.println("Held");
    }
    if ( button1.depressed == 0 ) {
      Serial.println("Released");
      function = 0;
    }
    // Rate limit...
    delay(5);
  }

  Homie.loop();
}
