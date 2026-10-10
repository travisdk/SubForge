# Licensgennemgang – SubForge

**Dato:** 2026-10-10
**Omfang:** hele repoet (commit `eb28825`), de biblioteker der bygges ind, og den færdige binær `SubForge.clap`
**Metode:** licenser er læst fra pakkernes egne licensfiler, fra Debian-copyrightfiler for systembiblioteker, fra JUCE 7-EULA'en (hentet fra juce.com 2026-10-10) og fra `strings`/`ldd` på den byggede binær. Hvor noget ikke kunne afgøres, står der **ukendt**.

> Dette er en teknisk gennemgang, ikke juridisk rådgivning. Inden et kommercielt salg bør konklusionerne bekræftes af en jurist med kendskab til softwarelicenser.

---

## 1. Afhængigheder

Projektet bruger **ingen pakkehåndtering** (ingen NuGet, npm, pip, vcpkg eller Conan). Afhængighederne ligger som kildekode i `~/SDKs` og bygges ind via CMake. Licenserne nedenfor er læst fra filerne i de mapper.

### 1a. Biblioteker der bygges ind i pluginnet

| Navn | Version | Licens (kilde) | Bruges til |
|---|---|---|---|
| JUCE – `juce_core`, `juce_events`, `juce_audio_basics`, `juce_audio_devices` | 7.0.9 (`git describe`) | **ISC** (`JUCE/LICENSE.md`: "permissively licensed under the terms of the ISC license") | Grundlag: strenge, filer, beskeder, lydbuffere, MIDI |
| JUCE – `juce_graphics`, `juce_gui_basics`, `juce_gui_extra`, `juce_data_structures`, `juce_audio_formats`, `juce_audio_processors`, `juce_audio_utils`, `juce_dsp` | 7.0.9 | **JUCE 7 EULA** (Personal/Indie/Pro/Educational) **eller GPLv3** (`JUCE/LICENSE.md` + EULA) | GUI, parametre (APVTS), plugin-ramme, oversampling (`juce::dsp::Oversampling`) |
| clap-juce-extensions | commit `54d83e1` (2023-11-20) | **MIT** (`LICENSE.md`, "Copyright 2019-2020, Paul Walker") | CLAP-wrapper rundt om JUCE-processoren |
| CLAP SDK | 1.1.8 (`git describe`) | **MIT** (`LICENSE`, "Copyright (c) 2021 Alexandre BIQUE") | CLAP-headere |
| clap-helpers | commit `41e1086` (2023-04-17) | **MIT** (`LICENSE`, "Copyright (c) 2021 Alexandre BIQUE") | Hjælpeklasser i CLAP-wrapperen |
| Inter (Medium, Bold) | 4.0 (fra fontens navnetabel) | **SIL OFL 1.1** (`/usr/share/doc/fonts-inter/copyright`: "License: OFL-1.1") | Skrifttype i GUI'en, indlejret via BinaryData |

### 1b. Tredjepartskode som JUCE bygger med ind

Fundet ved at læse modulernes konfiguration og tjekke den færdige binær med `strings`:

| Komponent | Ligger i | Licens (kilde) | Med i `SubForge.clap`? |
|---|---|---|---|
| IJG libjpeg | `juce_graphics/image_formats/jpglib` | **IJG-licens** (README l. 130–150): fri brug, også kommercielt, **men** binær distribution kræver at dokumentationen skriver *"this software is based in part on the work of the Independent JPEG Group"* | **Ja**: JPEG-strenge som `JPEGMEM` er i binæren |
| libpng | `juce_graphics/image_formats/pnglib` | **PNG Reference Library License v2** (`LICENSE`) | **Ja**: strenge som `libpng error: %s` er i binæren |
| zlib | `juce_core/zip/zlib` | **zlib-licens** (README, "Copyright notice") | **Ukendt**: slået til (`JUCE_INCLUDE_ZLIB_CODE 1`), men ikke entydigt fundet i binæren |
| FLAC | `juce_audio_formats/codecs/flac` | BSD-type (licensfil ikke læst i denne gennemgang) | **Nej**: slået til (`JUCE_USE_FLAC 1`), men fjernet af linkeren, ingen FLAC-strenge |
| Ogg Vorbis | `juce_audio_formats/codecs/oggvorbis` | BSD-3 (`libvorbis-1.3.7/COPYING`) | **Nej**: fjernet af linkeren |
| VST3 SDK, LV2 SDK, Oboe | `juce_audio_processors/format_types`, `juce_audio_devices/native` | GPLv3/proprietær (VST3), ISC (LV2), Apache-2.0 (Oboe) | **Nej**: de 4 "VST3"-strenge i binæren er JUCEs egne klassenavne (`juce::VST3ClientExtensions`), ikke Steinberg-kode |

