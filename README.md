# Abfallkalender Card

Zeigt den nächsten passenden Abholtag und alle zugehörigen Tonnen: Restmüll **pink**, Papier **blau**, Bio **grün**, Plastik **gelb**. Die Darstellung orientiert sich an `konzept/beispiel.jpg`. Das Plugin liest einen beliebig wählbaren Sensor; es schreibt keine HA-Entity und benötigt keine HA-Aktionsrechte.

## Datumsfarben

- Mehr als vier Tage bis zur Abholung: normale Schriftfarbe.
- Vier bis zwei Kalendertage vorher: **gelb** (auf hellem Hintergrund für besseren Kontrast dunkleres Gelb/Amber).
- Am Vortag und am Abholtag: **rot**.
- Ist das Datum veraltet oder die Bildschirmuhr noch nicht synchronisiert, wird keine Warnfarbe gezeigt. Die Berechnung verwendet das lokale Kalenderdatum des Displays; ein HA-Neustart oder eine zusätzliche Automation ist dafür nicht nötig.

## Beteiligung einer KI

Bei der Konzeption, Implementierung, Dokumentation und Prüfung dieses Plugins wurde ein KI-Assistent eingesetzt.

## Set up

1. In dieser Installation ist `sensor.tessera_abfallkalender` bereits eingerichtet. Die Automation `automation.tessera_abfallkalender_aktualisieren` liest `calendar.abfallkalender` beim HA-Start und stündlich für die nächsten 30 Tage, schreibt `input_text.tessera_abfallkalender_cache`; der UI-Template-Helper spiegelt den Cache-Zustand. **Nichts erneut installieren.**
2. Plugin im Tessera-App-Menü über einen GitHub-Link oder als lokales Testverzeichnis `/config/tessera-plugins/waste_card/` hinzufügen und den Bildschirm bauen. In Layout die Plugin-Kachel **Abfallkalender Card** platzieren und im Inspektor `sensor.tessera_abfallkalender` auswählen. Der lokale Ordner wird nicht automatisch erstellt.
3. Für andere Installationen stehen zwei alternative Producer-Beispiele bereit: `examples/compact_cache_automation.yaml` zeigt die Cache-Automation (benötigt einen Input-Text-Cache und einen Template-Sensor mit `{{ states('input_text.tessera_abfallkalender_cache') }}`); `examples/home_assistant_package.yaml` erzeugt stattdessen einen triggerbasierten Sensor mit Attributen. Kalender-ID, Ereignisnamen und bei Bedarf den Suchzeitraum anpassen. Die Beispiele werden nicht automatisch installiert; nicht beide für denselben Zweck zusätzlich aktivieren. Die Entity `sensor.kuchen_dashboard_nachstes_abfall_event` ist davon getrennt.

## Datenvertrag

Die Kachel unterstützt zwei gleichwertige Producer-Formate. Ein Sensor genügt:

**Kompakt (bereits installierter UI-Helper):** `state: "2026-11-12|1011"`. Nach dem ISO-Datum folgen genau vier Binärziffern in der Reihenfolge Rest/Papier/Bio/Plastik: `1` zeigt die Tonne, `0` blendet sie aus. Das Beispiel zeigt also pink, grün und gelb. Es werden keine Attribute benötigt. Mindestens eine Ziffer muss `1` sein; bei keinem Termin `none` statt `0000` senden.

**Alternativ: Sensorzustand `ready` mit Attributen:**

```yaml
state: ready
attributes:
  collection_day: '2026-11-12' # lokales Kalenderdatum YYYY-MM-DD
  rest: true                   # Pink
  paper: false                 # Blau
  bio: true                    # Grün
  plastic: true                # Gelb
```

Die vier Attribute müssen echte boolesche Werte sein; mindestens eines muss `true` sein. `collection_day` ist ein **Datum**, kein Zeitstempel. `state: none` bedeutet keine bekannten passenden Termine; `unknown`/`unavailable` bedeutet Warten. Ungültige Werte und bereits vergangene Termine werden nicht als bevorstehend gezeigt. Auch ein anderer Producer als das Beispiel ist möglich; er muss denselben Vertrag liefern und seine Werte bei Änderungen aktualisieren.

