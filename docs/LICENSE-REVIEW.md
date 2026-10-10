# Licensgennemgang – SubForge

**Dato:** 2026-10-10 (opdateret efter migrering til JUCE 8.0.15)
**Omfang:** hele repoet (branch `juce8`), de biblioteker der bygges ind, og den færdige binær `SubForge.clap` bygget mod JUCE 8.0.15
**Metode:** licenser er læst fra pakkernes egne licensfiler, fra Debian-copyrightfiler for systembiblioteker, fra JUCE 8-EULA'en (juce.com/legal/juce-8-licence, læst 2026-10-10) og fra `strings`/`ldd` på den byggede binær. Hvor noget ikke kunne afgøres, står der **ukendt**.

> ⚠️ **Det skal du selv gøre:** Bekræft vilkårene for **JUCE 8 Starter** på juce.com, og **tegn licensen**, før SubForge distribueres. Gennemgangen nedenfor bygger på EULA-teksten, som den så ud 2026-10-10. Vilkår, priser og omsætningsgrænser kan ændre sig.
>
> Dette er en teknisk gennemgang, ikke juridisk rådgivning. Inden et kommercielt salg bør konklusionerne bekræftes af en jurist med kendskab til softwarelicenser.

---

## 1. Afhængigheder

Projektet bruger **ingen pakkehåndtering** (ingen NuGet, npm, pip, vcpkg eller Conan). Afhængighederne ligger som kildekode i `~/SDKs` og bygges ind via CMake (`JUCE_DIR` og `CLAP_JUCE_EXTENSIONS_DIR` i `CMakeLists.txt`). Licenserne nedenfor er læst fra filerne i de mapper.

### 1a. Biblioteker der bygges ind i pluginnet

| Navn | Version | Licens (kilde) | Bruges til |
|---|---|---|---|
| JUCE – alle brugte moduler (`juce_core`, `juce_events`, `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`, `juce_data_structures`, `juce_audio_basics`, `juce_audio_devices`, `juce_audio_formats`, `juce_audio_processors` (+ `_headless`), `juce_audio_utils`, `juce_dsp`) | **8.0.15** (`git describe`) | **AGPLv3 eller JUCE 8 EULA** (`JUCE-8.0.15/LICENSE.md`: "dual-licensed under the AGPLv3 and the commercial JUCE licence"). I JUCE 8 er der **ingen ISC-moduler** længere | Grundlag, GUI, parametre, plugin-ramme, oversampling |
| clap-juce-extensions | commit `7adee3a` (2026-10-06) | **MIT** (`LICENSE.md`, "Copyright 2019-2020, Paul Walker") | CLAP-wrapper rundt om JUCE-processoren |
| CLAP SDK | 1.2.7 (`git describe`) | **MIT** (`LICENSE`, "Copyright (c) 2021 Alexandre BIQUE") | CLAP-headere |
| clap-helpers | commit `a61bcdf` (2025-11-28) | **MIT** (`LICENSE`, "Copyright (c) 2021 Alexandre BIQUE") | Hjælpeklasser i CLAP-wrapperen |
| Inter (Medium, Bold) | 4.0 (fra fontens navnetabel) | **SIL OFL 1.1** (`/usr/share/doc/fonts-inter/copyright`: "License: OFL-1.1"). Fonten indeholder teksten "Inter is a trademark of rsms" | Skrifttype i GUI'en, indlejret via BinaryData |

### 1b. Tredjepartskode som JUCE 8 bygger med ind

Listen bygger på `JUCE-8.0.15/LICENSE.md` ("The JUCE Framework Dependencies"), modulernes licensfiler og `strings` på den færdige `SubForge.clap`:

