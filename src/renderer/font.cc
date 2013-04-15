// font.cc -- Part of the Snow Engine
// Copyright (c) 2013 Noel R. Cower. All rights reserved.

#include "font.hh"
#include "../data/database.hh"
#include "../ext/utf8.h"
#include "draw_2d.hh"
#include <algorithm>


namespace u8 = utf8::unchecked;


namespace snow {


namespace {


// SQL for getting a single font's info from a database
const string font_info_query_string {
  "select * from font_info where name = :font_id limit 1"
};

// SQL for getting glyphs from a database
const string font_glyph_query_string {
  "select * from font_glyphs where font_id = :font_id"
};

// SQL for getting kernings from a database
const string font_kern_query_string {
  "select * from font_kernings where font_id = :font_id"
};


} // namespace <anon>


/*==============================================================================
  ctor(db, name)

    Constructs a font loaded from the database with the given name. If no such
    font is found, an exception is thrown. In that case, the font is invalid
    and you should not touch it ever again.
==============================================================================*/
rfont_t::rfont_t(database_t &db, const string &name) :
  name_(name)
{
  load_from_db(db);
} // rfont_t::rfont_t()



/*==============================================================================
  dtor
==============================================================================*/
rfont_t::~rfont_t()
{
  name_.clear();
  glyphs_.clear();
  kerns_.clear();
  pages_.clear();
}



/*==============================================================================
  name

    Returns the name of the font.
==============================================================================*/
const string &rfont_t::name() const
{
  return name_;
}



auto rfont_t::line_height() const -> float
{
  return line_height_;
}



auto rfont_t::leading() const -> float
{
  return leading_;
}



auto rfont_t::ascent() const -> float
{
  return ascent_;
}



auto rfont_t::descent() const -> float
{
  return descent_;
}



auto rfont_t::bbox_min() const -> const vec2f_t &
{
  return bbox_min_;
}



auto rfont_t::bbox_max() const -> const vec2f_t &
{
  return bbox_max_;
}



/*==============================================================================
  font_page_count

    Returns the number of font pages for this font.
==============================================================================*/
int rfont_t::font_page_count() const
{
  return pages_.size();
}



/*==============================================================================
  set_font_page

    Sets the font page to the given material.
==============================================================================*/
void rfont_t::set_font_page(int page, rmaterial_t *mat)
{

  pages_.at(page) = mat;
}



/*==============================================================================
  font_page

    Returns the font page for the given index (0..num_font_pages)
==============================================================================*/
rmaterial_t *rfont_t::font_page(int page) const
{
  return pages_.at(page);
}



/*==============================================================================
  draw_text

    Draws the given UTF-8 string on the screen at the given baseline. Does not
    use screen-scaling.

    TODO: add fix for screen-scaling
==============================================================================*/
void rfont_t::draw_text(
  rdraw_2d_t &draw,
  const vec2f_t &baseline,
  const string &text,
  const vec4_t<uint8_t> &color,
  float scale) const
{
  vec2f_t pos = {
    baseline.x,
    baseline.y// + ascent_ * scale
  };
  vec2f_t head = pos;

  uint32_t last_code = 0;
  auto iter = text.begin();
  auto str_end = utf8::find_invalid(iter, text.end());
  auto glyph_end = glyphs_.cend();
  // unknown character
  glyphmap_t::const_iterator unknown = glyphs_.find(0xFFFD /* replacement character */ );

  while (iter != str_end) {
    uint32_t code = u8::next(iter);

    if (code == '\n') {
      last_code = -1;
      head.y -= line_height_ * scale;
      pos = head;
      continue;
    }

    glyphmap_t::const_iterator glyph_iter = glyphs_.find(code);
    if (glyph_iter == glyph_end) {
      glyph_iter = unknown;
    }

    if (glyph_iter != glyph_end) {
      const glyph_t &glyph = glyph_iter->second;

      pos.x += kern_for(last_code, code) * scale;

      if (glyph.page < pages_.size()) {
        rmaterial_t *page_mat = pages_[glyph.page];

        if (glyph.size.x * glyph.size.y > 0) {
          draw.draw_rect_raw(
            pos + glyph.offset * scale, glyph.size * scale,
            color,
            page_mat,
            glyph.uv_min, glyph.uv_max);
        }
      }

      pos += glyph.advance * scale;
    }
  }
}



/*==============================================================================
  kern_for

    Returns the kerning for the provided glyphs (where the pair is
    {first, second}). If no kerning exists for the pair, returns 0.
==============================================================================*/
float rfont_t::kern_for(uint32_t first, uint32_t second) const
{
  const auto kiter = kerns_.find({first, second});
  if (kiter != kerns_.end())
    return kiter->second;
  return 0;
}



/*==============================================================================
  load_from_db

    Loads the font from the database.
==============================================================================*/
void rfont_t::load_from_db(database_t &db)
{
  if (!db.is_open()) {
    s_throw(std::invalid_argument, "Database is not open");
  }

  // To save on error handling code, let the DB throw if something bad happens
  bool throw_reset = db.throw_on_error();
  db.set_throw_on_error(true);

  int font_id = INT_MIN;    // Font row ID in the database
  unsigned num_glyphs, num_kerns;

  // Look for font info for the given name, throw exception if none found
  {
    dbstatement_t info_query = db.prepare(font_info_query_string);
    info_query.bind_text_static(":font_id", name_);

    for (dbresult_t &fir : info_query) {
      font_id      = fir.column_int("font_id");
      line_height_ = fir.column_float("line_height");
      leading_     = fir.column_float("leading");
      ascent_      = fir.column_float("ascent");
      descent_     = fir.column_float("descent");
      bbox_min_    = { fir.column_float("bbox_min_x"), fir.column_float("bbox_min_y") };
      bbox_max_    = { fir.column_float("bbox_max_x"), fir.column_float("bbox_max_y") };
      pages_.resize(fir.column_uint("pages"));
      page_size_   = {
        fir.column_uint("page_width"),
        fir.column_uint("page_height")
      };

      num_glyphs   = fir.column_uint("num_glyphs");
      num_kerns    = fir.column_uint("num_kernings");
    }
  }

  // Crap pants if no font found.
  if (font_id == INT_MIN) {
    s_throw(std::invalid_argument, "No font with given name found");
  }

  // Load kerns/glyphs as needed
  if (num_glyphs) {
    load_glyphs_from_db(db, font_id);
  }

  if (num_kerns) {
    load_kerns_from_db(db, font_id);
  }

  // TODO: Load page materials (<NAME>_<PAGENUM>.png)
  // pages[N] = get_material(...);

  db.set_throw_on_error(throw_reset);
}



/*==============================================================================
  load_glyphs_from_db

    Loads glyphs for the given font_id from the database.
==============================================================================*/
void rfont_t::load_glyphs_from_db(database_t &db, const int font_id)
{
  dbstatement_t glyph_query = db.prepare(font_glyph_query_string);
  glyph_query.bind_int(":font_id", font_id);

  vec2f_t page_scale = ((vec2f_t)page_size_).inverse();

  for (auto &fgr : glyph_query) {
    rfont_t::glyph_t glyph;

    glyph.page   = fgr.column_uint("page");
    // Round the glyph values to its pixel edges so they're used without
    // odd filtering issues.
    glyph.uv_min = {
      fgr.column_float("frame_x"),
      fgr.column_float("frame_y")
    };
    glyph.uv_max = glyph.uv_min + vec2f_t::make(
      fgr.column_float("frame_width"),
      fgr.column_float("frame_height")
    );
    glyph.size    = glyph.uv_max - glyph.uv_min;
    glyph.advance = {
      fgr.column_float("advance_x"),
      fgr.column_float("advance_y")
    };
    glyph.offset  = {
      fgr.column_float("offset_x"),
      fgr.column_float("offset_y")
    };

    glyph.uv_min   *= page_scale;
    glyph.uv_max   *= page_scale;
    glyph.uv_min.y = 1.0f - glyph.uv_min.y;
    glyph.uv_max.y = 1.0f - glyph.uv_max.y;
    // And swap the min/max V coordinates
    std::swap(glyph.uv_min.y, glyph.uv_max.y);

    glyphs_.insert({
      fgr.column_uint("code"),
      glyph
    });
  }
}



/*==============================================================================
  load_kerns_from_db

    Loads kernings for the given font_id from the given database. Will skip
    kernings for glyphs not in the font already.
==============================================================================*/
void rfont_t::load_kerns_from_db(database_t &db, const int font_id)
{
  dbstatement_t kern_query = db.prepare(font_kern_query_string);
  kern_query.bind(":font_id", font_id);
  const auto kterm = kerns_.cend();
  const auto gterm = glyphs_.cend();
  for (auto &fkr : kern_query) {
    const uint32_t first = fkr.column_uint("first_code");
    const uint32_t second = fkr.column_uint("second_code");
    const float amount = fkr.column_float("amount");
    const std::pair<uint32_t, uint32_t> code_pair { first, second };

    if (kerns_.find(code_pair) != kterm ||
        glyphs_.find(first) == gterm ||
        glyphs_.find(second) == gterm) {
      s_log_note("Skipping kerning for glyph pair <%u, %u>", first, second);
      continue;
    }

    kerns_.insert({
      // First/second glyph pair
      code_pair,
      // Kerning for glyph pair
      amount
    });
  }
}


} // namespace snow
