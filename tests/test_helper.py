import datetime
import ast
import pathlib
import unittest
import yaml
from jinja2 import Environment, StrictUndefined

ROOT = pathlib.Path(__file__).resolve().parents[1]
package = yaml.safe_load((ROOT / 'examples/home_assistant_package.yaml').read_text(encoding='utf-8'))
template = package['template'][0]['actions'][1]['variables']['waste_result']
compact_package = yaml.safe_load((ROOT / 'examples/compact_cache_automation.yaml').read_text(encoding='utf-8'))
compact_template = compact_package['actions'][1]['data']['value']
env = Environment(undefined=StrictUndefined)
class Clock:
    def date(self):
        return datetime.date(2026, 11, 10)

def calculate(events):
    text = env.from_string(template).render(waste_calendar_response={'calendar.abfallkalender': {'events': events}}, now=lambda: Clock())
    return ast.literal_eval(text.strip())

def calculate_compact(events):
    text = env.from_string(compact_template).render(waste_events={'calendar.abfallkalender': {'events': events}}, now=lambda: Clock())
    return text.strip()

def event(day, title):
    return {'start': day, 'summary': title}

class Contract(unittest.TestCase):
    def test_unsorted_and_shared_day(self):
        value = calculate([event('2026-11-20', 'Papier'), event('2026-11-12', 'Restmüll 4-Wo'), event('2026-11-12T08:00:00+01:00', 'Biomüll'), event('2026-11-12', 'Gelber Sack/Gelbe Tonne')])
        self.assertEqual(value, {'day': '2026-11-12', 'rest': True, 'paper': False, 'bio': True, 'plastic': True})
    def test_earlier_event_resets_flags(self):
        value = calculate([event('2026-11-14', 'Papier'), event('2026-11-12', 'Biomüll')])
        self.assertEqual(value, {'day': '2026-11-12', 'rest': False, 'paper': False, 'bio': True, 'plastic': False})
    def test_unrelated_and_past_are_ignored(self):
        value = calculate([event('2026-11-09', 'Papier'), event('2026-11-10', 'Termin'), event('2026-11-11', ' PAPIER ')])
        self.assertEqual(value['day'], '2026-11-11')
        self.assertTrue(value['paper'])
    def test_empty(self):
        self.assertEqual(calculate([]), {'day': '', 'rest': False, 'paper': False, 'bio': False, 'plastic': False})
    def test_four_same_day(self):
        value = calculate([event('2026-11-10', n) for n in ['Papier', 'Biomüll', 'Restmüll 4-Wo', 'Gelber Sack/Gelbe Tonne']])
        self.assertTrue(all(value[k] for k in ('rest', 'paper', 'bio', 'plastic')))

class CompactContract(unittest.TestCase):
    def test_all_bit_combinations_match_attribute_producer(self):
        names = ['Restmüll 4-Wo', 'Papier', 'Biomüll', 'Gelber Sack/Gelbe Tonne']
        for mask in range(1, 16):
            events = [event('2026-11-12', name) for i, name in enumerate(names) if mask & (1 << i)]
            events = [event('2026-11-20', 'Papier'), *reversed(events), event('2026-11-09', 'Papier')]
            result = calculate(events)
            expected = result['day'] + '|' + ''.join('1' if result[k] else '0' for k in ('rest', 'paper', 'bio', 'plastic'))
            with self.subTest(mask=mask):
                self.assertEqual(calculate_compact(events), expected)

    def test_example_and_empty(self):
        events = [event('2026-11-20', 'Papier'), event('2026-11-12', 'Restmüll 4-Wo'),
                  event('2026-11-12T08:00:00+01:00', 'Biomüll'), event('2026-11-12', 'Gelber Sack/Gelbe Tonne')]
        self.assertEqual(calculate_compact(events), '2026-11-12|1011')
        self.assertEqual(calculate_compact([]), 'none')
        self.assertEqual(calculate_compact([event('2026-11-09', 'Papier'), event('2026-11-10', 'Termin')]), 'none')

    def test_preview_uses_compact_and_none(self):
        preview = yaml.safe_load((ROOT / 'preview.yaml').read_text(encoding='utf-8'))
        self.assertEqual(preview['states']['sensor.tessera_abfallkalender']['state'], '2026-11-12|1111')
        self.assertEqual(preview['states']['sensor.tessera_abfall_leer']['state'], 'none')

if __name__ == '__main__':
    unittest.main()
