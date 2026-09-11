#include "core/SynthEngine.hpp"
#include <vector>
#include <fstream>
#include <iostream>
#include <cmath>

void writeWav(const std::string& filename, const std::vector<float>& samples, int sampleRate = 44100) {
    std::ofstream file(filename, std::ios::binary);

    int numSamples = static_cast<int>(samples.size());
    int dataSize = numSamples * 2;
    int chunkSize = 36 + dataSize;
    int byteRate = sampleRate * 2;

    // RIFF Header
    file.write("RIFF", 4);
    file.write(reinterpret_cast<const char*>(&chunkSize), 4);
    file.write("WAVE", 4);

    // fmt chunk
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

    // data chunk
    file.write("data", 4);
    file.write(reinterpret_cast<const char*>(&dataSize), 4);

    for (float s : samples) {
        float clamped = std::max(-1.0f, std::min(1.0f, s));
        short intVal = static_cast<short>(clamped * 32767.0f);
        file.write(reinterpret_cast<const char*>(&intVal), 2);
    }

    std::cout << "Wrote " << filename << " (" << samples.size() << " samples)\n";
}

int main() {
    syrebas::SynthEngine engine;
    engine.setSampleRate(44100.0);

    auto& params = engine.getParams();
    params.cutoff = 0.4f;       // Cutoff knob
    params.resonance = 0.85f;   // High resonance
    params.envMod = 0.8f;      // High env mod
    params.decay = 0.5f;       // Decay knob ~50%
    params.accent = 0.9f;      // High accent
    params.waveform = syrebas::Waveform::Saw;
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
        { sampleRate / 4, true, 36, 1.0f },    // C2 Accent 1 (accumulate cap)
        { sampleRate / 2, true, 36, 1.0f },    // C2 Accent 2 (accumulate cap further)
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

    writeWav("test_syrebas_303.wav", audioBuffer, sampleRate);

    // Run programmatic assertions on generated audio
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
        std::cerr << "ERROR: Audio buffer is silent!\n";
        return 1;
    }

    if (maxAbs > 1.2f) {
        std::cerr << "ERROR: Output clipped abnormally! Max abs: " << maxAbs << "\n";
        return 1;
    }

    std::cout << "DSP test completed successfully. Max peak amplitude: " << maxAbs << "\n";
    return 0;
}
