#include "parser.hh"
#include <cassert>
#include <cstdlib>
#include <snow/data/hash.hh>


namespace snow {


parser_t::parser_t(
  const tokenlist_t::const_iterator &start,
  const tokenlist_t::const_iterator &end
  ) :
  start_(start),
  iter_(start),
  end_(end)
{
  assert(start <= end);
}



parser_t::~parser_t()
{
  /* nop */
}



void parser_t::set_tokens(const tokenlist_t &tokens)
{
  set_tokens(tokens.cbegin(), tokens.cend());
}



void parser_t::set_tokens(
  const tokenlist_t::const_iterator &start,
  const tokenlist_t::const_iterator &end
  )
{
  assert(start <= end);
  start_ = start;
  iter_ = start;
  end_ = end;

  if (skip_ws_on_read_ && start != end) {
    skip_whitespace();
  }
}



int parser_t::read_token_hash32(token_kind_t kind, uint32_t &hash)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }

  if (iter_->kind == kind) {
    hash = hash32(iter_->value);
    ++iter_;
    return PARSE_OK;
  }

  return PARSE_NO_MATCH;
}



int parser_t::read_token_hash64(token_kind_t kind, uint64_t &hash)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }

  if (iter_->kind == kind) {
    hash = hash64(iter_->value);
    ++iter_;
    return PARSE_OK;
  }

  return PARSE_NO_MATCH;
}



int parser_t::read_keyword(const string &keyword)
{
  return read_token(TOK_ID, keyword);
}



int parser_t::read_null()
{
  return read_token(TOK_NULL_KW);
}



int parser_t::read_float(float &value)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }
  switch (iter_->kind) {
  case TOK_INTEGER_LIT: // fall
  case TOK_INTEGER_EXP_LIT:
    value = (float)std::atoi(iter_->value);
    break;
  case TOK_FLOAT_LIT: // fall
  case TOK_FLOAT_EXP_LIT:
    value = std::atof(iter_->value);
    break;
  case TOK_TRUE_KW:
    value = 1;
    break;
  case TOK_FALSE_KW:
    value = 0;
    break;
  default:
    set_error("Token is not a float or cannot be implicitly converted to one");
    return PARSE_NO_MATCH;
  }
  ++iter_;
  if (skip_ws_on_read_ && iter_ != end_) {
    skip_whitespace();
  }
  return PARSE_OK;
}



int parser_t::read_integer(int &value)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }
  switch (iter_->kind) {
  case TOK_INTEGER_LIT: // fall
  case TOK_INTEGER_EXP_LIT:
    value = std::atoi(iter_->value);
    break;
  case TOK_FLOAT_LIT: // fall
  case TOK_FLOAT_EXP_LIT:
    value = (int)std::atof(iter_->value);
    break;
  case TOK_TRUE_KW:
    value = 1;
    break;
  case TOK_FALSE_KW:
    value = 0;
    break;
  default:
    set_error("Token is not an integer or cannot be implicitly converted to one");
    return PARSE_NO_MATCH;
  }
  ++iter_;
  if (skip_ws_on_read_ && iter_ != end_) {
    skip_whitespace();
  }
  return PARSE_OK;
}



int parser_t::read_bool(bool &value)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }
  switch (iter_->kind) {
  case TOK_INTEGER_LIT: // fall
  case TOK_INTEGER_EXP_LIT:
    value = std::atoi(iter_->value) != 0;
    break;
  case TOK_FLOAT_LIT: // fall
  case TOK_FLOAT_EXP_LIT:
    value = std::atof(iter_->value) != 0.0f;
    break;
  case TOK_TRUE_KW:
    value = true;
    break;
  case TOK_FALSE_KW:
    value = false;
    break;
  default:
    set_error("Token is not a bool or cannot be implicitly converted to one");
    return PARSE_NO_MATCH;
  }
  ++iter_;
  if (skip_ws_on_read_ && iter_ != end_) {
    skip_whitespace();
  }
  return PARSE_OK;
}



int parser_t::read_string(string &value)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }
  switch (iter_->kind) {
  case TOK_NULL_KW:
    value.clear();
    break;
  case TOK_SINGLE_STRING_LIT:
  case TOK_DOUBLE_STRING_LIT:
    value = iter_->value;
    break;
  default:
    set_error("Token is not a string or null");
    return PARSE_NO_MATCH;
  }
  ++iter_;
  if (skip_ws_on_read_ && iter_ != end_) {
    skip_whitespace();
  }
  return PARSE_OK;
}



