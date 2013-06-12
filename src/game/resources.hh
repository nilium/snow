/*
  resources.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__RESOURCES_HH__
#define __SNOW__RESOURCES_HH__

#include "../config.hh"
#include "../ext/lexer.hh"
#include "../ext/memory_pool.hh"
#include "../renderer/constants.hh"
#include <snow/memory/ref_counter.hh>
#include <mutex>
#include <set>
#include <unordered_map>


namespace snow {


#define NULL_MATERIAL_NAME ("notex")


struct rfont_t;
struct rtexture_t;
struct rmaterial_t;
struct rprogram_t;
struct rshader_t;


struct resources_t
{
  using nameset_t = std::set<string>;

  resources_t();
  ~resources_t();

  void prepare_resources();

  rfont_t *load_font(const string &name);
  rtexture_t *load_texture(const string &path, bool mipmaps = true);
  rmaterial_t *load_material(const string &name);
  rprogram_t *load_program(const string &name);

  const nameset_t &definition_names() const;
  const nameset_t &definition_locations() const;

  bool name_is_material(const string &name) const;
  bool name_is_program(const string &name) const;

  void release_font(rfont_t *);
  void release_texture(rtexture_t *);
  void release_material(rmaterial_t *);
  void release_program(rprogram_t *);

  void release_all();

  static resources_t &default_resources();

private:

  friend struct resdef_parser_t;

  static const uint64_t font_seed;
  static const uint64_t material_seed;
  static const uint64_t texture_seed;
  static const uint64_t program_seed;
  static const uint64_t vert_shader_seed;
  static const uint64_t frag_shader_seed;

  enum res_kind_t : unsigned
  {
    KIND_FONT,
    KIND_TEXTURE,
    KIND_MATERIAL,
    KIND_PROGRAM,
    KIND_SHADER,
  };

  struct res_t {
    res_kind_t kind;
    union
    {
      rfont_t *font;
      rtexture_t *tex;
      rmaterial_t *mat;
      rprogram_t *prog;
      rshader_t *shader;
    };
  };

  struct resloc_t
  {
    size_t offset;
    size_t length;
    unsigned kind; // resdef_kind_t
    nameset_t::const_iterator resname;
    nameset_t::const_iterator matfile;
  };

  using resmap_t = std::unordered_map<uint64_t, res_t>;
  template <typename T>
  using res_store_t = typename std::aligned_storage<sizeof(uint64_t) + sizeof(T)>::type;
  using fontmap_t = std::unordered_map<uint64_t, nameset_t::const_iterator>;
  using locmap_t = std::unordered_map<uint64_t, resloc_t>;
  using str_inserter_t = std::back_insert_iterator<std::list<nameset_t::const_iterator>>;

  template <typename T, typename... ARGS> T *allocate_resource(uint64_t hash, ARGS&& ...args);
  template <typename T> void destroy_resource(T *res);

  rshader_t *load_shader(const string &path, unsigned kind);
  void release_shader(rshader_t *shader);

  void prepare_fonts();
  void prepare_definitions();

  // Recursively searches for resource files under defs/
  void find_definition_files(const char *dir, str_inserter_t &inserter);
  // Finds definitions within a given file
  void find_definitions(const nameset_t::const_iterator &path);
  // Finds definitions given a list of tokens
  void find_definitions_within(tokenlist_t::const_iterator iter, const tokenlist_t::const_iterator &end, const string::const_iterator &source_begin, const nameset_t::const_iterator &filepath);

  bool load_program_def(rprogram_t *prog, const locmap_t::const_iterator &from);
  bool read_program(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end);
  bool read_program_shader(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end);
  bool read_program_uniform(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end);
  bool read_program_attrib(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end);
  bool read_program_frag_out(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end);

  // Parses the material given the location and lexer for use
  bool load_material_from(rmaterial_t *mat, const locmap_t::const_iterator &from);

  mempool_t pool_;
  nameset_t filepaths_;
  nameset_t def_names_;
  ref_counter_t refs_;
  resmap_t resources_;
  fontmap_t font_dbs_;
  locmap_t res_files_;
  mutable std::recursive_mutex lock_;
};



template <typename T, typename... ARGS>
T *resources_t::allocate_resource(uint64_t hash, ARGS&& ...args)
{
  using res_type_t = res_store_t<T>;
  // res_type_t *store = new res_type_t;
  res_type_t *store = (res_type_t *)pool_malloc(&pool_, sizeof(*store), 1);
  if (!store) {
    s_log_error("Unable to allocate memory for resource");
    return nullptr;
  }
  uint64_t * hash_ptr = (uint64_t *)store;
  hash_ptr[0] = hash;
  T *obj = (T *)&hash_ptr[1];
  new(obj) T(std::forward<ARGS>(args)...);
  return obj;
}



template <typename T>
void resources_t::destroy_resource(T *res)
{
  using res_type_t = res_store_t<T>;
  uint64_t *hash_ptr = ((uint64_t *)res) - 1;
  resources_.erase(hash_ptr[0]);
  res->~T();
  pool_free((res_type_t *)hash_ptr);
  // delete (res_type_t *)hash_ptr;
}


} // namespace snow


#endif /* end __SNOW__RESOURCES_HH__ include guard */
