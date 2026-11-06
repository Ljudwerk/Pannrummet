# Pannrummet – plan

*Modell PR-89. Ett effektaggregat för kroppsmusik och industri, byggt som om det skruvats loss ur en styrpanel i en fabrikshall.*

**Status:** **1.0 klar.** Fas 0–3 klara och grind 0–3 godkända: stilprovet 3 november 2026, grind 1 och 2 3 november 2026, fas 3 (programmen, manualen, prestandan) och grind 3 5 november 2026. Nästa steg är kandidaterna i fas 5.

Effektplugin (VST3, AU, CLAP) för macOS, på samma stack som Kroppsmaskinen och KrangBox. Läggs som insert på ett spår – bas, trummor, röst, synt eller hela bussen. Allt i gränssnittet är på svenska.

## 1. Mål

**Pannrummet ska göra ett rent ljud till ett EBM- eller industriljud på en insert:** grinda det, pressa det, ge det metall, sätta det i en megafon och ett schakt och stänga en fabrikshall kring det.

Måttstock: Front 242, Nitzer Ebb, Die Krupps och DAF för EBM-sidan; Einstürzende Neubauten, Die Klinik och Current 93 för den industriella. Vi kopierar inget; vi lyssnar på dem för att veta när vi är framme.

Det som genren gör med ljud och som Pannrummet måste klara:

- **Gatat och avhugget:** trummor och basar som stängs tvärt, reverb som klipps av.
- **Överstyrt och sammanpressat:** rör- och transistordistorsion, likriktare, bitkross – men med nivån i behåll.
- **Metall:** plåt, rör, fjädrar och klockor som klingar med ljudet (Neubauten utan att behöva en byggarbetsplats).
- **Paroller:** röster genom megafon, telefon och radio – Nitzer Ebb-ropet.
- **Rum:** mörka, synkade ekon och stora gatade hallar.

### Inte i 1.0

- Fritt ordningsbar kedja (ordningen är fast; en växel låter Plåten gå före Pressen)
- Stegsekvenser som styr modulerna (*Stansen*, se fas 5)
- Vokoder, sampling, faltningsreverb med egna filer
- Windows och Linux

## 2. Aggregatet

Sex moduler i en fast kedja, en makrospak och en utgång. Namnen är arbetsnamn.

```
 IN ─▶ 1 GRINDEN ─▶ 2 PRESSEN ─▶ 3 PLÅTEN ─▶ 4 MEGAFONEN ─▶ 5 SCHAKTET ─▶ 6 HALLEN ─▶ TORR/VÅT ─▶ UT
          ▲              ▲            ▲             ▲              ▲             ▲
          └──────────────┴──── TRYCKET (makrospak) ─┴──────────────┴─────────────┘
 NÖDSTOPP = förbikoppling av hela aggregatet (mjuk övertoning, ingen klick)
```

Varje modul har en vippströmbrytare (PÅ/AV) och en signallampa. En avstängd modul räknar inte (ingen CPU).

### 1 Grinden – gate

Tröskel, hålltid, släpp och djup. Lägen på vridomkopplaren: **MJUK** (vanlig gate), **HÅRD** (stänger tvärt, EBM-trummor) och **TAKT** (grinden öppnar bara på valda sextondelar i DAW:ens takt – ett enkelt mönster, inte en hel sekvenser). Sidokedja från pluginets andra ingång om värden ger en.

### 2 Pressen – drivet

Driv, ton, tyngd (inbyggd kompression före drivet) och nivå. Lägen: **RÖR**, **TRANSISTOR**, **LIKRIKTARE** (helvågslikriktning – oktav upp och smuts) och **BITKROSS**. Översampling 2× eller 4× (växel), automatisk nivåkompensation. Utgår från `Drivet.h` i Kroppsmaskinen.

### 3 Plåten – metallresonatorn

