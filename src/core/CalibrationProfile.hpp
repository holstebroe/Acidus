#ifndef ACIDUS_CALIBRATION_PROFILE_HPP
#define ACIDUS_CALIBRATION_PROFILE_HPP

// Calibration profiles as JSON, the format of calibrations/*.json and of
// tools/calibration_profile.py / tools/fit_stage.py:
//
//   { "name": "...", "source": "...", "parameters": { "cutoffBaseHz": 184.076, ... } }
//
// The plugin exports its current calibration in this format and imports
// one back (GuiWindow's logo plate menu), so a fitted profile can be
// trimmed by ear and handed to the next fitting round.

#include "SynthEngine.hpp"

#include <cstddef>
#include <string>

namespace acidus {

struct CalibrationField {
    const char* name;                 // the SynthParameters member name, the JSON key
    float SynthParameters::*member;
};

// Every calibration constant of SynthParameters (all float fields except the
// front-panel controls).
const CalibrationField* calibrationFields();
size_t calibrationFieldCount();

// `params`' calibration constants as a profile.
std::string calibrationToJson(const SynthParameters& params, const std::string& name,
                              const std::string& source);

struct CalibrationImport {
    std::string name;       // the profile's "name", empty if it has none
    int loaded{0};          // constants read from "parameters"
    int unknown{0};         // keys in "parameters" this build does not know
    std::string error;      // set when the import failed
};

// Reads a profile's "parameters" into `params`; constants the profile lacks
// keep their value. Fails (and leaves `params` alone) on malformed JSON, a
// non-finite value, or a profile with no known constant.
bool calibrationFromJson(const std::string& json, SynthParameters& params, CalibrationImport& result);

} // namespace acidus

#endif // ACIDUS_CALIBRATION_PROFILE_HPP
