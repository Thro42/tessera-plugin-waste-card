#pragma once
// Small test double; production builds use Tessera and LVGL, not this file.
#include <functional>
#include <string>
#include <map>
#include <vector>
#include <cstdint>
#include <initializer_list>

struct lv_obj_t { int x=0,y=0,w=0,h=0,color=0; bool hidden=false; std::string text; };
using lv_color_t = int;
constexpr int LV_OBJ_FLAG_HIDDEN=1, LV_TEXT_ALIGN_CENTER=0, LV_PALETTE_PINK=1, LV_PALETTE_YELLOW=2;
inline lv_color_t lv_palette_main(int c) { return c; }
inline void lv_obj_set_size(lv_obj_t *o,int w,int h) {o->w=w;o->h=h;}
inline void lv_obj_set_width(lv_obj_t *o,int w) {o->w=w;}
inline void lv_obj_set_pos(lv_obj_t *o,int x,int y) {o->x=x;o->y=y;}
inline void lv_obj_set_style_text_align(lv_obj_t*,int,int) {}
inline void lv_obj_set_style_radius(lv_obj_t*,int,int) {}
inline void lv_obj_set_style_bg_color(lv_obj_t *o,int c,int) {o->color=c;}
inline void lv_obj_add_flag(lv_obj_t *o,int) {o->hidden=true;}
inline void lv_obj_clear_flag(lv_obj_t *o,int) {o->hidden=false;}

struct Value {
  std::string text; bool boolean=false, is_bool=false; std::map<std::string, Value> object;
  Value() = default;
  explicit Value(const std::string &s): text(s) {}
  explicit Value(bool b): boolean(b),is_bool(true) {}
  const Value &operator[](const char *key) const { auto it=object.find(key); static const Value empty; return it==object.end()?empty:it->second; }
  const char *operator|(const char *fallback) const { return text.empty()?fallback:text.c_str(); }
  template<class T> bool is() const { return is_bool; }
  template<class T> T as() const { return static_cast<T>(boolean); }
};
using JsonObjectConst = Value;
namespace theme { enum Role { INK, CARD, MARK_BLUE, MARK_GREEN }; }
namespace esphome::tessera {
enum class Font { HEADLINE, TITLE, BODY_LARGE, BODY };
struct LocalTime { int year,month,day; };
inline LocalTime local_time(uint32_t) { return {2026,11,10}; }
struct TileContext { lv_obj_t *parent; int width,height; };
class Tile { public: virtual ~Tile()=default; virtual void create(const TileContext&) {} virtual void on_state(JsonObjectConst) {} virtual void on_tick(uint32_t) {} virtual void on_theme() {} };
class Plugin { public: virtual ~Plugin()=default; const char *text(const char *s) const { return s; } void add_tile(const char*,std::function<Tile*()> f) { factory=f; } std::function<Tile*()> factory; };
namespace ui {
inline std::vector<lv_obj_t*> objects;
inline lv_obj_t *label(lv_obj_t*,Font,theme::Role) { auto *o=new lv_obj_t; objects.push_back(o); return o; }
inline lv_obj_t *block(lv_obj_t*,theme::Role) { auto *o=new lv_obj_t; objects.push_back(o); return o; }
inline int line_height(Font) { return 14; }
inline int text_width(const std::string &s,Font) { return int(s.size())*7; }
inline int px(int n) { return n; }
inline void set_color(lv_obj_t*,theme::Role) {}
inline void set_font(lv_obj_t*,Font) {}
inline void set_text(lv_obj_t *o,const std::string &s) { o->text=s; }
inline lv_color_t color(theme::Role r) { return 10 + r; }
}
}
