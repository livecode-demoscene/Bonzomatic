#include "../MIDI.h"
#include <CoreMIDI/CoreMIDI.h>

namespace MIDI {
//////////////////////////////////////////////////////////////////////////
/// Based on
/// https://comelearncocoawithme.blogspot.com/2011/08/reading-from-external-controllers-with.html

int nCCValues[256];
MIDIClientRef midiClient;
MIDIPortRef midiInputPort;
MIDIObjectRef midiSource;

static void midiInputCallback(const MIDIPacketList *list, void *procRef,
                              void *srcRef) {
  const MIDIPacket *packet = &list->packet[0];
  Byte control = packet->data[1];
  Byte value = packet->data[2];
  nCCValues[control] = value;
}

bool Open() {
  OSStatus result =
      MIDIClientCreate(CFSTR("MIDI Client"), NULL, NULL, &midiClient);

  if (result != noErr) {
    printf("[MIDI] Error creating MIDI Client\n");
    return false;
  }

  result = MIDIInputPortCreate(midiClient, CFSTR("Input"), midiInputCallback,
                               NULL, &midiInputPort);
  if (result != noErr) {
    printf("[MIDI] Error creating MIDI Input Port\n");
    return false;
  }

  for (int i = 0; i < MIDIGetNumberOfSources(); i++) {
    MIDIObjectRef endPoint = MIDIGetSource(i);
    result = MIDIPortConnectSource(midiInputPort, endPoint, NULL);

    if (result != noErr) {
      printf("[MIDI] Error connecting MIDI Port\n");
      continue;
    } else {
      // Connect to the first source detected.
      midiSource = endPoint;
      return true;
    }
  }

  return false;
}

bool Close() {
  MIDIPortDisconnectSource(midiInputPort, midiSource);
  MIDIPortDispose(midiInputPort);
  MIDIClientDispose(midiClient);
  return true;
}

float GetCCValue(unsigned char cc) { return nCCValues[cc] / 127.0f; }

//////////////////////////////////////////////////////////////////////////
}; // namespace MIDI
