/*
 * Four button membrane keypad sends MIDI messages.
 */

/* Install using the Arduino IDE library manager */
/* https://github.com/thomasfredericks/Bounce2 */
#include <Bounce2.h>
#include <MIDIUSB.h>

/* Not used except for some definitions such as midi::TimeCodeQuarterFrame */
#include <MIDI.h>

// These 4 digital pins are conveniently located next to a ground pin on a
// SparkFun Pro Micro.
const uint8_t BUTTONS[] = {3, 2, 5, 4};

// Instantiate a Bounce object
Bounce debouncer[sizeof(BUTTONS)];

/*************** MIDI USB functions ****************************/

// First parameter is the event type (0x09 = note on, 0x08 = note off).
// Second parameter is note-on/note-off, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the velocity (64 = normal, 127 = fastest).

void USBNoteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t evt = {0x08, (byte)(0x80 | (channel - 1)), pitch, velocity};
  MidiUSB.sendMIDI(evt);
}

void USBNoteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t evt = {0x09, (byte)(0x90 | (channel - 1)), pitch, velocity};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0A = polyphonic key pressure)
// Second parameter is poly-keypress, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the note number (48 = middle C).
// Fourth parameter is the pressure on the key after it "bottoms out"

void USBAfterTouchPoly(byte channel, byte pitch, byte pressure) {
  midiEventPacket_t evt = {0x0A, (byte)(0xA0 | (channel - 1)), pitch, pressure};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0B = control change).
// Second parameter is the event type, combined with the channel.
// Third parameter is the control number number (0-119).
// Fourth parameter is the control value (0-127).

void USBControlChange(byte channel, byte control, byte value) {
  midiEventPacket_t event = {0x0B, (byte)(0xB0 | (channel - 1)), control, value};
  MidiUSB.sendMIDI(event);
}

// First parameter is the event type (0x0C = program change)
// Second parameter is program number, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the program number.
// Fourth parameter is 0.

void USBProgramChange(byte channel, byte number) {
  midiEventPacket_t evt = {0x0C, (byte)(0xC0 | (channel - 1)), number, 0};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0D = after touch channel pressure)
// Second parameter is channel pressure, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the channel pressure value.
// Fourth parameter is 0.

void USBAfterTouchChannel(byte channel, byte pressure) {
  midiEventPacket_t evt = {0x0D, (byte)(0xD0 | (channel - 1)), pressure, 0};
  MidiUSB.sendMIDI(evt);
}

// First parameter is the event type (0x0E = pitch bend)
// Second parameter is pitch bend, combined with the channel.
// Channel can be anything between 0-15. Typically reported to the user as 1-16.
// Third parameter is the least significant 7 bits of pitch bend
// Fourth parameter is the most significant 7 bits of pitch bend

void USBPitchBend(byte channel, int bend) {
  uint16_t uint_bend = (uint16_t)(bend - MIDI_PITCHBEND_MIN);
  midiEventPacket_t evt = {0x0E, (byte)(0xE0 | (channel - 1)),
    (byte)(uint_bend & 0x7F), (byte)((uint_bend >> 7) & 0x7F)};
  MidiUSB.sendMIDI(evt);
}

// Be sure to include 0xF0 and 0xF7.
void USBSystemExclusive(unsigned size, byte *data) {
  uint8_t bytesOut;
  midiEventPacket_t evt;

  while (size > 0) {
    bytesOut = min(3, size);
    evt.byte1 = *data++;
    size -= bytesOut;
    switch (bytesOut) {
      case 1:
        evt.header = 0x05;
        evt.byte2 = evt.byte3 = 0;
        break;
      case 2:
        evt.header = 0x06;
        evt.byte2 = *data++;
        evt.byte3 = 0;
        break;
      case 3:
        evt.header = (size > 0) ? 0x04 : 0x07;
        evt.byte2 = *data++;
        evt.byte3 = *data++;
        break;
      default:
        break;
    }
    MidiUSB.sendMIDI(evt);
  }
}

