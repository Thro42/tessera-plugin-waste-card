#include "../components/waste_card/waste_card.cpp"
#include <cassert>
#include <iostream>
using namespace esphome::waste_card;
using esphome::tessera::ui::objects;

Value state(const std::string &day, bool rest, bool paper, bool bio, bool plastic) {
  Value data;
  data.object["state"] = Value(std::string("ready"));
  auto &a = data.object["attributes"].object;
  a["collection_day"] = Value(day);
  a["rest"] = Value(rest); a["paper"] = Value(paper); a["bio"] = Value(bio); a["plastic"] = Value(plastic);
  return data;
}
Value state_text(const std::string &s) {
  Value data;
  data.object["state"] = Value(s);
  return data;
}
void check_bins(unsigned mask) {
  for (int i = 0; i < 4; ++i) {
    const bool expected = (mask & (1U << i)) != 0;
    for (int part = 0; part < 4; ++part)
      assert(objects[2 + i * 4 + part]->hidden != expected);
  }
}
int main() {
  WasteCard plugin;
  plugin.setup();
  auto *tile = plugin.factory();
  lv_obj_t parent;
  tile->create({&parent, 240, 120});
  // Stub's local calendar date is 2026-11-10: test all warning boundaries,
  // theme changes and resetting colours for unavailable / expired data.
  for (const auto &s : {"2026-11-15|1000", "2026-12-31|1000"}) {
    tile->on_state(state_text(s)); tile->on_tick(1);
    assert(objects[1]->color == esphome::tessera::ui::color(theme::INK));
  }
  for (const auto &s : {"2026-11-14|1000", "2026-11-13|1000", "2026-11-12|1000"}) {
    tile->on_state(state_text(s)); tile->on_tick(1);
    assert(objects[1]->color == lv_palette_darken(LV_PALETTE_AMBER, 4));
    theme::dark = true; tile->on_theme();
    assert(objects[1]->color == LV_PALETTE_YELLOW);
    theme::dark = false; tile->on_theme();
    assert(objects[1]->color == lv_palette_darken(LV_PALETTE_AMBER, 4));
  }
  for (const auto &s : {"2026-11-11|1000", "2026-11-10|1000"}) {
    tile->on_state(state_text(s)); tile->on_tick(1);
    assert(objects[1]->color == LV_PALETTE_RED);
    tile->on_theme(); assert(objects[1]->color == LV_PALETTE_RED);
  }
  tile->on_state(state_text("none")); tile->on_tick(1);
  assert(objects[1]->color == esphome::tessera::ui::color(theme::INK));
  tile->on_state(state_text("2026-11-09|1000")); tile->on_tick(1);
  assert(objects[1]->text == "stale" && objects[1]->color == esphome::tessera::ui::color(theme::INK));
  tile->on_state(state_text("2026-11-11|1000")); tile->on_tick(0);
  assert(objects[1]->color == esphome::tessera::ui::color(theme::INK));
  // Both producer formats must render all 15 nonempty bit combinations identically.
  for (unsigned mask = 1; mask < 16; ++mask) {
    std::string bits;
    for (int i = 0; i < 4; ++i) bits += (mask & (1U << i)) ? '1' : '0';
    tile->on_state(state_text("2026-11-12|" + bits));
    tile->on_tick(1);
    assert(objects[1]->text == "Do., 12.11.2026");
    check_bins(mask);
    tile->on_state(state("2026-11-12", mask & 1, mask & 2, mask & 4, mask & 8));
    tile->on_tick(1);
    assert(objects[1]->text == "Do., 12.11.2026");
    check_bins(mask);
  }
  assert(objects[2]->color == LV_PALETTE_PINK);
  assert(objects[6]->color == esphome::tessera::ui::color(theme::MARK_BLUE));
  assert(objects[10]->color == esphome::tessera::ui::color(theme::MARK_GREEN));
  assert(objects[14]->color == LV_PALETTE_YELLOW);

  // Compact data needs no attributes and ignores any conflicting leftover ones.
  auto compact = state("2026-11-13", false, true, false, false);
  compact.object["state"] = Value(std::string("2026-11-12|1011"));
  tile->on_state(compact); tile->on_tick(1);
  assert(objects[1]->text == "Do., 12.11.2026"); check_bins(13);
  for (const auto &bad : {"bogus", "2026-11-12", "2026-11-12|0000", "2026-11-12|111",
                          "2026-11-12|11111", "2026-11-12|10x1", "2026-11-12|1012",
                          "2026-11-12/1011", " 2026-11-12|1011", "2026-11-12|1011 ",
                          "2026-02-30|1011", "2026-04-31|1011", "2027-02-29|1011",
                          "2100-02-29|1011", "2026-00-12|1011", "2026-13-12|1011",
                          "2026-11-00|1011", "1969-12-31|1011", "2026-1-12|1011",
                          "2026-11-12T00:00|1011"}) {
    tile->on_state(state_text(bad)); tile->on_tick(1);
    assert(objects[1]->text == "invalid"); check_bins(0);
  }
  tile->on_state(state_text("2028-02-29|0001")); tile->on_tick(1);
  assert(objects[1]->text == "Di., 29.02.2028"); check_bins(8);
  tile->on_state(state_text("2026-11-10|1000")); tile->on_tick(1);
  assert(objects[1]->text == "Di., 10.11.2026"); check_bins(1);
  for (const auto &s : {"2026-11-09|1111", "2026-01-01|0001"}) {
    tile->on_state(state_text(s)); tile->on_tick(1);
    assert(objects[1]->text == "stale"); check_bins(0);
  }
  tile->on_state(state("2026-11-12", false, false, false, false)); tile->on_tick(1);
  assert(objects[1]->text == "invalid"); check_bins(0);
  auto wrong_type = state("2026-11-12", true, true, true, true);
  wrong_type.object["attributes"].object["bio"] = Value(std::string("true"));
  tile->on_state(wrong_type); tile->on_tick(1);
  assert(objects[1]->text == "invalid"); check_bins(0);
  for (const auto &s : {"unknown", "unavailable"}) {
    tile->on_state(state_text(s)); tile->on_tick(1);
    assert(objects[1]->text == "waiting"); check_bins(0);
  }
  tile->on_state(state("2026-11-12", true, false, true, false));
  tile->on_tick(1);
  assert(objects[1]->text == "Do., 12.11.2026");
  assert(!objects[2]->hidden && objects[6]->hidden && !objects[10]->hidden && objects[14]->hidden);
  assert(objects[2]->color == LV_PALETTE_PINK);
  tile->on_state(state("2026-11-09", true, true, true, true));
  tile->on_tick(1);
  assert(objects[1]->text == "stale" && objects[2]->hidden);
  tile->on_state(state("2026-02-30", true, true, true, true));
  tile->on_tick(1);
  assert(objects[1]->text == "invalid");
  auto malformed = state("2026-11-12", true, true, true, true);
  malformed.object["attributes"].object.erase("bio");
  tile->on_state(malformed); tile->on_tick(1);
  assert(objects[1]->text == "invalid");
  Value empty; empty.object["state"] = Value(std::string("none"));
  tile->on_state(empty); tile->on_tick(1);
  assert(objects[1]->text == "none" && objects[2]->hidden);
  tile->on_state(state("2026-11-12", false, false, false, true)); tile->on_tick(1);
  assert(!objects[14]->hidden && objects[14]->color == LV_PALETTE_YELLOW);
  tile->on_state(Value()); tile->on_tick(1);
  assert(objects[1]->text == "choose" && objects[14]->hidden);
  delete tile;
  for (auto *object : objects) delete object;
  std::cout << "tile mock tests passed\n";
}