Modalsyntes: en bank av resonatorer som ljudet exciterar. Stämning (ett tonnamn), klang (decay), ljushet och blandning. **Plåten stäms från MIDI** (beslut 3 november 2026): pluginet har en MIDI-ingång, och med MIDI-lampan tänd gäller senaste inkommande ton; utan MIDI gäller ratten. Material: **PLÅT**, **RÖR**, **FJÄDER** och **KLOCKA**. Kan också köras som ringmodulator (**RING**). Växeln **FÖRE PRESSEN** flyttar Plåten före Pressen. Utgår från Stampverkets metall i Kroppsmaskinen.

### 4 Megafonen – lo-fi för röster

Bandpass, resonans, smuts (liten högtalare som bryter upp) och blandning. Lägen: **MEGAFON**, **TELEFON**, **RADIO** och **TRATT** (smal högtalarlur). Växeln **BRUS** lägger till ledningsbrus och sprak.

### 5 Schaktet – ekot

Synkad tid på vridomkopplaren (1/16 – 1/2, prickad och triol), återkoppling, mörker (filter i återkopplingen, mörknar för varje upprepning), svaj (BBD-lik modulation av tiden), blandning och ping-pong. Utgår från Ekot i Kroppsmaskinen (`Effekter.h`).

### 6 Hallen – gatad reverb

Storlek, hålltid, fall och blandning, lokalen (**VERKSTAD**, **HALL**, **VALV**, **GRUVA**) och **GRIND** av/på: med grinden på stängs hallen tvärt efter hålltiden – det klassiska virvelljudet. Utgår från Grindrummet i Kroppsmaskinen.

### Trycket – makrospaken

Som Spänningen i Kroppsmaskinen: en spak som drar allt mot mer – hårdare grind, mer driv, mer metall, smutsigare megafon, längre schakt, större hall. Nivån kompenseras. Visas på en manometer med rött fält.

### Utgången

In-nivå, torr/våt för hela aggregatet, utnivå och en mjuk limiter sist. Två analoga visarinstrument (IN och UT) och **NÖDSTOPPET** – en röd svampknapp som förbikopplar allt.

## 3. Gränssnittet – fabrikspanelen

### Idén

**En styrpanel ur en fabrikshall, 1960–80-tal.** Kroppsmaskinen är vardagsrummets svarta låda; Pannrummet är dess syster från verkstadsgolvet.

- **Chassit:** hammarlackad stålplåt i mörkt maskingrönt, med skruvar i hörnen och kantlister.
- **Skyltarna:** borstade aluminiumskyltar med svart graverad text, nitade i hörnen. Modellskylten har präglade versaler (PANNRUMMET).
- **Reglagen:** svarta bakelitrattar med vit visarlinje och tryckt skala 0–10, vridomkopplare med spetsig "kycklinghuvud"-ratt och lägena tryckta runt om, vippströmbrytare med metallarm.
- **Lamporna:** glasade signallampor (bärnsten, grön, röd) i kromade infattningar.
- **Instrumenten:** visarinstrument med benvit tavla (IN, UT) och en manometer för Trycket.
- **Varningen:** gul-svart varningsrand, röd nödstoppsknapp på gul skylt.
- **Flödet:** signalvägen ritas som ett rör med flödespilar mellan modulerna, som på ett processchema.
- **Räkneverket:** presetnumret på ett mekaniskt räkneverk med vita siffror på svarta valsar.

Färger: maskingrönt, aluminium, benvitt, signalgult och signalrött. Text i vitt silkscreentryck på lacken och svart gravyr på skyltarna, samma linjeskrift som Kroppsmaskinen.

### Layout (designstorlek 1200 × 760, skalas 60–160 %)