int parser_t::read_token(token_kind_t kind)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }
  if (iter_->kind == kind) {
    ++iter_;
    if (skip_ws_on_read_ && iter_ != end_) {
      skip_whitespace();
    }
    return PARSE_OK;
  }
  set_error("Token does not match");
  return PARSE_NO_MATCH;
}



int parser_t::read_token(token_kind_t kind, const string &value)
{
  if (iter_ == end_) {
    set_error("No more tokens to read from the token list");
    return PARSE_END_OF_TOKENS;
  }
  const bool kind_matches = (iter_->kind == kind);
  const bool value_matches = (iter_->value == value);
  if (kind_matches && value_matches) {
    ++iter_;
    if (skip_ws_on_read_ && iter_ != end_) {
      skip_whitespace();
    }
    return PARSE_OK;
  }
  if (kind_matches) {
    set_error("Token kind matches but token value does not");
  } else if (value_matches) {
    set_error("Token value matches but token kind does not");
  } else {
    set_error("Neither token kind nor value matches");
  }
  return PARSE_NO_MATCH;
}



int parser_t::read_vec2(vec2f_t &vec)
{
  int result = PARSE_NO_MATCH;
  const auto temp_mark = mark();

  if ((result = read_token(TOK_CURL_OPEN))) {
    return result;
  }
  if ((result = read_float(vec.x))) {
    goto read_vec2_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_vec2_reset_and_return;
  }
  if ((result = read_float(vec.y))) {
    goto read_vec2_reset_and_return;
  }
  if ((result = read_token(TOK_CURL_CLOSE))) {
    goto read_vec2_reset_and_return;
  }

  return result;

read_vec2_reset_and_return:
  reset(temp_mark);
  return result;
}



int parser_t::read_vec3(vec3f_t &vec)
{
  int result = PARSE_NO_MATCH;
  const auto temp_mark = mark();

  if ((result = read_token(TOK_CURL_OPEN))) {
    return result;
  }
  if ((result = read_float(vec.x))) {
    goto read_vec3_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_vec3_reset_and_return;
  }
  if ((result = read_float(vec.y))) {
    goto read_vec3_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_vec3_reset_and_return;
  }
  if ((result = read_float(vec.z))) {
    goto read_vec3_reset_and_return;
  }
  if ((result = read_token(TOK_CURL_CLOSE))) {
    goto read_vec3_reset_and_return;
  }

  return result;

read_vec3_reset_and_return:
  reset(temp_mark);
  return result;
}



int parser_t::read_vec4(vec4f_t &vec)
{
  int result = PARSE_NO_MATCH;
  const auto temp_mark = mark();

  if ((result = read_token(TOK_CURL_OPEN))) {
    return result;
  }
  if ((result = read_float(vec.x))) {
    goto read_vec4_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_vec4_reset_and_return;
  }
  if ((result = read_float(vec.y))) {
    goto read_vec4_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_vec4_reset_and_return;
  }
  if ((result = read_float(vec.z))) {
    goto read_vec4_reset_and_return;
  }
  if ((result = read_token(TOK_CURL_CLOSE)) == PARSE_OK) {
    vec.w = 1.0f;
  } else if (result == PARSE_NO_MATCH) {
    if ((result = read_token(TOK_COMMA))) {
      goto read_vec4_reset_and_return;
    }
    if ((result = read_float(vec.w))) {
      goto read_vec4_reset_and_return;
    }
    if ((result = read_token(TOK_CURL_CLOSE))) {
      goto read_vec4_reset_and_return;
    }
  }

  return result;

read_vec4_reset_and_return:
  reset(temp_mark);
  return result;
}



