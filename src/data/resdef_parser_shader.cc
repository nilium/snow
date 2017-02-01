/*
  resdef_parser_shader.cc -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#include "resdef_parser.hh"
#include "../game/resources.hh"
#include "../renderer/shader.hh"
#include "../renderer/constants.hh"
#include "../renderer/program.hh"


#define FAIL_IF(EXP, R, ERR) do { if ((EXP)) {                              \
  set_error(ERR);                                                           \
  return (R);                                                               \
} } while(0)


namespace snow {


namespace {


const std::map<string, int> g_named_uniforms {
  { "modelview",          UNIFORM_MODELVIEW },
  { "projection",         UNIFORM_PROJECTION },
  { "texture_matrix",     UNIFORM_TEXTURE_MATRIX },
  { "bones",              UNIFORM_BONES },
  { "texture0",           UNIFORM_TEXTURE0 },
  { "texture1",           UNIFORM_TEXTURE1 },
  { "texture2",           UNIFORM_TEXTURE2 },
  { "texture3",           UNIFORM_TEXTURE3 },
  { "texture4",           UNIFORM_TEXTURE4 },
  { "texture5",           UNIFORM_TEXTURE5 },
  { "texture6",           UNIFORM_TEXTURE6 },
  { "texture7",           UNIFORM_TEXTURE7 },
};


const std::map<string, GLuint> g_named_attribs {
  { "position",           ATTRIB_POSITION },
  { "color",              ATTRIB_COLOR },
  { "normal",             ATTRIB_NORMAL },
  { "binormal",           ATTRIB_BINORMAL },
  { "tangent",            ATTRIB_TANGENT },
  { "texcoord0",          ATTRIB_TEXCOORD0 },
  { "texcoord1",          ATTRIB_TEXCOORD1 },
  { "texcoord2",          ATTRIB_TEXCOORD1 },
  { "texcoord3",          ATTRIB_TEXCOORD1 },
  { "bone_weights",       ATTRIB_BONE_WEIGHTS },
  { "bone_indices",       ATTRIB_BONE_INDICES },
};


const std::map<string, GLuint> g_named_frag_outs {
  { "out0", 0 },
  { "out1", 1 },
  { "out2", 2 },
  { "out3", 3 },
};


const string SHADER_KW     { "shader" };


} // namespace <anon>



template <typename T, typename C>
static unsigned read_named_statement(resdef_parser_t &parser, const C &names, T &out_index, tokenlist_t::const_iterator &inner_name)
{
  const tokenlist_t::const_iterator current = parser.mark();
  int index = 0;
  if (parser.read_token(TOK_ID) == PARSE_OK) {
    {
      // named special
      const auto iter = names.find(current->value);
      if (iter == names.cend()) {
        parser.set_error("Unrecognized name");
        return false;
      }
      index = iter->second;
    }

    have_read_index:
    const tokenlist_t::const_iterator name_iter = parser.mark();
    if (parser.read_token(TOK_ID)) {
      parser.set_error("Expected name, but got an unexpected token");
      return false;
    }

    inner_name = name_iter;
    out_index = static_cast<T>(index);

    return parser.read_token(TOK_SEMICOLON) == PARSE_OK;;
  } else if (parser.read_integer(index) == PARSE_OK) {
    // custom uniform
    goto have_read_index;
  }
  parser.set_error("Unexpected token while reading shader statement");
  return false;
}



int resdef_parser_t::read_shader(rprogram_t &program, resources_t &res)
{
  static constexpr const uint32_t uniform_kw    = 0xe28ba5a4U;
  static constexpr const uint32_t attrib_kw     = 0x3aeacc59U;
  static constexpr const uint32_t frag_out_kw   = 0x0489c556U;
  static constexpr const uint32_t vert_kw       = 0x2b991038U;
  static constexpr const uint32_t frag_kw       = 0xc4e9df3cU;

  FAIL_IF(read_keyword(SHADER_KW), PARSE_NOT_MATERIAL, "Expected 'shader' but got invalid token");
  FAIL_IF(read_token(TOK_SINGLE_STRING_LIT) && read_token(TOK_DOUBLE_STRING_LIT),
    PARSE_UNEXPECTED_TOKEN, "Expected resource name, but got invalid token");
  FAIL_IF(read_token(TOK_CURL_OPEN), PARSE_UNEXPECTED_TOKEN,
    "Expected {, but got invalid token");

  while (!eof()) {
    uint32_t hash = 0;
    if (read_token(TOK_CURL_CLOSE) == PARSE_OK) {
      return PARSE_OK;
    } else if (read_token_hash32(TOK_ID, hash)) {
      set_error("Invalid token");
      skip_token();
    }

    switch (hash) {
    case uniform_kw: {
      int uniform_index = 0;
      tokenlist_t::const_iterator inner_name;

      if (read_named_statement(*this, g_named_uniforms, uniform_index, inner_name)) {
        program.bind_uniform(uniform_index, inner_name->value);
        break;
      }
    } goto shader_error_already_set; // uniform_kw

    case attrib_kw: {
      GLuint attrib_index = 0;
      tokenlist_t::const_iterator inner_name;

      if (read_named_statement(*this, g_named_attribs, attrib_index, inner_name)) {
        program.bind_attrib(attrib_index, inner_name->value);
        break;
      }
    } goto shader_error_already_set; // attrib_kw

    case frag_out_kw: {
      GLuint frag_index = 0;
      tokenlist_t::const_iterator inner_name;

      if (read_named_statement(*this, g_named_frag_outs, frag_index, inner_name)) {
        program.bind_frag_out(frag_index, inner_name->value);
        break;
      }
    } goto shader_error_already_set; // frag_out_kw

    case vert_kw:
    case frag_kw: {
      const tokenlist_t::const_iterator path = mark();
      const GLenum shader_type = (hash == vert_kw) ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER;
      if (read_string()) {
        set_error("Expected resource path, got unexpected token");
        goto shader_error_already_set;
      }

      rshader_t *shader = res.load_shader(path->value, shader_type);

      if (!shader) {
        set_error("Unable to load shader");
      } else {
        program.attach_shader(*shader);
      }

      if (read_token(TOK_SEMICOLON)) {
        set_error("Expected semicolon");
        goto shader_error_already_set;
      }
    } break; // vert_kw, frag_kw

    default: {
      shader_error_already_set:
      skip_through_token(TOK_SEMICOLON);
    } break;
    }
  }

  return PARSE_END_OF_TOKENS;
}


} // namespace snow