Beide Beispiele finden den **frühesten** noch nicht vergangenen Kalendertag mit einem passenden Titel und kombinieren alle passenden Ereignisse dieses Tages; sie sind unabhängig von der Reihenfolge der Kalenderantwort. Sie übernehmen nicht die Logik des alten Flows, der Flags über alle Termine mischte und das letzte Datum verwendete. Exakte, nach Klein-/Großschreibung und Rand-Leerzeichen normalisierte Titel werden im `names`-Mapping zugeordnet. Keine Treffer im Zeitraum → `none`. Für ganztägige und lokale ISO-Startwerte wird deren Datumsanteil verwendet; bei Terminen mit abweichendem UTC-Offset muss der Producer das gewünschte lokale Datum liefern. Eine ausgefallene Kalenderaktion kann einen früheren Sensorwert stehen lassen; nach dem betreffenden Tag kennzeichnet das Plugin diesen als veraltet. Die Anzeige kann vor der nächsten stündlichen Aktualisierung für kurze Zeit noch den Stand vom Vortag zeigen, wird aber als veraltet markiert. Der installierte Cache und der UI-Sensor wurden mit `2026-11-12|1011` in HA geprüft; ein Test auf dem Display steht noch aus.

Tessera sieht keine eigenen Rollen für Pink und Gelb vor: dafür nutzt die Kachel benannte LVGL-Palettenfarben, für Blau und Grün Tessera-Markenfarben; Text und Flächen verwenden Theme-Rollen. Das ist eine bewusste Ausnahme von der Tessera-Empfehlung, ausschließlich Theme-Rollen zu nutzen. Der Wochentag wird derzeit unabhängig von der Systemsprache deutsch formatiert, um dem Beispielbild zu entsprechen.

## Verifizierter Stand

- HA-Kalenderabruf und manuell ausgelöste Automation erfolgreich: `sensor.tessera_abfallkalender` und Cache lieferten `2026-11-12|1011` (Rest, Bio, Plastik). Der alte Küchen-Sensor blieb in Zustand und `last_updated` unverändert.
- Acht Python-Tests, C++-Datumstest und C++-Kacheltests mit Test-Doubles bestanden.
- Echter Tessera-Host-Firmware-Build mit SDL-Vorschau für CYD, Guition, Waveshare 4.3 Hochformat und JC8012P4A1: **4/4 erfolgreich, keine Warnungen/Fehler**. Drei reine Hinweise betreffen breite Kachelgrößen, die nicht ins einspaltige Hochformat-Raster passen; dort funktioniert 1x1/1x2.
- Die Vorschau liegt unter `.preview/index.html`; die Beispielkachel zeigt zur Farbkontrolle alle vier Tonnen, nicht den Live-Kalender.
- Für den Guition-Host-Test wurde ausschließlich in einer isolierten Tessera-Testkopie der fehlende `min_power: 0.0`-Parameter des simulierten Backlight-Outputs ergänzt. Das Plugin selbst benötigt diese Änderung nicht. SDL-Abhängigkeiten wurden benutzerlokal in WSL bereitgestellt. Ausgabe zunächst auf dem Linux-Dateisystem, danach nach `.preview` kopiert, um Dropbox-Snapshot-Sperren zu vermeiden.
- Noch nicht durchgeführt: Installation auf dem echten Tessera-Display, physischer Displaytest und Neustarttest von HA. Keine Commits oder Uploads vorgenommen.

## Tests

`python -m unittest discover -s tests -v` benötigt PyYAML und Jinja2 und prüft beide Beispiel-Templates mit simulierten Kalenderantworten, alle 15 nichtleeren Tonnenkombinationen und die Preview-Daten. `tests/test_date.cpp` ist ein eigenständiger C++-Test, z. B. `g++ -std=c++17 -Wall -Wextra -Werror tests/test_date.cpp -o test_date && ./test_date`. `tests/test_tile.cpp` kompiliert und testet den echten Kachel-Code mit Test-Doubles für Tessera/LVGL: `g++ -std=c++17 -Wall -Wextra -Werror -I tests/stubs tests/test_tile.cpp -o test_tile && ./test_tile`. Er prüft beide Producer, alle 15 Kombinationen und Farben, Übergänge zu `none`/`unknown`, abgelaufene Termine sowie ungültige Zustände, Bits und Kalenderdaten. Dies ist kein Firmware-Build. Tessera-Prüfung: `python ../tessera-plugins/tools/check.py .`. Die Firmware-Vorschau benötigt auf Windows WSL 2 mit Python ≥ 3.11 und SDL2; siehe `tessera-plugins/docs/PREVIEW.md`. Weder diese Tests noch der Checker ersetzen einen Build und Test auf dem echten Display.
