#ifndef __SNOW__SLOT_MASK_2D_HH__
#define __SNOW__SLOT_MASK_2D_HH__

#include "slot_mask.hh"
#include <snow/math/vec2.hh>


namespace snow {


template <typename S, typename T>
std::ostream &operator << (std::ostream &out, const std::pair<S,T> &in)
{
  return out << '{' << in.first << ", " << in.second << '}';
}


template <typename HT, typename CT>
struct slot_image_t;

template <typename HT, typename CT>
std::ostream &operator << (std::ostream &out, const slot_image_t<HT, CT> &in);


template <typename HT = int, typename CT = unsigned int>
struct slot_image_t
{
  using slot_row_t = slot_mask_t<HT, CT>;
  using handle_t = typename slot_row_t::handle_t;
  using count_t = typename slot_row_t::count_t;
  using dim_t = vec2_t<size_t>;

  slot_image_t(size_t width, size_t height);
  ~slot_image_t() = default;

  inline size_t width() const { return width_; }
  inline size_t height() const { return height_; }
  void resize(size_t width, size_t height);

  size_t columns_free_at(const dim_t &pos) const;
  size_t rows_free_at(const dim_t &pos) const;
  bool pos_is_free(const dim_t &pos) const;
  std::pair<bool, dim_t> find_free_pos(const dim_t &size) const;
  void consume_subimage(const dim_t &pos, const dim_t &size, handle_t handle);
  void release_subimage(const dim_t &pos, const dim_t &size, handle_t handle);

private:
  friend std::ostream &operator << <> (std::ostream &out, const slot_image_t<HT, CT> &in);

  inline bool rows_free(dim_t pos, dim_t size) const
  {
    while (size.y && pos.y < height_) {
      if (!slot_rows_[pos.y].index_is_free(pos.x, size.x)) {
        return false;
      }
      pos.y += 1;
      size.y -= 1;
    }
    return true;
  }

  size_t width_;
  size_t height_;
  std::vector<slot_row_t> slot_rows_;
};



template <typename HT, typename CT>
std::ostream &operator << (std::ostream &out, const slot_image_t<HT, CT> &in)
{
  out << '{';
  auto iter = in.slot_rows_.cbegin();
  auto end = in.slot_rows_.cend();
  while (iter != end) {
    out << *iter;
    ++iter;
    if (iter != end)
      out << std::endl << ' ';
  }
  return (out << '}');
}



template <typename HT, typename CT>
slot_image_t<HT, CT>::slot_image_t(size_t width, size_t height) :
  width_(width),
  height_(height),
  slot_rows_(height, slot_row_t(width))
{
  // nop
}



template <typename HT, typename CT>
void slot_image_t<HT, CT>::resize(size_t width, size_t height)
{
  if (height == 0) {
    throw std::invalid_argument("Height of slot image cannot be zero");
  } else if (width == 0) {
    throw std::invalid_argument("Height of slot image cannot be zero");
  }

  if (width_ != width) {
    width_ = width;
    for (slot_row_t &row : slot_rows_) {
      row.resize(width);
    }
  }

  if (height != height_) {
    if (height_ < height) {
      slot_rows_.resize(height, slot_row_t(width));
    } else {
      slot_rows_.resize(height);
    }
    height_ = height;
  }
}



template <typename HT, typename CT>
size_t slot_image_t<HT, CT>::columns_free_at(const dim_t &pos) const
{
  if (pos.y >= height_)
    return 0;
  else if (pos.x >= width_)
    return 0;

  return slot_rows_[pos.y].slots_free_at(pos.x);
}



template <typename HT, typename CT>
size_t slot_image_t<HT, CT>::rows_free_at(const dim_t &pos) const
{
  if (pos.y >= height_)
    return 0;
  else if (pos.x >= width_)
    return 0;

  size_t count = 0;
  size_t row = pos.y;
  for (; row < height_ && slot_rows_[row].slots_free_at(pos.x); ++row, ++count);
  return count;
}



template <typename HT, typename CT>
bool slot_image_t<HT, CT>::pos_is_free(const dim_t &pos) const
{
  return false;
}



template <typename HT, typename CT>
auto slot_image_t<HT, CT>::find_free_pos(const dim_t &size) const -> std::pair<bool, dim_t>
{
  const size_t max_col = width_ - size.x;
  const size_t max_row = height_ - size.y;

  dim_t pos = {0, 0};
  size_t free_cols = 0;
  std::pair<bool, size_t> pair;
  while (pos.y <= max_row) {
    const slot_row_t &slots = slot_rows_[pos.y];
    pos.x = 0;

    do {
      pair = slots.find_free_index(size.x, pos.x);
      if (!pair.first) {
        break;
      }
      pos.x = pair.second;

      free_cols = slots.slots_free_at(pair.second);
      while (free_cols >= size.x) {
        if (rows_free(pos, size))
          return { true, pos };

        free_cols -= 1;
        pos.x += 1;
      }
    } while (pos.x <= max_row);

    pos.y += 1;
  }
  return { false, dim_t::zero };
}



template <typename HT, typename CT>
void slot_image_t<HT, CT>::consume_subimage(const dim_t &pos, const dim_t &size, handle_t handle)
{
  size_t row = pos.y;
  const size_t term = pos.y + size.y;
  for (; row < term; ++row) {
    slot_rows_[row].consume_index(pos.x, size.x, handle);
  }
}



template <typename HT, typename CT>
void slot_image_t<HT, CT>::release_subimage(const dim_t &pos, const dim_t &size, handle_t handle)
{
  size_t row = pos.y;
  const size_t term = pos.y + size.y;
  for (; row < term; ++row) {
    slot_rows_[row].release_index(pos.x, size.x, handle);
  }
}


} // namespace snow

#endif /* end __SNOW__SLOT_MASK_2D_HH__ include guard */
