/*
  resources.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "resources.hh"
#include <snow-ext/hash.hh>
#include "../data/resdef_parser.hh"
#include "../data/database.hh"
#include "../renderer/texture.hh"
#include "../renderer/material.hh"
#include "../renderer/program.hh"
#include "../renderer/shader.hh"
#include "../renderer/font.hh"
#include <list>
#include <map>


#define FONTPAGE_MATERIAL_FORMAT ("fonts/%s_%d")
#define MAX_PATH_LEN (512)
#define RES_POOL_SIZE (128 * 1024 * 1024) /* 128mb */
#define throw_unless_kind(V_, KIND_, NAME_)                                    \
  if ((V_) != KIND_) {                                                         \
    s_throw(std::runtime_error, "Resource '%s' (%0x) is not of type " #KIND_,  \
      (NAME_).c_str(), hash);                                                  \
  }


namespace snow {


namespace {


using tokiter_t = typename tokenlist_t::const_iterator;


const string g_font_db_ext { "*.db" };
const string g_font_name_query {
  "SELECT name FROM 'font_info'"
};
const string g_font_name_col { "name" };


resources_t g_default_resources;


} // namespace <anon>


const uint32_t resources_t::font_seed        = 0x8a8133b9UL;
const uint32_t resources_t::material_seed    = 0x4ee70b42UL;
const uint32_t resources_t::texture_seed     = 0x8d8d956aUL;
const uint32_t resources_t::program_seed     = 0x7ab01992UL;
const uint32_t resources_t::vert_shader_seed = 0xf9cbb1aeUL;
const uint32_t resources_t::frag_shader_seed = 0x70e139c7UL;


resources_t &resources_t::default_resources()
{
  return g_default_resources;
}



resources_t::resources_t()
{
  pool_init(&pool_, RES_POOL_SIZE);
}



resources_t::~resources_t()
{
  release_all();
  pool_destroy(&pool_);
}



void resources_t::prepare_resources()
{
  // FIXME: Add reload_resources to check cached objects for changes and apply
  // changes as needed. (Should this reload all shaders?)
  std::lock_guard<decltype(lock_)> guard(lock_);

  release_all();

  /* Clear out cached data */
  filepaths_.clear();
  font_dbs_.clear();
  res_files_.clear();

  /* Reload everything */
  prepare_fonts();
  prepare_definitions();
}



rfont_t *resources_t::load_font(const string &name)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = murmur3::hash64(name, font_seed);

  {
    const resmap_t::const_iterator iter = resources_.find(hash);
    if (iter != resources_.cend()) {
      throw_unless_kind(iter->second.kind, KIND_FONT, name);
      return refs_.retain(iter->second.font);
    }
  }

  {
    const fontmap_t::const_iterator iter = font_dbs_.find(hash);
    rfont_t *font = nullptr;

    if (iter == font_dbs_.cend()) {
      return nullptr;
    } else {
      database_t db = database_t::read_physfs(*(iter->second), false);

      if (!db.has_error()) {
        s_log_note("Allocating resource %x of kind font", hash);
        font = allocate_resource<rfont_t>(hash, db, name);

        if (!font->valid()) {
          destroy_resource(font);
          font = nullptr;
        }
      }
    }

    int fontpage_index = 0;
    char temp_namebuffer[MAX_PATH_LEN];
    for (; fontpage_index < font->font_page_count(); ++fontpage_index) {
      sqlite3_snprintf(MAX_PATH_LEN, temp_namebuffer, FONTPAGE_MATERIAL_FORMAT,
        name.c_str(), fontpage_index);

      string window { temp_namebuffer };
      rmaterial_t *material = load_material(window);
      if (!material) {
        material = load_material(NULL_MATERIAL_NAME);
      }
      assert(material);

      font->set_font_page(fontpage_index, material);
      assert(font->font_page(fontpage_index));
    }

    if (font) {
      resmap_t::value_type pair {
        hash,
        {
          KIND_FONT,
          { .font = font }
        }
      };
      resources_.insert(pair);
    }

    return font;
  }
}



rtexture_t *resources_t::load_texture(const string &path, bool mipmaps)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = murmur3::hash64(path, texture_seed);

  {
    const resmap_t::const_iterator iter = resources_.find(hash);
    if (iter != resources_.cend()) {
      throw_unless_kind(iter->second.kind, KIND_TEXTURE, path);
      return refs_.retain(iter->second.tex);
    }
  }

  s_log_note("Allocating resource %x of kind texture", hash);
  rtexture_t *tex = allocate_resource<rtexture_t>(hash);
  if (!::snow::load_texture_2d(path, *tex, mipmaps, TEX_COMP_DEFAULT)) {
    destroy_resource(tex);
    return nullptr;
  }

  resmap_t::value_type pair {
    hash,
    {
      KIND_TEXTURE,
      { .tex = tex }
    }
  };
  resources_.insert(pair);

  return tex;
}