### 1c. Systembiblioteker (linket dynamisk, *ikke* distribueret med pluginnet)

Fra `ldd` på `SubForge.clap` og standalone. Licenser fra `/usr/share/doc/<pakke>/copyright`:

| Bibliotek | Licens | Bemærkning |
|---|---|---|
| libfreetype.so.6 | FTL (eller GPLv2, dobbeltlicens) | Brugeren har den i forvejen |
| libpng16.so.16 | libpng / expat | Trækkes ind af FreeType |
| libz.so.1 | Zlib | |
| libbz2.so.1.0 | BSD-variant | Debian-filen nævner også GPL-2, sandsynligvis for hjælpefiler, ikke for biblioteket |
| libbrotli | MIT | |
| libasound.so.2 (kun standalone) | LGPL-2.1+ | Dynamisk linkning: OK |
| libstdc++, libgcc_s, libc, libm | **ikke aflæst** (Debian-filerne har ikke maskinlæsbare `License:`-linjer) | Normalt GPL med GCC Runtime Library Exception og LGPL. Bør bekræftes, men er normalt uproblematisk ved dynamisk linkning |

### 1d. Byggeværktøjer (distribueres ikke)
CMake 3.28.3 og GCC 13.3. De påvirker ikke produktets licens.

---

## 2. Kode med mulig fremmed oprindelse

**Grundlag:**
- `grep` efter `copyright`, `license`, `http`, `www.`, `github`, `based on`, `adapted`, `ported` og navne på kendte projekter gav **ingen** fremmede copyright- eller licensheadere og **ingen** links til eksterne kilder i `Source/` eller `CMakeLists.txt`.
- Al kode i `Source/` er skrevet i dette projekt af en AI-assistent (Claude) i samarbejde med ejeren. Jeg kender ikke til, at noget er kopieret ordret fra andre projekter. En AI-model kan dog ikke helt udelukke, at korte, meget udbredte kodestykker ligner noget fra dens træningsdata. Det er markeret nedenfor, hvor det er relevant.