int parser_t::read_quat(quatf_t &quat)
{
  int result = PARSE_NO_MATCH;
  const auto temp_mark = mark();

  if ((result = read_token(TOK_CURL_OPEN))) {
    return result;
  }
  if ((result = read_float(quat.xyz.x))) {
    goto read_quat_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_quat_reset_and_return;
  }
  if ((result = read_float(quat.xyz.y))) {
    goto read_quat_reset_and_return;
  }
  if ((result = read_token(TOK_COMMA))) {
    goto read_quat_reset_and_return;
  }
  if ((result = read_float(quat.xyz.z))) {
    goto read_quat_reset_and_return;
  }
  if ((result = read_token(TOK_CURL_CLOSE)) == PARSE_OK) {
    quat.w = 1.0f;
  } else if (result == PARSE_NO_MATCH) {
    if ((result = read_token(TOK_COMMA))) {
      goto read_quat_reset_and_return;
    }
    if ((result = read_float(quat.w))) {
      goto read_quat_reset_and_return;
    }
    if ((result = read_token(TOK_CURL_CLOSE))) {
      goto read_quat_reset_and_return;
    }
  }

  return result;

read_quat_reset_and_return:
  reset(temp_mark);
  return result;
}



const token_t &parser_t::current() const
{
  assert(iter_ != end_);
  return *iter_;
}



token_kind_t parser_t::peek_kind() const
{
  if (iter_ >= end_ - 1) {
    return TOK_INVALID;
  }
  return (iter_ + 1)->kind;
}



bool parser_t::next_is(token_kind_t kind) const
{
  return peek_kind() == kind;
}



bool parser_t::next_is(token_kind_t kind, const string &value) const
{
  if (peek_kind() == kind && iter_ >= end_ - 1) {
    return (iter_ + 1)->value == value;
  }
  return false;
}



int parser_t::skip_token()
{
  if (iter_ == end_) {
    return PARSE_END_OF_TOKENS;
  }
  return ++iter_ == end_ ? PARSE_END_OF_TOKENS : PARSE_OK;
}



int parser_t::skip_tokens(size_t count)
{
  for (; count && iter_ != end_; ++iter_, --count) {
    // nop
  }
  return iter_ == end_ ? PARSE_END_OF_TOKENS : PARSE_OK;
}



int parser_t::skip_through_newline()
{
  if (iter_ == end_) {
    return PARSE_END_OF_TOKENS;
  }
  const size_t last_line = iter_->pos.line;
  do {
    const token_kind_t last_kind = iter_->kind;
    if (iter_->pos.line != last_line) {
      // newline not found (may be excluded by lexer) but the current token is
      // on a new line.
      break;
    }
    ++iter_;
    if (last_kind == TOK_NEWLINE) {
      break;
    }
  } while (iter_ != end_);

  return iter_ == end_ ? PARSE_END_OF_TOKENS : PARSE_OK;
}



int parser_t::skip_whitespace() {
  while (iter_ != end_) {
    switch (iter_->kind) {
    case TOK_LINE_COMMENT:    // fall
    case TOK_BLOCK_COMMENT:   // fall
    case TOK_NEWLINE:
      ++iter_;
      continue;
    default:
      break;
    }
    break;
  }
  return iter_ == end_ ? PARSE_END_OF_TOKENS : PARSE_OK;
}



int parser_t::skip_until_token(token_kind_t kind) {
  while (iter_ != end_) {
    if (iter_->kind == kind) {
      break;
    }
    ++iter_;
  }
  return iter_ == end_ ? PARSE_END_OF_TOKENS : PARSE_OK;
}



int parser_t::skip_through_token(token_kind_t kind) {
  while (iter_ != end_) {
    if (iter_->kind == kind) {
      ++iter_;
      break;
    }
    ++iter_;
  }
  return iter_ == end_ ? PARSE_END_OF_TOKENS : PARSE_OK;
}



tokenlist_t::const_iterator parser_t::mark() const
{
  return iter_;
}



void parser_t::reset(const tokenlist_t::const_iterator iter)
{
  assert(iter >= start_);
  assert(iter <= end_);
  iter_ = iter;
}



bool parser_t::eof() const
{
  return iter_ == end_;
}



const string &parser_t::error() const
{
  return error_;
}



const lexer_pos_t &parser_t::error_position() const
{
  return error_pos_;
}



void parser_t::set_error(const string &error)
{
  error_ = error;
  if (iter_ != end_) {
    error_pos_ = iter_->pos;
  } else {
    error_pos_.line = ~0;
    error_pos_.column = ~0;
  }
}



void parser_t::set_skip_whitespace_on_read(bool skip)
{
  skip_ws_on_read_ = skip;
}



bool parser_t::skips_whitespace_on_read() const
{
  return skip_ws_on_read_;
}


} // namespace snow
