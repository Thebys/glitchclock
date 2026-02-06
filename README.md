# GlitchClock

Hackerske hodiny s glitch efekty na WS2812 LED pasku, bezici na ESP8266 (LOLIN V3 / NodeMCU V3).

4 cifry x 7 segmentu x 2 LED + dvoutecka = 58 LED, rizene pres ESPHome.

## Funkce

- **Glitch engine** - time warp, segment decay, colon chaos, loading spinner, scramble, artefact scanline
- **Easter eggs** - nahodna hackerska slova (dEAd, bEEF, CAFE, root, SUdo...)
- **Rainbow times** - pastelova duha pri specialnich casech (11:11, 12:34, palindromy...)
- **420 Haze** - zeleno-fialova paleta s dychanim pri 04:20 a 16:20, 10x vyssi sance na tematicka slova
- **Demo mode** - sekvencni showcase vsech efektu (ovladani pres HA)
- **WiFi citlivost** - slaby signal = vice glitchu
- **Laditelne parametry** - glitch rate, corruption, drift, color mode pres Home Assistant

## Hardware

| Soucastka | Specifikace |
|-----------|-------------|
| MCU | ESP8266 LOLIN V3 (NodeMCU V3) |
| LED | WS2812 pasek, 58 LED, GRB |
| Data pin | GPIO2 (D4) |
| Napajeni | 5V, doporuceno min. 3A |

## Rychly start (lokalni)

```bash
# 1. Naklonuj repo
git clone https://github.com/Thebys/glitchclock.git
cd glitchclock

# 2. Vytvor secrets.yaml ze sablony
cp secrets.yaml.example secrets.yaml
# Vypln WiFi udaje a vygeneruj nove klice (viz instrukce v souboru)

# 3. Python 3.13 venv (3.14 nefunguje s ESPHome!)
python3.13 -m venv .venv
source .venv/bin/activate
pip install esphome

# 4. Kompilace a upload
esphome run glitchclock.yaml
```

## Deploy pres Home Assistant

Firmware lze kompilovat a OTA aktualizovat primo z HA ESPHome dashboardu.

**1. V HA ESPHome addon vytvor novy YAML** (nebo zkopiruj `device.yaml.example`):

```yaml
packages:
  glitchclock:
    url: https://github.com/Thebys/glitchclock
    files: [glitchclock-package.yaml]
    ref: master       # nebo konkretni tag, napr. v1.0.0
    refresh: 0s       # fetch jen pri manualni instalaci

esphome:
  name: glitchclock
  friendly_name: GlitchClock

api:
  encryption:
    key: !secret glitchclock_api_key

ota:
  - platform: esphome
    password: !secret glitchclock_ota_password

wifi:
  ssid: !secret wifi_ssid
  password: !secret wifi_password
  ap:
    ssid: "GlitchClock Fallback"
    password: !secret glitchclock_ap_password
```

**2. Pridej secrets do HA** (`/config/esphome/secrets.yaml`):

```yaml
glitchclock_api_key: "vygeneruj-base64-klic"
glitchclock_ota_password: "vygeneruj-hex-heslo"
glitchclock_ap_password: "vygeneruj-heslo"
```

Generovani klicu viz sekce Secrets nize.

**3. Klikni Install** v ESPHome dashboardu. HA stahne kod z GitHubu, zkompiluje a flashne OTA.

**4. Aktualizace**: push novou verzi na GitHub, v HA klikni Install znovu.

> **Poznamka**: C++ hlavicky (`gc_core.h`, `gc_glitch.h`) se stahnou jako soucast
> GitHub package. Pokud HA hlasi chybu s `includes:`, viz sekce Troubleshooting.

## Secrets - bezpecnostni pravidla

**`secrets.yaml` NIKDY nesmi byt commitnut do gitu.** Je v `.gitignore`.

Soubor `secrets.yaml` obsahuje:

| Klic | Ucel | Jak vygenerovat |
|------|------|-----------------|
| `wifi_ssid` | Nazev WiFi site | -- |
| `wifi_password` | Heslo WiFi | -- |
| `api_encryption_key` | Sifrovani komunikace ESPHome <-> HA | `python3 -c "import secrets,base64; print(base64.b64encode(secrets.token_bytes(32)).decode())"` |
| `ota_password` | Heslo pro OTA aktualizace firmware | `python3 -c "import secrets; print(secrets.token_hex(16))"` |
| `ap_password` | Heslo fallback AP (kdyz se nepripoji k WiFi) | `python3 -c "import secrets; print(secrets.token_urlsafe(9))"` |

### Pravidla pro praci se secrets

1. **Nikdy** nepis hesla, klice ani tokeny primo do YAML souboru - vzdy pouzij `!secret nazev`
2. **Pred commitem** zkontroluj `git diff` - nesmi obsahovat zadne citlive hodnoty
3. **Po klonovani** vzdy vytvor novy `secrets.yaml` z `secrets.yaml.example` s **novymi** vygenerovanymi hodnotami
4. **Pri zmene API klice** je nutne znovu autorizovat zarizeni v Home Assistant
5. **Pokud** omylem commitnes secret - okamzite rotuj vsechna hesla (stare povazuj za kompromitovana)

### Co delat pri leaku

```bash
# 1. Vygeneruj NOVA hesla (viz tabulka vyse)
# 2. Aktualizuj secrets.yaml
# 3. Flashni firmware s novymi hesly: esphome run glitchclock.yaml
# 4. V HA smaz a znovu pridej ESPHome integraci (novy API klic)
# 5. Zmen WiFi heslo na routeru (pokud bylo leaknuto)
```

## Troubleshooting

### HA remote package: includes not found

Pokud ESPHome v HA nenahradne C++ hlavicky z GitHub package, pouzij `external_components` pristup
nebo naklonuj repo rucne do ESPHome config adresare.

### Python 3.14 + ESPHome

ESPHome nepodporuje Python 3.14 (odstranen `ast.Str`). Pouzij Python 3.13.

## Struktura projektu

```
glitchclock.yaml           # Lokalni config (secrets + package import)
glitchclock-package.yaml   # Verejny package (vse krome secrets)
device.yaml.example        # Sablona pro HA remote deploy
gc_core.h                  # Segment tabulky, renderovani cifer, palety
gc_glitch.h                # Glitch engine, stavovy automat, efekty
secrets.yaml               # Tajemstvi (NENI v gitu)
secrets.yaml.example       # Sablona pro secrets
```

## Licence

MIT
