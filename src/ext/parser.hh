#ifndef __SNOW__PARSER_HH__
#define __SNOW__PARSER_HH__

#include "../config.hh"
#include "lexer.hh"


namespace snow {


enum parser_result_t : int
{
  PARSE_FATAL = -1, // Anything beneath 0 should be considered a fatal error.
  PARSE_OK = 0, // Parsing/reading/skipping/etc. was successful
  // End of token list was reached (not necessarily an error).
  PARSE_END_OF_TOKENS,
  // No match for read operation tried (not necessarily an error).
  PARSE_NO_MATCH,

  // If extending the parser and returning new error codes, your error codes
  // should use this as their base/starting value.
  PARSE_EXTENDED_RESULT_BASE = 2048,
};



/*==============================================================================

  Base parser class -- provides simple parsing methods, nothing complex. May be
  inherited from to provide more complex functionality. Existing functions
  cannot be overridden.

==============================================================================*/
struct parser_t
{
  parser_t() = default;
  parser_t(
    const tokenlist_t::const_iterator &start,
    const tokenlist_t::const_iterator &end
    );
  virtual ~parser_t();

  void set_tokens(const tokenlist_t &tokens);
  void set_tokens(
    const tokenlist_t::const_iterator &start,
    const tokenlist_t::const_iterator &end
    );

  /** Base reading functions ***/

  // Reads a TOK_ID keyword.
  int read_token_hash32(token_kind_t kind, uint32_t &hash);
  int read_token_hash64(token_kind_t kind, uint64_t &hash);
  int read_keyword(const string &keyword);
  // Returns PARSE_OK if null was read.
  int read_null();
  // Reads any number literal or bool value; if not specifically an integer or
  // float (whichever is requested), the result is implicitly converted.
  // True is converted to 1, false is converted to 0. Null is an invalid value.
  int read_float(float &value);
  int read_float();
  int read_integer(int &value);
  int read_integer();
  // true, false; if a number, != 0 is true, == 0 is false. null is an invalid
  // bool value.
  int read_bool(bool &value);
  int read_bool();
  // Requires a string literal. If the null keyword is encountered, the output
  // string is empty. Numbers will not be implicitly converted to strings.
  int read_string(string &value);
  int read_string();
  // Reads the next token if it is of the kind requested and, if provided, if
  // the token has a specific text value. read_keyword is the same as this, but
  // forces TOK_ID as the token kind.
  int read_token(token_kind_t kind);
  int read_token(token_kind_t kind, const string &value);

  /* Vector / quaternion format:
    { X, Y [, Z [, W]] }
    For 4-component vectors and quaternions, the W component is optional and
    defaults to 1. All components must be some form of number literal.
  */
  int read_vec2(vec2f_t &vec);
  int read_vec3(vec3f_t &vec);
  int read_vec4(vec4f_t &vec);
  int read_quat(quatf_t &quat);
  // int read_mat3(mat3f_t &mat);
  // int read_mat4(mat3f_t &mat);


  /*** Token checking ***/

  // If eof() is true, the result of this function is undefined.
  const token_t &current() const;
  // If there are no tokens after the current token, returns TOK_INVALID. This
  // is not an error.
  token_kind_t peek_kind() const;
  // Peeks ahead to see if the next token matches the given kind and, if given,
  // its text value is as expected.
  bool next_is(token_kind_t kind) const;
  bool next_is(token_kind_t kind, const string &value) const;

  /*** Skipping functions ***/

  // Skips a single token.
  int skip_token();
  // Skips count number of tokens.
  int skip_tokens(size_t count = 1);
  // Note: skip functions do not set error messages. They're intended to be
  // unobtrusive, so their only purpose is to increment the iterator until a
  // specific condition is met.
  // Reads everything through the next newline. If it reaches the end of the
  // token list, it still returns 0.
  int skip_through_newline();
  // Skips all whitespace, comments, etc.
  int skip_whitespace();
  // Skips all tokens until the given token kind is found or the end of the
  // token list is reached.
  int skip_until_token(token_kind_t kind);
  // Same as skip_until_token except the found token is also skipped.
  int skip_through_token(token_kind_t kind);


  /*** Mark and reset ***/

  tokenlist_t::const_iterator mark() const;
  void reset(const tokenlist_t::const_iterator iter);


  /*** Parser state / config ***/

  // Whether at the end of the token list
  bool eof() const;

  const string &error() const;
  const lexer_pos_t &error_position() const;
  void set_error(const string &error);

  void set_skip_whitespace_on_read(bool skip);
  bool skips_whitespace_on_read() const;

private:
  string error_;
  lexer_pos_t error_pos_;
  bool skip_ws_on_read_ = true;

protected:
  tokenlist_t::const_iterator start_;
  tokenlist_t::const_iterator iter_;
  tokenlist_t::const_iterator end_;
};


} // namespace snow

#endif /* end __SNOW__PARSER_HH__ include guard */
