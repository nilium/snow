#include "resdef_parser.hh"


namespace snow {


namespace {


const string MAT_KW        { "mat" };
const string SHADER_KW     { "shader" };


} // namespace <anon>



int resdef_parser_t::read_resource_def(
  resdef_kind_t &kind,
  string &name,
  string::const_iterator &from,
  string::const_iterator &to
  )
{
  from = iter_->from;

  int error = PARSE_OK;

  if ((error = read_keyword(MAT_KW)) == PARSE_OK) {
    kind = RESDEF_KIND_MATERIAL;
  } else if ((error = read_keyword(SHADER_KW)) == PARSE_OK) {
    kind = RESDEF_KIND_SHADER;
  } else {
    set_error("Expected either 'mat' or 'shader', got invalid token");
    goto read_resource_def_error;
  }

  if (read_string(name)) {
    set_error("Expected resource name, but got invalid token");
    error = PARSE_UNEXPECTED_TOKEN;
    goto read_resource_def_error;
  }

  if ((error = read_token(TOK_CURL_OPEN))) {
    set_error("Expected {, got invalid token");
    goto read_resource_def_error;
  }

  if ((error = skip_matched_braces())) {
    read_token(TOK_CURL_CLOSE);
    return error;
  } else if (iter_ == end_) {
    return PARSE_END_OF_TOKENS;
  }

  to = iter_->to;

  return read_token(TOK_CURL_CLOSE);

read_resource_def_error:
  skip_through_token(TOK_CURL_OPEN);
  skip_matched_braces();
  read_token(TOK_CURL_CLOSE);

  return error;
}



int resdef_parser_t::skip_matched_braces(size_t depth)
{
  if (!depth) {
    set_error("No brace to skip");
    return PARSE_OK; // not an error, but error message is still a diagnostic
  }

  while (iter_ != end_) {
    switch (iter_->kind) {
    case TOK_CURL_OPEN: ++depth; break;
    case TOK_CURL_CLOSE: --depth; break;
    default: break;
    }

    if (depth == 0) {
      break;
    }

    ++iter_;
  }

  if (depth) {
    set_error("Parser encountered an unclosed curly brace");
    return PARSE_UNMATCHED_BRACE;
  } else {
    return PARSE_OK;
  }
}


} // namespace snow
