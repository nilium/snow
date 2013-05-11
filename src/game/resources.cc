#include "resources.hh"
#include <snow/data/hash.hh>
#include "../data/database.hh"
#include "../renderer/texture.hh"
#include "../renderer/material.hh"
#include "../renderer/material_basic.hh"
#include "../renderer/program.hh"
#include "../renderer/shader.hh"
#include "../renderer/font.hh"
#include <list>
#include <map>


#define MAX_PATH_LEN (512)
#define RES_POOL_SIZE (128 * 1024 * 1024) /* 128mb */
#define RES_OBJECT(PTR) ( ( (uint64_t *)( (PTR) ) ) + 1 )
#define RES_HASH(PTR) ( ( (uint64_t *)( (PTR) ) ) [-1] )
#define throw_unless_kind(V_, KIND_, NAME_)                                    \
  if ((V_) != KIND_) {                                                         \
    s_throw(std::runtime_error, "Resource '%s' (%0x) is not of type " #KIND_,  \
      (NAME_).c_str(), hash);                                                  \
  }

#define return_error_if(VAL, COND, MESSAGE, ARGS...)                           \
  if ((COND)) {                                                                \
    s_log_error(MESSAGE, ##ARGS);                                              \
    return (VAL);                                                              \
  }


namespace snow {


using tokiter_t = typename tokenlist_t::const_iterator;


const std::map<string, int> g_named_uniforms {
  { "modelview", UNIFORM_MODELVIEW },
  { "projection", UNIFORM_PROJECTION },
  { "texture_matrix", UNIFORM_TEXTURE_MATRIX },
  { "bones", UNIFORM_BONES },
  { "texture0", UNIFORM_TEXTURE0 },
  { "texture1", UNIFORM_TEXTURE1 },
  { "texture2", UNIFORM_TEXTURE2 },
  { "texture3", UNIFORM_TEXTURE3 },
  { "texture4", UNIFORM_TEXTURE4 },
  { "texture5", UNIFORM_TEXTURE5 },
  { "texture6", UNIFORM_TEXTURE6 },
  { "texture7", UNIFORM_TEXTURE7 },
};


const std::map<string, GLuint> g_named_attribs {
  { "position", ATTRIB_POSITION },
  { "color", ATTRIB_COLOR },
  { "normals", ATTRIB_NORMAL },
  { "binormals", ATTRIB_BINORMAL },
  { "tangents", ATTRIB_TANGENT },
  { "texcoord0", ATTRIB_TEXCOORD0 },
  { "texcoord1", ATTRIB_TEXCOORD1 },
  { "texcoord2", ATTRIB_TEXCOORD1 },
  { "texcoord3", ATTRIB_TEXCOORD1 },
  { "bone_weights", ATTRIB_BONE_WEIGHTS },
  { "bone_indices", ATTRIB_BONE_INDICES },
};


const std::map<string, GLuint> g_named_frag_outs {
  { "out0", 0 },
  { "out1", 1 },
  { "out2", 2 },
  { "out3", 3 },
};


const uint64_t resources_t::font_seed = hash64("font+");
const uint64_t resources_t::material_seed = hash64("material+");
const uint64_t resources_t::texture_seed = hash64("texture+");
const uint64_t resources_t::program_seed = hash64("program+");
const uint64_t resources_t::vert_shader_seed = hash64("vertshader+");
const uint64_t resources_t::frag_shader_seed = hash64("fragshader+");



static resources_t g_default_resources;
resources_t &resources_t::default_resources()
{
  return g_default_resources;
}



static bool read_token(tokiter_t &iter, const tokiter_t &end, token_kind_t kind)
{
  if (iter != end && iter->kind == kind) {
    ++iter;
    return true;
  }
  return false;
}



static bool read_token_id(tokiter_t &iter, const tokiter_t &end, const string &value)
{
  if (iter != end && iter->kind == TOK_ID && iter->value == value) {
    ++iter;
    return true;
  }
  return false;
}



static void skip_through_semicolon(tokiter_t &iter, const tokiter_t &end)
{
  while (iter != end && !read_token(iter, end, TOK_SEMICOLON)) {
    ++iter;
  }
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
  lock_.lock();
  release_all();
  filepaths_.clear();
  font_dbs_.clear();
  res_files_.clear();
  prepare_fonts();
  prepare_definitions();
  lock_.unlock();
}



rfont_t *resources_t::load_font(const string &name)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = hash64(name, font_seed);

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
  const uint64_t hash = hash64(path, texture_seed);

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



static bool read_mat_opening(tokiter_t &iter, const tokiter_t &end)
{
  return_error_if(false, !read_token_id(iter, end, "mat"),
    "Unable to parse material: expected 'mat', got <%s>",
    iter->descriptor().c_str())

  return_error_if(false, iter == end || !iter->is_string(),
    "Unable to parse material: expected string, got <%s>",
    iter->descriptor().c_str());

  ++iter;

  return_error_if(false, !read_token(iter, end, TOK_CURL_OPEN),
    "Unable to parse material: expected {, got <%s>",
    iter->descriptor().c_str());

  return true;
}



bool resources_t::read_mat_texture(rmaterial_basic_t *mat,
  tokiter_t &iter, const tokiter_t &end)
{
  // Not an error if it's just not the right ID
  if (!read_token_id(iter, end, "map")) {
    return false;
  }

  return_error_if(false, iter == end,
    "Expected texture path but source ended");
  return_error_if(false, !iter->is_string(),
    "Expected texture path but got %s", iter->descriptor().c_str());

  const token_t &pathtok = *iter;
  rtexture_t *tex = load_texture(pathtok.value);
  ++iter;

  return_error_if(false, !read_token(iter, end, TOK_SEMICOLON),
    "Expected semicolon, token not found");
  return_error_if(true, !tex,
    "Unable to load texture '%s' for material", pathtok.value.c_str());

  rtexture_t *old_tex = mat->texture();

  mat->set_texture(tex);

  if (old_tex != nullptr) {
    s_log_warning("Texture redefined");
    release_texture(old_tex);
  }

  return true;
}



bool resources_t::read_mat_shader(rmaterial_basic_t *mat,
  tokiter_t &iter, const tokiter_t &end)
{
  if (!read_token_id(iter, end, "shader")) {
    return false;
  }

  return_error_if(false, iter == end,
    "Expected texture path but source ended");
  return_error_if(false, !iter->is_string(),
    "Expected texture path but got %s", iter->descriptor().c_str());

  const token_t &pathtok = *iter;
  rprogram_t *prog = load_program(pathtok.value);
  ++iter;

  return_error_if(false, !read_token(iter, end, TOK_SEMICOLON),
    "Expected semicolon, token not found");
  return_error_if(true, !prog,
    "Unable to load program '%s' for material", pathtok.value.c_str());

  rprogram_t *old_prog = mat->program();

  mat->set_program(prog, UNIFORM_PROJECTION, UNIFORM_MODELVIEW, UNIFORM_TEXTURE0);

  if (old_prog != nullptr) {
    s_log_warning("Shader redefined");
    release_program(old_prog);
  }

  return true;
}



bool resources_t::load_material_basic(rmaterial_basic_t *mat,
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

  bool texture_set = false;
  bool closed = false;
  const tokenlist_t &tokens = lex.tokens();
  auto iter = tokens.cbegin();
  auto end = tokens.cend();
  if (!read_mat_opening(iter, end)) return false;
  while (iter != end) {
    if (read_mat_texture(mat, iter, end)) {
      continue;
    } else if (read_mat_shader(mat, iter, end)) {
      continue;
    } else if (read_token(iter, end, TOK_CURL_CLOSE)) {
      closed = true;
      break;
    } else {
      s_log_error("Unknown material property: %s", iter->value.c_str());
      skip_through_semicolon(iter, end);
    }
  }

  return iter == end && closed;
}



rmaterial_t *resources_t::load_material(const string &name)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = hash64(name, material_seed);

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
  rmaterial_basic_t *mat = allocate_resource<rmaterial_basic_t>(hash);
  if (!mat) {
    s_log_error("Unable to allocate material '%s'", name.c_str());
    return nullptr;
  }

  if (!load_material_basic(mat, matloc)) {
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



static bool read_program_opening(tokiter_t &iter, const tokiter_t &end)
{
  {
    const token_t &idtok = *iter;

    if (idtok.kind != TOK_ID || idtok.value != "shader") {
      s_log_error("Unable to read shader, invalid opening token");
      return false;
    }
  }

  ++iter;

  if (iter == end) {
    s_log_error("Unexpected end of shader");
    return false;
  }

  const token_t &nametok = *iter;
  if (!nametok.is_string()) {
    s_log_error("Expected name string for shader, found <%s>",
      nametok.descriptor().c_str());
    return false;
  }

  ++iter;

  return read_token(iter, end, TOK_CURL_OPEN);
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

  const tokenlist_t &tokens = lex.tokens();
  auto iter = tokens.cbegin();
  auto end = tokens.cend();

  return read_program(prog, iter, end);
}



bool resources_t::read_program(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end)
{
  if (!read_program_opening(iter, end)) {
    s_log_error("Unable to read program opening, exiting early");
    return false;
  }

  bool read_close = false;

  while (iter != end) {
    if (read_program_shader(prog, iter, end)) {
      continue;
    } else if (read_program_uniform(prog, iter, end)) {
      continue;
    } else if (read_program_attrib(prog, iter, end)) {
      continue;
    } else if (read_program_frag_out(prog, iter, end)) {
      continue;
    } else if ((read_close = read_token(iter, end, TOK_CURL_CLOSE))) {
      break;
    } else {
      s_log_error("Unexpected token at [%d:%d] <%s>",
        iter->pos.line, iter->pos.column, iter->descriptor().c_str());
      skip_through_semicolon(iter, end);
    }
  }

  return read_close && iter == end;
}



bool resources_t::read_program_shader(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end)
{
  GLenum shader_kind = GL_VERTEX_SHADER;

  if (read_token_id(iter, end, "frag")) {
    s_log_note("Frag read");
    shader_kind = GL_FRAGMENT_SHADER;
  } else if (read_token_id(iter, end, "vert")) {
    // nop
  } else {
    return false;
  }

  if (iter == end) {
    s_log_error("Unexpected end of source - expected shader path");
    return false;
  } else if (!iter->is_string()) {
    s_log_error("Unexpected token <%s>, expected shader path.",
      iter->descriptor().c_str());
    return false;
  }

  const token_t &pathtok = *iter;
  ++iter;

  if (!read_token(iter, end, TOK_SEMICOLON)) {
    s_log_error("Expected semicolon, got <%s>", iter->descriptor().c_str());
    return false;
  }

  rshader_t *shader = load_shader(pathtok.value, shader_kind);
  if (shader) {
    prog->attach_shader(*shader);
  }

  return true;
}



bool resources_t::read_program_uniform(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end)
{
  if (!read_token_id(iter, end, "uniform")) {
    return false;
  }

  if (iter == end) {
    s_log_error("Unexpected end of source - expected uniform binding");
    return false;
  }

  int binding = 0;

  switch (iter->kind) {
  case TOK_ID: {
    auto name_iter = g_named_uniforms.find(iter->value);
    if (name_iter == g_named_uniforms.end()) {
      s_log_error("%s is not a predefined uniform binding", iter->value.c_str());
      skip_through_semicolon(iter, end);
      return true;
    }
    binding = name_iter->second;
  } break;
  case TOK_INTEGER_LIT:
    binding = std::atoi(iter->value.c_str());
    break;
  default:
    s_log_error("Unexpected token <%s>", iter->descriptor().c_str());
    skip_through_semicolon(iter, end);
    return true;
  }

  ++iter;

  if (iter == end) {
    s_log_error("Unexpected end of source - expected uniform name");
    return false;
  } else if (iter->kind != TOK_ID) {
    s_log_error("Unexpected token <%s> - expected uniform name", iter->value.c_str());
    skip_through_semicolon(iter, end);
    return true;
  }

  const token_t &nametok = *iter;
  ++iter;

  if (!read_token(iter, end, TOK_SEMICOLON)) {
    s_log_error("Expected semicolon at end of uniform");
    return false;
  }

  s_log_note("Binding uniform %d to '%s'", binding, nametok.value.c_str());
  prog->bind_uniform(binding, nametok.value);

  return true;
}



bool resources_t::read_program_attrib(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end)
{
  if (!read_token_id(iter, end, "attrib")) {
    return false;
  }

  if (iter == end) {
    s_log_error("Unexpected end of source - expected attrib binding");
    return false;
  }

  GLuint binding = 0;

  switch (iter->kind) {
  case TOK_ID: {
    auto name_iter = g_named_attribs.find(iter->value);
    if (name_iter == g_named_attribs.end()) {
      s_log_error("%s is not a predefined attrib binding", iter->value.c_str());
      skip_through_semicolon(iter, end);
      return true;
    }
    binding = name_iter->second;
  } break;
  case TOK_INTEGER_LIT:
    binding = std::atoi(iter->value.c_str());
    break;
  default:
    s_log_error("Unexpected token <%s>", iter->descriptor().c_str());
    skip_through_semicolon(iter, end);
    return true;
  }

  ++iter;

  if (iter == end) {
    s_log_error("Unexpected end of source - expected attrib name");
    return false;
  } else if (iter->kind != TOK_ID) {
    s_log_error("Unexpected token <%s> - expected attrib name", iter->value.c_str());
    skip_through_semicolon(iter, end);
    return true;
  }

  const token_t &nametok = *iter;
  ++iter;

  if (!read_token(iter, end, TOK_SEMICOLON)) {
    s_log_error("Expected semicolon at end of attrib");
    return false;
  }

  s_log_note("Binding attrib %d to '%s'", binding, nametok.value.c_str());
  prog->bind_attrib(binding, nametok.value);

  return true;
}



bool resources_t::read_program_frag_out(rprogram_t *prog, tokenlist_t::const_iterator &iter, const tokenlist_t::const_iterator &end)
{
  if (!read_token_id(iter, end, "frag_out")) {
    return false;
  }

  if (iter == end) {
    s_log_error("Unexpected end of source - expected frag_out binding");
    return false;
  }

  GLuint binding = 0;

  switch (iter->kind) {
  case TOK_ID: {
    auto name_iter = g_named_frag_outs.find(iter->value);
    if (name_iter == g_named_frag_outs.end()) {
      s_log_error("%s is not a predefined frag_out binding", iter->value.c_str());
      skip_through_semicolon(iter, end);
      return true;
    }
    binding = name_iter->second;
  } break;
  case TOK_INTEGER_LIT:
    binding = std::atoi(iter->value.c_str());
    break;
  default:
    s_log_error("Unexpected token <%s>", iter->descriptor().c_str());
    skip_through_semicolon(iter, end);
    return true;
  }

  ++iter;

  if (iter == end) {
    s_log_error("Unexpected end of source - expected frag_out name");
    return false;
  } else if (iter->kind != TOK_ID) {
    s_log_error("Unexpected token <%s> - expected frag_out name", iter->value.c_str());
    skip_through_semicolon(iter, end);
    return true;
  }

  const token_t &nametok = *iter;
  ++iter;

  if (!read_token(iter, end, TOK_SEMICOLON)) {
    s_log_error("Expected semicolon at end of frag_out");
    return false;
  }

  prog->bind_frag_out(binding, nametok.value);

  return true;
}



rprogram_t *resources_t::load_program(const string &name)
{
  std::lock_guard<std::recursive_mutex> lock((lock_));
  const uint64_t hash = hash64(name, program_seed);

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
  const uint64_t hash = hash64(path, kind == GL_FRAGMENT_SHADER
    ? frag_shader_seed : vert_shader_seed);

  {
    const resmap_t::const_iterator iter = resources_.find(hash);
    if (iter != resources_.cend()) {
      throw_unless_kind(iter->second.kind, KIND_SHADER, path);
      return refs_.retain(iter->second.shader);
    }
  }

  std::vector<char> buffer;
  PHYSFS_File *file = PHYSFS_openRead(path.c_str());
  if (!file) {
    s_log_error("Unable to open shader file at '%s'", path.c_str());
    return nullptr;
  }
  const auto len = PHYSFS_fileLength(file);
  buffer.reserve(len);
  PHYSFS_readBytes(file, buffer.data(), len);
  PHYSFS_close(file);

  s_log_note("Allocating resource %x of kind shader", hash);
  rshader_t *shader = allocate_resource<rshader_t>(hash, kind);
  shader->load_source(buffer.data(), len);
  if (!shader->compile()) {
    s_log_error("Unable to compile shader '%s': %s", path.c_str(), shader->error_string());
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
  rtexture_t *tex = nullptr;
  rprogram_t *prog = nullptr;
  rmaterial_basic_t *bmat = (rmaterial_basic_t *)material;
  std::lock_guard<std::recursive_mutex> lock((lock_));
  if (refs_.release(material)) {
    tex = bmat->texture();
    if (tex) {
      bmat->set_texture(nullptr);
      release_texture(tex);
    }

    rprogram_t *program = bmat->program();
    if (prog) {
      bmat->set_program(nullptr);
      release_program(prog);
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
  static const string font_db_ext { "*.db" };
  static const string font_name_query {
    "SELECT name FROM 'font_info'"
  };
  static const string name_col { "name" };

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
        dbstatement_t stmt = db.prepare(font_name_query);
        for (auto &result : stmt) {
          const char *fontname = result.column_text_ptr(name_col);
          const int len = result.column_blob_size(name_col);
          const uint64_t hash = hash64(fontname, len, font_seed);
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



bool resources_t::skip_to_end_of_def(tokiter_t &iter, const tokiter_t &end)
{
  size_t depth = 0;
  for (; iter != end; ++iter) {
    if (iter->kind == TOK_CURL_CLOSE) {
      --depth;
      if (depth == 0) {
        break;
      }
    } else if (iter->kind == TOK_CURL_OPEN) {
      ++depth;
    }
  }

  if (depth != 0) {
    s_log_error("Unclosed { in definition");
  }

  return (depth == 0);
}



void resources_t::find_definitions_within(tokiter_t iter,
  const tokiter_t &end,
  const string::const_iterator &source_begin,
  const fileset_t::const_iterator &filepath)
{
  for (; iter != end; ++iter) {
    uint64_t def_seed = material_seed;
    const token_t &begin_tok = *iter;

    if (read_token_id(iter, end, "mat")) {
      // nop
    } else if (read_token_id(iter, end, "shader")) {
      def_seed = program_seed;
    } else {
      s_log_error("[%d:%d] Unexpected token <%s>",
        iter->pos.line, iter->pos.column, iter->descriptor().c_str());
      break;
    }

    if (!iter->is_string()) {
      s_log_error("[%d:%d] Expected name string, got <%s>",
        iter->pos.line, iter->pos.column, iter->descriptor().c_str());
      break;
    }

    const token_t &nametok = *iter;

    if (!skip_to_end_of_def(iter, end)) {
      // Failed to find an entire definition
      break;
    }

    const uint64_t hash = hash64(nametok.value, def_seed);

    const locmap_t::const_iterator existing = res_files_.find(hash);
    if (existing != res_files_.cend()) {
      s_log_error("Material '%s' already exists, skipping redefinition",
        nametok.value.c_str());
      continue;
    }

    resloc_t loc = {
      (size_t)std::distance(source_begin, begin_tok.from),
      (size_t)std::distance(begin_tok.from, iter->to),
      filepath
    };

    res_files_.insert({ hash, loc });
  }
}



void resources_t::find_definitions(const fileset_t::const_iterator &path)
{
  s_log_note("Scanning '%s' for resources", path->c_str());
  string file_source;
  {
    #define MAT_BUFF_LEN (256)
    char temp_buff[MAT_BUFF_LEN];
    PHYSFS_sint64 len = 0;
    PHYSFS_File *file = PHYSFS_openRead(path->c_str());

    if (!file) {
      s_log_error("Unable to read file '%s': %s", path->c_str(), PHYSFS_getLastError());
      return;
    }

    len = PHYSFS_fileLength(file);
    if (len < 0) {
      PHYSFS_close(file);
      s_log_error("Unable to determine length of file '%s'", path->c_str());
      return;
    }

    file_source.reserve(len);
    while (!PHYSFS_eof(file)) {
      auto count = PHYSFS_readBytes(file, temp_buff, MAT_BUFF_LEN);
      if (count > 0) {
        file_source.append(temp_buff, count);
      }
    }

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
  std::list<fileset_t::const_iterator> matpaths;
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


} // namespace snow
