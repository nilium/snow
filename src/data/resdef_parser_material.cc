#include "resdef_parser.hh"
#include "../renderer/material.hh"
#include "../game/resources.hh"
#include "../renderer/sgl.hh"
#include <cstdlib>


#define FAIL_IF(EXP, R, ERR) do { if ((EXP)) {                              \
  set_error(ERR);                                                           \
  return (R);                                                               \
} } while(0)


namespace snow {


namespace {


const string MAT_KW        { "mat" };
const string SHADER_KW     { "shader" };
const string PASS_KW       { "pass" };
const string BLEND_KW      { "blend" };


} // namespace <anon>


int resdef_parser_t::read_material(rmaterial_t &material, resources_t &res)
{
  FAIL_IF(read_keyword(MAT_KW), PARSE_NOT_MATERIAL, "Expected 'mat' but got invalid token");
  FAIL_IF(read_token(TOK_SINGLE_STRING_LIT) && read_token(TOK_DOUBLE_STRING_LIT),
    PARSE_UNEXPECTED_TOKEN, "Expected resource name, but got invalid token");
  FAIL_IF(read_token(TOK_CURL_OPEN), PARSE_UNEXPECTED_TOKEN,
    "Expected {, but got invalid token");

  size_t pass_count = 0;
  while (!eof()) {
    if (read_token(TOK_CURL_CLOSE) == PARSE_OK) {
      return PARSE_OK;
    } else if (read_keyword(PASS_KW) == PARSE_OK) {
      const size_t pass_index = pass_count++;
      material.set_num_passes(pass_count);
      if (read_material_pass(material.pass(pass_index), res)) {
        skip_through_token(TOK_CURL_CLOSE);
      }
    } else {
      set_error("Invalid token");
      skip_token();
    }
  }

  return PARSE_END_OF_TOKENS;
}



int resdef_parser_t::read_material_pass(rpass_t &pass, resources_t &res)
{
  static constexpr const uint32_t shader_kw               = 0xf47665ceU;
  static constexpr const uint32_t blend_kw                = 0xc793e9baU;
  static constexpr const uint32_t zero_kw                 = 0x0e63e815U;
  static constexpr const uint32_t one_kw                  = 0x56679080U;
  // depth options
  static constexpr const uint32_t depthfunc_kw            = 0x88465521U;
  static constexpr const uint32_t depthwrite_kw           = 0x3641e614U;
  static constexpr const uint32_t never_kw                = 0xa4c51aa1U;
  static constexpr const uint32_t less_kw                 = 0xde77bbf6U;
  static constexpr const uint32_t equal_kw                = 0x40ccff27U;
  static constexpr const uint32_t lequal_kw               = 0xf976fa44U;
  static constexpr const uint32_t greater_kw              = 0xbb741676U;
  static constexpr const uint32_t notequal_kw             = 0x8c06b40cU;
  static constexpr const uint32_t gequal_kw               = 0x323855daU;
  static constexpr const uint32_t always_kw               = 0xc9f711bfU;
  // stencil options
  static constexpr const uint32_t stencilop_kw            = 0x782811ecU;
  static constexpr const uint32_t stencilfunc_kw          = 0x602dc8d9U;
  static constexpr const uint32_t stencilmask_kw          = 0xb970c2dcU;
  static constexpr const uint32_t keep_kw                 = 0xa6a00bf1U;
  static constexpr const uint32_t replace_kw              = 0xa965a517U;
  static constexpr const uint32_t incr_kw                 = 0x0ecdbb09U;
  static constexpr const uint32_t incr_wrap_kw            = 0xc2645431U;
  static constexpr const uint32_t decr_kw                 = 0xf04e932bU;
  static constexpr const uint32_t decr_wrap_kw            = 0x5e53be90U;
  static constexpr const uint32_t invert_kw               = 0xdef66a05U;
  // blend factors
  static constexpr const uint32_t src_color_kw            = 0xc2267eedU;
  static constexpr const uint32_t one_minus_src_color_kw  = 0xa6802e4bU;
  static constexpr const uint32_t dst_color_kw            = 0xf7ced121U;
  static constexpr const uint32_t one_minus_dst_color_kw  = 0x45506c09U;
  static constexpr const uint32_t src_alpha_kw            = 0x0f48344dU;
  static constexpr const uint32_t one_minus_src_alpha_kw  = 0x4e64d9aaU;
  static constexpr const uint32_t dst_alpha_kw            = 0x34b7a091U;
  static constexpr const uint32_t one_minus_dst_alpha_kw  = 0xd15b9586U;
  // special single-param blend modes
  static constexpr const uint32_t opaque_kw               = 0x46a69a6cU;
  static constexpr const uint32_t screen_kw               = 0xe528bd0dU;
  static constexpr const uint32_t multiply_kw             = 0x261b1992U;
  static constexpr const uint32_t alpha_kw                = 0x9be27d8aU;

  if (read_token(TOK_CURL_OPEN)) {
    set_error("Expected { following 'pass'");
    return PARSE_UNEXPECTED_TOKEN;
  }

  string temp_name;
  size_t texture_index = 0;
  while (!eof()) {
    uint32_t hash = 0;

    if (read_token(TOK_CURL_OPEN) == PARSE_OK) {
      assert(texture_index < rpass_t::MAX_TEXTURE_UNITS);
      read_material_map(pass, texture_index, res);
      ++texture_index;
      continue;
    } else if (read_token(TOK_CURL_CLOSE) == PARSE_OK) {
      // end pass
      return PARSE_OK;
    } else if (read_token_hash32(TOK_ID, hash)) {
      goto skip_pass_statement;
    }

    switch (hash) {

    case shader_kw: {
      if (read_string(temp_name)) {
        set_error("Expected resource path, but got invalid token");
        goto skip_through_error;
      }

      rprogram_t *old_program = pass.program;
      assert(old_program == nullptr);

      pass.program = res.load_program(temp_name);

      if (old_program) {
        res.release_program(old_program);
      }
    } break; // shader_kw

    case blend_kw: {
      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      // sfactor and special modes
      switch (hash) {
      case zero_kw:                 pass.blend.sfactor = GL_ZERO; break;
      case one_kw:                  pass.blend.sfactor = GL_ONE; break;
      case src_color_kw:            pass.blend.sfactor = GL_SRC_COLOR; break;
      case one_minus_src_color_kw:  pass.blend.sfactor = GL_ONE_MINUS_SRC_COLOR; break;
      case dst_color_kw:            pass.blend.sfactor = GL_DST_COLOR; break;
      case one_minus_dst_color_kw:  pass.blend.sfactor = GL_ONE_MINUS_DST_COLOR; break;
      case src_alpha_kw:            pass.blend.sfactor = GL_SRC_ALPHA; break;
      case one_minus_src_alpha_kw:  pass.blend.sfactor = GL_ONE_MINUS_SRC_ALPHA; break;
      case dst_alpha_kw:            pass.blend.sfactor = GL_DST_ALPHA; break;
      case one_minus_dst_alpha_kw:  pass.blend.sfactor = GL_ONE_MINUS_DST_ALPHA; break;
      case opaque_kw:
      pass.blend.sfactor = GL_ONE;
      pass.blend.dfactor = GL_ZERO;
        goto read_special_blend_mode;
      case screen_kw:
        pass.blend.sfactor = GL_SRC_ALPHA;
        pass.blend.dfactor = GL_ONE;
        goto read_special_blend_mode;
      case multiply_kw:
        pass.blend.sfactor = GL_DST_COLOR;
        pass.blend.dfactor = GL_ONE_MINUS_SRC_ALPHA;
        goto read_special_blend_mode;
      case alpha_kw:
        pass.blend.sfactor = GL_SRC_ALPHA;
        pass.blend.dfactor = GL_ONE_MINUS_SRC_ALPHA;
        read_special_blend_mode:
        goto finished_pass_statement;
      default: goto skip_pass_statement;
      } // sfactor

      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      switch (hash) {
      case zero_kw:                 pass.blend.dfactor = GL_ZERO; break;
      case one_kw:                  pass.blend.dfactor = GL_ONE; break;
      case src_color_kw:            pass.blend.dfactor = GL_SRC_COLOR; break;
      case one_minus_src_color_kw:  pass.blend.dfactor = GL_ONE_MINUS_SRC_COLOR; break;
      case dst_color_kw:            pass.blend.dfactor = GL_DST_COLOR; break;
      case one_minus_dst_color_kw:  pass.blend.dfactor = GL_ONE_MINUS_DST_COLOR; break;
      case src_alpha_kw:            pass.blend.dfactor = GL_SRC_ALPHA; break;
      case one_minus_src_alpha_kw:  pass.blend.dfactor = GL_ONE_MINUS_SRC_ALPHA; break;
      case dst_alpha_kw:            pass.blend.dfactor = GL_DST_ALPHA; break;
      case one_minus_dst_alpha_kw:  pass.blend.dfactor = GL_ONE_MINUS_DST_ALPHA; break;
      default: goto skip_pass_statement;
      } // dfactor
    } break; // blend_kw

    case depthwrite_kw: {
      bool write = true;
      if (read_bool(write)) {
        goto skip_pass_statement;
      }

      pass.depth.write = (GLboolean)write;
    } break; // depthwrite_kw

    case depthfunc_kw: {
      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      switch (hash) {
      case never_kw: pass.depth.func = GL_NEVER; break;
      case less_kw: pass.depth.func = GL_LESS; break;
      case equal_kw: pass.depth.func = GL_EQUAL; break;
      case lequal_kw: pass.depth.func = GL_LEQUAL; break;
      case greater_kw: pass.depth.func = GL_GREATER; break;
      case notequal_kw: pass.depth.func = GL_NOTEQUAL; break;
      case gequal_kw: pass.depth.func = GL_GEQUAL; break;
      case always_kw: pass.depth.func = GL_ALWAYS; break;
      default: goto skip_pass_statement;
      }
    } break; // depthfunc_kw

    case stencilmask_kw: {
      const tokenlist_t::const_iterator mask_iter = iter_;
      if (read_integer()) {
        goto skip_pass_statement;
      }

      pass.stencil.mask = static_cast<GLuint>(std::strtoul(mask_iter->value, NULL, 0));
    } break; // stencilmask_kw

    case stencilfunc_kw: {
      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      switch (hash) {
      case never_kw: pass.stencil.func = GL_NEVER; break;
      case less_kw: pass.stencil.func = GL_LESS; break;
      case equal_kw: pass.stencil.func = GL_EQUAL; break;
      case lequal_kw: pass.stencil.func = GL_LEQUAL; break;
      case greater_kw: pass.stencil.func = GL_GREATER; break;
      case notequal_kw: pass.stencil.func = GL_NOTEQUAL; break;
      case gequal_kw: pass.stencil.func = GL_GEQUAL; break;
      case always_kw: pass.stencil.func = GL_ALWAYS; break;
      default: goto skip_pass_statement;
      }

      int ref = 0;
      if (read_integer(ref)) {
        goto skip_pass_statement;
      }
      pass.stencil.ref_mask = ref;

      const tokenlist_t::const_iterator mask_iter = iter_;
      if (read_integer()) {
        goto skip_pass_statement;
      }

      pass.stencil.ref_mask = static_cast<GLuint>(std::strtoul(mask_iter->value, NULL, 0));
    } break; // stencilfunc_kw

    case stencilop_kw: {
      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      switch (hash) {
      case keep_kw: pass.stencil.fail = GL_KEEP; break;
      case replace_kw: pass.stencil.fail = GL_REPLACE; break;
      case incr_kw: pass.stencil.fail = GL_INCR; break;
      case incr_wrap_kw: pass.stencil.fail = GL_INCR_WRAP; break;
      case decr_kw: pass.stencil.fail = GL_DECR; break;
      case decr_wrap_kw: pass.stencil.fail = GL_DECR_WRAP; break;
      case invert_kw: pass.stencil.fail = GL_INVERT; break;
      default: goto skip_pass_statement;
      } // stencil fail

      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      switch (hash) {
      case keep_kw: pass.stencil.depth_fail = GL_KEEP; break;
      case replace_kw: pass.stencil.depth_fail = GL_REPLACE; break;
      case incr_kw: pass.stencil.depth_fail = GL_INCR; break;
      case incr_wrap_kw: pass.stencil.depth_fail = GL_INCR_WRAP; break;
      case decr_kw: pass.stencil.depth_fail = GL_DECR; break;
      case decr_wrap_kw: pass.stencil.depth_fail = GL_DECR_WRAP; break;
      case invert_kw: pass.stencil.depth_fail = GL_INVERT; break;
      default: goto skip_pass_statement;
      } // depth fail

      if (read_token_hash32(TOK_ID, hash)) {
        goto skip_pass_statement;
      }

      switch (hash) {
      case keep_kw: pass.stencil.depth_pass = GL_KEEP; break;
      case replace_kw: pass.stencil.depth_pass = GL_REPLACE; break;
      case incr_kw: pass.stencil.depth_pass = GL_INCR; break;
      case incr_wrap_kw: pass.stencil.depth_pass = GL_INCR_WRAP; break;
      case decr_kw: pass.stencil.depth_pass = GL_DECR; break;
      case decr_wrap_kw: pass.stencil.depth_pass = GL_DECR_WRAP; break;
      case invert_kw: pass.stencil.depth_pass = GL_INVERT; break;
      default: goto skip_pass_statement;
      } // depth pass
    } break; // stencilop_kw

    default: goto skip_pass_statement;
    }

    finished_pass_statement:
    if (read_token(TOK_SEMICOLON)) {
      set_error("Expected semicolon, token not found");
      goto skip_through_error;
      skip_pass_statement:
      set_error(string("Invalid token found: ") + iter_->value);
      skip_through_error:
      skip_through_token(TOK_SEMICOLON);
    }
  }

  return PARSE_END_OF_TOKENS;
}



int resdef_parser_t::read_material_map(rpass_t &pass, size_t index, resources_t &res)
{
  static constexpr const uint32_t map_kw                    = 0x2887751dU;
  static constexpr const uint32_t filter_kw                 = 0x55de6353U;
  static constexpr const uint32_t nearest_kw                = 0x0d15c833U;
  static constexpr const uint32_t linear_kw                 = 0x1200a8c4U;
  static constexpr const uint32_t nearest_mipmap_nearest_kw = 0xbfcc748cU;
  static constexpr const uint32_t linear_mipmap_nearest_kw  = 0xa05fcedbU;
  static constexpr const uint32_t nearest_mipmap_linear_kw  = 0xaf8df3c2U;
  static constexpr const uint32_t linear_mipmap_linear_kw   = 0xa86b0a38U;
  static constexpr const uint32_t wrap_kw                   = 0x91c6382eU;
  static constexpr const uint32_t edge_kw                   = 0x1ea489eaU;
  static constexpr const uint32_t mirrored_kw               = 0x3f472f58U;
  static constexpr const uint32_t repeat_kw                 = 0xa3348007U;

  while (!eof()) {
    uint32_t hash = 0;

    if (read_token(TOK_CURL_CLOSE) == PARSE_OK) {
      return PARSE_OK;
    } else if (read_token_hash32(TOK_ID, hash)) {
      goto map_error;
    }

    switch(hash) {

    case map_kw: {
      string path;

      if (read_string(path)) {
        goto map_error;
      }

      rtexture_t *old_tex = pass.textures[index].texture;
      assert(old_tex == nullptr);

      pass.textures[index].texture = res.load_texture(path, true);

      if (old_tex) {
        res.release_texture(old_tex);
      }
    } break; // map_kw

    case filter_kw: {
      // min filter
      if (read_token_hash32(TOK_ID, hash)) {
        goto map_error;
      }

      switch (hash) {
      case nearest_kw:                pass.textures[index].min_filter = GL_NEAREST; break;
      case linear_kw:                 pass.textures[index].min_filter = GL_LINEAR; break;
      case nearest_mipmap_nearest_kw: pass.textures[index].min_filter = GL_NEAREST_MIPMAP_NEAREST; break;
      case linear_mipmap_nearest_kw:  pass.textures[index].min_filter = GL_LINEAR_MIPMAP_NEAREST; break;
      case nearest_mipmap_linear_kw:  pass.textures[index].min_filter = GL_NEAREST_MIPMAP_LINEAR; break;
      case linear_mipmap_linear_kw:   pass.textures[index].min_filter = GL_LINEAR_MIPMAP_LINEAR; break;
      default: goto map_error;
      }

      // mag filter
      if (read_token_hash32(TOK_ID, hash)) {
        goto map_error;
      }

      switch (hash) {
      case nearest_kw:  pass.textures[index].mag_filter = GL_NEAREST; break;
      case linear_kw:   pass.textures[index].mag_filter = GL_LINEAR; break;
      default: goto map_error;
      }
    } break; // filter_kw

    case wrap_kw: {
      // X wrap
      if (read_token_hash32(TOK_ID, hash)) {
        goto map_error;
      }

      switch (hash) {
      case edge_kw:     pass.textures[index].x_wrap = pass.textures[index].y_wrap = GL_CLAMP_TO_EDGE; break;
      case repeat_kw:   pass.textures[index].x_wrap = pass.textures[index].y_wrap = GL_REPEAT; break;
      case mirrored_kw: pass.textures[index].x_wrap = pass.textures[index].y_wrap = GL_MIRRORED_REPEAT; break;
      default: goto map_error;
      }

      if (read_token_hash32(TOK_ID, hash)) {
        break;
      }

      switch (hash) {
      case edge_kw:     pass.textures[index].y_wrap = GL_CLAMP_TO_EDGE; break;
      case repeat_kw:   pass.textures[index].y_wrap = GL_REPEAT; break;
      case mirrored_kw: pass.textures[index].y_wrap = GL_MIRRORED_REPEAT; break;
      default: goto map_error;
      }

    } break; // wrap_kw

    default: break;
    }

    if (read_token(TOK_SEMICOLON)) {
      map_error:
      set_error("Unexpected token");
      skip_through_token(TOK_SEMICOLON);
    }
  }

  return PARSE_END_OF_TOKENS;
}


} // namespace snow
