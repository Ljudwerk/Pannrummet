#!/usr/bin/env python3
# Bygger grind 2-testlåten "Fabrikshallen" som Reaper-projekt: Kroppsmaskinens Fabriksgolv uppdelad på
# ett spår per kanal, med Pannrummet efter Kroppsmaskinen på varje spår, och ett röstspår med paroller.
#
#   python3 tools/reaper/gor_reaper_projekt.py <PRRender> <Fabriksgolv.json> <utkatalog> [--reaper-id <tal>]
#
# <Fabriksgolv.json> är Kroppsmaskinens låtbeskrivning (kropps-maskinen/Reaper/Fabriksgolv.json, skriven av
# Kroppsmaskinens skript). Bredvid den måste Fabriksgolv.RPP ligga: därifrån läses Kroppsmaskinens
# Reaper-id och parameterindex för automationen.
#
# Varje Kroppsmaskinen-spår har samma läge och automation som i Fabriksgolv, men de andra kanalerna är
# tysta, så att spåret bara spelar sin kanal. Pannrummets läge och automation byggs här (PANNRUMMET nedan);
# parametrarnas ordning läses med "PRRender --parametrar" och lägena skrivs med "PRRender --state".
#
# Pannrummets Reaper-id läses ur Reapers pluginlista (~/Library/Application Support/REAPER/
# reaper-vst*plugins*.ini) när skriptet körs på Macen; annars --reaper-id eller ett reservvärde ('Pnrm').
# Reaper hittar ändå pluginet på filnamnet Pannrummet.vst3.
#
# Copyright (C) 2026 Pannrummet-teamet. Licens: AGPLv3, se LICENSE.
import base64, glob, json, os, re, struct, subprocess, sys, tempfile, uuid

PPQ = 960
PR_GUID = 'ABCDEF019182FAEB506E726D50723839'   # JUCE: ABCDEF01 9182FAEB + 'Pnrm' + 'Pr89'
PR_RESERV_ID = 0x506E726D                        # 'Pnrm'
KM_GUID = 'ABCDEF019182FAEB4B706D6B4B6D3837'
# JUCE:s privata data efter tillståndet i VST3 (samma som Kroppsmaskinens skript).
JUCE_PRIVAT = bytes.fromhex('0000000000000000004a554345507269766174654461746100010142797061737300010103001d00000000000000'
                            '4a5543455072697661746544617461')

def takt(t):
    return (t - 1) * 4.0

# --- Pannrummet på varje spår -------------------------------------------------------------------------
# Låtens delar (från Fabriksgolv): A 1 intro, B 9 bas, C 17 bas 2 och stötar, D 33 mer rörelse,
# E 41 break med uppbygge, F 49 fullt, G 73 avbrott, H 81 fullt med mest spänning, I 97 utklingning.

# Trycket följer Kroppsmaskinens Spänning, en aning lägre.
TRYCKET = [(1, 0.0), (33, 0.25), (41, 0.0), (42, 0.1), (43, 0.2), (44, 0.3), (45, 0.4), (46, 0.5), (47, 0.6), (48, 0.7),
           (49, 0.45), (73, 0.15), (81, 0.65), (97, 0.25)]

