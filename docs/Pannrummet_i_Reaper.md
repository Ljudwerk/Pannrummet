# Pannrummet i Reaper

Hur Pannrummet används i Reaper: som insert, med sidokedja, med MIDI till Plåten, med automation – och testlåten *Fabrikshallen*. Reglagen och modulerna beskrivs i [manualen](Manual.md).

## Installera

Bygg och installera med `./real.sh` i pannrummet-mappen (VST3, AU och CLAP hamnar i `~/Library/Audio/Plug-Ins/`). Starta om Reaper, eller välj *Options → Preferences → Plug-ins → VST → Re-scan*.

**Använd VST3-versionen** (*VST3: Pannrummet (Pannrummet)*) eller CLAP. AU-versionen fungerar men har ingen sidokedja.

## Som insert

1. Klicka på **FX** på spåret och lägg till *VST3: Pannrummet*.
2. Välj ett program (klick på Dymo-remsan) eller börja från *01 Init*.
3. Pannrummet rapporterar sin latens (38 sample i 48 kHz) och Reaper kompenserar den – spåret ligger i takt med de andra.

Pannrummet fungerar lika bra efter ett instrument (till exempel Kroppsmaskinen eller KrangBox) i samma FX-kedja som på ett ljudspår eller en buss.

**Reapers presetmeny** (+-knappen i FX-fönstret) visar fabriksbandets 30 program. Egna program väljs från panelen.

## Sidokedjan

Grinden kan styras av ett annat spår – typiskt bastrumman, så att basen eller en liggande ton öppnas i takt med den.

1. Lägg Pannrummet på spåret som ska grindas (t.ex. basen).
2. På bastrummans spår: **Route** → *Add new send* → basspåret. Ställ sändningen till **Audio: 1/2 → 3/4** (kanal 3/4 är Pannrummets andra ingång, *Sidokedja*).
3. Basspåret måste ha minst fyra kanaler: i basspårets **Route**-fönster, sätt *Track channels* till **4**.
4. I Pannrummet: Grinden **PÅ**, LÄGE **HÅRD** eller **MJUK**, **SIDOKEDJA** på. Ställ **TRÖSKEL** så att bastrumman öppnar grinden.

Sänd gärna sändningen *pre-fader* om bastrummans volym ska kunna ändras utan att grinden påverkas.

## MIDI till Plåten

Plåten kan stämmas av MIDI-toner, så att metallen eller ringmodulatorn följer en basgång eller melodi.

1. Slå på **MIDI** överst på panelen (grön lampa bredvid DRIFT) och Plåtens egen **MIDI**-lampa.
2. Ge Pannrummet MIDI på något av sätten:
   - **Samma spår:** lägg ett MIDI-objekt på spåret. Ligger ett instrument före Pannrummet i FX-kedjan får det också tonerna – vill du inte det, lägg Pannrummet på ett eget spår.
   - **Från ett annat spår:** på MIDI-spåret, **Route** → send till Pannrummets spår, *Audio: None*, *MIDI: All → All*.
3. Senaste tonen gäller och Plåten glider dit på 20 ms. Plåtens STÄMNING visar "MIDI" och tonen.

Med **MIDI** av struntar Pannrummet i alla toner – byt program och spela MIDI på spåret utan att något händer.

## Automation

Alla reglage kan automatiseras. Klicka på **Trim/Read** på spåret (eller på *Param* i FX-fönstret) och välj parametern; namnen börjar med modulen, till exempel *Hallen grind*, *Pressen driv*, *Trycket*.

Bra att automatisera:

| Parameter | Till vad |
| --- | --- |
| **Trycket** | Uppbyggen: 0 → 70 % över fyra eller åtta takter. |
| **Modulernas *på*** (t.ex. *Grinden på*) | Slå på en effekt för en sektion – Grinden i TAKT i en break. |
| **Hallen grind** | Öppna rummet (av) i en break och stäng det (på) igen på ettan. |
| **Megafonen läge** | Byt från MEGAFON till RADIO i ett avbrott. |
| **Nödstopp** | Torrt ljud i en enstaka takt. |

Vridomkopplare och brytare ändras i steg; använd *square*-formen på automationspunkterna.

## Testlåten Fabrikshallen

`Reaper/Fabrikshallen.RPP` är grind 2-låten: Kroppsmaskinens *Fabriksgolv* (125 BPM, 3:35) uppdelad på ett spår per kanal – **Stampverket**, **Kolvbasen**, **Kolvbas 2** och **Hugget** – med Pannrummet efter Kroppsmaskinen på varje spår, och ett spår **Paroller** med röster (`Reaper/ljud/`) genom Megafonen, Schaktet och Hallen.

Öppna den för att höra hur Pannrummet låter i en hel låt, och titta på automationen på varje spår för att se hur Trycket och modulerna styrs genom låten. Den kräver VST3-versionerna av både Kroppsmaskinen och Pannrummet.

Projektet byggs om med:

```sh
python3 tools/reaper/gor_reaper_projekt.py <PRRender> ../kropps-maskinen/Reaper/Fabriksgolv.json Reaper
```

## Rendera

Pannrummet låter likadant i realtid och vid rendering (*File → Render*), även när Reaper renderar snabbare än realtid. Ekon och rum klingar ut efter sista ljudet; lägg till en sekund eller två svans i renderingen (*Tail*).
