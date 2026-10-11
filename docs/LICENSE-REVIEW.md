# Licensgennemgang – SubForge

**Dato:** 2026-10-11 (gennemgået igen i fase 6, efter VST3 (fase 4) og Windows-forberedelse (fase 5). Opdateret samme dag til udgivelse som open source under AGPLv3, se §5.1 og §6)
**Omfang:** hele repoet, de biblioteker der bygges ind, og de færdige binærer `SubForge.clap` og `SubForge.vst3` bygget mod JUCE 8.0.15 – både Linux-udgaverne (lokal build, commit `17d16ff`) og Windows-udgaverne fra CI (GitHub Actions run 38084238176, MSVC, samme commit)
**Metode:** licenser er læst fra pakkernes egne licensfiler, fra Debian-copyrightfiler for systembiblioteker, fra JUCE 8-EULA'en (juce.com/legal/juce-8-licence, læst 2026-10-10) og fra `strings`/`ldd` på de byggede Linux-binærer og `objdump -p` (DLL-imports) på Windows-binærerne. Hvor noget ikke kunne afgøres, står der **ukendt**.

> **Beslutning 2026-10-11:** SubForge sælges ikke. Det udgives gratis som open source under **GNU AGPLv3** (`LICENSE`), og JUCE bruges under AGPLv3 i stedet for JUCE 8 Starter. Afsnit, der handler om salg med lukket kildekode, er bevaret som historik og markeret.
>
> Dette er en teknisk gennemgang, ikke juridisk rådgivning.

---

## 1. Afhængigheder

Projektet bruger **ingen pakkehåndtering** (ingen NuGet, npm, pip, vcpkg eller Conan). Afhængighederne ligger som kildekode i `~/SDKs` og bygges ind via CMake (`JUCE_DIR` og `CLAP_JUCE_EXTENSIONS_DIR` i `CMakeLists.txt`). Licenserne nedenfor er læst fra filerne i de mapper. CI (`.github/workflows/build.yml`) henter de **samme versioner** (JUCE-tag `8.0.15`, clap-juce-extensions-commit `7adee3a` med submoduler), så Windows- og Linux-pakkerne fra CI indeholder de samme komponenter.

### 1a. Biblioteker der bygges ind i pluginnet

| Navn | Version | Licens (kilde) | Bruges til |
|---|---|---|---|
| JUCE – alle brugte moduler (`juce_core`, `juce_events`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`, `juce_data_structures`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`, `juce_audio_processors` (+ `_headless`), `juce_audio_utils`, `juce_dsp`) | **8.0.15** (`git describe`) | **AGPLv3 eller JUCE 8 EULA** (`JUCE-8.0.15/LICENSE.md`: "dual-licensed under the AGPLv3 and the commercial JUCE licence"). I JUCE 8 er der **ingen ISC-moduler** længere | Grundlag, GUI, parametre, plugin-ramme, oversampling |
| clap-juce-extensions | commit `7adee3a` (2026-10-06) | **MIT** (`LICENSE.md`, "Copyright 2019-2020, Paul Walker") | CLAP-wrapper rundt om JUCE-processoren |
| CLAP SDK | 1.2.7 (`git describe`) | **MIT** (`LICENSE`, "Copyright (c) 2021 Alexandre BIQUE") | CLAP-headere |
| clap-helpers | commit `a61bcdf` (2025-11-28) | **MIT** (`LICENSE`, "Copyright (c) 2021 Alexandre BIQUE") | Hjælpeklasser i CLAP-wrapperen |
| Chakra Petch (SemiBold, Bold) | 1.0 (fra fontens navnetabel) | **SIL OFL 1.1** (`Resources/Fonts/ChakraPetch/OFL.txt`, "Copyright 2018 The Chakra Petch Project Authors"). Hentet fra Googles officielle font-repo (`github.com/google/fonts`, `ofl/chakrapetch`). Ingen reserverede fontnavne | Etiketter og overskrifter, indlejret via BinaryData |
| JetBrains Mono (Medium) | 2.305 (fra fontens navnetabel) | **SIL OFL 1.1** (`Resources/Fonts/JetBrainsMono/OFL.txt`, "Copyright 2020 The JetBrains Mono Project Authors"). Hentet fra `github.com/JetBrains/JetBrainsMono`. Ingen reserverede fontnavne | Parameterværdier, indlejret via BinaryData |

