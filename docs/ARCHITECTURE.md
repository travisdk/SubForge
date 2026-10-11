# SubForge – arkitektur

Dette dokument forklarer, hvordan SubForge er bygget op, og hvorfor de vigtigste valg er truffet. Det er skrevet, så du kan forklare og forsvare koden over for en kunde, en revisor eller en anden udvikler. Henvisninger som `Source/BassSynth.h:172` peger på stedet i koden (commit `11cb005`).

**Kort fortalt:** SubForge er en monofonisk bas-synth med en indbygget step-sequencer. Den er skrevet i C++17 oven på JUCE 8 og leveres som CLAP og VST3 (og en standalone-app til test) på Linux og Windows. Al lydkode er skrevet til projektet. Der bruges ingen eksterne DSP-biblioteker ud over JUCEs oversampler.

---

## 1. Overblik

```
 DAW / host
   │  MIDI, transport (tempo, position), parameterændringer
   ▼
 SubForgeProcessor (PluginProcessor.*)          ◄── lydtråden
   ├─ readParams()          parametre → BassParams (én gang pr. blok)
   ├─ buildVoiceMidi()      MIDI ind + SequencerEngine → én MIDI-strøm
   ├─ juce::dsp::Oversampling (4x)
   │     └─ BassSynth::render()  osc → filter → amp → distortion
   └─ mono → alle udgangskanaler
          ▲                         ▲
          │ atomics                 │ ChangeBroadcaster (asynkront)
 Pattern (16 steps)         SoundPresets / PatternPresets
          ▲                         ▲
          └──── MainView (GUI) ─────┘                ◄── beskedtråden
```

| Lag | Filer | Ansvar | Afhænger af JUCE? |
|---|---|---|---|
| DSP | `BassSynth.h` | Oscillatorer, filtre, envelopes, LFO, distortion, stemmen | **Nej**. Ren C++17 |
| Sequencer | `Sequencer.h`, `PatternLibrary.h` | Mønster, timing, swing, slide, 42 fabriksmønstre | **Nej** |
| Lyddata | `Patches.h`, `FilterResponse.h` | 89 presets, random-patch, niveaunormalisering, filterkurve til GUI | **Nej** |
| Plugin | `PluginProcessor.*` | Parametre, MIDI, transport, oversampling, tilstand | Ja |
| Presets | `SoundPresets.*`, `PatternPresets.*`, `PresetFiles.h` | Indlæs og gem lyde og mønstre, brugerfiler | Ja |
| GUI | `PluginEditor.*`, `MainView.*`, `Widgets.*`, `StepGrid.*`, `SubForgeLookAndFeel.*`, `AboutOverlay.*` | Alt, hvad brugeren ser | Ja |

**Hvorfor den opdeling:** DSP og sequencer er uafhængige af JUCE. De kan derfor testes med almindelige C++-programmer (`tests/`) uden at bygge pluginnet, og de kan flyttes til en anden ramme, hvis JUCE-licensen en dag ikke passer.

---

## 2. Tråde og realtidsregler

En plugin kører på mindst to tråde:

- **Lydtråden** kalder `processBlock()` (`Source/PluginProcessor.cpp:201`). Den må aldrig vente på låse, allokere hukommelse eller røre filer. Ellers kan lyden klikke eller falde ud.
- **Beskedtråden** kører GUI'en, preset-indlæsning og filhåndtering.

Sådan deler de data uden låse:

| Data | Skrives af | Læses af | Mekanisme |
|---|---|---|---|
| Parametre (knapper) | GUI, host-automation | Lydtråden | JUCEs `AudioProcessorValueTreeState`. Lydtråden læser `getRawParameterValue()`, som er `std::atomic<float>` |
| Mønstrets 16 steps | GUI (`StepGrid`), presets | Lydtråden | `Pattern` (`Source/Sequencer.h:43`) gemmer hvert step pakket i én `std::atomic<uint32_t>` (gate, accent, slide og tone i 32 bit, `pack()` linje 129). Et step læses eller skrives derfor altid helt |
| Afspillet step (playhead) | Lydtråden | GUI | `std::atomic<int> currentStep` (`Sequencer.h:282`). GUI'en spørger 30 gange i sekundet (`MainView::timerCallback`, `MainView.cpp:346`) |
| "Preset ændret" | Preset-klasserne | GUI | `juce::ChangeBroadcaster`. Beskeden leveres asynkront på beskedtråden |