PANNRUMMET = {
    'sv': {   # Stampverket: hård grind, transistorpress, gatad hall (utan grind i breaket)
        'namn': 'Stampverket',
        'lage': {'pr_grind_pa': 1, 'pr_grind_lage': 1, 'pr_grind_troskel': -34, 'pr_grind_hall': 40, 'pr_grind_slapp': 35,
                 'pr_press_pa': 1, 'pr_press_lage': 1, 'pr_press_driv': 0.3, 'pr_press_tyngd': 0.4, 'pr_press_ton': 0.55,
                 'pr_hall_pa': 1, 'pr_hall_lokal': 1, 'pr_hall_storlek': 0.75, 'pr_hall_halltid': 200, 'pr_hall_fall': 30,
                 'pr_hall_blandning': 0.4, 'pr_hall_grind': 1},
        'automation': {'pr_hall_grind': [(1, 1), (41, 0), (49, 1)],
                       'pr_hall_lokal': [(1, 1), (41, 2), (49, 1)],
                       'pr_hall_blandning': [(1, 0.4), (81, 0.5), (97, 0.4)]},
    },
    'kb': {   # Kolvbasen: rörpress, takt-grind i breakets andra halva, eko i avbrottet
        'namn': 'Kolvbasen',
        'lage': {'pr_press_pa': 1, 'pr_press_lage': 0, 'pr_press_driv': 0.45, 'pr_press_ton': 0.55,
                 'pr_grind_pa': 0, 'pr_grind_lage': 2, 'pr_grind_hall': 90, 'pr_grind_slapp': 20,
                 'pr_grind_steg01': 1, 'pr_grind_steg02': 0, 'pr_grind_steg03': 1, 'pr_grind_steg04': 1,
                 'pr_grind_steg05': 0, 'pr_grind_steg06': 1, 'pr_grind_steg07': 0, 'pr_grind_steg08': 1,
                 'pr_grind_steg09': 1, 'pr_grind_steg10': 0, 'pr_grind_steg11': 1, 'pr_grind_steg12': 1,
                 'pr_grind_steg13': 0, 'pr_grind_steg14': 1, 'pr_grind_steg15': 0, 'pr_grind_steg16': 1,
                 'pr_schakt_pa': 0, 'pr_schakt_tid': 3, 'pr_schakt_aterkoppling': 45, 'pr_schakt_morker': 0.7, 'pr_schakt_blandning': 0.3},
        'automation': {'pr_grind_pa': [(1, 0), (45, 1), (49, 0)],
                       'pr_schakt_pa': [(1, 0), (73, 1), (81, 0)]},
    },
    'kb2': {  # Kolvbas 2: likriktare och ringmodulerad plåt
        'namn': 'Kolvbas 2',
        'lage': {'pr_press_pa': 1, 'pr_press_lage': 2, 'pr_press_driv': 0.35, 'pr_press_ton': 0.5, 'pr_press_over4': 1,
                 'pr_plat_pa': 1, 'pr_plat_material': 4, 'pr_plat_stamning': 45, 'pr_plat_klang': 0.3, 'pr_plat_ljushet': 0.3,
                 'pr_plat_blandning': 0.25, 'pr_plat_midi': 0},
        'automation': {'pr_plat_blandning': [(1, 0.25), (81, 0.4), (97, 0.25)]},
    },
    'hu': {   # Hugget: klockplåt, ping-pong-eko och ett stenvalv
        'namn': 'Hugget',
        'lage': {'pr_plat_pa': 1, 'pr_plat_material': 3, 'pr_plat_stamning': 57, 'pr_plat_klang': 2.0, 'pr_plat_ljushet': 0.5,
                 'pr_plat_blandning': 0.25, 'pr_plat_midi': 0,
                 'pr_schakt_pa': 1, 'pr_schakt_tid': 3, 'pr_schakt_aterkoppling': 45, 'pr_schakt_morker': 0.5,
                 'pr_schakt_blandning': 0.3, 'pr_schakt_pingpong': 1,
                 'pr_hall_pa': 1, 'pr_hall_lokal': 2, 'pr_hall_storlek': 0.6, 'pr_hall_blandning': 0.25, 'pr_hall_grind': 0},
        'automation': {'pr_plat_pa': [(1, 0), (49, 1), (73, 0), (81, 1)]},
    },
}