bool resources_t::load_material_from(rmaterial_t *mat,
  const locmap_t::const_iterator &from)
{
  string source;
  {
    const resloc_t &loc = from->second;
    source.resize(loc.length);
    PHYSFS_File *file = PHYSFS_openRead(loc.matfile->c_str());
    PHYSFS_seek(file, loc.offset);
    PHYSFS_readBytes(file, source, loc.length);
    PHYSFS_close(file);
  }

  lexer_t lex;
  lex.set_skip_comments(true);
  lex.set_skip_newlines(true);
  lex.run(source);

  resdef_parser_t parser;
  parser.set_tokens(lex.tokens());
  return parser.read_material(*mat, *this) == PARSE_OK;
}



rmaterial_t *resources_t::load_material(const string &name)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = murmur3::hash64(name, material_seed);

  {
    const resmap_t::const_iterator iter = resources_.find(hash);
    if (iter != resources_.cend()) {
      throw_unless_kind(iter->second.kind, KIND_MATERIAL, name);
      return refs_.retain(iter->second.mat);
    }
  }

  const locmap_t::const_iterator matloc = res_files_.find(hash);
  if (matloc == res_files_.end()) {
    s_log_error("No material named '%s' was found", name.c_str());
    return nullptr;
  }

  s_log_note("Allocating resource %x of kind material", hash);
  rmaterial_t *mat = allocate_resource<rmaterial_t>(hash);
  if (!mat) {
    s_log_error("Unable to allocate material '%s'", name.c_str());
    return nullptr;
  }

  if (!load_material_from(mat, matloc)) {
    destroy_resource(mat);
    return nullptr;
  }

  resmap_t::value_type pair {
    hash,
    {
      KIND_MATERIAL,
      { .mat = mat }
    }
  };
  resources_.insert(pair);

  return mat;
}



bool resources_t::load_program_def(rprogram_t *prog, const locmap_t::const_iterator &from)
{
  string source;
  {
    const resloc_t &loc = from->second;
    source.resize(loc.length);
    PHYSFS_File *file = PHYSFS_openRead(loc.matfile->c_str());
    PHYSFS_seek(file, loc.offset);
    PHYSFS_readBytes(file, source, loc.length);
    PHYSFS_close(file);
  }

  lexer_t lex;
  lex.set_skip_comments(true);
  lex.set_skip_newlines(true);
  lex.run(source);

  resdef_parser_t parser;
  parser.set_tokens(lex.tokens());
  return parser.read_shader(*prog, *this) == PARSE_OK;
}



rprogram_t *resources_t::load_program(const string &name)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = murmur3::hash64(name, program_seed);

  {
    const resmap_t::const_iterator iter = resources_.find(hash);
    if (iter != resources_.cend()) {
      throw_unless_kind(iter->second.kind, KIND_PROGRAM, name);
      return refs_.retain(iter->second.prog);
    }
  }

  const locmap_t::const_iterator progloc = res_files_.find(hash);
  if (progloc == res_files_.end()) {
    s_log_error("No program named '%s' was found", name.c_str());
    return nullptr;
  }

  s_log_note("Allocating resource %x of kind program", hash);
  rprogram_t *prog = allocate_resource<rprogram_t>(hash);

  if (!prog) {
    s_log_error("Unable to allocate program '%s'", name.c_str());
    return nullptr;
  }

  if (!load_program_def(prog, progloc)) {
    destroy_resource(prog);
    return nullptr;
  } else if (!prog->link()) {
    s_log_error("Unable to link program '%s': %s",
      name.c_str(), prog->error_string().c_str());
  }

  resmap_t::value_type pair {
    hash,
    {
      KIND_PROGRAM,
      { .prog = prog }
    }
  };
  resources_.insert(pair);

  return prog;
}



