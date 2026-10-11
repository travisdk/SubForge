# Kodesignering på Windows – hvad det kræver

**Status:** Intet er købt eller sat op. Dette dokument beskriver, hvad der skal til, så du kan vælge.

> ⚠️ Priser, regler og hvilke tjenester der er tilgængelige for privatpersoner i Danmark ændrer sig. Tjek de aktuelle vilkår hos udbyderne, før du vælger. Punkter med *(tjek)* er særligt usikre.

## Hvorfor signere?

- **Uden signatur** viser Windows ofte en SmartScreen-advarsel ("Windows beskyttede din pc" eller "ukendt udgiver"), når brugeren åbner en downloadet fil. Det koster tillid.
- **Med signatur** står dit navn som udgiver, og filen kan ikke ændres ubemærket efter signeringen.
- **DAW'er** kræver normalt ikke signerede plugins. Det er først og fremmest download- og installationsoplevelsen, signaturen påvirker.
- **SmartScreen-omdømme** bygges op over tid ud fra antal downloads uden problemer, også med signatur. De første brugere kan derfor stadig se en advarsel *(tjek, hvordan Microsoft håndterer det aktuelt)*.

## Hvad skal signeres?

| Fil | Hvad det er |
|---|---|
| `SubForge.clap` | En Windows-DLL med en anden filendelse |
| `SubForge.vst3/Contents/x86_64-win/SubForge.vst3` | Selve VST3-binæren inde i bundlen (også en DLL) |
| En evt. installer (`.exe`/`.msi`) | Hvis du senere laver en installer i stedet for en zip |

Signering sker med Microsofts `signtool` (fra Windows SDK) med SHA-256 og et **tidsstempel** fra certifikatudstederens tidsstempelserver. Tidsstemplet sikrer, at signaturen stadig er gyldig, efter certifikatet er udløbet.

Eksempel (konceptuelt):

```
signtool sign /fd SHA256 /tr <tidsstempel-URL> /td SHA256 /n "<dit navn>" SubForge.clap
```

## Typer af certifikater

1. **OV-certifikat (Organization/Individual Validation)** fra en certifikatudsteder, fx DigiCert, Sectigo, GlobalSign eller SSL.com.
   - Kræver identitetsvalidering, som tager dage. Nogle udstedere validerer privatpersoner, andre kun firmaer *(tjek)*.
   - Siden juni 2023 skal den private nøgle ligge på **hardware**: et USB-token eller HSM, eller en udbyders cloud-HSM. Du kan derfor ikke bare få en fil, der kan ligge på GitHub.
   - Pris: typisk nogle hundrede USD om året *(tjek)*.
2. **EV-certifikat (Extended Validation)**
   - Strengere validering, typisk kun for registrerede firmaer.
   - Gav tidligere straks SmartScreen-omdømme. Microsoft har ændret dette, så EV ikke længere automatisk fjerner advarslen *(tjek de aktuelle regler)*.
   - Dyrere end OV.
3. **Cloud-signering fra Microsoft** (Azure-baseret signering, tidligere "Trusted Signing")
   - Lav månedlig pris, ingen hardware-token, og kan bruges direkte fra GitHub Actions.
   - Tilgængeligheden for privatpersoner og enkeltmandsvirksomheder afhænger af land og kræver identitetsvalidering *(tjek, om den er åben for dig i Danmark)*.

## Signering og CI

- **Med USB-token:** Signering kan ikke ske på GitHubs servere. Du henter Windows-zippen fra Actions, signerer lokalt på en Windows-maskine med tokenet sat i og pakker igen.
- **Med cloud-signering:** Et ekstra trin i `.github/workflows/build.yml` kan signere automatisk. Adgangsnøglerne gemmes som GitHub Secrets og aldrig i repoet.

## Forslag til rækkefølge

1. Udgiv først usigneret til betatestere, som accepterer SmartScreen-advarslen.
2. Afklar, om du sælger som privatperson eller via firma (CVR). Det afgør, hvilke certifikater du kan få.
3. Vælg cloud-signering, hvis den er tilgængelig for dig. Ellers et OV-certifikat med token.
4. Tilføj signeringen til release-processen og evt. til CI.

*macOS (Developer ID og notarisering) er ikke dækket her.*
