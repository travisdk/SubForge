// Sequencer engine: timing independent of the host block size, swing,
// no stuck notes after slides, loops or stopping.
#include "Sequencer.h"
#include "TestUtils.h"

#include <cstdlib>
#include <vector>

using namespace subforge;

struct Event
{
    long sample;
    bool on;
    int note;
};

static std::vector<Event> run (int blockSize, SequencerSettings settings, const Pattern& pattern, double seconds,
                               double loopAtBeat = -1.0)
{
    constexpr double sampleRate = 48000.0, bpm = 120.0;
    SequencerEngine engine;
    std::vector<Event> events;
    double ppq = 0.0;
    long t = 0;
    while (t < seconds * sampleRate)
    {
        if (loopAtBeat > 0.0 && ppq >= loopAtBeat)
        {
            ppq = 0.0;
            loopAtBeat = -1.0;
        }
        engine.process (true, ppq, bpm, sampleRate, blockSize, settings, pattern,
                        [&] (int s, bool on, int note, float) { events.push_back ({ t + s, on, note }); });
        ppq += blockSize * bpm / 60.0 / sampleRate;
        t += blockSize;
    }
    engine.process (false, 0.0, bpm, sampleRate, 64, settings, pattern,
                    [&] (int s, bool on, int note, float) { events.push_back ({ t + s, on, note }); });
    return events;
}

static int openNotes (const std::vector<Event>& events)
{
    int open = 0;
    for (const auto& e : events)
        open += e.on ? 1 : -1;
    return open;
}

int main()
{
    Pattern pattern; // default acid line with accents and slides
    SequencerSettings settings;
    settings.enabled = true;

    // identical event times for different block sizes (compare the common span)
    const auto a = run (64, settings, pattern, 4.0), b = run (1024, settings, pattern, 4.0), c = run (441, settings, pattern, 4.0);
    const long horizon = 180000;
    auto prefix = [&] (const std::vector<Event>& events)
    {
        std::vector<Event> out;
        for (const auto& e : events)
            if (e.sample < horizon)
                out.push_back (e);
        return out;
    };
    const auto pa = prefix (a), pb = prefix (b), pc = prefix (c);
    CHECK (pa.size() == pb.size() && pa.size() == pc.size());
    for (size_t i = 0; i < pa.size() && i < pb.size() && i < pc.size(); ++i)
    {
        CHECK (pa[i].sample == pb[i].sample && pa[i].on == pb[i].on && pa[i].note == pb[i].note);
        CHECK (pa[i].sample == pc[i].sample && pa[i].on == pc[i].on && pa[i].note == pc[i].note);
    }

    // no stuck notes at the end, after a loop jump, and at most two notes overlap (slides)
    CHECK (openNotes (a) == 0);
    CHECK (openNotes (run (512, settings, pattern, 3.0, 2.0)) == 0);
    int open = 0, maxOpen = 0;
    for (const auto& e : a)
    {
        open += e.on ? 1 : -1;
        maxOpen = std::max (maxOpen, open);
    }
    CHECK (maxOpen <= 2);

    // swing 0.66 at 1/16 and 120 bpm: odd steps start at 0.66 * 12000 samples
    Pattern straight;
    straight.clear();
    settings.swing = 0.66f;
    std::vector<long> ons;
    for (const auto& e : run (256, settings, straight, 0.5))
        if (e.on)
            ons.push_back (e.sample);
    CHECK (ons.size() >= 4);
    if (ons.size() >= 4)
    {
        CHECK (ons[0] == 0);
        CHECK (std::abs (ons[1] - 7920) <= 1);
        CHECK (ons[2] == 12000);
        CHECK (std::abs (ons[3] - 19920) <= 1);
    }

    return finish ("sequencer");
}