### 1b. Tredjepartskode som JUCE 8 bygger med ind

Listen bygger på `JUCE-8.0.15/LICENSE.md` ("The JUCE Framework Dependencies"), modulernes licensfiler og `strings` på den færdige `SubForge.clap`:

| Komponent | Ligger i | Licens (fra licensfilen) | Med i `SubForge.clap`? |
|---|---|---|---|
| **HarfBuzz** (ny i JUCE 8) | `juce_graphics/fonts/harfbuzz` | **"Old MIT"** (`COPYING`) | **Ja**: 852 `hb_`-spor |
| **SheenBidi** (ny i JUCE 8) | `juce_graphics/unicode/sheenbidi` | **Apache-2.0** (`LICENSE`). Ingen NOTICE-fil. JUCEs ændringer står i `JUCE_CHANGES.txt` | **Sandsynligvis**: 1 spor fundet. Bruges af JUCE 8's tekstmotor |
| IJG libjpeg | `juce_graphics/image_formats/jpglib` | **IJG-licens** (README, samme tekst som i JUCE 7): binær distribution kræver, at dokumentationen skriver *"this software is based in part on the work of the Independent JPEG Group"* | **Ja**: `JPEGMEM` |
| libpng | `juce_graphics/image_formats/pnglib` | **PNG Reference Library License v2** (`LICENSE`). *JUCEs `LICENSE.md` kalder den "zlib", men selve licensfilen er PNG v2. Rapporten følger filen* | **Ja**: `libpng error` |
| zlib | `juce_core/zip/zlib` | **zlib-licens** (README) | **Ja**: symboler i `juce::zlibNamespace` (`inflate_table` m.fl.) |
| VST3 SDK 3.8.0 | `juce_audio_processors_headless/format_types/VST3_SDK` | **MIT** (`LICENSE.txt`). Navnet "VST" og VST-logoet er Steinbergs varemærker. Brugen i markedsføring styres af `VST3_Usage_Guidelines.pdf` i samme mappe | **Ja, i VST3-udgaven** (`SubForge.vst3`, 636 `Steinberg`-spor). Ikke i CLAP-udgaven |
| FLAC, Ogg Vorbis | `juce_audio_formats/codecs` | BSD (ifølge JUCEs `LICENSE.md`) | **Nej**: fjernet af linkeren |
| CHOC/QuickJS, Box2D, GLEW, Oboe, AAX, ASIO, AudioUnitSDK, LV2 | diverse moduler | ISC/MIT, zlib, BSD/MIT, Apache-2.0, proprietær/GPLv3, proprietær/GPLv3, Apache-2.0, ISC | **Nej**: modulerne bruges ikke, eller funktionerne er slået fra. AAX og ASIO er **ikke** i brug |

### 1c. Systembiblioteker (linket dynamisk, *ikke* distribueret med pluginnet)

Fra `ldd` på JUCE 8-binæren. Licenser fra `/usr/share/doc/<pakke>/copyright`:

| Bibliotek | Licens | Bemærkning |
|---|---|---|
| libfreetype.so.6 | FTL (eller GPLv2, dobbeltlicens) | |
| libfontconfig.so.1 (ny i JUCE 8) | Permissiv licens i X11/MIT-stil ("Copyright © 2001,2003 Keith Packard …") | Debian-filen er ikke maskinlæsbar. Licensen er aflæst fra teksten |
| libexpat.so.1 (ny i JUCE 8) | MIT | Trækkes ind af fontconfig |
| libpng16.so.16 | libpng / expat | Trækkes ind af FreeType |
| libz.so.1 | Zlib | |
| libbz2.so.1.0 | BSD-variant | Debian-filen nævner også GPL-2, sandsynligvis for hjælpefiler |
| libbrotli | MIT | |
| libasound.so.2 (kun standalone) | LGPL-2.1+ | Dynamisk linkning: OK |
| libstdc++, libgcc_s | GPL-3 med **GCC Runtime Library Exception** (`/usr/share/doc/libstdc++6/copyright`) | Undtagelsen tillader at distribuere programmer bygget med GCC under enhver licens |
| libc, libm | **LGPL-2.1** (`/usr/share/doc/libc6/copyright`) | Dynamisk linkning: OK |

### 1c-2. Windows: systembiblioteker og C++-runtime (fra `objdump -p` på CI-binærerne)