```
┌─────────────────────────────────────────────────────────────────────────────┐
│ [PANNRUMMET PR-89 skylt]   [IN-mätare][UT-mätare]   [MANOMETER] TRYCKET   [NÖDSTOPP] │
├─────────────────────────┬─────────────────────────┬─────────────────────────┤
│ 1 GRINDEN               │ 2 PRESSEN               │ 3 PLÅTEN                │
│ lampa vippa  rattar     │ lampa vippa  rattar     │ lampa vippa  rattar     │
├══ röret med flödespilar ═══════════════════════════════════════════════════╡
│ 4 MEGAFONEN             │ 5 SCHAKTET              │ 6 HALLEN                │
├─────────────────────────┴─────────────────────────┴─────────────────────────┤
│ [räkneverk PRESET ◀ ▶ SPARA MENY]   IN-NIVÅ  TORR/VÅT  UTNIVÅ   ▨▨ varningsrand │
└─────────────────────────────────────────────────────────────────────────────┘
```

En vy, inga flikar: alla parametrar syns på en gång.

### Teknik

- Allt ritas i kod, inga bild- eller typsnittsfiler: hammarlack, borstad aluminium, bakelit, glas.
- Panelen cachas och ritas bara om när något ändras; bara visarinstrumenten och lamporna animeras (högst 30 bilder/s). Mål: under 1 % CPU för gränssnittet på en M1 i 100 %.
- `PRSnapshot` ritar panelen och stilprovet till PNG för granskning utan DAW.

## 4. Kod och struktur

Samma stack som Kroppsmaskinen: **JUCE 8, CMake, clap-juce-extensions, C++20, AGPLv3.** Namnrum `st`.

### krang-libs – det delade biblioteket

Pannrummet är första plugin som använder `../krang-libs` (header-only, CMake-målet `krang::libs`, namnrum `krang`). Dit flyttas det som är gemensamt för alla effektlådor och inte hör till ett visst utseende:

| I krang-libs | Ursprung | Status |
| --- | --- | --- |
| `krang/stil/Linjeskrift.h` | Kroppsmaskinen `ui/stil/Linjeskrift.h` | Flyttad (fas 0) |
| `krang/stil/Textur.h` (slump, borstad metall, plastnarv) | Kroppsmaskinen `ui/stil/Textur.h` | Flyttad (fas 0) |
| `krang/ui/ValueFormat.h` | Kroppsmaskinen `ui/ValueFormat.h` | Flyttad (fas 0) |
| `krang/dsp/Grund.h` (biquad, följare, fördröjning, nivåkompensation) | Ny | Fas 1 |
| `krang/dsp/Drivet.h` | Kroppsmaskinen `dsp/Drivet.h` | Ersattes av Pressen (JUCE-översampling); flyttas inte |
| `krang/dsp/Grindrummet.h`, `Ekot.h` | Kroppsmaskinen `dsp/Effekter.h` | Fas 2 |
| `krang/dsp/Schaktet.h` | Pannrummet `dsp/Schaktet.h` | *Flyttad 6 november 2026, när Röstverket behövde ekot. `Source/dsp/Schaktet.h` är en vidarekoppling. `PRRender --test` går igenom (173 OK), och renderingar med Schaktet är bitidentiska före och efter. Provat på Linux; Mac-bygget inte provat.* |
| `krang/presets/Programbank.h` | Ny (Kroppsmaskinens PresetManager var för specifik) | Fas 3, klar |

Kroppsmaskinen behåller sina egna kopior tills vidare; rättningar förs över för hand. När krang-libs är stabilt kan Kroppsmaskinen gå över.

Det som är Pannrummets eget ligger i `Source/ui/stil/` (fabrikspanelens ritverktyg) och `Source/dsp/` (modulerna).

Regler som gäller här också:

- **Parameter-ID:n får aldrig döpas om** när de väl finns i ett släppt bygge. Formen `st_<modul>_<vad>`.
- Ljudtråden allokerar inget, låser inget. Rapporterad latens stämmer (översamplingen).
- Varje modul får ett testfall i `PRRender` (NaN, topp, DC, tystnad efter tystnad, förbikoppling bitexakt) innan den räknas som klar.

## 5. Faser och grindar

### Fas 0 – Grunden och utseendet