**Allokering på lydtråden:** `voiceMidi` får plads til 8 KB MIDI i `prepareToPlay()` (`PluginProcessor.cpp:164`), så buffer-tilføjelser normalt ikke allokerer. Oversamplerens buffere oprettes også her. `BassSynth` bruger kun faste arrays.

**Kendt begrænsning:** `getStateInformation()` læser preset-navnene (`juce::String`) uden lås. De fleste hosts kalder den på beskedtråden, hvor det er sikkert. En host, der gemmer projektet fra en anden tråd, mens brugeren samtidig skifter preset, kan i teorien få et forkert navn gemt. Selve lyden påvirkes ikke.

---

## 3. Signalvejen (`Source/BassSynth.h`)

`BassSynth::render()` (linje 614) beregner én sample ad gangen. Hele kæden kører med **4x oversampling**:

```
 osc1 + osc2 (detune) ──┐
                        ├─► + sub-osc ─► drive ─► filter ─► reso-makeup ─► + click ─► × amp-env × velocity × accent ─► distortion ─► × volume
 pitch: tone + glide + bend + oktav + vibrato + pitch-envelope
 cutoff: knap + filter-env + accent(+sweep) + LFO + key track + modhjul/aftertouch
```

### 3.1 Oscillatorer (`Oscillator`, linje 122)

- Fem bølgeformer: saw, square, pulse 25 %, sine og triangle.
- **Anti-aliasing:**
  - Saw og firkant får et polynomielt korrektionsled ved hvert spring (`stepResidual`, linje 86, PolyBLEP-metoden). Udledningen står i kommentaren over funktionen.
  - Triangle får den integrerede udgave (`polyBlamp`, linje 107) i knækkene.
  - `tests/test_oscillator.cpp` måler, at fejlen er mindst 6 dB (saw/firkant) og 2 dB (triangle) lavere end de naive bølgeformer.
- **Hvorfor PolyBLEP:** Det er billigt (et par multiplikationer pr. spring), har ingen tabeller og fungerer sammen med oversamplingen.

### 3.2 Filtre

- **`LadderFilter`** (linje 172): 4-polet transistor-ladder, løst som **zero-delay feedback** (TPT-metoden fra Zavalishins *The Art of VA Filter Design*).
  - Feedback-sløjfen beregnes for den aktuelle sample i stedet for med en samples forsinkelse. Derfor passer cutoff og resonans til knappen også ved høje frekvenser, og filteret kan stå stabilt i selvsvingning.
  - `tanh` på indgangen giver blød mætning.
  - LP12 og BP12 tages fra mellemtrinnene.
- **`DiodeLadderFilter`** (linje 208): diode-ladderen fra klassiske acid-baslinjer. Trinene belaster hinanden, så de fire tilstande løses samlet som et **tridiagonalt ligningssystem** for hver sample.
  - Konstanterne `kPeakRatio`, `kCriticalFeedback` og `kPassbandGain` er målt på modellen og står i kommentaren.
  - Et 60 Hz highpass i feedbacken gør resonansen tyndere ved lave frekvenser, som i originalerne.
- **Cutoff-udglatning:** Cutoff glattes i log-domænet over ca. 5 ms (`cutoffSmoothCoef`), så automation ikke giver lynlyde.
- **GUI-kurven** (`Source/FilterResponse.h`) er den analytiske overføringsfunktion for de samme modeller. `tests/test_filter_curve.cpp` kontrollerer, at den stemmer med det målte filter inden for 0,5 dB.

### 3.3 Envelopes, accent og glide

- **`Envelope`** (linje 407): eksponentiel ADSR, hvor tiden betyder "tid til -60 dB".
- **Accent:** Velocity ≥ `kAccentVelocity` sætter accent. Den åbner filteret og hæver niveauet. Med *Accent Sweep* lader hurtige accenter en "kondensator" op (`sweepState`), så filteret åbner mere og mere. Det er den karakteristiske acid-opførsel.
- **Mono-logik:** Nøgler, der holdes nede, gemmes i en stak (`held[128]`). Legato glider til den nye tone, og når den øverste tone slippes, glider stemmen tilbage til den, der stadig holdes.

