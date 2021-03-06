/*
 *******************************************************************************
   MIDI WiFi 1 button controller or keyboard

   Copyright (C) 2017 gdsport625@gmail.com

   Use with any ESP8266. Tested using NodeMCU devkit 1.0 because it has a button on GPIO0.
   Use with any ESP32. Tested using ESP Dev Module because it has a button on IO0.

   DIN, UART, and USB hardware are NOT used. All MIDI is sent over WiFi.

   Supports Apple MIDI (rtpMIDI) using this library. Install using the IDE.
      https://github.com/lathoub/Arduino-AppleMIDI-Library

   USB MIDI -> RTP MIDI

   Uses for a 1 button MIDI controller.
   1. Send ALL NOTES OFF on all channels. Sometimes called a MIDI panic button.
   2. Send Control Change messages for pedals such as Sustain/Damper. It might be possible
   to install the ESP and battery inside a foot pedal. No wires to trip over.
   3. A 1 key piano? Of course, adding more buttons is easy. Arcade buttons look very useful.

 *******************************************************************************
*/

/*
   MIT License

   Copyright (c) 2017 gdsports625@gmail.com

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to deal
   in the Software without restriction, including without limitation the rights
   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
   copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in all
   copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/

// Debounce buttons and switches, https://github.com/thomasfredericks/Bounce2/wiki
// Define the following here or in Bounce2.h. This make button change detection more responsive.
//#define BOUNCE_LOCK_OUT
#include <Bounce2.h>

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ESP8266WiFiMulti.h>
ESP8266WiFiMulti wifiMulti;
#elif defined(ESP32)
#include <WiFi.h>
#include <WiFiUdp.h>
#include <WiFiMulti.h>
WiFiMulti wifiMulti;
#else
#error This works only on ESP8266 or ESP32
#endif

#include "AppleMidi.h"

APPLEMIDI_CREATE_INSTANCE(WiFiUDP, AppleMIDI); // see definition in AppleMidi_Defs.h
bool AppleMIDIConnected = false;

const char SSID[] = "xxxxxxxx";           //  your network SSID (name)
const char PASS[] = "yyyyyyyyyyyyyyy";    // your network password

#define MIDI_CHANNEL  (0)

#define BUTTON_PIN 0
// Instantiate a Bounce object
Bounce debouncer = Bounce();

void AppleMIDI_setup()
{
  Serial.println(F("AppleMIDI setup"));

  Serial.println();
  Serial.print(F("IP address is "));
  Serial.println(WiFi.localIP());

  Serial.println(F("OK, now make sure you an rtpMIDI session that is Enabled"));
  Serial.print(F("Add device named Arduino with Host/Port "));
  Serial.print(WiFi.localIP());
  Serial.println(F(":5004"));
  Serial.println(F("Then press the Connect button"));
  Serial.println(F("Then open a MIDI listener (eg MIDI-OX) and monitor incoming notes"));

  // Create a session and wait for a remote host to connect to us
  AppleMIDI.stop();
  AppleMIDI.begin("midi1button");

  AppleMIDI.OnConnected(OnAppleMidiConnected);
  AppleMIDI.OnDisconnected(OnAppleMidiDisconnected);

  AppleMIDI.OnReceiveNoteOn(OnAppleMidiNoteOn);
  AppleMIDI.OnReceiveNoteOff(OnAppleMidiNoteOff);
}

void setup()
{
  Serial.begin(115200);
  Serial.println(F("\nmidi1button setup"));

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  // After setting up the button, setup the Bounce instance :
  debouncer.attach(BUTTON_PIN);
  debouncer.interval(5); // interval in ms

  wifiMulti.addAP(SSID, PASS);
  // Add more SSID and passwords. WiFiMulti will connect to the available AP
  // with the strongest signal.
  //wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
  //wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");
}

void loop()
{
  static bool connected = false;

  if (wifiMulti.run() == WL_CONNECTED) {
    if (!connected) {
      Serial.println(F("WiFi connected!"));
      AppleMIDI_setup();
      connected = true;
    }
  }
  else {
    if (connected) {
      Serial.println(F("WiFi not connected!"));
      connected = false;
    }
    delay(500);
  }

  // Listen to incoming notes
  AppleMIDI.run();

  // Update the Bounce instance :
  debouncer.update();

  // Enable only 1 of the following sections of code.

#if false
  // MIDI Panic button. Send All Notes Off on all channels.
  // Useful if a MIDI device has a note stuck on.
  if (debouncer.fell()) {
    for (uint8_t chan = 0; chan < 16; chan++) {
      // button pressed so send All Notes Off on all channels
      AppleMIDI.controlChange(0x7b, 0x00, chan);
      delay(2);
    }
  }
#endif

#if false
  // Sustain/Damper pedal
  if (debouncer.fell()) {
    // button pressed so send Control Change Sustain On
    AppleMIDI.controlChange(0x40, 0x7F, MIDI_CHANNEL);
  }
  else if (debouncer.rose()) {
    // button release so send Control Change Sustain Off
    AppleMIDI.controlChange(0x40, 0x00, MIDI_CHANNEL);
  }
#endif

#if true
  // 1 Key piano
  if (debouncer.fell()) {
    // button pressed so send Note On
    AppleMIDI.noteOn(0x2b, 0x64, MIDI_CHANNEL);
  }
  else if (debouncer.rose()) {
    // button released so semd Note Off
    AppleMIDI.noteOff(0x2b, 0x64, MIDI_CHANNEL);
  }
#endif
}

// ====================================================================================
// Event handlers for incoming MIDI messages
// ====================================================================================

// -----------------------------------------------------------------------------
// rtpMIDI session. Device connected
// -----------------------------------------------------------------------------
void OnAppleMidiConnected(uint32_t ssrc, char* name) {
  AppleMIDIConnected  = true;
  Serial.print(F("Apple MIDI connected to session "));
  Serial.println(name);
}

// -----------------------------------------------------------------------------
// rtpMIDI session. Device disconnected
// -----------------------------------------------------------------------------
void OnAppleMidiDisconnected(uint32_t ssrc) {
  AppleMIDIConnected  = false;
  Serial.println(F("Apple MIDI disconnected"));
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOn(byte channel, byte note, byte velocity) {
  Serial.print(F("Incoming NoteOn from channel:"));
  Serial.print(channel);
  Serial.print(F(" note:"));
  Serial.print(note);
  Serial.print(F(" velocity:"));
  Serial.print(velocity);
  Serial.println();
}

// -----------------------------------------------------------------------------
//
// -----------------------------------------------------------------------------
void OnAppleMidiNoteOff(byte channel, byte note, byte velocity) {
  Serial.print(F("Incoming NoteOff from channel:"));
  Serial.print(channel);
  Serial.print(F(" note:"));
  Serial.print(note);
  Serial.print(F(" velocity:"));
  Serial.print(velocity);
  Serial.println();
}