- Repo, CMake, krang-libs, tomt effektplugin som släpper igenom ljudet.
- Planen.
- **Stilprov:** fabrikspanelen ritad i `PRSnapshot` – chassi, skyltar, rattar, vridomkopplare, vippor, lampor, instrument, manometer, nödstopp, räkneverk. Bara bilder, ingen funktion.

**Grind 0:** Stilprovet godkänt. Pluginet laddar i Reaper och släpper igenom ljudet bitexakt.

### Fas 1 – Kroppen

- Grinden, Pressen och Megafonen.
- In-nivå, torr/våt, utnivå, limiter, Nödstoppet.
- `PRRender`: renderar Kroppsmaskinens renders (`renders/*.wav`) genom kedjan till filer för lyssning.

**Grind 1:** Tre exempel lyssnade A/B: trumloop genom Grinden + Pressen, bas genom Pressen, röst genom Megafonen.

*Klar. Grind 1 godkänd (3 november 2026). Avvikelser från planen:*

- *Panelen är redan kopplad till parametrarna (`Source/ui/Panelen.h`): samma ritkod som stilprovet, med bakgrunden cachad och bara mätare och rörliga lampor omritade. Det var tänkt för fas 3, men det gör fas 1 och 2 möjliga att prova i Reaper utan DAW:ens generiska vy. Plåten, Schaktet och Hallen är märkta "EJ INKOPPLAD" tills fas 2. Programmen (◀ ▶ SPARA) kommer i fas 3; MENY har storleken.*
- *Alla sex modulernas parametrar finns från början, så att ID:n och ordning ligger fast inför fas 2.*
- *Grindens TAKT-läge har 16 egna stegparametrar (`pr_grind_steg01`–`16`); stegraden visas i modulen i stället för sidokedjebrytaren när läget är TAKT.*
- *Latensen är alltid Pressens 4×-latens (38 sample i 48 kHz), även när Pressen är av, så att DAW:en aldrig behöver räkna om den. Den torra signalen och Nödstoppet fördröjs lika mycket.*
- *Limitern är en toppbegränsare utan framförhållning (ingen extra latens), tak −0,3 dBFS.*
- *Röstexemplet är talsyntes (espeak-ng), eftersom ingen inspelad röst fanns. Grind 1 bör också göras med en riktig röst i Reaper.*
- *`krang-libs` fick `krang/dsp/Grund.h`: biquad, följare, fördröjning, nivåkompensation och brus. Pressen använder JUCE:s översampling i stället för Kroppsmaskinens egna halvbandsfilter.*

### Fas 2 – Metallen och rummet

- Plåten, Schaktet och Hallen.
- Trycket.

**Grind 2:** En testlåt i Reaper (byggd av ett skript som `tools/reaper/gor_reaper_projekt.py` i Kroppsmaskinen) där Kroppsmaskinens kanaler går genom Pannrummet på varje spår. Lyssnad och godkänd.

*Klar. Grind 2 godkänd (3 november 2026) med testlåten `Reaper/Fabrikshallen.RPP`. Avvikelser och detaljer:*