rshader_t *resources_t::load_shader(const string &path, unsigned kind)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = murmur3::hash64(path, kind == GL_FRAGMENT_SHADER
    ? frag_shader_seed : vert_shader_seed);

  {
    const resmap_t::const_iterator iter = resources_.find(hash);
    if (iter != resources_.cend()) {
      throw_unless_kind(iter->second.kind, KIND_SHADER, path);
      return refs_.retain(iter->second.shader);
    }
  }

  string buffer;
  PHYSFS_File *file = PHYSFS_openRead(path.c_str());
  if (!file) {
    s_log_error("Unable to open shader file at '%s'", path.c_str());
    return nullptr;
  }
  const GLint len = (GLint)PHYSFS_fileLength(file);
  buffer.resize(len);
  PHYSFS_readBytes(file, buffer.data(), len);
  PHYSFS_close(file);

  s_log_note("Allocating resource %x of kind shader", hash);
  rshader_t *shader = allocate_resource<rshader_t>(hash, kind);
  shader->load_source(buffer);
  if (!shader->compile()) {
    s_log_error("Unable to compile shader '%s': %s", path.c_str(), shader->error_string().c_str());
    destroy_resource(shader);
    return nullptr;
  }

  resmap_t::value_type pair {
    hash,
    {
      KIND_SHADER,
      { .shader = shader }
    }
  };
  resources_.insert(pair);

  return shader;
}



void resources_t::release_font(rfont_t *font)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  if (refs_.release(font)) {
    const size_t num_pages = font->font_page_count();
    for (size_t page_index = 0; page_index < num_pages; ++page_index) {
      rmaterial_t *material = font->font_page(page_index);
      if (!material) {
        continue;
      }
      font->set_font_page(page_index, nullptr);
      release_material(material);
    }
    destroy_resource(font);
  }
}



void resources_t::release_texture(rtexture_t *texture)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  if (refs_.release(texture)) {
    destroy_resource(texture);
  }
}



void resources_t::release_material(rmaterial_t *material)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  if (refs_.release(material)) {
    const size_t num_passes = material->num_passes();
    for (size_t pass_index = 0; pass_index < num_passes; ++pass_index) {
      rpass_t &pass = material->pass(pass_index);
      for (size_t tex_index = 0; tex_index < rpass_t::MAX_TEXTURE_UNITS; ++tex_index) {
        if (pass.textures[tex_index].texture) {
          release_texture(pass.textures[tex_index].texture);
          pass.textures[tex_index].texture = nullptr;
        }
      }

      if (pass.program) {
        release_program(pass.program);
      }
    }

    destroy_resource(material);
  }
}



void resources_t::release_program(rprogram_t *program)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  if (refs_.release(program)) {
    destroy_resource(program);
  }
}



void resources_t::release_all()
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  if (!resources_.empty()) {
    std::list<res_t> to_free;
    for (const auto &kvpair : resources_) {
      to_free.push_back(kvpair.second);
    }

    for (const auto &res : to_free) {
      switch (res.kind) {
      case KIND_FONT: destroy_resource(res.font); break;
      case KIND_TEXTURE: destroy_resource(res.tex); break;
      case KIND_MATERIAL: destroy_resource(res.mat); break;
      case KIND_PROGRAM: destroy_resource(res.prog); break;
      case KIND_SHADER: destroy_resource(res.shader); break;
      };
    }
  }

  pool_free_all(&pool_);
}



void resources_t::prepare_fonts()
{
  char temp_path[MAX_PATH_LEN] = {'\0'};
  char **fdbs = PHYSFS_enumerateFiles("fonts/");
  char **fdb_iter = fdbs;
  for (; *fdb_iter; ++fdb_iter) {
    sqlite3_snprintf(MAX_PATH_LEN, temp_path, "fonts/%s", *fdb_iter);
    const string fdb_str { temp_path };
    {
      auto end = fdb_str.crbegin();
      if (*end != 'b' && *end != 'B') continue; else ++end;
      if (*end != 'd' && *end != 'D') continue; else ++end;
      if (*end != '.') continue;
    }

    const auto pair = filepaths_.insert(fdb_str);

    {
      database_t db = database_t::read_physfs(fdb_str, false);
      if (!db.has_error()) {
        dbstatement_t stmt = db.prepare(g_font_name_query);
        for (auto &result : stmt) {
          const char *fontname = result.column_text_ptr(g_font_name_col);
          const int len = result.column_blob_size(g_font_name_col);
          const uint64_t hash = murmur3::hash64(fontname, len, font_seed);
          font_dbs_.emplace(hash, pair.first);
          s_log_note("Located font '%s' in <%s>", fontname, *fdb_iter);
        }
      } else {
        s_log_error("Unable to open font DB <%s>: %s", *fdb_iter,
          db.error_msg().c_str());
      }
    }
  }
  PHYSFS_freeList(fdbs);
}



