# Tjekliste for udgivelser

SubForge udgives gratis som open source under **GNU AGPLv3** og vedligeholdes ikke aktivt. Denne liste erstatter den tidligere salgstjekliste.

> Dette er en praktisk tjekliste, ikke juridisk rådgivning.

## Ved hver udgivelse

1. **Version:** Hæv `project(SubForge VERSION …)` i `CMakeLists.txt`. Brug `1.0.1` til rettelser, `1.1.0` til nye funktioner og `2.0.0`, hvis gamle projekter eller presets ikke længere virker.
2. **Ændringslog:** Flyt punkterne under "Unreleased" i `CHANGELOG.md` til en ny versionssektion med dato, og opdatér linkene nederst.
3. **Tredjepartsnoter:** Kør `python3 scripts/generate-third-party-notices.py`, hvis en afhængighed er ændret. CI fejler, hvis filen er forældet.
4. **Commit og push** til `master`, og vent på, at CI ("Build") er grøn.
5. **Tag og release:** `git tag vX.Y.Z` og `git push origin vX.Y.Z`. Opret en GitHub Release fra tagget med de to pakker fra CI-kørslen (`SubForge-linux-x86_64.tar.gz` og `SubForge-windows-x64.zip`) og ændringerne fra `CHANGELOG.md`. Navnene må ikke ændres, for README linker direkte til dem via `releases/latest/download/…`.
6. **Skærmbillede:** Er udseendet ændret, så lav `docs/images/screenshot.png` igen: `cmake --build build --target SubForge_Screenshot` og `build/SubForge_Screenshot docs/images/screenshot.png`. Det bruger standardlyd, -mønster og Lime-temaet, uanset dine egne indstillinger.
7. **Test i Ardour:** Husk at scanne pluginnet igen (Window → Plugin Manager → Re-scan), ellers skjuler Ardour den nye build.

## Skal altid overholdes

- **Kildekoden skal være offentlig** for hver version, der udgives som binær (AGPLv3). Det er opfyldt, så længe repoet er offentligt, og hver release har et tag.
- **`LICENSE`, `THIRD-PARTY-NOTICES.txt` og `INSTALL.txt` skal følge med** i pakkerne. Det sker automatisk i `package-linux` og `package-windows`.
- **Ændr aldrig plugin-ID'erne:** `Hjuh` / `SbFg` og `dk.hjuhl.subforge` i `CMakeLists.txt` og parameter-ID'erne i `PluginProcessor.cpp`. Ellers kan brugernes projekter ikke længere åbne pluginnet. Se `ARCHITECTURE.md` §5.
- **VST-varemærket:** Hver side, der nævner VST (README, release-noter, ændringslog), skal vise Steinbergs "VST Compatible"-logo (`docs/images/VST_Compatible_Logo_Steinberg.png`). Første gang "VST" nævnes, skrives "VST®". Sætningen "VST is a registered trademark of Steinberg Media Technologies GmbH." skal stå i dokumentationen. Ord som "VSTi" må ikke bruges. Se `VST3_Usage_Guidelines.pdf` i VST3 SDK'et.
- **Andres varemærker:** Skriv ikke "lyder som en TB-303" eller lignende. Brug beskrivende ord som "acid-bas" og "diode-ladder-filter".
- **JUCE-version:** SubForge bruger JUCE under AGPLv3. Opgraderes JUCE (fx til 9.x), så tjek, at JUCE stadig kan bruges under AGPLv3, og opdatér `docs/LICENSE-REVIEW.md`.

## Repoet på GitHub

- **Kun du har skriveadgang.** Andre kan forke repoet og sende pull requests, som du selv vælger at merge eller lade ligge. Giv ikke andre skriveadgang, medmindre du vil have en medudvikler.
- **README siger, at projektet ikke vedligeholdes aktivt.** Vil du slet ikke have henvendelser, kan Issues slås fra under Settings → General → Features.
- **E-mail i commits:** Commits laves med `7153988+travisdk@users.noreply.github.com` (sat i repoets egen git-konfiguration). Slå "Keep my email addresses private" og "Block command line pushes that expose my email" til under GitHub → Settings → Emails.

## Kan overvejes

- **Kodesignering på Windows** (se `docs/WINDOWS-SIGNING.md`). Uden signatur kan Windows vise en SmartScreen-advarsel ved download. For et gratis projekt er det normalt at springe det over.
- **Test på en rigtig Windows-maskine.** Windows-udgaven er kun bygget af CI.
- **pluginval** (gratis testværktøj) mod VST3-udgaven.
- **macOS** kræver en Mac, Apple Developer-medlemskab, signering og notarisering. Ikke planlagt.
