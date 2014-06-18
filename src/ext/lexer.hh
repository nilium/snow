/*
  lexer.hh -- Copyright (c) 2013 Noel Cower. All rights reserved.
  See COPYING under the project root for the source code license. If this file
  is not present, refer to <https://raw.github.com/nilium/snow/master/COPYING>.
*/
#ifndef __SNOW__LEXER_HH__
#define __SNOW__LEXER_HH__

#include "../config.hh"
#include <cstdlib>
#include <vector>

namespace snow {


enum token_kind_t : int {
  TOK_INVALID=0,

  TOK_TRUE_KW,
  TOK_FALSE_KW,

  TOK_NULL_KW,

  TOK_DOT,
  TOK_DOUBLE_DOT,
  TOK_TRIPLE_DOT,

  TOK_BANG,
  TOK_NOT_EQUAL,
  TOK_QUESTION,
  TOK_HASH,
  TOK_AT,
  TOK_DOLLAR,
  TOK_PERCENT,
  TOK_PAREN_OPEN,
  TOK_PAREN_CLOSE,
  TOK_BRACKET_OPEN,
  TOK_BRACKET_CLOSE,
  TOK_CURL_OPEN,
  TOK_CURL_CLOSE,
  TOK_CARET,
  TOK_TILDE,
  TOK_GRAVE,
  TOK_BACKSLASH,
  TOK_SLASH,
  TOK_COMMA,
  TOK_SEMICOLON,
  TOK_GREATER_THAN,
  TOK_SHIFT_RIGHT,
  TOK_GREATER_EQUAL,
  TOK_LESS_THAN,
  TOK_SHIFT_LEFT,
  TOK_LESSER_EQUAL,
  TOK_EQUALS,
  TOK_EQUALITY,
  TOK_PIPE,
  TOK_OR,
  TOK_AMPERSAND,
  TOK_AND,
  TOK_COLON,
  TOK_DOUBLE_COLON,
  TOK_MINUS,
  TOK_DOUBLE_MINUS,
  TOK_ARROW,
  TOK_PLUS,
  TOK_DOUBLE_PLUS,
  TOK_ASTERISK,
  TOK_DOUBLE_ASTERISK,
  TOK_NEWLINE,

  TOK_ID,

  TOK_INTEGER_LIT,
  TOK_FLOAT_LIT,
  TOK_INTEGER_EXP_LIT,
  TOK_FLOAT_EXP_LIT,
  TOK_HEX_LIT,
  TOK_BIN_LIT,
  TOK_SINGLE_STRING_LIT,
  TOK_DOUBLE_STRING_LIT,

  TOK_LINE_COMMENT,
  TOK_BLOCK_COMMENT,

  TOK_LAST=TOK_BLOCK_COMMENT,
  TOK_COUNT
};


struct lexer_pos_t
{
  size_t line;
  size_t column;
};


std::ostream &operator << (std::ostream &out, const lexer_pos_t &in);


struct token_t
{
  token_t() = default;
  token_t(const token_t &) = default;
  token_t &operator = (const token_t &) = default;
  token_t(token_t &&);
  token_t &operator = (token_t &&);

  token_kind_t kind = TOK_INVALID;
  lexer_pos_t pos;
  string::const_iterator from;
  string::const_iterator to;
  string value;

  const string &descriptor() const;

  inline bool is_int() const
  {
    return kind == TOK_INTEGER_LIT || kind == TOK_INTEGER_EXP_LIT || kind == TOK_HEX_LIT;
  }

  inline bool is_float() const
  {
    return kind == TOK_FLOAT_LIT || kind == TOK_FLOAT_EXP_LIT;
  }

  inline bool is_number() const
  {
    return kind >= TOK_INTEGER_LIT && kind <= TOK_HEX_LIT;
  }

  inline bool is_string() const
  {
    return kind == TOK_SINGLE_STRING_LIT || kind == TOK_DOUBLE_STRING_LIT;
  }

  inline bool is_comment() const
  {
    return kind == TOK_LINE_COMMENT || kind == TOK_BLOCK_COMMENT;
  }
};


using tokenlist_t = std::vector<token_t>;


enum lexer_error_t : unsigned
{
  LEXER_FINISHED = 0,
  LEXER_INVALID_TOKEN,
  LEXER_MALFORMED_BASENUM,
  LEXER_MULTIPLE_EXPONENT,
  LEXER_NO_EXPONENT,
  LEXER_MALFORMED_UNICODE,
  LEXER_UNTERMINATED_STRING,
  LEXER_UNTERMINATED_COMMENT,
  LEXER_COUNT_REACHED,
  LEXER_TOKEN_FOUND
};


/*
  Disabled set_source/lexer_t(const string &) because they could be temporaries
  and I don't want to make a copy of the source string to keep in the lexer.
*/
struct lexer_t
{
  // Initializes the lexer with an empty string
  lexer_t();

  // Resets the line/column counters
  void reset();

  // Clears previous tokens from the lexer
  void clear();

  // Tokenizes the current source string -- assumes it is entirely valid UTF8.
  // Call utf8::find_invalid beforehand if you're not sure if the string is
  // valid UTF8.
  // Note: end should be the end of a line or the end of a document. The lexer
  // will not carry over state from the last run and continue lexing from it.
  // Basically, all calls to run should extend from the beginning of a line to
  // either the end of a line or the end of a document. Stopping in the middle
  // of whitespace is also acceptable, but not a great idea. Stopping in the
  // middle of a possible token will result in errors.
  // If a token kind other than TOK_INVALID (which the lexer will always stop
  // for) is provided, the lexer will stop if it encounters that token. The
  // token that stopped it will be the last token in the token list.
  // If the lexer reads in _count_ number of tokens, it will stop afterward.
  // By default, the count is SIZE_MAX, so you're unlikely to ever see the
  // counter return because of that. Still, check the return code.
  lexer_error_t run(
    string::const_iterator &begin, const string::const_iterator &end,
    token_kind_t until = TOK_INVALID, size_t count = SIZE_MAX);
  lexer_error_t run(const string &source);

  bool skip_comments() const;
  void set_skip_comments(bool skip);
  bool skip_newlines() const;
  void set_skip_newlines(bool skip);

  bool has_error() const;
  lexer_error_t error_code() const;
  const lexer_pos_t &error_position() const;
  const string &error_message() const;
  const tokenlist_t &tokens() const;

private:

  struct token_mark_t
  {
    size_t token;
    uint32_t code;
    lexer_pos_t pos;
    string::const_iterator place;
  };

  void set_error(const char *errlit, lexer_error_t error, const lexer_pos_t &pos);
  token_t &new_token(token_t &&token);
  void merge_tokens(size_t from, size_t to, token_kind_t newKind);
  token_mark_t current_mark() const;
  // void reset_to_mark(token_mark_t mark);
  uint32_t read_next();
  uint32_t peek_next() const;
  void skip_whitespace();
  void read_base_number(token_t &token);
  void read_number(token_t &token);
  void read_word(token_t &token);
  void read_string(token_t &token, const uint32_t delim);
  void read_line_comment(token_t &token);
  void read_block_comment(token_t &token);

  string::const_iterator source_end_;
  token_mark_t current_;
  tokenlist_t tokens_;
  bool skip_comments_ = false;
  bool skip_newlines_ = false;
  bool at_end_ = false;

  struct
  {
    lexer_error_t code;
    lexer_pos_t pos;
    string message;
  } error_;
};


} // namespace snow


#endif /* end __SNOW__LEXER_HH__ include guard */
