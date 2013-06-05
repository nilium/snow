// font.hh -- Part of the Snow Engine
// Copyright (c) 2013 Noel R. Cower. All rights reserved.

#ifndef __SNOW__FONT_HH__
#define __SNOW__FONT_HH__

#include "../config.hh"
#include <snow/math/math3d.hh>
#include <map>
#include <utility>
#include <vector>


namespace snow {


struct rdraw_2d_t;
struct rfont_t;
struct rmaterial_t;
struct database_t;


struct rfont_t
{
  rfont_t(database_t &db, const string &name);
  ~rfont_t();

  bool valid() const;

  auto name() const -> const string &;
  auto line_height() const -> float;
  auto leading() const -> float;
  auto ascent() const -> float;
  auto descent() const -> float;
  auto bbox_min() const -> const vec2f_t &;
  auto bbox_max() const -> const vec2f_t &;

  auto font_page_count() const -> size_t;
  void set_font_page(int page, rmaterial_t *mat);
  auto font_page(int page) const -> rmaterial_t *;

  void draw_text(rdraw_2d_t &draw, const vec2f_t &baseline, const string &text,
                 const vec4f_t &color = { 255, 255, 255, 255 },
                 bool ignore_newlines = true, float scale = 1.0f) const;



private:

  void load_from_db(database_t &db);
  void load_glyphs_from_db(database_t &db, const int font_id);
  void load_kerns_from_db(database_t &db, const int font_id);
  auto kern_for(uint32_t first, uint32_t second) const -> float;


  struct glyph_t
  {
    glyph_t() = default;

    uint32_t page    = -1;
    vec2f_t  uv_min  = vec2f_t::zero;
    vec2f_t  uv_max  = vec2f_t::zero;
    vec2f_t  size    = vec2f_t::zero;
    vec2f_t  advance = vec2f_t::zero;
    vec2f_t  offset  = vec2f_t::zero;
  };

  using glyphmap_t = std::map<uint32_t, glyph_t>;
  // yes, this is totally sane
  using kern_pair_t = std::pair<uint32_t, uint32_t>;
  using kernmap_t = std::map<kern_pair_t, float>;

  bool                       valid_ = false;

  float                      line_height_;
  float                      leading_;
  float                      ascent_;
  float                      descent_;

  vec2f_t                    bbox_min_;
  vec2f_t                    bbox_max_;
  vec2_t<uint32_t>           page_size_;

  string                     name_;
  glyphmap_t                 glyphs_;
  kernmap_t                  kerns_;
  std::vector<rmaterial_t *> pages_;

};


} // namespace snow

#endif /* end __SNOW__FONT_HH__ include guard */
