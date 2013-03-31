#ifndef __SNOW__FONT_HH__
#define __SNOW__FONT_HH__

#include <snow/config.hh>
#include <snow/math/math3d.hh>
#include <map>
#include <vector>


namespace snow {


struct rdraw_2d;
struct rfont_t;
struct rmaterial_t;
struct database_t;


struct rfont_t
{
  rfont_t(database_t &db, const string &name);
  ~rfont_t();

  inline const string &name() const { return name_; }

  void set_font_page(int page, rmaterial_t *mat);
  rmaterial_t *font_page(int page) const;

  void draw_text(rdraw_2d &draw, const vec2f_t &baseline, const string &text, float scale = 1.0f) const;

  // void add_glyph(int page, )
private:
  struct glyph_t
  {
    inline glyph_t() :
      page(-1), uv_min(vec2f_t::zero), uv_max(vec2f_t::zero), size(vec2f_t::zero),
      advance(vec2f_t::zero), offset(vec2f_t::zero)
    {
      // nop
    }
    int page;
    vec2f_t uv_min;
    vec2f_t uv_max;
    vec2f_t size;
    vec2f_t advance;
    vec2f_t offset;
  };

  using glyphmap_t = std::map<int, glyph_t>;
  // yes, this is totally sane
  using kern_pair_t = std::pair<int, int>;
  using kernmap_t = std::map<kern_pair_t, float>;

  string name_;
  glyphmap_t glyphs_;
  kernmap_t  kerns_;
  std::vector<rmaterial_t *> pages_;
  vec2_t<int> page_size_;
  float line_height_;
  float leading_;
  float ascent_;
  float descent_;
  vec2f_t bbox_min_;
  vec2f_t bbox_max_;


  inline float kern_for(int first, int second) const
  {
    const auto kiter = kerns_.find({first, second});
    if (kiter != kerns_.end())
      return kiter->second;
    return 0;
  }


};


} // namespace snow

#endif /* end __SNOW__FONT_HH__ include guard */