ROST = {      # Parollerna genom megafonen, schaktet och gruvan
    'lage': {'pr_mega_pa': 1, 'pr_mega_lage': 0, 'pr_mega_band': 1600, 'pr_mega_resonans': 0.5, 'pr_mega_smuts': 0.5,
             'pr_schakt_pa': 1, 'pr_schakt_tid': 4, 'pr_schakt_aterkoppling': 50, 'pr_schakt_morker': 0.6,
             'pr_schakt_svaj': 0.3, 'pr_schakt_blandning': 0.35, 'pr_schakt_pingpong': 1,
             'pr_hall_pa': 1, 'pr_hall_lokal': 3, 'pr_hall_storlek': 0.7, 'pr_hall_blandning': 0.25, 'pr_hall_grind': 0},
    'automation': {'pr_mega_lage': [(1, 0), (73, 2), (81, 0)], 'pr_mega_brus': [(1, 0), (73, 1), (81, 0)]},
    'paroller': [(17, 'rost_arbeta.wav'), (21, 'rost_ror_dig.wav'), (33, 'rost_maskin.wav'), (41, 'rost_stampa.wav'),
                 (49, 'rost_marschera.wav'), (65, 'rost_arbeta.wav'), (73, 'rost_maskin.wav'), (81, 'rost_stampa.wav'),
                 (89, 'rost_ror_dig.wav'), (97, 'rost_marschera.wav')],
}

# --- Reaper-formatet --------------------------------------------------------------------------------

def guid():
    return '{' + str(uuid.uuid4()).upper() + '}'

def stampet_id():
    if '--reaper-id' in sys.argv:
        return int(sys.argv[sys.argv.index('--reaper-id') + 1])
    for ini in glob.glob(os.path.expanduser('~/Library/Application Support/REAPER/reaper-vst*plugins*.ini')):
        for rad in open(ini, encoding='utf-8', errors='replace'):
            if rad.lower().startswith('pannrummet.vst3='):
                falt = rad.split('=', 1)[1].split(',')
                if len(falt) > 1 and '{' in falt[1]:
                    return int(falt[1].split('{')[0])
    print('Hittade inte Pannrummet i Reapers pluginlista; använder reservvärdet %d.' % PR_RESERV_ID)
    return PR_RESERV_ID

def vst3_block(state_xml, program, rid, ingangar, utgangar):
    """Reapers VST3-block: huvud (id, magi, stiften), JUCE:s tillstånd ('VC2!' + längd + XML + privat data)
    och programnamnet. Samma form som Kroppsmaskinens skript."""
    xml = state_xml.encode('utf-8')
    komponent = b'VC2!' + struct.pack('<I', len(xml)) + xml + JUCE_PRIVAT
    kropp = struct.pack('<II', len(komponent), 1) + komponent + bytes(8)
    huvud = struct.pack('<II', rid, 4276969198)
    for stift in (ingangar, utgangar):
        huvud += struct.pack('<I', len(stift))
        for m in stift:
            huvud += struct.pack('<Q', m)
    huvud += struct.pack('<III', len(kropp), 1, 0)
    namn = b'\x00' + program.encode('utf-8') + bytes(5)
    rader = [base64.b64encode(huvud).decode()]
    for i in range(0, len(kropp), 96):
        rader.append(base64.b64encode(kropp[i:i + 96]).decode())
    rader.append(base64.b64encode(namn).decode())
    return rader

def parmenv(index, punkter, sek):
    """punkter: [(fjärdedelar, normaliserat värde)]. Fyrkantiga steg (form 1)."""
    ut = ['      <PARMENV %d 0.000000 1.000000 %.6f' % (index, punkter[0][1]),
          '        EGUID %s' % guid(), '        ACT 1 -1', '        VIS 1 1 1', '        LANEHEIGHT 0 0',
          '        ARM 0', '        DEFSHAPE 1 -1 -1']
    ut += ['        PT %.6f %.6f 1' % (sek(max(0.0, s)), v) for s, v in punkter]
    return ut + ['      >']

