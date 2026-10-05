#include "CalibrationProfile.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace acidus {

#define ACIDUS_CAL_FIELD(field) { #field, &SynthParameters::field }

static const CalibrationField kFields[] = {
    ACIDUS_CAL_FIELD(oscCouplingHz),
    ACIDUS_CAL_FIELD(resCouplingHz),
    ACIDUS_CAL_FIELD(filterFeedbackGain),
    ACIDUS_CAL_FIELD(filterPostHpHz),
    ACIDUS_CAL_FIELD(filterNotchHz),
    ACIDUS_CAL_FIELD(filterNotchBandwidthHz),
    ACIDUS_CAL_FIELD(filterAllpassHz),
    ACIDUS_CAL_FIELD(filterInputCouplingHz),
    ACIDUS_CAL_FIELD(filterOutputCouplingHz),
    ACIDUS_CAL_FIELD(filterCapScale1),
    ACIDUS_CAL_FIELD(filterCapScale2),
    ACIDUS_CAL_FIELD(filterCapScale3),
    ACIDUS_CAL_FIELD(filterCapScale4),
    ACIDUS_CAL_FIELD(filterLadderInputScale),
    ACIDUS_CAL_FIELD(filterLadderTopology),
    ACIDUS_CAL_FIELD(vegDecaySec),
    ACIDUS_CAL_FIELD(vcaGateOffMs),
    ACIDUS_CAL_FIELD(vcaGateOffAccentMs),
    ACIDUS_CAL_FIELD(vcaResTapRatio),
    ACIDUS_CAL_FIELD(vcaGainSaturationDrive),
    ACIDUS_CAL_FIELD(vcoOctaveScale),
    ACIDUS_CAL_FIELD(cutoffBaseHz),
    ACIDUS_CAL_FIELD(cutoffSpanOct),
    ACIDUS_CAL_FIELD(cutoffMaxHz),
    ACIDUS_CAL_FIELD(cutoffTaperExp),
    ACIDUS_CAL_FIELD(envModScaleC0),
    ACIDUS_CAL_FIELD(envModScaleC0Slope),
    ACIDUS_CAL_FIELD(envModScaleC1),
    ACIDUS_CAL_FIELD(envModScaleC1Slope),
    ACIDUS_CAL_FIELD(envModOffset),
    ACIDUS_CAL_FIELD(envModTaperExp),
    ACIDUS_CAL_FIELD(envModTaperMid),
    ACIDUS_CAL_FIELD(envModTaperWidth),
    ACIDUS_CAL_FIELD(envModOffsetCutSlope),
    ACIDUS_CAL_FIELD(accentSweepDepthOct),
    ACIDUS_CAL_FIELD(accentVcaDepth),
    ACIDUS_CAL_FIELD(accentChargeBaseSec),
    ACIDUS_CAL_FIELD(accentChargePotSec),
    ACIDUS_CAL_FIELD(accentDiodeDrop),
    ACIDUS_CAL_FIELD(accentMixSec),
    ACIDUS_CAL_FIELD(oscSawLpfHz),
    ACIDUS_CAL_FIELD(oscSawShape),
    ACIDUS_CAL_FIELD(oscSquareDutyDepth),
    ACIDUS_CAL_FIELD(oscSquareLevel),
    ACIDUS_CAL_FIELD(vcfAttackMs),
    ACIDUS_CAL_FIELD(vcaNormalDelayMs),
    ACIDUS_CAL_FIELD(vcaAttackMs),
    ACIDUS_CAL_FIELD(vcfDecayMinSec),
    ACIDUS_CAL_FIELD(vcfDecayMaxSec),
    ACIDUS_CAL_FIELD(vcfDecayTaper),
    ACIDUS_CAL_FIELD(accentDecaySec),
    ACIDUS_CAL_FIELD(filterResonanceSkew),
    ACIDUS_CAL_FIELD(filterResonanceLimit),
};

#undef ACIDUS_CAL_FIELD

const CalibrationField* calibrationFields() { return kFields; }
size_t calibrationFieldCount() { return sizeof(kFields) / sizeof(kFields[0]); }

static std::string jsonString(const std::string& s) {
    std::string out = "\"";
    for (unsigned char c : s) {
        if (c == '"' || c == '\\') { out += '\\'; out += static_cast<char>(c); }
        else if (c < 0x20) { char buf[8]; std::snprintf(buf, sizeof(buf), "\\u%04x", c); out += buf; }
        else out += static_cast<char>(c);
    }
    return out + "\"";
}

std::string calibrationToJson(const SynthParameters& params, const std::string& name, const std::string& source) {
    std::string out = "{\n \"name\": " + jsonString(name) + ",\n \"source\": " + jsonString(source)
                      + ",\n \"parameters\": {\n";
    for (size_t i = 0; i < calibrationFieldCount(); ++i) {
        char num[40];
        // 9 significant digits round-trip a float exactly.
        std::snprintf(num, sizeof(num), "%.9g", static_cast<double>(params.*(kFields[i].member)));
        out += "  " + jsonString(kFields[i].name) + ": " + num + (i + 1 < calibrationFieldCount() ? ",\n" : "\n");
    }
    return out + " }\n}\n";
}

// A small JSON reader: enough for profiles (objects, arrays, strings,
// numbers, true/false/null), strict about syntax, with a depth limit.
namespace {

struct Reader {
    const char* p;
    const char* end;
    std::string error;
    int depth{0};

    void ws() { while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) ++p; }
    bool fail(const char* what) { if (error.empty()) error = what; return false; }
    bool eat(char c) { ws(); if (p < end && *p == c) { ++p; return true; } return false; }

