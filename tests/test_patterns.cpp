// Factory patterns, the compact text form and the project-state string form.
#include "PatternLibrary.h"
#include "TestUtils.h"

#include <sstream>

using namespace subforge;

static bool samePattern (const Pattern& a, const Pattern& b)
{
    for (int i = 0; i < kMaxSteps; ++i)
        if (! (a.get (i) == b.get (i)))
            return false;
    return true;
}

int main()
{
    CHECK (kNumFactoryPatterns == 42);

    for (const auto& factory : kFactoryPatterns)
    {
        std::istringstream tokens (factory.steps);
        int count = 0;
        for (std::string token; tokens >> token;)
            ++count;
        CHECK (count == kMaxSteps);

        Pattern parsed, again;
        CHECK (parseCompactPattern (factory.steps, parsed));
        CHECK (parseCompactPattern (toCompactPattern (parsed).c_str(), again));
        CHECK (samePattern (parsed, again));
        CHECK (factory.length >= 1 && factory.length <= kMaxSteps);
    }

    std::mt19937 rng (7);
    for (int i = 0; i < 500; ++i)
    {
        Pattern original, restored;
        original.randomize (rng);
        restored.clear();
        CHECK (restored.fromString (original.toString()));
        CHECK (samePattern (original, restored));
    }

    Pattern p;
    CHECK (! p.fromString ("garbage"));
    CHECK (! p.fromString ("1:0:0:0 1:2"));
    CHECK (! parseCompactPattern ("hello world", p));
    CHECK (! parseCompactPattern ("0 0 0", p));

    return finish ("patterns");
}