### 3.4 Distortion (`Distortion`, linje 304)

- Fire typer: Overdrive, Distortion, Fuzz og Tube Amp. Hver består af en enkel kurve (tanh, kubisk eller asymmetrisk) plus et-polede tonefiltre. Der er **ingen kredsløbsmodeller** af bestemte pedaler.
- **Makeup-gain** fra en målt tabel (`makeupGain`) holder niveauet nogenlunde konstant hen over Gain-knappen.
- Distortion kører på det oversamplede signal, så overtonerne ikke folder tilbage som aliasing.

### 3.5 Oversampling (`PluginProcessor.cpp:201`)

- `juce::dsp::Oversampling` med 2 trin × 2 = **4x** og polyfase-IIR-halvbåndsfiltre. Stemmen forberedes med 4 × host-samplerate.
- **Latens:** Oversamplerens latens meldes til hosten med `setLatencySamples()`, så hosten kan kompensere.
- **MIDI-timing:** MIDI-hændelser placeres på den rigtige sample inden for blokken. Stemmen renderes i stykker mellem hændelserne, og positionerne ganges med 4.
- **Hvorfor IIR og ikke FIR:** lav latens og lavt CPU-forbrug. Faseforvrængningen høres ikke på en bas.
- Stemmen er mono. Til sidst kopieres den til alle udgangskanaler.

### 3.6 Niveau på fabrikspresets (`Patches.h:280`)

Når et fabrikspreset indlæses, renderer `normalizedVolumeDb()` en kort tone offline og sætter Volume, så alle presets spiller omtrent lige højt. Det sker på beskedtråden og ikke under afspilning. Brugerpresets gemmer deres Volume direkte og normaliseres ikke.

---

## 4. Sequencer (`Source/Sequencer.h`)

### 4.1 Timing

`SequencerEngine::process()` (linje 169) kører i starten af hver blok og udsender note on/off med sample-præcise positioner.

- **Låst til hostens tidslinje:** Positionen beregnes ud fra hostens PPQ (antal kvartnoder fra projektets start), ikke ud fra en intern tæller. Sequenceren står derfor altid rigtigt efter loop, spol eller tempoændringer.
- **Uafhængig af blokstørrelse:** En hændelse ligger på den første sample på eller efter sin beat-position (`sampleFor`, linje 195). Derfor rammer en step-start præcis samme sample, uanset om hosten bruger blokke på 64, 441 eller 1024 samples. `tests/test_sequencer.cpp` renderer de tre blokstørrelser og kræver identiske hændelser.
- **Swing** (`stepStart`, linje 229) forsinker hvert andet step inden for et par.
- **Slide:** Et step med slide får ingen note-off. Næste tone sendes *før* den forrige slippes, så stemmen glider legato i stedet for at starte forfra (linje 260).
- **Spring i tidslinjen** (loop eller spol) registreres (linje 184), og en klingende tone stoppes, så ingen toner hænger.

### 4.2 Transponering

Når sequenceren kører, spiller de taster, man holder nede, ikke selv. I stedet flytter de mønstrets grundtone (`buildVoiceMidi`, `PluginProcessor.cpp:264`). Det er den klassiske acid-arbejdsgang. Den senest trykkede tast vinder, og slippes alle, bruges *Root*-parameteren igen.

### 4.3 Mønstre

- Et step er gate, tone (-12…+24 halvtoner fra grundtonen), accent og slide.
- **Fabriksmønstre:** De 42 fabriksmønstre ligger i kompakt tekstform i `PatternLibrary.h`.
- **Generator:** Generatoren (`Pattern::randomize`, linje 101) laver nye linjer efter regler: skala, oktavspring, accent- og slide-sandsynlighed.

---

## 5. Parametre og tilstand

### 5.1 Parametre (`createParameterLayout`, `PluginProcessor.cpp:66`)

Alle parametre har et fast ID og version 1 (`ParameterID { id, 1 }`). **ID'erne må aldrig ændres eller genbruges.** Hosts gemmer automation og projekter under dem.

- **Lydparametre:** alle, hvis ID ikke starter med `seq`.
- **Sequencerparametre:** `seqOn`, `seqRate`, `seqLength`, `seqGate`, `seqSwing` og `seqRoot`.

Opdelingen bruges af preset-systemet (`SoundPresets::isSoundParameter`).