    bool string(std::string& out) {
        ws();
        if (p >= end || *p != '"') return fail("expected a string");
        ++p;
        out.clear();
        while (p < end && *p != '"') {
            char c = *p++;
            if (static_cast<unsigned char>(c) < 0x20) return fail("control character in a string");
            if (c == '\\') {
                if (p >= end) return fail("unterminated string");
                const char e = *p++;
                switch (e) {
                    case '"': case '\\': case '/': out += e; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case 'n': out += '\n'; break;
                    case 'r': out += '\r'; break;
                    case 't': out += '\t'; break;
                    case 'u': {
                        if (end - p < 4) return fail("bad \\u escape");
                        unsigned v = 0;
                        for (int i = 0; i < 4; ++i) {
                            const char h = *p++;
                            v <<= 4;
                            if (h >= '0' && h <= '9') v |= static_cast<unsigned>(h - '0');
                            else if (h >= 'a' && h <= 'f') v |= static_cast<unsigned>(h - 'a' + 10);
                            else if (h >= 'A' && h <= 'F') v |= static_cast<unsigned>(h - 'A' + 10);
                            else return fail("bad \\u escape");
                        }
                        out += (v < 0x80) ? static_cast<char>(v) : '?';   // names only: ASCII is enough
                        break;
                    }
                    default: return fail("bad escape");
                }
            } else {
                out += c;
            }
        }
        if (p >= end) return fail("unterminated string");
        ++p;
        return true;
    }

    bool number(double& out) {
        ws();
        const char* s = p;
        if (p < end && *p == '-') ++p;
        if (p >= end || !(*p >= '0' && *p <= '9')) return fail("expected a number");
        while (p < end && ((*p >= '0' && *p <= '9') || *p == '.' || *p == 'e' || *p == 'E' || *p == '+' || *p == '-')) ++p;
        const std::string text(s, p);
        char* stop = nullptr;
        out = std::strtod(text.c_str(), &stop);
        if (!stop || *stop != '\0') return fail("bad number");
        return true;
    }

    bool literal(const char* word) {
        const size_t n = std::strlen(word);
        if (static_cast<size_t>(end - p) < n || std::strncmp(p, word, n) != 0) return fail("unexpected character");
        p += n;
        return true;
    }

    // Any value, discarded.
    bool skip() {
        ws();
        if (p >= end) return fail("unexpected end");
        if (*p == '"') { std::string s; return string(s); }
        if (*p == '{' || *p == '[') {
            const char close = (*p == '{') ? '}' : ']';
            const bool object = (*p == '{');
            if (++depth > 64) return fail("nested too deep");
            ++p;
            if (eat(close)) { --depth; return true; }
            do {
                if (object) { std::string k; if (!string(k) || !eat(':')) return fail("expected a key"); }
                if (!skip()) return false;
            } while (eat(','));
            if (!eat(close)) return fail("expected , or a closing bracket");
            --depth;
            return true;
        }
        if (*p == 't') return literal("true");
        if (*p == 'f') return literal("false");
        if (*p == 'n') return literal("null");
        double d;
        return number(d);
    }
};

} // namespace

bool calibrationFromJson(const std::string& json, SynthParameters& params, CalibrationImport& result) {
    result = CalibrationImport{};
    Reader r{json.data(), json.data() + json.size(), {}};
    SynthParameters out = params;
    bool sawParameters = false;
    if (!r.eat('{')) { result.error = "not a JSON object"; return false; }
    if (!r.eat('}')) {
        do {
            std::string key;
            if (!r.string(key) || !r.eat(':')) { result.error = r.error.empty() ? "expected a key" : r.error; return false; }
            if (key == "name") {
                r.ws();
                if (r.p < r.end && *r.p == '"') {
                    if (!r.string(result.name)) { result.error = r.error; return false; }
                } else if (!r.skip()) { result.error = r.error; return false; }
            } else if (key == "parameters") {
                sawParameters = true;
                if (!r.eat('{')) { result.error = "\"parameters\" is not an object"; return false; }
                if (r.eat('}')) continue;
                do {
                    std::string name;
                    if (!r.string(name) || !r.eat(':')) { result.error = r.error.empty() ? "expected a key" : r.error; return false; }
                    const CalibrationField* field = nullptr;
                    for (size_t i = 0; i < calibrationFieldCount(); ++i) {
                        if (name == kFields[i].name) { field = &kFields[i]; break; }
                    }
                    if (!field) {
                        if (!r.skip()) { result.error = r.error; return false; }
                        ++result.unknown;
                        continue;
                    }
                    double v = 0.0;
                    if (!r.number(v)) { result.error = name + ": " + r.error; return false; }
                    if (!std::isfinite(v) || std::abs(v) > 3.0e38) { result.error = name + ": not a finite number"; return false; }
                    out.*(field->member) = static_cast<float>(v);
                    ++result.loaded;
                } while (r.eat(','));
                if (!r.eat('}')) { result.error = "expected , or } in \"parameters\""; return false; }
            } else if (!r.skip()) {
                result.error = r.error;
                return false;
            }
        } while (r.eat(','));
        if (!r.eat('}')) { result.error = r.error.empty() ? "expected , or }" : r.error; return false; }
    }
    r.ws();
    if (r.p != r.end) { result.error = "text after the JSON object"; return false; }
    if (!sawParameters || result.loaded == 0) { result.error = "no calibration parameters in the file"; return false; }
    params = out;
    return true;
}

} // namespace acidus