`SubForge.clap` og `SubForge.vst3` (Windows) importerer de samme DLL'er:

| DLL'er | Hvad | Licens / bemærkning |
|---|---|---|
| `KERNEL32`, `USER32`, `GDI32`, `ADVAPI32`, `SHELL32`, `SHLWAPI`, `ole32`, `OLEAUT32`, `COMCTL32`, `COMDLG32`, `IMM32`, `WINMM`, `WININET`, `WS2_32`, `dwmapi`, `d2d1`, `d3d11`, `dxgi`, `dcomp`, `api-ms-win-shcore-scaling-*` | Windows' egne systembiblioteker | Del af Windows. Distribueres ikke |
| `api-ms-win-crt-*` | Universal C Runtime | Del af Windows 10 og nyere. Distribueres ikke |
| `MSVCP140.dll`, `VCRUNTIME140.dll`, `VCRUNTIME140_1.dll` | Visual C++ 2015–2022-runtime | Fundet i buildet fra commit `17d16ff`. **Ikke en licensrisiko**, men et brugerproblem: Runtime skulle være installeret på brugerens pc. **Løst efter fase 6:** Runtime bygges nu statisk ind i pluginnet (§5.6), så disse DLL'er ikke længere importeres |

Ingen GPL- eller LGPL-biblioteker indgår i Windows-udgaverne.

### 1d. Byggeværktøjer (distribueres ikke)
- **Linux, lokalt:** CMake 3.28.3 og GCC 13.3.
- **CI:** GitHub Actions på `ubuntu-24.04` (GCC, Ninja) og `windows-2022` (MSVC). Actions `actions/checkout`, `actions/setup-python` og `actions/upload-artifact` er MIT. De bruges kun til at bygge og distribueres ikke.
- `scripts/generate-third-party-notices.py` bruger kun Pythons standardbibliotek.
- Testene i `tests/` bruger kun C++-standardbiblioteket.

---

## 2. Kode med mulig fremmed oprindelse

**Grundlag:**
- `grep` efter `copyright`, `license`, `http`, `www.`, `github`, `based on`, `adapted`, `ported` og navne på kendte projekter gav **ingen** fremmede copyright- eller licensheadere og **ingen** links til eksterne kilder i `Source/` eller `CMakeLists.txt`.
- Al kode i `Source/` er skrevet i dette projekt af en AI-assistent (Claude) i samarbejde med ejeren. Jeg kender ikke til, at noget er kopieret ordret fra andre projekter. En AI-model kan dog ikke helt udelukke, at korte, meget udbredte kodestykker ligner noget fra dens træningsdata.
- Migreringen til JUCE 8 ændrede kun to linjer i `Source/` (`FontOptions` i `SubForgeLookAndFeel.h`).
- **Fase 6, ny søgning (2026-10-11):** Den samme `grep` over `Source/`, `tests/`, `scripts/` og `CMakeLists.txt` fandt kun projektets egne henvisninger (til denne rapport og til licensfilerne, som scriptet læser). Der er ikke tilføjet afhængigheder siden sidste gennemgang. Fase 4 og 5 tilføjede kun JUCEs egen VST3-understøttelse, CI-opsætning og egen kode.
- **Varemærkeord:** En søgning efter kendte produkt- og firmanavne (Moog, Roland, TB-303, Big Muff, Tube Screamer m.fl.) fandt kun én kommentar i `Source/BassSynth.h:374` med et pedal-varemærke. Den kom ikke med i binæren eller GUI'en, men er nu omformuleret til "Scooped fuzz tone" (§5.5). Ny søgning efter rettelsen finder ingen varemærkeord i `Source/`, `tests/` eller `packaging/`. `kDiode303` er et internt enum-navn, som vises som "Diode".

