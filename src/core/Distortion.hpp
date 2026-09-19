#ifndef ACIDUS_DISTORTION_HPP
#define ACIDUS_DISTORTION_HPP

namespace acidus {

// MXR Distortion+ (M-104) emulation: a Distortion-knob-dependent op-amp
// gain/EQ stage (its pole, plateau gain and 741 gain-bandwidth-limited
// treble rolloff all move together), asymmetric op-amp rail saturation, and
// a dynamic germanium-diode shunt clipper solved per-sample with the
// trapezoidal rule. See docs/MXR_Distortion_Plus_Emulation_Compendium.md
// for the circuit analysis this is derived from.
//
// drive = 0 fully bypasses the stage (dry passthrough), like the pedal's
// footswitch disengaged; drive in (0, 1] engages it, sweeping the reissue's
// gain range from its minimum (~9.5 dB, 500 kohm pot) to its maximum
// (~46.6 dB, 0 ohm pot).
class Distortion {
public:
    Distortion();
    ~Distortion() = default;

    void setSampleRate(double sampleRate);
    void reset();

    float processSample(float input, float drive);

private:
    double sampleRate_{44100.0};
    double oversampledRate_{352800.0};

    float prevInput_{0.0f};

    // Distortion-knob-dependent gain/EQ shelf (direct form 1 state)
    float shelfX1_{0.0f};
    float shelfY1_{0.0f};

    // 741 gain-bandwidth-limited treble rolloff (one-pole lowpass state)
    float gbwState_{0.0f};

    // C4 output coupling cap: DC blocking after the asymmetric clip
    float couplingState_{0.0f};

    // Dynamic antiparallel-diode shunt clipper node
    float diodeV_{0.0f};
    float diodeGPrev_{0.0f};
};

} // namespace acidus

#endif // ACIDUS_DISTORTION_HPP