- *Plåten: åtta tvåpoliga bandpassresonatorer (nollor i ±1, ingen likspänning), lätt isärstämda mellan kanalerna. Deltoner under 25 Hz hoppas över. RING är en ringmodulator på grundtonen; där gör Ljushet bärvågen fyrkantigare och Klang ger en kort kamresonans. Nivån kompenseras med ett långt mätfönster (1,5 s), så att klangen inte trycks ned i svansen.*
- *Schaktet: högpass 120 Hz, lågpass (Mörker, 12 kHz – 700 Hz) och mjuk mättnad i återkopplingen; Svaj upp till 4 ms. Ping-pong matar vänster och studsar. Återkopplingen är en procentparameter (0–95 %).*
- *Hallen: FDN med åtta linjer och Hadamard-blandning. Lokalen sätter linjelängder, dämpning, fördröjning före rummet och svaj; Storlek sätter rummet och efterklangstiden (0,3–6 s, längre i Valv och Gruva). Grinden styrs av det torra ljudet (−40 dBFS). Högpass 150 Hz på ingången.*
- *Blandning i Plåten, Schaktet och Hallen: det torra står kvar fullt upp till 50 %, det våta når fullt vid 50 %.*
- *En modul med svans som slås på börjar från tystnad (inga gamla rester).*
- *Trycket (bara på påslagna moduler): Grinden +10 dB tröskel, −40 % hålltid, −60 % släpp; Pressen +0,5 driv, +0,4 tyngd; Plåten klang ×2, +30 % blandning; Megafonen +0,5 smuts; Schaktet +25 % återkoppling, +20 % blandning; Hallen +0,3 storlek, +20 % blandning. Nivån hålls av modulernas nivåkompensation och limitern.*
- *Testlåten `Reaper/Fabrikshallen.RPP` är Kroppsmaskinens Fabriksgolv med en Kroppsmaskinen per kanal (de andra kanalerna tysta) och Pannrummet efter, plus ett röstspår med paroller (talsyntes). Den är inte renderad här – Kroppsmaskinen finns bara på Macen – utan lyssnas i Reaper.*
- *En allmän **MIDI**-brytare (`pr_midi`, sist i parameterlistan, av från början): av = Pannrummet struntar i inkommande MIDI och glömmer senaste tonen, så att presetbyten och MIDI på spåret inte sätter igång något (önskemål 3 november 2026).*
- *AU-versionen har ingen sidokedja (enkel 2 in / 2 ut), eftersom ljudet inte gick igenom AU-versionen med sidokedja i Reaper. VST3 och CLAP har sidokedjan.*

### Fas 3 – Panelen och fabriksbandet (1.0)

- Hela gränssnittet enligt stilprovet, skalning, animering av instrumenten.
- 30 presets: EBM, Industri, Röst och Trummor.
- Svensk manual i README med bilder från `PRSnapshot`.

**Grind 3:** Någon som inte byggt Pannrummet gör om en Kroppsmaskinen-låt med bara fabriksbandet och manualen.

*Pågår. Gjort:*

- *Programmen: `krang/presets/Programbank.h` (ny i krang-libs) och `Source/presets/Fabriksbandet.h`. 30 fabriksprogram – Init, 8 Trummor, 9 EBM, 7 Röst, 5 Industri – plus egna program som XML (`.prprog`, formatversion 1) i `~/Library/Application Support/Pannrummet/Program/`. Ett program sätter alla parametrar (det som inte nämns går till grundvärdet) men aldrig Nödstoppet eller MIDI in. Programmets namn sparas i tillståndet. DAW:ens programlista är fabriksbandet; värdens programbyte till samma program gör ingenting (så att ett inläst projekt inte nollställs).*
- *Panelen: ◀ ▶ bläddrar, SPARA frågar efter ett namn, klick på räkneverket eller Dymo-remsan ger listan per kategori, MENY kan visa mappen med egna program.*
- *Programmen är nivåanpassade med Utnivå (mätt med `PRRender --program`), så att man kan bläddra utan stora hopp. Alla 30 är renderade i `renders/program/` (mp3).*
- *Linjeskriften visar nu gemena åäö (toUpperCase klarar dem inte alltid).*

- *Manualen: `docs/Manual.md` (snabbstart, panelen, varje modul med bild och reglagetabell, Trycket, instrumenten och MIDI, utgången, programmen, recept, felsökning) och `docs/Pannrummet_i_Reaper.md` (installation, insert, sidokedja, MIDI till Plåten, automation, testlåten, rendering). README länkar till dem och behåller översikten. Bilderna `docs/manual_*.png` är utsnitt ur PRSnapshot-bilder i 200 %. Avvikelse: manualen ligger i `docs/` (som i Kroppsmaskinen) i stället för i README.*
- *Sidokedjebussen (VST3, CLAP) är nu på från början, så att Reaper alltid visar kanal 3/4 som sidokedja; en okopplad sidokedja är tyst och påverkar inget.*

