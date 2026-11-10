#include "waste_card.h"
#include "date_util.h"
#include <algorithm>
#include <string>

namespace esphome::waste_card {
using tessera::Font;
namespace ui = tessera::ui;

class NextTile : public tessera::Tile {
 public:
  explicit NextTile(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    title_ = ui::label(c.parent, Font::TITLE, theme::INK);
    date_ = ui::label(c.parent, Font::HEADLINE, theme::INK);
    lv_obj_set_width(title_, width_);
    lv_obj_set_style_text_align(title_, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_width(date_, width_);
    lv_obj_set_style_text_align(date_, LV_TEXT_ALIGN_CENTER, 0);
    for (int i = 0; i < 4; ++i) {
      body_[i] = ui::block(c.parent, theme::MARK_BLUE);
      lid_[i] = ui::block(c.parent, theme::MARK_BLUE);
      slit1_[i] = ui::block(c.parent, theme::CARD);
      slit2_[i] = ui::block(c.parent, theme::CARD);
      for (auto *part : {body_[i], lid_[i], slit1_[i], slit2_[i]}) {
        lv_obj_set_style_radius(part, 0, 0);
        lv_obj_add_flag(part, LV_OBJ_FLAG_HIDDEN);
      }
    }
    ui::set_text(title_, plugin_->text("title"));
    on_theme();
  }
  void on_state(JsonObjectConst data) override {
    const std::string state = data["state"] | "";
    status_ = "invalid";
    valid_ = false;
    for (bool &v : active_) v = false;
    if (state.empty()) { status_ = "choose"; return; }
    if (state == "unknown" || state == "unavailable") { status_ = "waiting"; return; }
    if (state == "none") { status_ = "none"; return; }
    bool any = false;
    if (state == "ready") {
      JsonObjectConst attrs = data["attributes"];
      const std::string date = attrs["collection_day"] | "";
      if (!parse_day(date, year_, month_, day_)) return;
      const char *keys[] = {"rest", "paper", "bio", "plastic"};
      for (int i = 0; i < 4; ++i) {
        if (!attrs[keys[i]].is<bool>()) return;
        active_[i] = attrs[keys[i]].as<bool>();
        any = any || active_[i];
      }
    } else {
      // One atomic sensor state: YYYY-MM-DD|rpby (rest, paper, bio, plastic).
      if (state.size() != 15 || state[10] != '|' ||
          !parse_day(state.substr(0, 10), year_, month_, day_)) return;
      for (int i = 0; i < 4; ++i) {
        const char bit = state[11 + i];
        if (bit != '0' && bit != '1') return;
        active_[i] = bit == '1';
        any = any || active_[i];
      }
    }
    if (!any) return;
    valid_ = true;
  }
  void on_tick(uint32_t epoch) override {
    bool show = valid_;
    const char *status = status_;
    if (show && epoch) {
      const auto t = tessera::local_time(epoch);
      if (civil_days(year_, month_, day_) < civil_days(t.year, t.month, t.day)) {
        show = false;
        status = "stale";
      }
    }
    const std::string text = show ? date_label(year_, month_, day_) : plugin_->text(status);
    Font face = Font::HEADLINE;
    for (Font f : {Font::HEADLINE, Font::TITLE, Font::BODY_LARGE, Font::BODY}) {
      face = f;
      if (ui::text_width(text, f) <= width_ && ui::line_height(f) <= height_ / 3) break;
    }
    ui::set_font(date_, face);
    ui::set_text(date_, text);
    const int title_h = ui::line_height(Font::TITLE), date_h = ui::line_height(face);
    lv_obj_set_size(title_, width_, title_h);
    lv_obj_set_pos(title_, 0, 0);
    lv_obj_set_size(date_, width_, date_h);
    lv_obj_set_pos(date_, 0, title_h + ui::px(3));
    const int top = title_h + date_h + ui::px(7);
    const int available = std::max(1, height_ - top);
    const int slot = std::max(1, width_ / 4);
    const int w = std::max(1, std::min(slot * 3 / 5, available * 3 / 4));
    const int h = std::max(1, std::min(available * 4 / 5, w * 5 / 4));
    const int y = top + std::max(0, (available - h) / 2);
    for (int i = 0; i < 4; ++i) {
      bool visible = show && active_[i];
      for (auto *part : {body_[i], lid_[i], slit1_[i], slit2_[i]}) {
        if (visible) lv_obj_clear_flag(part, LV_OBJ_FLAG_HIDDEN);
        else lv_obj_add_flag(part, LV_OBJ_FLAG_HIDDEN);
      }
      if (!visible) continue;
      const int x = i * slot + (slot - w) / 2;
      const int lid_h = std::max(1, h / 8), slit_w = std::max(1, w / 9);
      lv_obj_set_pos(body_[i], x, y + lid_h);
      lv_obj_set_size(body_[i], w, std::max(1, h - lid_h));
      lv_obj_set_pos(lid_[i], x, y);
      lv_obj_set_size(lid_[i], w, lid_h);
      lv_obj_set_pos(slit1_[i], x + w / 3, y + h / 3);
      lv_obj_set_pos(slit2_[i], x + w * 2 / 3, y + h / 3);
      lv_obj_set_size(slit1_[i], slit_w, std::max(1, h / 2));
      lv_obj_set_size(slit2_[i], slit_w, std::max(1, h / 2));
    }
  }
  void on_theme() override {
    ui::set_color(title_, theme::INK);
    ui::set_color(date_, theme::INK);
    // Pink and yellow are not Tessera mark roles. LVGL's named palette preserves
    // the user's required bin colours on both themes; text uses theme roles.
    const lv_color_t colours[] = {lv_palette_main(LV_PALETTE_PINK), ui::color(theme::MARK_BLUE),
                                  ui::color(theme::MARK_GREEN), lv_palette_main(LV_PALETTE_YELLOW)};
    for (int i = 0; i < 4; ++i) {
      lv_obj_set_style_bg_color(body_[i], colours[i], 0);
      lv_obj_set_style_bg_color(lid_[i], colours[i], 0);
      lv_obj_set_style_bg_color(slit1_[i], ui::color(theme::CARD), 0);
      lv_obj_set_style_bg_color(slit2_[i], ui::color(theme::CARD), 0);
    }
  }
 private:
  const tessera::Plugin *plugin_;
  int width_ = 0, height_ = 0, year_ = 0, month_ = 0, day_ = 0;
  bool active_[4] = {false, false, false, false}, valid_ = false;
  const char *status_ = "choose";
  lv_obj_t *title_{}, *date_{};
  lv_obj_t *body_[4]{}, *lid_[4]{}, *slit1_[4]{}, *slit2_[4]{};
};

void WasteCard::setup() { add_tile("next", [this]() { return new NextTile(this); }); }
}