| Komponent | Ligger i | Licens (fra licensfilen) | Med i `SubForge.clap`? |
|---|---|---|---|
| **HarfBuzz** (ny i JUCE 8) | `juce_graphics/fonts/harfbuzz` | **"Old MIT"** (`COPYING`) | **Ja**: 852 `hb_`-spor |
| **SheenBidi** (ny i JUCE 8) | `juce_graphics/unicode/sheenbidi` | **Apache-2.0** (`LICENSE`). Ingen NOTICE-fil. JUCEs ændringer står i `JUCE_CHANGES.txt` | **Sandsynligvis**: 1 spor fundet. Bruges af JUCE 8's tekstmotor |
| IJG libjpeg | `juce_graphics/image_formats/jpglib` | **IJG-licens** (README, samme tekst som i JUCE 7): binær distribution kræver, at dokumentationen skriver *"this software is based in part on the work of the Independent JPEG Group"* | **Ja**: `JPEGMEM` |
| libpng | `juce_graphics/image_formats/pnglib` | **PNG Reference Library License v2** (`LICENSE`). *JUCEs `LICENSE.md` kalder den "zlib", men selve licensfilen er PNG v2. Rapporten følger filen* | **Ja**: `libpng error` |
| zlib | `juce_core/zip/zlib` | **zlib-licens** (README) | **Ukendt**: ikke entydigt fundet i binæren |
| VST3 SDK | `juce_audio_processors_headless/format_types/VST3_SDK` | **MIT** (`LICENSE.txt`) | **Nej**, ikke endnu: der bygges ikke VST3 (fase 4). Bliver relevant med VST3 |
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
| libstdc++, libgcc_s, libc, libm | **ikke aflæst** | Normalt GPL med GCC Runtime Library Exception og LGPL. Bør bekræftes, men er normalt uproblematisk ved dynamisk linkning |

### 1d. Byggeværktøjer (distribueres ikke)
CMake 3.28.3 og GCC 13.3.

---

## 2. Kode med mulig fremmed oprindelse

**Grundlag:**
- `grep` efter `copyright`, `license`, `http`, `www.`, `github`, `based on`, `adapted`, `ported` og navne på kendte projekter gav **ingen** fremmede copyright- eller licensheadere og **ingen** links til eksterne kilder i `Source/` eller `CMakeLists.txt`.
- Al kode i `Source/` er skrevet i dette projekt af en AI-assistent (Claude) i samarbejde med ejeren. Jeg kender ikke til, at noget er kopieret ordret fra andre projekter. En AI-model kan dog ikke helt udelukke, at korte, meget udbredte kodestykker ligner noget fra dens træningsdata.
- Migreringen til JUCE 8 ændrede kun to linjer i `Source/` (`FontOptions` i `SubForgeLookAndFeel.h`).

| Fil / linjer | Hvad | Sandsynlig kilde | Licens | Vurdering |
|---|---|---|---|---|
| `Source/BassSynth.h` 73–87 `polyBlep()` | Anti-aliasing af saw og firkant | Algoritmen er fra forskningen (PolyBLEP, Välimäki m.fl.). **Den præcise udformning** (`t + t - t*t - 1`, `t*t + t + t + 1`) er identisk med en meget udbredt tutorial-version, bl.a. på Martin Finkes blog ("Making Audio Plugins") | **Ukendt** for tutorial-koden. Stykket er 14 linjer og næsten bestemt af matematikken | **UKENDT** (lav risiko). Skrives om i fase 3 |
| `Source/BassSynth.h` 89–102 `polyBlamp()` | Anti-aliasing af trekant | Udledt i projektet som integralet af `polyBlep`. Svarer til formler i litteraturen. Fortegnet er verificeret numerisk | Selv genereret | OK |
| `Source/BassSynth.h` 104–151 `Oscillator` | Bølgeformer | Selv skrevet | Selv genereret | OK |
| `Source/BassSynth.h` 153–187 `LadderFilter` | 4-polet ladder-filter, zero-delay feedback | Metoden er fra Vadim Zavalishins bog *The Art of VA Filter Design*. Koden er skrevet ud fra metoden, ikke kopieret. LP12/BP12-koefficienterne er Oberheim Xpander-teknikken fra litteraturen | Selv genereret | OK |
| `Source/BassSynth.h` 189–258 `DiodeLadderFilter` | Diode-ladder i 303-stil | Ligningssystemet og konstanterne er udledt og målt i projektet. Dominique Wurtz' open source-implementering blev overvejet fra hukommelsen, men **ikke brugt** | Selv genereret | OK |
| `Source/BassSynth.h` 260–386 `OnePole`, `Distortion` | Distortion-typer | Selv skrevet. Kun *inspireret* af pedalnavne, ingen kredsløbsmodeller | Selv genereret | OK |
| `Source/BassSynth.h` 388–719 | Envelopes, LFO, stemme | Selv skrevet | Selv genereret | OK |
| `Source/Sequencer.h`, `Source/PatternLibrary.h` | Sequencer, mønstre, generator | Selv skrevet | Selv genereret | OK |
| `Source/PluginProcessor.cpp` `moveToTrash()` | Papirkurv på Linux | Skrevet ud fra freedesktop.org's *Trash Specification*. Ingen kode kopieret | Selv genereret | OK (bliver platformsuafhængig i fase 5) |
| Øvrige `Source/*.cpp` | Plugin, GUI, presets | Almindelig brug af JUCEs API | Selv genereret (bundet til JUCE-licensen, se §4) | OK |
| `CMakeLists.txt` `clap_juce_extensions_plugin(...)` | Byggeopsætning | Kaldets form blev set i Vaporizer2 (GPL). Det er wrapperens dokumenterede API, ikke beskyttet udtryk | Selv genereret | OK |

