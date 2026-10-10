// The GUI's filter curve (FilterResponse.h) must match the DSP filters: drive
// each filter with small sine waves and compare the measured gain.
#include "FilterResponse.h"
#include "TestUtils.h"

using namespace subforge;

static double measuredGain (int mode, float cutoff, float resonance, float hz)
{
    constexpr double fs = 192000.0;
    LadderFilter ladder;
    DiodeLadderFilter diode;
    const bool useDiode = mode == kDiode303;
    const float k = useDiode ? resonance * DiodeLadderFilter::kCriticalFeedback : resonance * 3.95f;
    const float makeup = useDiode ? 1.0f + 0.6f * resonance : 1.0f + 0.5f * k;
    const float g = useDiode ? std::tan (kPi * cutoff / (DiodeLadderFilter::kPeakRatio * (float) fs))
                             : std::tan (kPi * cutoff / (float) fs);
    const float highpass = std::tan (kPi * 60.0f / (float) fs);

    double sum = 0.0;
    int n = 0;
    const int total = (int) (fs * 0.6);
    for (int i = 0; i < total; ++i)
    {
        const float x = 0.001f * std::sin (2.0f * kPi * hz * (float) i / (float) fs);
        const float y = (useDiode ? diode.process (x, g, k, highpass) : ladder.process (x, g, k, mode)) * makeup;
        if (i > total / 2)
        {
            sum += (double) y * y;
            ++n;
        }
    }
    return std::sqrt (2.0 * sum / n) / 0.001;
}

int main()
{
    for (int mode = 0; mode < 4; ++mode)
        for (float resonance : { 0.0f, 0.7f })
            for (float hz : { 60.0f, 200.0f, 500.0f, 1000.0f, 2000.0f, 5000.0f })
            {
                const double measured = measuredGain (mode, 1000.0f, resonance, hz);
                if (measured < 0.01) // below -40 dB the comparison is noise
                    continue;
                const double curve = filterMagnitude (mode, 1000.0f, resonance, hz);
                CHECK (std::abs (20.0 * std::log10 (curve / measured)) < 0.5);
            }
    return finish ("filter_curve");
}
