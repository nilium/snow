#ifndef __SNOW__RESDEF_PARSER_HH__
#define __SNOW__RESDEF_PARSER_HH__

#include "../config.hh"
#include "../ext/parser.hh"


namespace snow {


struct rmaterial_t;
struct rpass_t;
struct rprogram_t;
struct resources_t;


enum resdef_kind_t : unsigned
{
  RESDEF_KIND_UNKNOWN,
  RESDEF_KIND_SHADER,
  RESDEF_KIND_MATERIAL,
};


enum resdef_result_t : int
{
  PARSE_INVALID_RESDEF = PARSE_EXTENDED_RESULT_BASE,
  PARSE_NOT_MATERIAL,
  PARSE_NOT_SHADER,
  PARSE_UNMATCHED_BRACE,
  PARSE_UNEXPECTED_TOKEN
};


struct resdef_parser_t : public parser_t
{
  // Returns the resource definition and returns its kind, name, and range in
  // the source string as iterators. The return values may be inconsistent if
  // the function does not return PARSE_OK.
  int read_resource_def(resdef_kind_t &kind, string &name,
    string::const_iterator &from, string::const_iterator &to);
  // Reads the next definition into the given material if the next definition
  // is a material. If it is not a material or an error occurs, the material
  // may be incomplete but should be usable.
  int read_material(rmaterial_t &material, resources_t &res);
  // Reads the next definition into the program if the next definition is a
  // shader. If the result is not PARSE_OK, the program should be considered
  // incomplete and may be unusable.
  int read_shader(rprogram_t &program, resources_t &res);

private:

  int read_material_pass(rpass_t &pass, resources_t &res);
  int read_material_map(rpass_t &pass, size_t index, resources_t &res);

  // Assumes an opening brace has already been opened. A depth of 0 is a nop.
  int skip_matched_braces(size_t depth = 1);
};


} // namespace snow


#endif /* end __SNOW__RESDEF_PARSER_HH__ include guard */