// Be sure to include 0xF0 and 0xF7.
// This version reads a buffer stored in PROGMEM
void USBSystemExclusive_P(unsigned size, byte *data) {
  uint8_t bytesOut;
  midiEventPacket_t evt;

  while (size > 0) {
    bytesOut = min(3, size);
    evt.byte1 = pgm_read_byte_near(data++);
    size -= bytesOut;
    switch (bytesOut) {
      case 1:
        evt.header = 0x05;
        evt.byte2 = evt.byte3 = 0;
        break;
      case 2:
        evt.header = 0x06;
        evt.byte2 = pgm_read_byte_near(data++);
        evt.byte3 = 0;
        break;
      case 3:
        evt.header = (size > 0) ? 0x04 : 0x07;
        evt.byte2 = pgm_read_byte_near(data++);
        evt.byte3 = pgm_read_byte_near(data++);
        break;
      default:
        break;
    }
    MidiUSB.sendMIDI(evt);
  }
}

void USBTimeCodeQuarterFrame(byte data)
{
  midiEventPacket_t evt = {0x02, midi::TimeCodeQuarterFrame, data, 0};
  MidiUSB.sendMIDI(evt);
}

void USBSongSelect(byte songnumber)
{
  midiEventPacket_t evt = {0x02, midi::SongSelect, songnumber, 0};
  MidiUSB.sendMIDI(evt);
}

void USBSongPosition(unsigned beats)
{
  midiEventPacket_t evt = {0x03, midi::SongPosition, (byte)(beats & 0x7F),
    (byte)((beats >> 7) & 0x7F)};
  MidiUSB.sendMIDI(evt);
}

inline void USBRealTime(midi::MidiType midiType)
{
  midiEventPacket_t evt = {0x0F, (byte)midiType, 0, 0};
  MidiUSB.sendMIDI(evt);
}

void USBTuneRequest(void)
{
  USBRealTime(midi::TuneRequest);
}

void USBTimingClock(void)
{
  USBRealTime(midi::Clock);
}

void USBStart(void)
{
  USBRealTime(midi::Start);
}

void USBContinue(void)
{
  USBRealTime(midi::Continue);
}

void USBStop(void)
{
  USBRealTime(midi::Stop);
}

void USBActiveSensing(void)
{
  USBRealTime(midi::ActiveSensing);
}

void USBSystemReset(void)
{
  USBRealTime(midi::SystemReset);
}

void setup()
{
  SERIAL_PORT_MONITOR.begin(115200);
  SERIAL_PORT_MONITOR.println("Buttons 4 all!");

  for (uint8_t i = 0; i < sizeof(BUTTONS); i++) {
    pinMode(BUTTONS[i], INPUT_PULLUP);
    debouncer[i] = Bounce();
    // After setting up the button, setup the Bounce instance :
    debouncer[i].attach(BUTTONS[i]);
    debouncer[i].interval(5); // interval in ms
  }
}

const byte SYSEX1[] PROGMEM = {
  0xF0, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
  0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0xF7
};

void loop() {
  for (uint8_t i = 0; i < sizeof(BUTTONS); i++) {
    debouncer[i].update();
    if (debouncer[i].fell()) {
      SERIAL_PORT_MONITOR.print(F("Button "));
      SERIAL_PORT_MONITOR.print(i);
      SERIAL_PORT_MONITOR.println(F(" down"));
      switch (i) {
        case 0:
          USBNoteOn(1, 60, 127);
          break;
        case 1:
          USBProgramChange(1, 16);
          break;
        case 2:
          USBSystemReset();
          break;
        case 3:
          USBSystemExclusive_P(sizeof(SYSEX1), SYSEX1);
          break;
        default:
          SERIAL_PORT_MONITOR.println(F("This should never happen!"));
          break;
      }
      MidiUSB.flush();
    }
    else if (debouncer[i].rose()) {
      SERIAL_PORT_MONITOR.print(F("Button "));
      SERIAL_PORT_MONITOR.print(i);
      SERIAL_PORT_MONITOR.println(F(" up"));
      switch (i) {
        case 0:
          USBNoteOff(1, 60, 127);
          break;
        case 1:
          break;
        case 2:
          break;
        case 3:
          break;
        default:
          SERIAL_PORT_MONITOR.println(F("This should never happen either!"));
          break;
      }
      MidiUSB.flush();
    }
  }
}
