#pragma once

#include <algorithm>
#include <cmath>
#include <iterator>

// Monophonic bass voice: two detunable oscillators + sub, 4-pole ladder
// filter with its own decay envelope, ADSR amp, legato glide.
namespace subforge
{

constexpr float kPi = 3.14159265358979f;

struct BassParams
{
    int wave = 0;          // 0 saw, 1 square, 2 pulse 25%, 3 sine, 4 triangle
    int octave = 0;        // -2..+1
    float sub = 0.5f;      // sub oscillator level
    int subWave = 0;       // 0 sine, 1 square
    float detuneCents = 0.0f;
    float cutoffHz = 400.0f;
    float resonance = 0.3f;
    float envAmount = 0.5f;
    float filterDecayMs = 250.0f;
    float drive = 0.2f;
    float attackMs = 1.0f;
    float decayMs = 600.0f;
    float sustain = 0.7f;
    float releaseMs = 60.0f;
    float glideMs = 50.0f;
    float velocitySens = 0.5f;
    float gain = 0.5f;
};

inline float polyBlep (float t, float dt)
{
    if (t < dt)
    {
        t /= dt;
        return t + t - t * t - 1.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt;
        return t * t + t + t + 1.0f;
    }
    return 0.0f;
}

// Integrated polyBlep, smooths slope discontinuities (triangle corners).
inline float polyBlamp (float t, float dt)
{
    if (t < dt)
    {
        t = t / dt - 1.0f;
        return -t * t * t / 3.0f;
    }
    if (t > 1.0f - dt)
    {
        t = (t - 1.0f) / dt + 1.0f;
        return t * t * t / 3.0f;
    }
    return 0.0f;
}

struct Oscillator
{
    float phase = 0.0f;

    void reset() { phase = 0.0f; }

    // wave: 0 saw, 1 square, 2 pulse 25%, 3 sine, 4 triangle
    float next (float inc, int wave)
    {
        const float t = phase;
        float out;

        if (wave == 0)
        {
            out = 2.0f * t - 1.0f - polyBlep (t, inc);
        }
        else if (wave == 3)
        {
            out = std::sin (2.0f * kPi * t);
        }
        else if (wave == 4)
        {
            float t2 = t + 0.5f;
            if (t2 >= 1.0f)
                t2 -= 1.0f;
            out = 1.0f - 4.0f * std::abs (t - 0.5f);
            out += 4.0f * inc * (polyBlamp (t, inc) - polyBlamp (t2, inc));
        }
        else
        {
            const float pw = wave == 1 ? 0.5f : 0.25f;
            out = t < pw ? 1.0f : -1.0f;
            out += polyBlep (t, inc);
            float t2 = t + 1.0f - pw;
            if (t2 >= 1.0f)
                t2 -= 1.0f;
            out -= polyBlep (t2, inc);
            out -= 2.0f * pw - 1.0f; // remove DC of asymmetric pulse
        }

        phase += inc;
        if (phase >= 1.0f)
            phase -= 1.0f;
        return out;
    }
};

// Zero-delay-feedback 4-pole ladder (Zavalishin TPT) with tanh on the input.
struct LadderFilter
{
    float s[4] {};

    void reset() { std::fill (std::begin (s), std::end (s), 0.0f); }

    float process (float x, float g, float k)
    {
        const float G = g / (1.0f + g);
        const float G2 = G * G;
        const float G4 = G2 * G2;
        const float S = (G2 * G * s[0] + G2 * s[1] + G * s[2] + s[3]) / (1.0f + g);
        const float y4 = (G4 * x + S) / (1.0f + k * G4);

        float u = std::tanh (x - k * y4);
        for (auto& state : s)
        {
            const float v = (u - state) * G;
            const float y = v + state;
            state = y + v;
            u = y;
        }
        return u;
    }
};

struct Envelope
{
    enum class Stage { Idle, Attack, Decay, Release };

    Stage stage = Stage::Idle;
    float value = 0.0f;
    float attackInc = 1.0f, decayCoef = 0.0f, sustain = 1.0f, releaseCoef = 0.0f;

    static float coefFor (float ms, float fs)
    {
        // reaches -60 dB after the given time
        return std::exp (-6.9f / std::max (1.0f, ms * 0.001f * fs));
    }

    void set (float fs, float attackMs, float decayMs, float sustainLevel, float releaseMs)
    {
        attackInc = 1.0f / std::max (1.0f, attackMs * 0.001f * fs);
        decayCoef = coefFor (decayMs, fs);
        sustain = sustainLevel;
        releaseCoef = coefFor (releaseMs, fs);
    }

    void noteOn() { stage = Stage::Attack; } // attacks from the current value, no click
    void noteOff() { if (stage != Stage::Idle) stage = Stage::Release; }
    void reset() { stage = Stage::Idle; value = 0.0f; }
    bool isActive() const { return stage != Stage::Idle; }