---

## 3. MIDI-mønstre og andet indhold

**Der findes ingen MIDI-filer (`.mid`) i projektet.** Alt musikalsk indhold ligger som data i kildekoden. **Ingen** af det er hentet fra hjemmesider eller andre kilder.

| Indhold | Placering | Oprindelse | Kildesidens licens | Generisk eller ligner kendt materiale? |
|---|---|---|---|---|
| 89 synth-presets | `Source/Patches.h` | **Selv genereret** | – | Generiske indstillinger. **Varemærkenær navngivning** (omdøbes i fase 3): "8-Oh-8 Sub", "8-Oh-8 Boom", "Moogie Classic", "Fat Moogy", "Fat 12dB Moogy", "Muffin Bass" |
| 42 sequencer-mønstre | `Source/PatternLibrary.h` | **Selv genereret** (komponeret ud fra genrebeskrivelser) | – | **Generiske genrefigurer:** Goa Offbeat, Psy Roller, Disco Octaves, Octave Bounce, Body Drive, Slide Riser, Pentatonic Run, Lonely Note, Twelve Step Tresillo (traditionel rytme). **Acid-stil (de øvrige):** lighed med kendte numre er **ikke systematisk kontrolleret**. Korte 16-trins figurer kan ligne eksisterende numre tilfældigt. Ingen er bevidst skrevet efter et bestemt nummer |
| Standardmønster | `Source/Sequencer.h` `setDefault()` | **Selv genereret** | – | Acid-stil, se ovenfor |
| Mønstergenerator (Random) | `Source/Sequencer.h` `randomize()` | **Selv genereret**, regelbaseret | – | Genererer nye mønstre |
| Inter-fonte | `Resources/Fonts/Inter-*.otf` | Systempakken `fonts-inter` | **SIL OFL 1.1**: må indlejres og distribueres kommercielt sammen med software. Må ikke sælges alene. Licensteksten skal følge med | – |
| OFL-licenstekst | `Resources/Fonts/OFL.txt` | Udtrukket fra `/usr/share/doc/fonts-inter/copyright` | – | – |
| Demo-WAV'er | `build/`, `build-juce8/` | **Selv genereret** | – | Udeladt via `.gitignore`. Distribueres ikke |

---

## 4. Klassificering