| Fil / linjer | Hvad | Sandsynlig kilde | Licens | Vurdering |
|---|---|---|---|---|
| `Source/BassSynth.h` `stepResidual()`, `stepResidualAtWrap()` (tidl. `polyBlep()`) | Anti-aliasing af saw og firkant | **Omskrevet i fase 3** ud fra specifikationen i §5.2. Udledningen står trin for trin i kodekommentaren. Den tidligere version var identisk med en udbredt tutorial-formulering. **Forbehold:** Omskrivningen er lavet af den samme AI-assistent, som skrev den tidligere version, og den var derfor ikke et strengt clean room. Resultatet er numerisk identisk med den tidligere version (forskel ≤ 6·10⁻⁸), fordi matematikken entydigt bestemmer det. Koden har nu sin egen struktur og navngivning | Selv genereret ud fra publiceret matematik | OK |
| `Source/BassSynth.h` 107–119 `polyBlamp()` | Anti-aliasing af trekant | Udledt i projektet som integralet af `polyBlep`. Svarer til formler i litteraturen. Fortegnet er verificeret numerisk | Selv genereret | OK |
| `Source/BassSynth.h` 122–170 `Oscillator` | Bølgeformer | Selv skrevet | Selv genereret | OK |
| `Source/BassSynth.h` 172–203 `LadderFilter` | 4-polet ladder-filter, zero-delay feedback | Metoden er fra Vadim Zavalishins bog *The Art of VA Filter Design*. Koden er skrevet ud fra metoden, ikke kopieret. LP12/BP12-koefficienterne er Oberheim Xpander-teknikken fra litteraturen | Selv genereret | OK |
| `Source/BassSynth.h` 208–276 `DiodeLadderFilter` | Diode-ladder i 303-stil | Ligningssystemet og konstanterne er udledt og målt i projektet. Dominique Wurtz' open source-implementering blev overvejet fra hukommelsen, men **ikke brugt** | Selv genereret | OK |
| `Source/BassSynth.h` 279–405 `OnePole`, `Distortion` | Distortion-typer | Selv skrevet. Kun *inspireret* af pedalnavne, ingen kredsløbsmodeller | Selv genereret | OK |
| `Source/BassSynth.h` 407–738 | Envelopes, LFO, stemme | Selv skrevet | Selv genereret | OK |
| `Source/Sequencer.h`, `Source/PatternLibrary.h` | Sequencer, mønstre, generator | Selv skrevet | Selv genereret | OK |
| `Source/PresetFiles.h` `moveToTrash()` | Papirkurv | Linux: skrevet ud fra freedesktop.org's *Trash Specification*. Ingen kode kopieret. Andre platforme: JUCEs `File::moveToTrash()` | Selv genereret | OK |
| Øvrige `Source/*` (`MainView`, `Widgets`, `StepGrid`, `FilterResponse.h`, `SoundPresets`, `PatternPresets` m.fl.) | Plugin, GUI, presets | Almindelig brug af JUCEs API | Selv genereret (bundet til JUCE-licensen, se §4) | OK |
| `tests/*.cpp` | Enhedstest | Selv skrevet, kun C++-standardbiblioteket | Selv genereret | OK |
| `.github/workflows/build.yml`, `scripts/*.py` | CI og notices-generator | Selv skrevet | Selv genereret | OK |
| `CMakeLists.txt` `clap_juce_extensions_plugin(...)` | Byggeopsætning | Kaldets form blev set i Vaporizer2 (GPL). Det er wrapperens dokumenterede API, ikke beskyttet udtryk | Selv genereret | OK |

---

## 3. MIDI-mønstre og andet indhold

**Der findes ingen MIDI-filer (`.mid`) i projektet.** Alt musikalsk indhold ligger som data i kildekoden. **Ingen** af det er hentet fra hjemmesider eller andre kilder.