### 5.2 Projekttilstand (`getStateInformation`, `PluginProcessor.cpp:308`)

Hostprojektet gemmer én XML-blok med:
- alle parameterværdier (fra `AudioProcessorValueTreeState`)
- lydpresetets navn og indeks (`SoundPresets::writeState`)
- mønstrets steps som tekst og mønstrets navn (`PatternPresets::writeState`)

### 5.3 Plugin-identitet (`CMakeLists.txt`)

| Hvad | Værdi | Må ændres? |
|---|---|---|
| Producentkode og pluginkode | `Hjuh` / `SbFg` | **Aldrig.** VST3-klassens ID beregnes ud fra dem. Ændres de, finder eksisterende projekter ikke pluginnet |
| CLAP-ID | `dk.hjuhl.subforge` | **Aldrig**, af samme grund |
| Produktnavn, version | `SubForge`, `0.1.0` | Version: ja, ved hver udgivelse |

---

## 6. Presets og filer

Lyd og mønstre er **to uafhængige systemer**. Et lydpreset rører aldrig sequenceren, og et mønster rører aldrig lyden. Brugeren kan derfor skifte lyd under et mønster og omvendt.

| | Lydpresets (`SoundPresets.*`) | Mønstre (`PatternPresets.*`) |
|---|---|---|
| Fabriksindhold | 89 presets i `Patches.h` | 42 mønstre i `PatternLibrary.h` |
| Brugerfiler | `.sfpreset` (XML) | `.sfpattern` (XML) |
| Standardmappe | `Dokumenter/SubForge/Presets` | `Dokumenter/SubForge/Patterns` (søstermappe til presetmappen) |
| Indhold | Alle lydparametre som rene værdier | Steps, længde, swing, rate og gate. *Ikke* grundtonen, så mønstret følger tonearten |
| Andet | Randomize, Mutate. Hostens "programs" er fabrikspresets | Generate, Clear, kopier/indsæt som én tekstlinje |