def midi_handelser(noter, langd):
    h = []
    for start, l, ton, vel in noter:
        pa = int(round(start * PPQ)); av = int(round((start + l) * PPQ)) - 1
        h.append((pa, 1, 0x90, ton, max(1, min(127, int(round(vel * 127))))))
        h.append((max(pa + 1, av), 0, 0x80, ton, 0))
    h.sort()
    ut, senast = [], 0
    for t, _, status, d1, d2 in h:
        ut.append('E %d %02x %02x %02x' % (t - senast, status, d1, d2))
        senast = t
    ut.append('E %d b0 7b 00' % max(0, int(langd * PPQ) - senast))
    return ut

# --- Bygget ------------------------------------------------------------------------------------------

def las_km(json_fil):
    lat = json.load(open(json_fil, encoding='utf-8'))
    rpp = open(os.path.join(os.path.dirname(json_fil), 'Fabriksgolv.RPP'), encoding='utf-8').read()
    m = re.search(r'Kroppsmaskinen\.vst3 0 "" (\d+)\{', rpp)
    km_id = int(m.group(1))
    # Kroppsmaskinens skript skriver automationen i samma (sorterade) ordning som i json-filen.
    # Automationsblocken kopieras ordagrant (med nya EGUID), så att normaliseringen blir exakt densamma.
    block = re.findall(r'(      <PARMENV \d+.*?\n      >)', rpp, re.S)
    km_kurvor = {a['param']: block[n] for n, a in enumerate(lat['automation'])}
    return lat, km_id, km_kurvor

def stampet_lage(strender, inst, tmp):
    fil = os.path.join(tmp, 'st.xml')
    subprocess.run([strender, '--state', fil] + ['%s=%s' % (k, v) for k, v in inst.items()], check=True)
    return open(fil, encoding='utf-8').read().strip()