| Indhold | Placering | Oprindelse | Kildesidens licens | Generisk eller ligner kendt materiale? |
|---|---|---|---|---|
| 89 synth-presets | `Source/Patches.h` | **Selv genereret** | – | Generiske indstillinger. **Neutrale navne:** 15 presets, der direkte eller indirekte hentydede til varemærker eller kunstnernavne (Moog, TR-808, Big Muff, Taurus, Hoover, Tube Screamer, "Reese"), er omdøbt (fx Fat Ladder, Long Sub, Woolly Fuzz, Detuned Saw). Indstillingerne er uændrede |
| 42 sequencer-mønstre | `Source/PatternLibrary.h` | **Selv genereret** (komponeret ud fra genrebeskrivelser) | – | **Generiske genrefigurer:** Goa Offbeat, Psy Roller, Disco Octaves, Octave Bounce, Body Drive, Slide Riser, Pentatonic Run, Lonely Note, Twelve Step Tresillo (traditionel rytme). **Acid-stil (de øvrige):** lighed med kendte numre er **ikke systematisk kontrolleret**. Korte 16-trins figurer kan ligne eksisterende numre tilfældigt. Ingen er bevidst skrevet efter et bestemt nummer |
| Standardmønster | `Source/Sequencer.h` `setDefault()` | **Selv genereret** | – | Acid-stil, se ovenfor |
| Mønstergenerator (Random) | `Source/Sequencer.h` `randomize()` | **Selv genereret**, regelbaseret | – | Genererer nye mønstre |
| Chakra Petch- og JetBrains Mono-fonte | `Resources/Fonts/ChakraPetch/*.ttf`, `Resources/Fonts/JetBrainsMono/*.ttf` | Officielle repos (se §1a) | **SIL OFL 1.1**: må indlejres og distribueres kommercielt sammen med software. Må ikke sælges alene. Licensteksten skal følge med | – |
| OFL-licenstekster | `Resources/Fonts/ChakraPetch/OFL.txt`, `Resources/Fonts/JetBrainsMono/OFL.txt` | Fra de samme repos som fontene | – | – |
| GUI-designreference | `docs/design/syre.png` | Leveret af ejeren | – | Kun til udvikling. Distribueres ikke med pluginnet |
| Demo-WAV'er | `build/`, `build-juce8/` | **Selv genereret** | – | Udeladt via `.gitignore`. Distribueres ikke |

---

## 4. Klassificering

| Fund | Klasse | Begrundelse |
|---|---|---|
| **JUCE 8.0.15** (alle brugte moduler) | **STÆRK COPYLEFT** (AGPLv3) | Se §5.1. SubForge står derfor selv under AGPLv3, og kildekoden er offentlig |
| clap-juce-extensions, CLAP SDK, clap-helpers | **OK** | MIT. Copyright-teksten skal følge med |
| HarfBuzz | **OK** | "Old MIT" (permissiv) |
| SheenBidi | **OK** | Apache-2.0. Licensteksten skal følge med |
| IJG libjpeg | **OK** (permissiv) | Kræver attribution i dokumentationen |
| libpng | **OK** (permissiv) | PNG Reference Library License v2 |
| zlib (i binæren) | **OK** (permissiv) | zlib-licens. Står allerede i `THIRD-PARTY-NOTICES.txt` |
| VST3 SDK 3.8.0 (i VST3-udgaven) | **OK** | MIT (følger med i JUCE 8). Varemærket "VST" er ikke en licensbetingelse, men se §6 |
| FLAC, Vorbis, AAX, ASIO m.fl. | **Ikke relevant** | Ikke med i binæren |
| Systembiblioteker (dynamiske) | **OK** | Distribueres ikke |
| libstdc++/libgcc (Linux) | **OK** | GCC Runtime Library Exception (aflæst i fase 6) |
| libc/libm (Linux) | **OK** | LGPL-2.1, dynamisk linket, distribueres ikke |
| Windows-systembiblioteker og UCRT | **OK** | Del af Windows |
| Visual C++-runtime (`MSVCP140` m.fl.) | **OK** (licens), men se §5.6 | Kræver, at brugeren har runtime installeret |
| Chakra Petch og JetBrains Mono | **SVAG COPYLEFT** (kun selve fontene) | OFL kræver kun, at fontene forbliver under OFL, og at licensteksterne følger med. Påvirker ikke din kode |
| `stepResidual()` (tidl. `polyBlep()`) | **OK** | Omskrevet ud fra specifikationen (§5.2). Se forbeholdet om clean room i §2 |
| Al øvrig kode i `Source/` | **OK** | Selv genereret |
| Presets og mønstre | **OK** | Selv genereret, neutrale navne (lighedsforbehold for mønstre, se §3) |

---

## 5. Erstatningsforslag

### 5.1 JUCE – AGPLv3 (valgt 2026-10-11)

**Valgt løsning:** SubForge bygges mod **JUCE 8.0.15** under **AGPLv3**, og SubForge udgives selv under AGPLv3 (`LICENSE`, den officielle tekst fra gnu.org). JUCE skriver i `LICENSE.md`, at modulerne er "dual-licensed under the AGPLv3 and the commercial JUCE licence". Der skal derfor ikke tegnes nogen JUCE-licens, og der er ingen omsætningsgrænse.

Det betyder:

| Krav i AGPLv3 | Sådan opfyldes det |
|---|---|
| Hele programmet (SubForge + JUCE) skal stå under AGPLv3, når det distribueres | `LICENSE` i repoet, `LICENSE.txt` i begge pakker, note i README, `INSTALL.txt`, `THIRD-PARTY-NOTICES.txt` og "About SubForge" |
| Modtagere skal kunne få den tilsvarende kildekode (§6) | Repoet er offentligt (`github.com/travisdk/SubForge`), og hver release har et git-tag. README og CI angiver de præcise versioner af JUCE (tag `8.0.15`) og clap-juce-extensions (commit `7adee3a`), som hentes fra deres officielle repos. *Forbehold:* Strengt taget skal du sikre, at kildekoden er tilgængelig, så længe du tilbyder binærerne. Vil du være helt uafhængig af JUCEs og CLAP-projektets repos, kan du lægge et kildearkiv med SDK'erne ved hver release |
| Netværksbrug (§13, det der adskiller AGPL fra GPL) | Ikke relevant. SubForge tilbyder ingen netværkstjeneste |
| Ingen ekstra begrænsninger | Der er ingen EULA, licensnøgle eller kopibeskyttelse |
| Permissive komponenter (MIT, Old MIT, Apache-2.0, zlib, libpng, IJG) og fontene (OFL) | Alle kan indgå i et AGPLv3-program. Apache-2.0 (SheenBidi) er forenelig med GPLv3/AGPLv3. Deres licenstekster står i `THIRD-PARTY-NOTICES.txt` |
| Splashskærm | JUCE 8 har ingen splashskærm. JUCE ignorerer `JUCE_DISPLAY_SPLASH_SCREEN` med en compiler-advarsel (`juce_gui_basics.cpp:60`) |

**Historik (salg med lukket kildekode, fravalgt):** Den oprindelige plan var en **JUCE 8 Starter**-licens. Ifølge JUCE 8-EULA'en (læst 2026-10-10):

| Punkt | EULA-tekst | Betydning for SubForge |
|---|---|---|
| Pris og grænse | §1.2: Starter "Free", omsætning eller funding "Up to $20,000". §1.2.1: målt over de seneste 12 måneder | Gratis, så længe din **samlede** omsætning *eller funding* er højst 20.000 USD over 12 måneder. Ikke kun fra SubForge |
| Splashskærm | Ikke nævnt for Starter | `JUCE_DISPLAY_SPLASH_SCREEN=0` beholdes |
| Distribution | §1.9: ret til at "Distribute and sell Products where you are the Owner" | Salg med lukket kildekode følger af §1.9. Det står ikke eksplicit for Starter |
| Open source | §2.3: JUCE må ikke gøres underlagt licenser, der kræver kildekode | Ingen copyleft-afhængigheder må tilføjes (overholdes) |
| JUCE alene | §1.17: JUCE må ikke distribueres for sig selv | Overholdes |
| Versioner | §1.15: licensen dækker JUCE 8's mindre versioner og tidligere hovedversioner, men **ikke nye hovedversioner** | 8.0.15 er dækket. **JUCE 9** (findes, nyeste er 9.0.3) er **ikke** dækket af en JUCE 8-licens |