*Klar. Fas 3 testad och grind 3 godkänd (5 november 2026).*

*Efter 1.0 – rättat mot Ljudwerks styrdokument (6 november 2026):*

- *Limitern ligger i krang-libs (`krang/dsp/Limiter.h`); `Source/dsp/Utgangen.h` är en vidarekoppling. Flytten provades med alla 30 fabriksprogram och alla exempel renderade före och efter: filerna är bitidentiska.*
- *`PRSnapshot --klicktest`: drar, Skift-drar, rullar och dubbelklickar varje ratt, stegar varje vridomkopplare fram, bak och runt, växlar varje brytare och Grindens 16 taktsteg, och provar FÖRRA/NÄSTA – och kontrollerar att rätt parameter, och bara den, ändrades. 206 kontroller, alla går igenom. SPARA, MENY och programlistan öppnar dialoger och menyer och provas på Macen.*
- *`PRSnapshot --manual docs` ritar manualens bilder (förut utsnitt för hand); bilderna blev pixel för pixel lika (utom mätarnas visare).*
- *`PRSnapshot --prestanda`: i molnets mjukvarurenderare tar hela panelen 40 ms (100 %) och 107 ms (200 %) att rita om, de rörliga delarna 1,1 ms respektive 2,5 ms per bild. CPU-målet godkändes på Macen i grind 3.*
- *pluginval nivå 5 för VST3 på Linux: går igenom, både utan och med panelen (xvfb). `PRRender --test`: 173 kontroller, alla går igenom.*
- *CI: `.github/workflows/build-macos.yml` bygger, kör testerna och klicktestet, ritar bilderna och validerar VST3 och AU med pluginval nivå 5 och `auval -v aumf Pr89 Pnrm`. krang-libs hämtas från `mikkeschiren/KrangLibs` (en token behövs om det repot är privat).*

### Fas 5 – Efter 1.0 (kandidater)

- **Stansen:** en stegsekvenser (16 steg) som kan styra valfri parameter i takt.
- Fri ordning i kedjan.
- Kroppsmaskinen flyttar över till krang-libs.

## 6. Risker

| Risk | Motåtgärd |
| --- | --- |
| Omfånget växer | Listan "Inte i 1.0" gäller. Nya moduler kräver att grind 3 är passerad. |
| Plåten låter som en billig kamfilterreverb | Modalsyntes med riktiga materialtabeller, A/B mot Neubauten-referenser vid grind 2. |
| Aliasing i Pressen och likriktaren | Översampling, renderingstest med spektrumkontroll. |
| Kedjan blir för tung för en insert på varje spår | Avstängda moduler räknar inte; mätning per modul i `PRRender`. |
| krang-libs och Kroppsmaskinen glider isär | Tabellen ovan; rättningar förs över för hand tills Kroppsmaskinen går över. |

## 7. Beslut

- **Namnet är Pannrummet, modell PR-89** (beslut 3 november 2026; arbetsnamnet var Stampet). Allt byttes: plugin-koder (`Pnrm`/`Pr89`), bundle-ID `org.pannrummet.pr89`, parameterprefix `pr_`, namnrum `pr`, verktygen `PRRender` och `PRSnapshot`.
- Utseendet är en **fabrikspanel** (beslut 3 november 2026), inte Kroppsmaskinens VHS-låda.
- Pannrummet är en **fast kedja av moduler** med en makrospak (beslut 3 november 2026). Rytmisk styrning skjuts till *Stansen* efter 1.0.
- Stilprovet ritas i **JUCE-kod** som i Kroppsmaskinen (beslut 3 november 2026).
- Modulnamnen är arbetsnamn.

- **Plåten stäms från MIDI** (beslut 3 november 2026). Pannrummet har därför en MIDI-ingång; i AU är det en *music effect* (`aumf`), i VST3 och CLAP en vanlig effekt med noter in.

Inga öppna frågor just nu.