def main():
    if len(sys.argv) < 4:
        print('gor_reaper_projekt.py <PRRender> <Fabriksgolv.json> <utkatalog> [--reaper-id <tal>]')
        sys.exit(1)
    strender, km_json, utkatalog = os.path.abspath(sys.argv[1]), sys.argv[2], sys.argv[3]
    os.makedirs(utkatalog, exist_ok=True)
    lat, km_id, km_kurvor = las_km(km_json)
    pr_id = stampet_id()
    tempo, slag = lat['tempo'], lat['langdSlag']
    sek = lambda s: s * 60.0 / tempo

    with tempfile.TemporaryDirectory() as tmp:
        par = os.path.join(tmp, 'p.txt')
        subprocess.run([strender, '--parametrar', par], check=True)
        pr_index, pr_intervall = {}, {}
        for rad in open(par, encoding='utf-8'):
            i, pid, lo, hi = rad.rstrip('\n').split('\t')
            pr_index[pid] = int(i); pr_intervall[pid] = (float(lo), float(hi))

        def pr_norm(pid, v):
            lo, hi = pr_intervall[pid]
            return (v - lo) / (hi - lo) if hi > lo else 0.0

        def stampet_fx(inst, automation, program):
            state = stampet_lage(strender, inst, tmp)
            ut = ['      <VST "VST3: Pannrummet (Pannrummet)" Pannrummet.vst3 0 "" %d{%s} ""' % (pr_id, PR_GUID)]
            ut += ['        ' + r for r in vst3_block(state, program, pr_id, [1, 2, 0, 0], [1, 2])]
            ut += ['      >', '      FLOATPOS 0 0 0 0', '      FXID %s' % guid()]
            kurvor = dict(automation)
            kurvor['pr_trycket'] = TRYCKET
            for pid, pts in sorted(kurvor.items()):
                ut += parmenv(pr_index[pid], [(takt(t), pr_norm (pid, v)) for t, v in pts], sek)
            ut += ['      WAK 0 0']
            return ut

        def km_fx(kanal):
            # De andra kanalerna tystas i läget, och deras tyst-automation tas bort.
            state = lat['state']
            for k in ('kb', 'kb2', 'sv', 'hu'):
                if k != kanal:
                    state = re.sub(r'id="km_%s_tyst" value="[^"]*"' % k, 'id="km_%s_tyst" value="1.0"' % k, state)
            ut = ['      <VST "VST3i: Kroppsmaskinen (Kroppsmaskinen)" Kroppsmaskinen.vst3 0 "" %d{%s} ""' % (km_id, KM_GUID)]
            ut += ['        ' + r for r in vst3_block(state, lat['preset'], km_id, [], [1, 2])]
            ut += ['      >', '      FLOATPOS 0 0 0 0', '      FXID %s' % guid()]
            for p, text in km_kurvor.items():
                if p.endswith('_tyst') and p != 'km_%s_tyst' % kanal:
                    continue
                ut += re.sub(r'EGUID \{[^}]*\}', 'EGUID ' + guid(), text).split('\n')
            ut += ['      WAK 0 0']
            return ut

        def spar(namn, fx, extra=[]):
            ut = ['  <TRACK %s' % guid(), '    NAME "%s"' % namn, '    VOLPAN 1 0 -1 -1 1', '    MUTESOLO 0 0 0',
                  '    NCHAN 2', '    FX 1', '    MAINSEND 1 0',
                  '    <FXCHAIN', '      SHOW 0', '      LASTSEL 0', '      DOCKED 0', '      BYPASS 0 0 0']
            ut += fx + ['    >'] + extra + ['  >']
            return ut

        midi = ['    <ITEM', '      POSITION 0', '      LENGTH %.6f' % sek(slag), '      LOOP 0', '      NAME "Transponering"',
                '      IGUID %s' % guid(), '      <SOURCE MIDI', '        HASDATA 1 %d QN' % PPQ]
        midi += ['        ' + e for e in midi_handelser(lat['noter'], slag)]
        midi += ['        GUID %s' % guid(), '      >', '    >']

        ut = ['<REAPER_PROJECT 0.1 "7.0/macOS-arm64" 0 0',
              '  <NOTES 0 2', '    |Fabrikshallen – Pannrummets grind 2-låt, byggd av pannrummet/tools/reaper/gor_reaper_projekt.py',
              '    |Kroppsmaskinens Fabriksgolv, en kanal per spår, med Pannrummet efter på varje spår och paroller genom Megafonen.',
              '    |Trycket och modulbyten är automation på Pannrummet.', '  >',
              '  TEMPO %g 4 4 0' % tempo, '  SAMPLERATE 48000 0 0', '  LOOP 0',
              '  MASTER_VOLUME 1 0 -1 -1 1', '  SELECTION 0 %g' % sek(slag), '  CURSOR 0']
        for kanal in ('sv', 'kb', 'kb2', 'hu'):
            s = PANNRUMMET[kanal]
            ut += spar(s['namn'], km_fx(kanal) + stampet_fx(s['lage'], s['automation'], s['namn']), midi)

        objekt = []
        ljud = os.path.join(utkatalog, 'ljud')
        for t, fil in ROST['paroller']:
            langd = 3.0
            try:
                import wave
                with wave.open(os.path.join(ljud, fil)) as w:
                    langd = w.getnframes() / w.getframerate()
            except Exception:
                pass
            objekt += ['    <ITEM', '      POSITION %.6f' % sek(takt(t)), '      LENGTH %.6f' % langd, '      LOOP 0',
                       '      NAME "%s"' % fil[:-4], '      IGUID %s' % guid(),
                       '      <SOURCE WAVE', '        FILE "ljud/%s"' % fil, '      >', '    >']
        ut += spar('Paroller', stampet_fx(ROST['lage'], ROST['automation'], 'Paroller'), objekt)
        ut += ['>']

        with open(os.path.join(utkatalog, 'Fabrikshallen.RPP'), 'w', encoding='utf-8') as f:
            f.write('\n'.join(ut) + '\n')
        s = sek(slag)
        print('%s: %d takter i %g BPM (%d:%02d), 5 spår' % (os.path.join(utkatalog, 'Fabrikshallen.RPP'),
                                                         slag / 4, tempo, int(s // 60), int(round(s % 60))))

if __name__ == '__main__':
    main()
