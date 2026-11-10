#include "../components/waste_card/date_util.h"
#include <cassert>
#include <iostream>
using namespace esphome::waste_card;
int main() {
  int y, m, d;
  assert(parse_day("2026-11-12", y, m, d));
  assert(date_label(y, m, d) == "Do., 12.11.2026");
  assert(parse_day("2024-02-29", y, m, d));
  assert(!parse_day("2023-02-29", y, m, d));
  assert(!parse_day("2026-04-31", y, m, d));
  assert(!parse_day("2026-1-01", y, m, d));
  assert(!parse_day("2026-11-12T00:00", y, m, d));
  assert(civil_days(2027, 1, 1) - civil_days(2026, 12, 31) == 1);
  std::cout << "date tests passed\n";
}