void resources_t::find_definition_files(const char *dir, str_inserter_t &inserter)
{
  char temp_path[MAX_PATH_LEN] = {'\0'};
  temp_path[0] = '\0';
  PHYSFS_Stat pstat;
  char **files = PHYSFS_enumerateFiles(dir);
  char **iter = files;
  for (; *iter; ++iter) {
    sqlite3_snprintf(MAX_PATH_LEN, temp_path, "%s/%s", dir, *iter);
    const size_t len = strnlen(temp_path, MAX_PATH_LEN);
    if (!PHYSFS_stat(temp_path, &pstat)) {
      s_log_error("Unable to get PhysFS stat for '%s': %s", temp_path, PHYSFS_getLastError());
      continue;
    }

    switch (pstat.filetype) {
    case PHYSFS_FILETYPE_REGULAR: {
      const auto pair = filepaths_.insert(string(temp_path, len));
      *inserter = pair.first;
      break;
    }
    case PHYSFS_FILETYPE_DIRECTORY:
      find_definition_files(temp_path, inserter);
      break;
    default: continue; // skip
    }
  }
  PHYSFS_freeList(files);
}



void resources_t::find_definitions_within(tokiter_t iter,
  const tokiter_t &end,
  const string::const_iterator &source_begin,
  const nameset_t::const_iterator &filepath)
{
  resdef_parser_t parser;
  parser.set_tokens(iter, end);

  resdef_kind_t kind;
  string name;
  string::const_iterator from;
  string::const_iterator to;
  uint64_t hash;

  while (!parser.eof()) {
    if (parser.read_resource_def(kind, name, from, to) != PARSE_OK) {
      continue;
    }

    switch (kind) {
    case RESDEF_KIND_MATERIAL:  hash = murmur3::hash64(name, material_seed); break;
    case RESDEF_KIND_SHADER:    hash = murmur3::hash64(name, program_seed); break;
    default:
      s_log_error("Error parsing material file: %s", parser.error().c_str());
      continue;
    }

    const locmap_t::const_iterator existing = res_files_.find(hash);
    if (existing != res_files_.cend()) {
      s_log_error("'%s' already defined, skipping redefinition", name.c_str());
      continue;
    }

    auto name_result = def_names_.insert(name);

    resloc_t loc = {
      (size_t)(from - source_begin),
      (size_t)(to - from),
      kind,
      name_result.first,
      filepath
    };

    res_files_.insert({ hash, loc });
  }
}



void resources_t::find_definitions(const nameset_t::const_iterator &path)
{
  s_log_note("Scanning '%s' for resources", path->c_str());
  string file_source;
  {
    PHYSFS_sint64 len = 0;
    PHYSFS_File *file = PHYSFS_openRead(path->c_str());

    if (!file) {
      s_log_error("Unable to read file '%s': %s", path->c_str(), PHYSFS_getLastError());
      return;
    }

    len = PHYSFS_fileLength(file);
    if (len <= 0) {
      PHYSFS_close(file);
      s_log_error("Unable to determine length of file '%s'", path->c_str());
      return;
    }

    file_source.resize(len);
    PHYSFS_readBytes(file, file_source.data(), len);
    PHYSFS_close(file);
  }

  lexer_t lex;
  lex.set_skip_comments(true);
  lex.set_skip_newlines(true);
  if (lex.run(file_source) == LEXER_FINISHED) {
    const tokenlist_t &tokens = lex.tokens();
    const tokiter_t tok_begin = tokens.cbegin();
    const tokiter_t tok_end = tokens.cend();

    find_definitions_within(tok_begin, tok_end, file_source.cbegin(), path);
  } else {
    s_log_error("Error lexing resource file: %s", lex.error_message().c_str());
  }
}



void resources_t::prepare_definitions()
{
  std::list<nameset_t::const_iterator> matpaths;
  {
    str_inserter_t path_inserter = std::back_inserter(matpaths);
    find_definition_files("defs", path_inserter);
  }

  if (matpaths.empty()) {
    return;
  }

  for (const auto &path : matpaths) {
    find_definitions(path);
  }
}



auto resources_t::definition_names() const -> const nameset_t &
{
  return def_names_;
}



bool resources_t::name_is_material(const string &name) const
{
  const uint64_t hash = murmur3::hash64(name, material_seed);
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const locmap_t::const_iterator iter = res_files_.find(hash);
  return iter != res_files_.cend() && iter->second.kind == RESDEF_KIND_MATERIAL;
}



bool resources_t::name_is_program(const string &name) const
{
  const uint64_t hash = murmur3::hash64(name, program_seed);
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const locmap_t::const_iterator iter = res_files_.find(hash);
  return iter != res_files_.cend() && iter->second.kind == RESDEF_KIND_SHADER;
}


} // namespace snow
