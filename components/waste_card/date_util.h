#pragma once
#include <cstdio>
#include <string>
#include <cstdint>

namespace esphome::waste_card {
inline int32_t civil_days(int y, unsigned m, unsigned d) {
  y -= m <= 2;
  const int era = (y >= 0 ? y : y - 399) / 400;
  const unsigned yoe = static_cast<unsigned>(y - era * 400);
  const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  return era * 146097 + static_cast<int32_t>(yoe * 365 + yoe / 4 - yoe / 100 + doy) - 719468;
}
inline bool parse_day(const std::string &s, int &y, int &m, int &d) {
  if (s.size() != 10 || s[4] != '-' || s[7] != '-') return false;
  for (int i : {0,1,2,3,5,6,8,9}) if (s[i] < '0' || s[i] > '9') return false;
  if (std::sscanf(s.c_str(), "%4d-%2d-%2d", &y, &m, &d) != 3 || y < 1970 || y > 9999 || m < 1 || m > 12 || d < 1) return false;
  static const int lengths[] = {31,28,31,30,31,30,31,31,30,31,30,31};
  const bool leap = y % 4 == 0 && (y % 100 != 0 || y % 400 == 0);
  return d <= lengths[m - 1] + (m == 2 && leap ? 1 : 0);
}
inline std::string date_label(int y, int m, int d) {
  static const char *weekdays[] = {"So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa."};
  const int weekday = (civil_days(y, m, d) + 4) % 7;
  char text[24];
  std::snprintf(text, sizeof(text), "%s, %02d.%02d.%04d", weekdays[weekday], d, m, y);
  return text;
}
}
