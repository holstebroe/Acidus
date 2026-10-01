#include "core/SynthEngine.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>
#include <cstdlib>

void writeWav(const std::string& filename, const std::vector<float>& samples, int sampleRate = 44100) {
    std::ofstream file(filename, std::ios::binary);

    int numSamples = static_cast<int>(samples.size());
    int dataSize = numSamples * 2;
    int chunkSize = 36 + dataSize;
    int byteRate = sampleRate * 2;

    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&chunkSize), 4);
    file.write("WAVE", 4);

    file.write("fmt ", 4);
    int subchunk1Size = 16;
    short audioFormat = 1; // PCM
    short numChannels = 1; // Mono
    short bitsPerSample = 16;
    short blockAlign = 2;

    file.write(reinterpret_cast<const char*>(&subchunk1Size), 4);
    file.write(reinterpret_cast<const char*>(&audioFormat), 2);
    file.write(reinterpret_cast<const char*>(&numChannels), 2);
    file.write(reinterpret_cast<const char*>(&sampleRate), 4);
    file.write(reinterpret_cast<const char*>(&byteRate), 4);
    file.write(reinterpret_cast<const char*>(&blockAlign), 2);
    file.write(reinterpret_cast<const char*>(&bitsPerSample), 2);

    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);

    for (float s : samples) {
        float clamped = std::max(-1.0f, std::min(1.0f, s));
        short intVal = static_cast<short>(clamped * 32767.0f);
        file.write(reinterpret_cast<const char*>(&intVal), 2);
    }

    std::cout << "Wrote " << filename << " (" << samples.size() << " samples)\n";
}

// Oscillator frequency from the saw's reset edges (the falling ramp jumps up
// once per cycle), over 10 s: within ~0.05 % at 50-250 Hz.
static double measureOscHz(float octaveScale, float tuningCents, int note) {
    const double sr = 44100.0;
    acidus::Oscillator osc;
    osc.setSampleRate(sr);
    osc.setWaveform(acidus::Waveform::Saw);
    osc.setOctaveScale(octaveScale);
    osc.setTuningCents(tuningCents);
    osc.noteOn(note, false);
    // The band-limited reset spreads the jump over a couple of samples:
    // count the start of each rising run (the ramp itself only falls).
    float prev = osc.processNextSample();
    bool rising = false;
    int edges = 0;
    long first = -1, last = -1;
    for (long i = 1; i < static_cast<long>(sr * 10.0); ++i) {
        float x = osc.processNextSample();
        bool up = (x - prev) > 0.1f;
        if (up && !rising) {
            if (first < 0) first = i;
            last = i;
            ++edges;
        }
        rising = up;
        prev = x;
    }
    return (edges > 1) ? (edges - 1) * sr / static_cast<double>(last - first) : 0.0;
}

static void testOctaveScale() {
    struct Case { float scale; float cents; int note; double expectHz; const char* what; };
    const Case cases[] = {
        { 1.0f, 0.0f, 45, 110.0, "scale 1: A2 = 110 Hz" },
        { 1.0f, 0.0f, 57, 220.0, "scale 1: A3 = 2 x A2" },
        { 1.03f, 0.0f, 45, 110.0, "scale 1.03: A2 stays at the 110 Hz pivot" },
        { 1.03f, 0.0f, 57, 110.0 * std::pow(2.0, 1.03), "scale 1.03: A3 = 2^1.03 x A2" },
        { 1.03f, 0.0f, 33, 110.0 * std::pow(2.0, -1.03), "scale 1.03: A1 = 2^-1.03 x A2" },
        { 1.03f, 700.0f, 45, 110.0 * std::pow(2.0, 1.03 * 7.0 / 12.0), "scale 1.03 scales Tuning too" },
    };
    for (const auto& c : cases) {
        double hz = measureOscHz(c.scale, c.cents, c.note);
        double errPct = 100.0 * (hz / c.expectHz - 1.0);
        std::cout << (std::abs(errPct) < 0.15 ? "PASS: " : "FAIL: ") << c.what
                  << " (" << hz << " Hz, expected " << c.expectHz << ")\n";
        if (std::abs(errPct) >= 0.15) std::exit(1);
    }
}

int main() {
    testOctaveScale();

    acidus::SynthEngine engine;
    engine.setSampleRate(44100.0);

    auto& params = engine.getParams();
    params.cutoff = 0.4f;
    params.resonance = 0.85f;
    params.envMod = 0.8f;
    params.decay = 0.5f;
    params.accent = 0.9f;
    params.waveform = acidus::Waveform::Saw;
    params.masterVolume = 0.8f;

    std::vector<float> audioBuffer;
    int sampleRate = 44100;
    int frameSize = 256;
    std::vector<float> left(frameSize);
    std::vector<float> right(frameSize);

    struct Event {
        int sampleOffset;
        bool isNoteOn;
        int note;
        float vel;
    };

    std::vector<Event> events = {
        { 0, true, 36, 0.5f },                 // C2 normal
        { sampleRate / 4, true, 36, 1.0f },    // C2 Accent 1
        { sampleRate / 2, true, 36, 1.0f },    // C2 Accent 2
        { 3 * sampleRate / 4, true, 36, 1.0f },// C2 Accent 3
        { sampleRate, true, 48, 1.0f },       // C3 slide + accent
        { 5 * sampleRate / 4, false, 48, 0.0f },// Note off
        { 3 * sampleRate / 2, true, 43, 0.5f },// G2 normal
        { 7 * sampleRate / 4, true, 36, 0.5f },// C2 slide
        { sampleRate * 2, false, 36, 0.0f }
    };

    int currentSample = 0;
    int totalSamples = sampleRate * 3;
    size_t eventIdx = 0;

    while (currentSample < totalSamples) {
        while (eventIdx < events.size() && events[eventIdx].sampleOffset <= currentSample) {
            const auto& ev = events[eventIdx];
            if (ev.isNoteOn) {
                engine.noteOn(ev.note, ev.vel);
            } else {
                engine.noteOff(ev.note);
            }
            eventIdx++;
        }

        engine.processAudio(left.data(), right.data(), frameSize);
        for (int i = 0; i < frameSize; ++i) {
            audioBuffer.push_back(left[i]);
        }
        currentSample += frameSize;
    }

    writeWav("test_acidus.wav", audioBuffer, sampleRate);

    bool hasNonZeroOutput = false;
    float maxAbs = 0.0f;
    for (float sample : audioBuffer) {
        if (std::abs(sample) > 0.0001f) {
            hasNonZeroOutput = true;
        }
        if (std::abs(sample) > maxAbs) {
            maxAbs = std::abs(sample);
        }
    }

    if (!hasNonZeroOutput) {
        std::cerr << "ERROR: Audio buffer is silent for test_acidus.wav!\n";
        return 1;
    }

    if (maxAbs > 1.5f) {
        std::cerr << "ERROR: Output clipped abnormally! Max abs: " << maxAbs << "\n";
        return 1;
    }

    std::cout << "Acidus DSP test completed. Max peak amplitude: " << maxAbs << "\n";
    return 0;
}