| Fil / linjer | Hvad | Sandsynlig kilde | Licens | Vurdering |
|---|---|---|---|---|
| `Source/BassSynth.h` 73–87 `polyBlep()` | Anti-aliasing af saw og firkant | Algoritmen er fra forskningen (PolyBLEP, Välimäki m.fl.). **Den præcise udformning** (`t + t - t*t - 1`, `t*t + t + t + 1`) er identisk med en meget udbredt tutorial-version, bl.a. på Martin Finkes blog ("Making Audio Plugins") | **Ukendt** for tutorial-koden. Algoritmer er ikke ophavsretligt beskyttet, men konkret kode kan være det. Stykket er 14 linjer og næsten bestemt af matematikken | **UKENDT** (lav risiko) |
| `Source/BassSynth.h` 89–102 `polyBlamp()` | Anti-aliasing af trekant | Udledt i projektet som integralet af `polyBlep`. Svarer til formler i litteraturen (PolyBLAMP, Esqueda/Välimäki). Fortegnet er verificeret numerisk i projektet | Selv genereret | OK |
| `Source/BassSynth.h` 104–151 `Oscillator` | Bølgeformer | Selv skrevet | Selv genereret | OK |
| `Source/BassSynth.h` 153–187 `LadderFilter` | 4-polet ladder-filter, zero-delay feedback | Metoden er fra Vadim Zavalishins bog *The Art of VA Filter Design* (frit downloadbar, © forfatteren). Koden er skrevet ud fra metoden, ikke kopieret. Mixkoefficienterne til LP12/BP12 (`y2`, `2·(y1−y2)`) er Oberheim Xpander-teknikken fra forskningslitteraturen | Matematiske metoder er ikke beskyttet. Koden er selv genereret | OK |
| `Source/BassSynth.h` 189–258 `DiodeLadderFilter` | Diode-ladder i 303-stil | Ligningssystemet og konstanterne (1,8978 / 46,14 / 0,2) er udledt og målt numerisk i projektet. Under udviklingen overvejede assistenten Dominique Wurtz' open source-implementering fra hukommelsen, men **brugte den ikke** | Selv genereret | OK |
| `Source/BassSynth.h` 260–386 `OnePole`, `Distortion` | Distortion-typer | Selv skrevet. Kun *inspireret* af pedalnavne, ingen kredsløbsmodeller. Niveautabellen er målt i projektet | Selv genereret | OK |
| `Source/BassSynth.h` 388–719 `Envelope`, `Lfo`, `BassSynth` | Envelopes, LFO, stemme | Selv skrevet. Envelope-koefficienter efter standardformler (exp-henfald) | Selv genereret | OK |
| `Source/Sequencer.h` (hele) | Sequencer-motor og mønstergenerator | Selv skrevet | Selv genereret | OK |
| `Source/PluginProcessor.cpp` 55–~85 `moveToTrash()` | Papirkurv på Linux | Skrevet ud fra freedesktop.org's *Trash Specification* (kommentaren nævner specifikationen, men der er intet link). Ingen kode kopieret | Selv genereret | OK |
| `Source/PluginProcessor.cpp`, `PluginEditor.cpp`, `StepGrid.cpp`, `SubForgeLookAndFeel.cpp` | Plugin, GUI, presets | Almindelig brug af JUCEs API (APVTS, LookAndFeel, AlertWindow). Ingen fremmede headere | Selv genereret (men bundet til JUCEs licens, se §4) | OK |
| `CMakeLists.txt` l. 42 ff. `clap_juce_extensions_plugin(...)` | Byggeopsætning | Kaldets form blev set i Vaporizer2's `CMakeLists.txt` (GPL). Det er clap-juce-extensions' dokumenterede API, ikke beskyttet udtryk | Selv genereret | OK |

---

## 3. MIDI-mønstre og andet indhold

**Der findes ingen MIDI-filer (`.mid`) i projektet.** Alt musikalsk indhold ligger som data i kildekoden. **Ingen** af det er hentet fra hjemmesider eller andre kilder. Der er ikke slået noget op på nettet under udviklingen, bortset fra JUCE-EULA'en til denne rapport.