    float next()
    {
        switch (stage)
        {
            case Stage::Attack:
                value += attackInc;
                if (value >= 1.0f)
                {
                    value = 1.0f;
                    stage = Stage::Decay;
                }
                break;
            case Stage::Decay:
                value = sustain + (value - sustain) * decayCoef;
                break;
            case Stage::Release:
                value *= releaseCoef;
                if (value < 1.0e-5f)
                    reset();
                break;
            case Stage::Idle:
                break;
        }
        return value;
    }
};

class BassSynth
{
public:
    void prepare (double sampleRate)
    {
        fs = (float) sampleRate;
        cutoffSmoothCoef = std::exp (-1.0f / (0.005f * fs));
        reset();
        setParams (params);
        logCutoff = std::log2 (params.cutoffHz);
    }

    void reset()
    {
        numHeld = 0;
        amp.reset();
        filterEnv.reset();
        ladder.reset();
        osc1.reset();
        osc2.reset();
        subOsc.reset();
    }

    void setParams (const BassParams& p)
    {
        params = p;
        amp.set (fs, p.attackMs, p.decayMs, p.sustain, p.releaseMs);
        filterEnv.set (fs, 1.0f, p.filterDecayMs, 0.0f, p.filterDecayMs);
        glideCoef = p.glideMs <= 0.0f ? 0.0f : std::exp (-1.0f / (p.glideMs * 0.001f * fs));
        detuneRatio = std::exp2 (p.detuneCents / 1200.0f);
    }

    void setPitchBend (float semitones) { bend = semitones; }

    void noteOn (int note, float velocity)
    {
        const bool legato = numHeld > 0;
        removeHeld (note);
        if (numHeld < kMaxHeld)
            held[numHeld++] = note;

        targetNote = (float) note;

        if (! legato)
        {
            currentNote = targetNote;
            velScale = 1.0f - params.velocitySens * (1.0f - velocity);
            if (! amp.isActive())
            {
                osc1.reset();
                osc2.reset();
                subOsc.reset();
            }
            amp.noteOn();
            filterEnv.noteOn();
        }
    }

    void noteOff (int note)
    {
        const bool wasTop = numHeld > 0 && held[numHeld - 1] == note;
        removeHeld (note);

        if (numHeld == 0)
        {
            amp.noteOff();
            filterEnv.noteOff();
        }
        else if (wasTop)
        {
            targetNote = (float) held[numHeld - 1]; // glide back to the still-held note
        }
    }

    void allNotesOff()
    {
        numHeld = 0;
        amp.noteOff();
        filterEnv.noteOff();
    }

    void render (float* out, int numSamples)
    {
        const float k = params.resonance * 3.95f;
        const float resoMakeup = 1.0f + 0.5f * k;
        const float driveGain = 1.0f + params.drive * 9.0f;
        const float driveMakeup = 1.0f / (1.0f + params.drive * 2.0f);
        const float targetLogCutoff = std::log2 (params.cutoffHz);
        const float envOctaves = params.envAmount * 7.0f;
        const float maxCutoff = 0.45f * fs;
        const int subWave = params.subWave == 0 ? 3 : 1;

        for (int i = 0; i < numSamples; ++i)
        {
            logCutoff = targetLogCutoff + (logCutoff - targetLogCutoff) * cutoffSmoothCoef;

            if (! amp.isActive())
            {
                out[i] = 0.0f;
                continue;
            }

            currentNote = targetNote + (currentNote - targetNote) * glideCoef;
            const float pitch = currentNote + bend + 12.0f * (float) params.octave;
            const float inc = std::min (0.45f, 440.0f * std::exp2 ((pitch - 69.0f) / 12.0f) / fs);

            const float osc = 0.5f * (osc1.next (inc, params.wave) + osc2.next (inc * detuneRatio, params.wave));
            const float x = osc + params.sub * subOsc.next (inc * 0.5f, subWave);

            const float fe = filterEnv.next() * velScale;
            const float cutoff = std::min (maxCutoff, std::exp2 (logCutoff + envOctaves * fe));
            const float g = std::tan (kPi * cutoff / fs);
            const float y = ladder.process (x * driveGain, g, k) * resoMakeup * driveMakeup;

            out[i] = y * amp.next() * velScale * params.gain;
        }
    }

private:
    static constexpr int kMaxHeld = 128;

    void removeHeld (int note)
    {
        auto end = std::remove (held, held + numHeld, note);
        numHeld = (int) (end - held);
    }

    BassParams params;
    float fs = 44100.0f;

    Oscillator osc1, osc2, subOsc;
    LadderFilter ladder;
    Envelope amp, filterEnv;

    int held[kMaxHeld] {};
    int numHeld = 0;

    float targetNote = 36.0f, currentNote = 36.0f;
    float glideCoef = 0.0f, detuneRatio = 1.0f, bend = 0.0f, velScale = 1.0f;
    float logCutoff = 8.6f, cutoffSmoothCoef = 0.0f;
};

} // namespace subforge
