#ifndef __SNOW__SLOT_MASK_HH__
#define __SNOW__SLOT_MASK_HH__

#include <cstdint>
#include <iostream>
#include <vector>


namespace snow {


template <typename HT, typename CT>
struct slot_mask_t;

template <typename HT, typename CT>
std::ostream &operator << (std::ostream &out, const slot_mask_t<HT, CT> &in);

template <typename HT = int, typename CT = unsigned int>
struct slot_mask_t
{
  using handle_t = HT;
  using count_t = CT;

  static const handle_t NO_HANDLE = 0;

  slot_mask_t(size_t size);
  ~slot_mask_t() = default;

  inline size_t size() const { return slots_.size(); }
  void resize(size_t new_size);

  size_t slots_free_at(size_t index) const;
  bool index_is_free(size_t index, size_t slots) const;
  std::pair<bool, size_t> find_free_index(size_t slots, size_t from = 0) const;
  // Handle may not be NO_HANDLE (0)
  void consume_index(size_t index, size_t slots, handle_t handle);
  // Must pass same handle for slots otherwise the consumed slots
  // cannot be released.
  void release_index(size_t index, size_t slots, handle_t handle);

private:
  friend std::ostream &operator << <> (std::ostream &out, const slot_mask_t<HT, CT> &in);

  void join_same(size_t from);

  struct slot_t
  {
    handle_t handle;    // 0 = unused
    count_t count;      // number to right, counting self
  };

  std::vector<slot_t> slots_;
};



template <typename HT, typename CT>
std::ostream &operator << (std::ostream &out, const slot_mask_t<HT, CT> &in)
{
  out << '{';
  for (const auto &slot : in.slots_)
    out << ((slot.handle == 0) ? '-' : '+');
  return (out << '}');
}



template <typename HT, typename CT>
slot_mask_t<HT, CT>::slot_mask_t(size_t size)
{
  resize(size);
}



template <typename HT, typename CT>
void slot_mask_t<HT, CT>::resize(size_t new_size)
{
  slots_.resize(new_size, { NO_HANDLE, 1 });
  if (new_size == 0) {
    return;
  }
  join_same(new_size - 1);
}



template <typename HT, typename CT>
size_t slot_mask_t<HT, CT>::slots_free_at(size_t index) const
{
  if (index >= slots_.size())
    return 0;

  return slots_[index].count;
}



template <typename HT, typename CT>
bool slot_mask_t<HT, CT>::index_is_free(size_t index, size_t slots) const
{
  if (index + slots > slots_.size())
    return false;

  const slot_t &slot = slots_[index];
  return slot.count >= slots && slot.handle == NO_HANDLE;
}



template <typename HT, typename CT>
std::pair<bool, size_t> slot_mask_t<HT, CT>::find_free_index(size_t slots, size_t from) const
{
  if (slots == 0) {
    return { false, 0 };
  }

  const size_t length = slots_.size() - (slots - 1);
  size_t index = from;
  for (; index < length; ++index) {
    const slot_t &slot = slots_[index];
    if (slot.handle == 0 && slots <= slot.count) {
      return { true, index };
    } else {
      index += slot.count - 1;
    }
  }
  return { false, 0 };
}



template <typename HT, typename CT>
void slot_mask_t<HT, CT>::consume_index(size_t index, size_t slots, handle_t handle)
{
  size_t from = index;
  while (slots) {
    slot_t &slot = slots_[index];
    slot.handle = handle;
    slot.count = slots;
    index += 1;
    slots -= 1;
  }
  if (from > 0) {
    join_same(from - 1);
  }
}



template <typename HT, typename CT>
void slot_mask_t<HT, CT>::release_index(size_t index, size_t slots, handle_t handle)
{
  size_t from = index;
  while (slots) {
    slot_t &slot = slots_[index];
    if (slot.handle != handle) {
      if (index > 0 && index != from) {
        join_same(index - 1);
      }
      return;
    }
    slot.handle = NO_HANDLE;
    slot.count = slots;
    index += 1;
    slots -= 1;
  }
  if (from > 0) {
    join_same(from - 1);
  }
}



template <typename HT, typename CT>
void slot_mask_t<HT, CT>::join_same(size_t from)
{
  const int handle = slots_[from].handle;
  int counter = 1;
  {
    const size_t right = from + 1;
    if (right < slots_.size() && handle == slots_[right].handle) {
      counter += slots_[right].count;
    }
  }
  for (;;) {
    slot_t &slot = slots_[from];
    if (slot.handle != handle) {
      return;
    }
    slot.count = counter;
    if (from == 0) {
      return;
    }
    counter += 1;
    from -= 1;
  }
}


} // namespace snow

#endif /* end __SNOW__SLOT_MASK_HH__ include guard */
