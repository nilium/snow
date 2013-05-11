#include "resdef_parser.hh"
#include "../game/resources.hh"
#include "../renderer/shader.hh"
#include "../renderer/constants.hh"
#include "../renderer/program.hh"


namespace snow {


static const std::map<string, int> g_named_uniforms {
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


static const std::map<string, GLuint> g_named_attribs {
  { "position",           ATTRIB_POSITION },
  { "color",              ATTRIB_COLOR },
  { "normal",            ATTRIB_NORMAL },
  { "binormal",          ATTRIB_BINORMAL },
  { "tangent",           ATTRIB_TANGENT },
  { "texcoord0",          ATTRIB_TEXCOORD0 },
  { "texcoord1",          ATTRIB_TEXCOORD1 },
  { "texcoord2",          ATTRIB_TEXCOORD1 },
  { "texcoord3",          ATTRIB_TEXCOORD1 },
  { "bone_weights",       ATTRIB_BONE_WEIGHTS },
  { "bone_indices",       ATTRIB_BONE_INDICES },
};


static const std::map<string, GLuint> g_named_frag_outs {
  { "out0", 0 },
  { "out1", 1 },
  { "out2", 2 },
  { "out3", 3 },
};


static const string SHADER_KW     { "shader" };
static const string SHADER_KW     { "shader" };
static const string UNIFORM_KW    { "uniform" };
static const string ATTRIB_KW     { "attrib" };
static const string FRAG_OUT_KW   { "frag_out" };
static const string VERT_KW       { "vert" };
static const string FRAG_KW       { "frag" };


} // namespace snow