- **Fælles filhjælpere** (`PresetFiles.h`): sorteret liste, sikkert filnavn, parameterændring med host-notifikation (`beginChangeGesture`/`endChangeGesture`), og sletning til papirkurven. På Linux skrives en `.trashinfo` efter freedesktop-specifikationen, så filen kan gendannes fra filhåndteringen.
- **Globale indstillinger:** accentfarve, UI-størrelse, sequencer vist eller skjult, og presetmappe. De gemmes i `SubForge.settings`:
  - Linux: `~/.config/SubForge/`
  - Windows: `%APPDATA%\SubForge\`

  Indstillingerne er fælles for alle instanser og hører ikke til projektet.
- **Ældre filer:** Filerne har et versionsnummer. Lydpresets fra version 1, som også indeholdt sequencerdata, indlæses fortsat. Sequencerdelen ignoreres.

---

## 7. GUI

- **`PluginEditor`** er en tynd ramme om **`MainView`**, som indeholder hele brugerfladen i en fast designstørrelse:
  - 920 × 680 med sequencer
  - 920 × 510, når sequencerens steps er skjult
- **Skalering (75–200 %):** Editoren skalerer `MainView` med en `AffineTransform` (`PluginEditor.cpp:52`). Alt tegnes som vektorer, så det er skarpt i alle størrelser. Størrelsen begrænses, så vinduet altid passer på skærmen (`largestFittingScale`).
- **Egen grafik:** Alle knapper, drejeknapper og felter tegnes af `SubForgeLookAndFeel` og `Widgets.*`. Der er ingen billedfiler, kun to indlejrede fonte (OFL).
  - Farvetema: én accentfarve, som brugeren vælger, plus en fast orange til accent-steps.
- **Bindinger:** Knapper er bundet til parametre med JUCEs `SliderAttachment` og `ParameterAttachment`. Ændringer fra GUI'en bliver derfor til host-automation, og automation flytter GUI'en.
- **`StepGrid`** tegner og redigerer de 16 steps direkte i `Pattern`. Det er sikkert, fordi stepsene er atomics, se §2.
- **`FilterCurve`** genberegner den analytiske filterkurve højst 30 gange i sekundet og kun, når en relevant parameter har ændret sig.

---

## 8. Build, test og CI

### 8.1 Build (`CMakeLists.txt`)

- CMake henter JUCE 8.0.15 og clap-juce-extensions fra `~/SDKs` (kan ændres med `-DJUCE_DIR` og `-DCLAP_JUCE_EXTENSIONS_DIR`).
- **Formater:**
  - VST3 og Standalone kommer fra `juce_add_plugin`.
  - CLAP kommer fra `clap_juce_extensions_plugin`.
- **Kompileringsflag:**
  - `JUCE_DISPLAY_SPLASH_SCREEN=0` (tilladt under JUCE 8-licensen)
  - `JUCE_WEB_BROWSER=0` og `JUCE_USE_CURL=0` (færre afhængigheder)
- **Windows:** C++-runtime linkes statisk (`CMAKE_MSVC_RUNTIME_LIBRARY`, `/MT`). Pluginnet kræver derfor ingen Visual C++ Redistributable og bruger aldrig en ældre runtime-DLL, som hosten har indlæst.
- `SUBFORGE_INSTALL_AFTER_BUILD` kopierer pluginnet til brugerens pluginmapper efter hver build. CI slår det fra.
- **Pakker:**
  - `package-linux` laver en tar.gz.
  - `package-windows` laver en zip.

  Begge indeholder CLAP, VST3, `THIRD-PARTY-NOTICES.txt` og `INSTALL.txt`.

### 8.2 Test (`tests/`, kørt med `ctest`)

| Test | Kontrollerer |
|---|---|
| `test_presets` | 89 unikke navne, fornuftigt niveau, og at Randomize sjældent rammer grænserne |
| `test_patterns` | 42 mønstre, tekstformat frem og tilbage, afvisning af ugyldigt input |
| `test_sequencer` | Identisk timing ved tre blokstørrelser, ingen hængende toner, swing, loop-spring |
| `test_filter_curve` | GUI-kurven stemmer med filteret (±0,5 dB) |
| `test_oscillator` | Anti-aliasing virker (målt mod naive bølgeformer) |

Testene bruger kun C++-standardbiblioteket, fordi DSP og sequencer ikke afhænger af JUCE.

**Lyttetest:** Efter hver fase er der renderet en demo-WAV, som er sammenlignet bit for bit med den forrige. Ændringer, der kun rører GUI eller dokumentation, skal give en identisk fil.

### 8.3 CI (`.github/workflows/build.yml`)

Ved hvert push bygger GitHub Actions på `ubuntu-24.04` (GCC) og `windows-2022` (MSVC). Hvert build gør følgende:
1. Kører testene.
2. Kontrollerer, at `THIRD-PARTY-NOTICES.txt` passer til licensfilerne (`--check`).
3. Laver pakken og lægger den op som artifact.

JUCE- og clap-juce-extensions-versionerne er låst i workflowet.

---

## 9. Designvalg kort begrundet

| Valg | Hvorfor | Alternativ fravalgt |
|---|---|---|
| JUCE 8 | Modent, dækker GUI, parametre og VST3/standalone på tværs af platforme. Gratis under Starter-licensen op til 20.000 USD | Egen ramme (meget arbejde), iPlug2 (mindre økosystem) |
| CLAP via clap-juce-extensions (MIT) | JUCE 8 har ikke CLAP indbygget. Wrapperen er den gængse løsning | – |
| Mono-stemme | En bas-synth i acid-stil er mono. Det gør glide, legato og accent enkle og billige | Polyfoni (giver ingen mening for formålet) |
| Zero-delay-feedback-filtre | Korrekt cutoff og resonans, stabil selvsvingning, god modulation | Simple filtre med en samples forsinkelse (ustemte ved høj resonans) |
| 4x IIR-oversampling | Mindre aliasing fra filter og distortion med lav latens | Ingen oversampling (aliasing), FIR (højere latens) |
| Atomics frem for låse | Lydtråden må aldrig vente | Mutex (risiko for udfald) |
| Sequencer låst til hostens PPQ | Altid i takt med DAW'en, også efter loop og spol | Intern tæller (driver og falder ud af takt) |
| Lyd og mønstre adskilt | Brugeren kan kombinere frit, og ingen af dem overskriver den anden | Ét samlet preset (forvirrende) |
| DSP uden JUCE | Testbar uden plugin-build. Mindre bundet til én leverandør | – |
