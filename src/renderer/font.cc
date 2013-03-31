#include "font.hh"
#include "../data/database.hh"
#include "draw_2d.hh"
#include <algorithm>


namespace snow {


namespace {


enum font_column_t : int {
  // font_glyphs columns
  KCODE=0,
  KPAGE,
  KFRAME_X,
  KFRAME_Y,
  KFRAME_WIDTH,
  KFRAME_HEIGHT,
  KADVANCE_X,
  KADVANCE_Y,
  KOFFSET_X,
  KOFFSET_Y,

  // font_info columns
  KFONT_ID=0,
  KPAGES,
  KNUM_GLYPHS,
  KNUM_KERNINGS,
  KLINE_HEIGHT,
  KLEADING,
  KASCENT,
  KDESCENT,
  KPAGE_WIDTH,
  KPAGE_HEIGHT,
  KBBOX_MIN_X,
  KBBOX_MAX_X,
  KBBOX_MIN_Y,
  KBBOX_MAX_Y,
  KNAME,

  // font_kernings columns
  KFIRST_CODE=0,
  KSECOND_CODE,
  KAMOUNT
};


const string font_info_query_string {
  "select "
  "font_id, "
  "pages, "
  "num_glyphs, "
  "num_kernings, "
  "line_height, "
  "leading, "
  "ascent, "
  "descent, "
  "page_width, "
  "page_height, "
  "bbox_min_x, "
  "bbox_max_x, "
  "bbox_min_y, "
  "bbox_max_y "
  " from font_info where name = ? limit 1"
};

const string font_glyph_query_string {
  "select "
  "code, "
  "page, "
  "frame_x, "
  "frame_y, "
  "frame_width, "
  "frame_height, "
  "advance_x, "
  "advance_y, "
  "offset_x, "
  "offset_y "
  " from font_glyphs where font_id = ?"
};

const string font_kern_query_string {
  "select "
  "first_code, "
  "second_code, "
  "amount "
  " from font_kernings where font_id = ?"
};


} // namespace <anon>


rfont_t::rfont_t(database_t &db, const string &name) :
  name_(name)
{
  if (!db.is_open()) {
    throw std::invalid_argument("Database is not open");
  }

  // To save on error handling code, let the DB throw if something bad happens
  bool throw_reset = db.throw_on_error();
  db.set_throw_on_error(true);

  try {
    int font_id = INT_MIN;    // Font row ID in the database
    vec2f_t page_scale = vec2f_t::zero;

    // Look for font info for the given name, throw exception if none found
    {
      dbstatement_t info_query = db.prepare(font_info_query_string);
      info_query.bind_text_static(1, name);

      info_query.execute([&](dbresult_t &fir) {
        font_id      = fir.column_int(KFONT_ID);
        line_height_ = fir.column_float(KLINE_HEIGHT);
        leading_     = fir.column_float(KLEADING);
        ascent_      = fir.column_float(KASCENT);
        descent_     = fir.column_float(KDESCENT);
        bbox_min_    = { fir.column_float(KBBOX_MIN_X), fir.column_float(KBBOX_MIN_Y) };
        bbox_max_    = { fir.column_float(KBBOX_MAX_X), fir.column_float(KBBOX_MAX_Y) };
        pages_.resize(fir.column_int(KPAGES));
        page_size_   = {
          fir.column_int(KPAGE_WIDTH),
          fir.column_int(KPAGE_HEIGHT)
        };
        page_scale   = page_size_;
        page_scale.invert();
      });
    }

    // Crap pants if no font found.
    if (font_id == INT_MIN) {
      throw std::invalid_argument("No font with given name found");
    }

    // Load glyphs
    {
      dbstatement_t glyph_query = db.prepare(font_glyph_query_string);
      glyph_query.bind_int(1, font_id);

      glyph_query.execute([&](dbresult_t &fgr) {
        rfont_t::glyph_t glyph;

        glyph.page = fgr.column_int(KPAGE);
        glyph.uv_min = {
          fgr.column_float(KFRAME_X),
          fgr.column_float(KFRAME_Y)
        };
        glyph.uv_max = {
          fgr.column_float(KFRAME_WIDTH),
          fgr.column_float(KFRAME_HEIGHT)
        };
        glyph.advance = {
          fgr.column_float(KADVANCE_X),
          fgr.column_float(KADVANCE_Y)
        };
        glyph.offset = {
          fgr.column_float(KOFFSET_X),
          fgr.column_float(KOFFSET_Y)
        };

        // Round the glyph frame to its pixel edges
        glyph.uv_max   += glyph.uv_min;

        glyph.uv_min.x = std::floor(glyph.uv_min.x);
        glyph.uv_min.y = std::floor(glyph.uv_min.y);

        glyph.uv_max.x = std::ceil(glyph.uv_max.x);
        glyph.uv_max.y = std::ceil(glyph.uv_max.y);

        glyph.size     = glyph.uv_max - glyph.uv_min;

        glyph.uv_min   *= page_scale;
        glyph.uv_max   *= page_scale;
        glyph.uv_min.y = 1.0f - glyph.uv_min.y;
        glyph.uv_max.y = 1.0f - glyph.uv_max.y;
        std::swap(glyph.uv_min.y, glyph.uv_max.y);


        glyphs_.insert({
          fgr.column_int(KCODE),
          glyph
        });
      });
    }

    // Load kernings
    {
      dbstatement_t kern_query = db.prepare(font_kern_query_string);
      kern_query.bind_int(1, font_id);
      kern_query.execute([&](dbresult_t &fkr) {
        kerns_.insert({
          // First/second glyph pair
          { fkr.column_int(KFIRST_CODE), fkr.column_int(KSECOND_CODE) },
          // Kerning for glyph pair
          fkr.column_float(KAMOUNT)
        });
      });
    }

    db.set_throw_on_error(throw_reset);
  // end try
  } catch (std::exception &ex) {
    // Restore previous throw_on_error state for the DB then continue the throw
    db.set_throw_on_error(throw_reset);
    throw;
  }
} // rfont_t::rfont_t()



rfont_t::~rfont_t()
{
  name_.clear();
  glyphs_.clear();
  kerns_.clear();
  pages_.clear();
}



void rfont_t::set_font_page(int page, rmaterial_t *mat)
{
  pages_.resize(std::max((size_t)page + 1, pages_.size()));
  pages_[page] = mat;
}



rmaterial_t *rfont_t::font_page(int page) const
{
  return pages_[page];
}



void rfont_t::draw_text(
  rdraw_2d &draw,
  const vec2f_t &baseline,
  const string &text,
  float scale) const
{
  vec2f_t loc = {
    baseline.x,
    baseline.y - ascent_ * scale
  };
  vec2f_t head = loc;
  int last_code = -1;
  for (int code : text) {
    if (code == '\n') {
      last_code = -1;
      head.y -= line_height_ * scale;
      loc = head;
    }

    if (code < ' ')
      continue;

    const auto iter = glyphs_.find(code);
    const glyph_t &glyph = iter->second;

    loc.x += kern_for(last_code, code) * scale;

    if (glyph.page >= 0) {
      rmaterial_t *page_mat = pages_[glyph.page];
      if (glyph.size.x * glyph.size.y > 0) {
        draw.draw_rect_raw(
          loc + glyph.offset * scale, glyph.size * scale,
          {255, 255, 255, 255},
          page_mat,
          glyph.uv_min, glyph.uv_max);
      }
    }
    loc += glyph.advance * scale;
  }
}


} // namespace snow
