// @license
// Copyright (c) Audanika. All Rights Reserved.
//
// Use of this source code is governed by terms that can be
// found in the LICENSE file in the root of this package.

// A host's MIDI as events of the graph: each message a MIDI 1.0 channel
// voice UMP at its sample offset on the graph's event input (port 0). Values
// a host hands over normalized, from 0 to 1, are rounded to 7 or 14 bits.
// The functions keep no state; the realtime thread calls them.

#ifndef AUD_PLUGIN_MIDI_HPP
#define AUD_PLUGIN_MIDI_HPP

#include <cstdint>

#include "aud_abi.h"

namespace aud_plugin {

// #############################################################################
// A normalized value as a 7-bit MIDI value, rounded.
uint32_t sevenBits(double normalized);

// #############################################################################
// The event of a note on: `velocity` from 0 to 1 becomes 1 to 127, since a
// MIDI note on of velocity 0 is a note off.
AudEvent noteOnEvent(uint32_t channel, uint32_t pitch, float velocity,
                     uint32_t offset);

// #############################################################################
// The event of a note off with its release velocity.
AudEvent noteOffEvent(uint32_t channel, uint32_t pitch, float velocity,
                      uint32_t offset);

// #############################################################################
// The event of poly pressure on one note.
AudEvent polyPressureEvent(uint32_t channel, uint32_t pitch, float pressure,
                           uint32_t offset);

// #############################################################################
// The event of control change `cc` with a 7-bit `value`.
AudEvent controlChangeEvent(uint32_t channel, uint32_t cc, uint32_t value,
                            uint32_t offset);

// #############################################################################
// The event of channel pressure from 0 to 1.
AudEvent channelPressureEvent(uint32_t channel, double pressure,
                              uint32_t offset);

// #############################################################################
// The event of a pitch bend of 14 bits whose centre is the normalized 0.5.
AudEvent pitchBendEvent(uint32_t channel, double normalized, uint32_t offset);

// #############################################################################
// The event of a MIDI 1.0 channel voice message: `status` with its channel,
// one or two data bytes.
AudEvent midiEvent(uint32_t status, uint32_t data1, uint32_t data2,
                   uint32_t offset);

// #############################################################################
// The bytes a channel voice message with `status` carries after it: 1 for
// program change and channel pressure, 2 for the others, 0 for no channel
// voice status.
uint32_t midiDataBytes(uint32_t status);

}  // namespace aud_plugin

#endif  // AUD_PLUGIN_MIDI_HPP