**Status:** Uklarheden fra JUCE 7-varianten (§1.15 over for EULA'ens indledning om JUCE 7-brugere) **udgår**, fordi SubForge nu bygges direkte mod JUCE 8.

**Overskrides grænsen på 20.000 USD:** Opgradér til en højere JUCE 8-licens (se priserne på juce.com). Ellers skal produktet stå under AGPLv3, og så skal kildekoden være åben.

**Indsats:** **lille**. Kun licenstegning hos JUCE, som du selv skal gøre.

### 5.2 `polyBlep()` – udført (fase 3)

**Clean room:** Skriv funktionen igen kun ud fra specifikationen, uden at se på den nuværende version eller tutorialen. Specifikationen: *"Træk et 2-punkts polynomielt residual fra den naive bølgeform omkring hver diskontinuitet. Residualet er differencen mellem et båndbegrænset step, tilnærmet ved integralet af en trekantkerne over én sample på hver side, og et ideelt step."* Bekræft resultatet med den numeriske test, der sammenligner med en båndbegrænset reference.

**Status:** Udført. `polyBlep()` er erstattet af `stepResidual()` og `stepResidualAtWrap()`. Den numeriske test viser 7,5–10,8 dB mindre fejl end de naive bølgeformer ved 440 Hz–3 kHz og samme lyd som før (pluginnets output er bit-identisk). Ønskes et strengt clean room, hvor implementøren aldrig har set originalen, kan funktionen skrives igen af en anden person eller en ny agent uden adgang til historikken.

### 5.3 Fontene Chakra Petch og JetBrains Mono (SVAG COPYLEFT, kun selve fontene)

Ingen erstatning er nødvendig. Krav:
- Begge `OFL.txt` skal følge med distributionen (de står i `THIRD-PARTY-NOTICES.txt`).
- Fonten må ikke sælges alene.
- Fontnavnene må ikke bruges som produktnavn.

**Indsats: lille.**

### 5.4 Attributionskrav – udført (fase 2)

`THIRD-PARTY-NOTICES.txt` genereres af `scripts/generate-third-party-notices.py` direkte fra licensfilerne (`--check` verificerer, at den stadig passer). Den vises i pluginnet under tandhjul → "About SubForge…" og ligger i både Linux-pakken (`package-linux`, tar.gz) og Windows-pakken (`package-windows`, zip). Kontrolleret i fase 6 i CI-pakken `SubForge-0.1.0-windows-x64.zip`. CI kører `--check` ved hver build, så filen ikke kan blive forældet uden at buildet fejler. Den indeholder de fulde licenstekster for:
- clap-juce-extensions, CLAP SDK og clap-helpers (MIT)
- HarfBuzz (Old MIT)
- SheenBidi (Apache-2.0)
- IJG libjpeg (inklusive den krævede sætning)
- libpng og zlib
- Chakra Petch og JetBrains Mono (OFL)
- VST3 SDK (MIT)

JUCE 8 har ingen ISC-moduler, så der er ingen JUCE-ISC-note. JUCE selv bruges under AGPLv3 (§5.1), og overskriften i `THIRD-PARTY-NOTICES.txt` siger det. Den indeholder også Steinbergs varemærkesætning.

**Indsats: lille.**

### 5.5 Mønstre og presets – udført

Alt er selv genereret. De 15 presetnavne med varemærkepræg er omdøbt til neutrale navne, og kommentarer i kildekoden beskriver nu acid-stilen uden varemærkenavne.

**Rest fundet i fase 6:** én kodekommentar i `Source/BassSynth.h:374` med et pedal-varemærke. **Udført efter fase 6 efter din beslutning:** Kommentaren hedder nu "Scooped fuzz tone". Kun kommentaren er ændret, og lyden er bit-identisk.

### 5.6 Visual C++-runtime på Windows (ikke licens, men salg og support) – udført

Windows-binærerne kræver Microsoft Visual C++ Redistributable (§1c-2). Mangler den, indlæses pluginnet ikke, og DAW'en viser typisk kun "kunne ikke indlæses". Det giver supporthenvendelser.

**Forslag:** Byg med statisk runtime (`set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")` i `CMakeLists.txt`, svarer til `/MT`). Det er almindelig praksis for plugins. Ifølge Microsofts licensvilkår for Visual Studio (afsnittet om "Distributable Code") må runtime bygges ind i dine egne programmer. Vilkårene stiller dog betingelser, fx at dit produkt tilfører væsentlig egen funktionalitet *(tjek de aktuelle vilkår)*. Der kræves ingen notits i `THIRD-PARTY-NOTICES.txt`. Alternativt kan `INSTALL-windows.txt` henvise til Microsofts download af Redistributable. **Indsats: lille** (én CMake-linje og en ny CI-build).

**Status:** Udført efter fase 6 efter din beslutning. `CMAKE_MSVC_RUNTIME_LIBRARY` er sat til `MultiThreaded` i `CMakeLists.txt`, før JUCE og CLAP-wrapperen indlæses, så alle dele bruger samme runtime. Linux påvirkes ikke. **Bekræftet** i CI-run 38137286377 (commit `e4120be`): `objdump -p` viser, at hverken `SubForge.clap` eller `SubForge.vst3` importerer `MSVCP140`, `VCRUNTIME140` eller `api-ms-win-crt-*`. Tilbage er kun Windows' egne systembiblioteker. Plugin-filerne voksede til ca. 7,5 MB (CLAP) og 7,8 MB (VST3).

**Licens for statisk runtime:** Microsofts Visual C++-runtime er "Distributable Code" under Visual Studio-licensen. Det gælder både, når den distribueres som DLL, og når den linkes statisk ind i dit eget program. Betingelserne er bl.a.:
- Dit program skal tilføje væsentlig egen funktionalitet. Det gør en synth.
- Du må ikke bruge Microsofts varemærker i dit produktnavn.
- Dine slutbrugervilkår må ikke give mindre beskyttelse af Microsofts kode end Microsofts egne vilkår. En almindelig EULA, der forbyder reverse engineering i det omfang loven tillader, dækker det.
- **Med AGPLv3 (2026-10-11):** Microsofts vilkår forbyder, at Distributable Code bliver underlagt en licens, der kræver kildekode ("Excluded License"). Visual C++-runtime hører under GPL-licensernes undtagelse for **systembiblioteker** (AGPLv3 §1, "System Libraries"), så den er ikke en del af SubForges kildekode og bliver ikke underlagt AGPLv3. Det er almindelig praksis for GPL-plugins på Windows. *Forbehold:* Det er ikke afprøvet i retten. Vil du helt undgå spørgsmålet, kan runtime linkes dynamisk igen (`/MD`), men så skal brugerne have Visual C++ Redistributable installeret *(tjek)*.

Du skal selv have ret til at bruge den compiler, der bygger. **Visual Studio Community** er gratis til kommerciel brug for enkeltpersoner. GitHub Actions' Windows-runner har Visual Studio installeret, og byggeriet dér sker under GitHubs vilkår for runner-billedet *(tjek begge dele)*.

---

## 6. Opsummering

**Udgivelse som open source (gældende fra 2026-10-11):** SubForge kan udgives gratis under **AGPLv3** med de medfølgende presets og mønstre. Der skal ikke tegnes nogen JUCE-licens. Kravene er opfyldt:

1. ✅ `LICENSE` (AGPLv3) i repoet og som `LICENSE.txt` i begge pakker, med licensnote i README, `INSTALL.txt`, `THIRD-PARTY-NOTICES.txt` og "About SubForge".
2. ✅ Kildekoden er tilgængelig i det offentlige repo med et tag pr. release, og SDK-versionerne står i README og CI (se forbeholdet i §5.1).
3. ✅ `THIRD-PARTY-NOTICES.txt` følger med og kontrolleres af CI.
4. ✅ VST-varemærket: "VST®" ved første omtale, Steinbergs "VST Compatible"-logo i README og ændringslog, og varemærkesætningen i dokumentationen og "About SubForge" (via `THIRD-PARTY-NOTICES.txt`). Se `docs/RELEASE-CHECKLIST.md`.

**Historik – salg med lukket kildekode (fravalgt). Kan SubForge sælges med lukket kildekode og de medfølgende mønstre? Ja, når to ting er på plads:**

1. **Du har tegnet JUCE 8 Starter-licensen** og bekræftet vilkårene på juce.com (§5.1). Det gælder kun, så længe din samlede omsætning eller funding er højst 20.000 USD over 12 måneder.
2. **`THIRD-PARTY-NOTICES.txt` følger med.** ✅ Udført i fase 2: den ligger i Linux- og Windows-pakken og vises i "About SubForge…". CI kontrollerer, at den er opdateret.

**Fase 3 er udført:** `polyBlep()` er omskrevet ud fra specifikationen (med forbeholdet i §2), og presetnavnene er neutrale.

Der er ikke fundet andet, der forhindrer salg med lukket kildekode. Fase 6-gennemgangen (2026-10-11) fandt ingen nye afhængigheder og ingen copyleft-kode, og Windows-udgaverne linker ikke GPL- eller LGPL-biblioteker.

**Begge anbefalinger fra fase 6 er udført:** Kommentarrettelsen (§5.5) og den statiske Visual C++-runtime (§5.6).

**Varemærke (ikke licens):** VST3-udgaven må distribueres under MIT-licensen. Bruger du ordet "VST" eller VST-logoet i markedsføring, på websider eller i pakken, skal Steinbergs retningslinjer i `VST3_Usage_Guidelines.pdf` følges: logoet på hver webside, der nævner VST, "VST®" ved første omtale og varemærkesætningen i dokumentation og credits. Logoet i `docs/images/` er hentet fra Steinbergs officielle repo `steinbergmedia/vst3_doc` (`artwork/`, MIT).