| Indhold | Placering | Oprindelse | Kildesidens licens | Generisk eller ligner kendt materiale? |
|---|---|---|---|---|
| 89 synth-presets | `Source/Patches.h` 70–208 | **Selv genereret** (parameterværdier valgt af assistenten) | – | Generiske indstillinger. **Varemærkenær navngivning:** "8-Oh-8 Sub", "8-Oh-8 Boom" (Rolands TR-808), "Moogie Classic", "Fat Moogy", "Fat 12dB Moogy" (Moog Music), "Muffin Bass" (Electro-Harmonix' Big Muff). Det er ikke et licensspørgsmål, men et varemærkespørgsmål ved salg |
| 42 sequencer-mønstre | `Source/PatternLibrary.h` 70–132 | **Selv genereret** (komponeret ud fra genrebeskrivelser) | – | **Generiske genrefigurer:** Goa Offbeat og Psy Roller (psytrance "rolling bass" mellem kick-slagene), Disco Octaves og Octave Bounce (oktavbas), Body Drive (EBM-puls), Slide Riser (kromatisk løb), Pentatonic Run (skalaøvelse), Lonely Note, Twelve Step Tresillo (tresillo-rytmen er en traditionel figur). **Acid-stil (de øvrige):** komponeret i stilen. Ligheden med kendte numre er **ikke systematisk kontrolleret**. 16-trins basfigurer er korte og bruger få toner, så tilfældige ligheder med eksisterende numre kan ikke udelukkes. Ingen er bevidst skrevet efter et bestemt nummer |
| Standardmønster | `Source/Sequencer.h` 52–64 `setDefault()` | **Selv genereret** | – | Acid-stil, se ovenfor |
| Mønstergenerator (Random) | `Source/Sequencer.h` 101 ff. | **Selv genereret**, regelbaseret (skala, sandsynligheder) | – | Genererer nye mønstre, gemmer ingen eksisterende |
| Inter-fonte | `Resources/Fonts/Inter-*.otf` | Kopieret fra systempakken `fonts-inter` | **SIL OFL 1.1**: må indlejres og distribueres kommercielt sammen med software. Fonten må ikke sælges alene, og licensteksten skal følge med. Fonten indeholder teksten "Inter is a trademark of rsms" | – |
| OFL-licenstekst | `Resources/Fonts/OFL.txt` | Udtrukket fra `/usr/share/doc/fonts-inter/copyright` | – | – |
| Demo-WAV'er | `build/acid-demo.wav`, `build/acid-amps-demo.wav` | **Selv genereret** (renderet af SubForge) | – | Ligger i `build/`, som er udeladt via `.gitignore`. Distribueres ikke |

---

## 4. Klassificering

| Fund | Klasse | Begrundelse |
|---|---|---|
| JUCE ISC-moduler | **OK** | ISC |
| JUCE øvrige moduler (GUI, plugin, dsp …) | **STÆRK COPYLEFT** *eller* **kommerciel licens** | Brug kræver enten GPLv3 for hele produktet eller en JUCE-licens. Personal er gratis under 50.000 USD i omsætning, **men kræver splashskærmen** (EULA §1.11). Indie koster 40 USD/md. (under 500.000 USD), Pro 130 USD/md. Projektet sætter i dag `JUCE_DISPLAY_SPLASH_SCREEN=0` (`CMakeLists.txt` l. 42). **Det er kun tilladt under GPLv3, Indie eller Pro, ikke under Personal** |
| clap-juce-extensions, CLAP SDK, clap-helpers | **OK** | MIT. Copyright-teksten skal følge med |
| IJG libjpeg (i binæren) | **OK** (permissiv) | Ikke på din OK-liste, men permissiv. **Kræver attribution** i dokumentationen ved binær distribution |
| libpng (i binæren) | **OK** (permissiv) | PNG Reference Library License v2 |
| zlib (muligvis i binæren) | **OK** (permissiv) | zlib-licens |
| FLAC, Vorbis, VST3, LV2, Oboe | **Ikke relevant** | Ikke med i den færdige binær |
| Systembiblioteker (dynamiske) | **OK** | Distribueres ikke. LGPL (ALSA) linket dynamisk |
| libstdc++/libc | **UKENDT** (formelt) | Licens ikke aflæst lokalt. Normalt uproblematisk |
| Inter-fonten | **SVAG COPYLEFT** (kun selve fonten) | OFL kræver kun, at fonten og ændringer af den forbliver under OFL, og at licensteksten følger med. Påvirker ikke din kode |
| `polyBlep()` | **UKENDT** | Identisk med en udbredt tutorial-formulering. Licensen på tutorialen er ukendt |
| Al øvrig kode i `Source/` | **OK** | Selv genereret |
| 89 presets, 42 mønstre, standardmønster | **OK** | Selv genereret (med varemærke- og lighedsforbehold, se §3) |

---

## 5. Erstatningsforslag

### 5.1 JUCE (STÆRK COPYLEFT eller kommerciel)

Den gode nyhed er, at **al lydkode allerede er uafhængig af JUCE**: `BassSynth.h`, `Sequencer.h`, `Patches.h` og `PatternLibrary.h` bruger kun standard-C++. Kun plugin-rammen, GUI'en, parameterhåndteringen, preset-filerne og oversampling bruger JUCE.

| Mulighed | Hvad det kræver | Licens | Indsats |
|---|---|---|---|
| **A. JUCE Personal** (gratis, under 50.000 USD i omsætning) | Slå splashskærmen til igen (fjern `JUCE_DISPLAY_SPLASH_SCREEN=0`) og tjek, at den vises i pluginnet | JUCE 7 EULA | **Lille** |
| **B. JUCE Indie** (40 USD/md., under 500.000 USD) eller **Pro** (130 USD/md.) | Abonnement. Splash må slås fra | JUCE 7 EULA | **Lille** (kun pris) |
| **C. Udgiv som open source** | GPLv3-licensfil, kildekoden tilgængelig. Kan stadig sælges, men koden skal være åben | GPLv3 | **Lille**, men opfylder ikke ønsket om lukket kildekode |
| **D. Skift JUCE ud med permissive komponenter** | Plugin-ramme: **CLAP SDK + clap-helpers** (MIT, som allerede bruges) direkte, eller **DPF / DISTRHO Plugin Framework** (ISC, bredt brugt til Linux-plugins, understøtter CLAP/LV2/VST3/JACK). GUI: fx **Dear ImGui** (MIT, meget udbredt) med **pugl** (ISC) til vinduer, eller DPF's egen GUI-del. Oversampling: **HIIR** af Laurent de Soras (WTFPL, permissiv), eller et halvbånds-polyfase-IIR skrevet fra bunden ud fra en publiceret specifikation (clean room). Parametre og presets: egen enkel parameterliste plus XML/JSON, fx med pugixml (MIT) eller nlohmann/json (MIT). Licenserne og udbredelsen ovenfor er fra min viden og bør tjekkes på projekternes egne sider før valg | MIT / ISC / WTFPL | **Stor**: GUI og plugin-ramme skal skrives om. DSP, sequencer og presets kan genbruges |

### 5.2 `polyBlep()` (UKENDT)

**Clean room:** Skriv funktionen igen kun ud fra specifikationen, uden at se på den nuværende version eller tutorialen. Specifikationen: *"Træk et 2-punkts polynomielt residual fra den naive bølgeform omkring hver diskontinuitet. Residualet er differencen mellem et båndbegrænset step, tilnærmet ved integralet af en trekantkerne over én sample på hver side, og et ideelt step."* Bekræft resultatet med den eksisterende numeriske test, der sammenligner med en båndbegrænset reference.

**Indsats: lille.** Risikoen er i forvejen lav, fordi stykket er kort og næsten bestemt af matematikken.

### 5.3 Inter-fonten (SVAG COPYLEFT, kun selve fonten)

Ingen erstatning er nødvendig. Krav:
- `OFL.txt` skal følge med distributionen, fx i installationspakken og i en "Om"-side.
- Fonten må ikke sælges alene.
- "Inter" må ikke bruges som produktnavn.

**Indsats: lille.** Vil du helt undgå font-licenser, kan du bruge systemets standardskrift. Det giver dog et mindre ensartet udseende på tværs af maskiner.

### 5.4 Attributionskrav (IJG, libpng, zlib, MIT-komponenter)

Lav en `THIRD-PARTY-NOTICES.txt`, der følger med pluginnet. Den skal indeholde:
- MIT-teksterne for clap-juce-extensions, CLAP og clap-helpers
- IJG-sætningen *"This software is based in part on the work of the Independent JPEG Group"*
- libpng- og zlib-noterne
- ISC-noten for JUCEs ISC-moduler
- OFL for Inter

**Indsats: lille.**

### 5.5 Mønstre og presets

Alle er selv genererede, så **der er intet at erstatte** efter dine kriterier. Valgfrit:
- **Presetnavne med varemærkepræg** ("Moogy", "Moogie", "8-Oh-8", "Muffin"): omdøb til neutrale navne før salg. **Indsats: lille.**
- **Lighed med kendte numre:** kan ikke kontrolleres automatisk. Vil du være helt sikker, kan mønstrene erstattes med mønstre, der genereres af den regelbaserede generator med fast seed og navngives efter stil og nummer, fx "Acid 01". Så er der ingen håndkomponerede figurer tilbage. **Indsats: lille.**

---

## 6. Opsummering

**Kan SubForge sælges med lukket kildekode og de medfølgende mønstre i dag? Nej, ikke som det er bygget nu.** Det skyldes udelukkende JUCE og manglende licensnoter. Mønstre, presets og den øvrige kode er ikke problemet.

**Minimum, der skal ændres:**
1. **JUCE-licensen skal være på plads.** Vælg én:
   - JUCE **Personal**: slå splashskærmen til igen. Gratis, kun under 50.000 USD i samlet omsætning eller funding.
   - JUCE **Indie** eller **Pro**: abonnement, splash må være slået fra.
   - Erstat JUCE med permissive komponenter (§5.1 D). Stor indsats.
2. **Læg en `THIRD-PARTY-NOTICES.txt` ved** med MIT, ISC, IJG, libpng, zlib og OFL (§5.4).

**Anbefalet, men ikke strengt nødvendigt:** omskriv `polyBlep()` clean room (§5.2) og omdøb de varemærkeprægede presetnavne (§5.5).

Når 1 og 2 er på plads, er der efter denne gennemgang ikke fundet andet, der forhindrer salg med lukket kildekode.