| Fund | Klasse | Begrundelse |
|---|---|---|
| **JUCE 8.0.15** (alle brugte moduler) | **Kommerciel licens (JUCE 8 Starter)**. Uden licens: **STÆRK COPYLEFT** (AGPLv3) | Se §5.1. Kræver, at du har tegnet Starter-licensen |
| clap-juce-extensions, CLAP SDK, clap-helpers | **OK** | MIT. Copyright-teksten skal følge med |
| HarfBuzz | **OK** | "Old MIT" (permissiv) |
| SheenBidi | **OK** | Apache-2.0. Licensteksten skal følge med |
| IJG libjpeg | **OK** (permissiv) | Kræver attribution i dokumentationen |
| libpng | **OK** (permissiv) | PNG Reference Library License v2 |
| zlib (muligvis i binæren) | **OK** (permissiv) | zlib-licens |
| VST3 SDK (når VST3 bygges) | **OK** | MIT (følger med i JUCE 8) |
| FLAC, Vorbis, AAX, ASIO m.fl. | **Ikke relevant** | Ikke med i binæren |
| Systembiblioteker (dynamiske) | **OK** | Distribueres ikke |
| libstdc++/libc | **UKENDT** (formelt) | Licens ikke aflæst lokalt. Normalt uproblematisk |
| Inter-fonten | **SVAG COPYLEFT** (kun selve fonten) | OFL kræver kun, at fonten forbliver under OFL, og at licensteksten følger med. Påvirker ikke din kode |
| `polyBlep()` | **UKENDT** | Identisk med en udbredt tutorial-formulering. Skrives om i fase 3 |
| Al øvrig kode i `Source/` | **OK** | Selv genereret |
| Presets og mønstre | **OK** | Selv genereret (med varemærke- og lighedsforbehold, se §3) |

---

## 5. Erstatningsforslag

### 5.1 JUCE – JUCE 8 Starter

**Valgt løsning:** SubForge bygges mod **JUCE 8.0.15** under en **JUCE 8 Starter**-licens. Ifølge JUCE 8-EULA'en (læst 2026-10-10):

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

### 5.2 `polyBlep()` (UKENDT) – fase 3

**Clean room:** Skriv funktionen igen kun ud fra specifikationen, uden at se på den nuværende version eller tutorialen. Specifikationen: *"Træk et 2-punkts polynomielt residual fra den naive bølgeform omkring hver diskontinuitet. Residualet er differencen mellem et båndbegrænset step, tilnærmet ved integralet af en trekantkerne over én sample på hver side, og et ideelt step."* Bekræft resultatet med den numeriske test, der sammenligner med en båndbegrænset reference.

**Indsats: lille.**

### 5.3 Inter-fonten (SVAG COPYLEFT, kun selve fonten)

Ingen erstatning er nødvendig. Krav:
- `OFL.txt` skal følge med distributionen.
- Fonten må ikke sælges alene.
- "Inter" må ikke bruges som produktnavn.

**Indsats: lille.**

### 5.4 Attributionskrav – fase 2

Lav en `THIRD-PARTY-NOTICES.txt` med de fulde licenstekster for:
- clap-juce-extensions, CLAP SDK og clap-helpers (MIT)
- HarfBuzz (Old MIT)
- SheenBidi (Apache-2.0)
- IJG libjpeg (inklusive den krævede sætning)
- libpng og zlib
- Inter (OFL)
- VST3 SDK (MIT), når VST3 bygges

JUCE 8 har ingen ISC-moduler, så der er ingen JUCE-ISC-note. JUCE selv er dækket af den kommercielle licens.

**Indsats: lille.**

### 5.5 Mønstre og presets – fase 3

Alt er selv genereret. Presetnavne med varemærkepræg omdøbes til neutrale navne. **Indsats: lille.**

---

## 6. Opsummering

**Kan SubForge sælges med lukket kildekode og de medfølgende mønstre? Ja, når to ting er på plads:**

1. **Du har tegnet JUCE 8 Starter-licensen** og bekræftet vilkårene på juce.com (§5.1). Det gælder kun, så længe din samlede omsætning eller funding er højst 20.000 USD over 12 måneder.
2. **`THIRD-PARTY-NOTICES.txt` følger med** (fase 2). Den skal indeholde MIT, Old MIT, Apache-2.0, IJG, libpng, zlib, OFL og senere VST3 MIT.

**Anbefalet (fase 3):** omskriv `polyBlep()` clean room, og omdøb de varemærkeprægede presetnavne.

Der er ikke fundet andet, der forhindrer salg med lukket kildekode.
