// lexer.hh -- Noel Cower -- Public Domain
// Part of the Snow engine

#ifndef __SNOW__LEXER_HH__
#define __SNOW__LEXER_HH__

#include "../config.hh"
#include <cstdlib>
#include <list>

namespace snow {


enum token_kind_t : unsigned {
  TOK_INVALID=0,

  TOK_TRUE_KW,
  TOK_FALSE_KW,

  TOK_NULL_KW,

  TOK_COLON,
  TOK_QUESTION,
  TOK_BANG,
  TOK_NOT_EQUAL,
  TOK_HASH,
  TOK_DOT,
  TOK_DOUBLE_DOT,
  TOK_TRIPLE_DOT,
  TOK_AT,
  TOK_DOLLAR,
  TOK_PERCENT,
  TOK_PAREN_OPEN,
  TOK_PAREN_CLOSE,
  TOK_BRACKET_OPEN,
  TOK_BRACKET_CLOSE,
  TOK_CURL_OPEN,
  TOK_CURL_CLOSE,
  TOK_GREATER_THAN,
  TOK_SHIFT_RIGHT,
  TOK_GREATER_EQUAL,
  TOK_LESS_THAN,
  TOK_SHIFT_LEFT,
  TOK_LESSER_EQUAL,
  TOK_EQUALS,
  TOK_EQUALITY,
  TOK_MINUS,
  TOK_PLUS,
  TOK_ASTERISK,
  TOK_CARET,
  TOK_TILDE,
  TOK_GRAVE,
  TOK_BACKSLASH,
  TOK_SLASH,
  TOK_COMMA,
  TOK_SEMICOLON,
  TOK_OR,
  TOK_PIPE,
  TOK_AMPERSAND,
  TOK_AND,
  TOK_NEWLINE,

  TOK_ID,

  TOK_NUMBER_LIT,
  TOK_HEX_LIT,
  TOK_BIN_LIT,
  TOK_SINGLE_STRING_LIT,
  TOK_DOUBLE_STRING_LIT,

  TOK_LINE_COMMENT,
  TOK_BLOCK_COMMENT,

  TOK_LAST=TOK_BLOCK_COMMENT,
  TOK_COUNT
};


struct token_t
{
  token_t() = default;
  token_t(const token_t &) = default;
  token_t &operator = (const token_t &) = default;
  token_t(token_t &&);
  token_t &operator = (token_t &&);

  token_kind_t kind = TOK_INVALID;
  size_t line = 0;
  size_t column = 0;

  const string &value() const;
  const string &descriptor() const;

private:
  friend struct lexer_t;

  string value_;
};


using tokenlist_t = std::list<token_t>;


/*
  Disabled set_source/lexer_t(const string &) because they could be temporaries
  and I don't want to make a copy of the source string to keep in the lexer.
*/
struct lexer_t
{
  // Initializes the lexer with an empty string
  lexer_t();

  // Clears previous tokens from the lexer
  void reset();

  // Tokenizes the current source string -- assumes it is entirely valid UTF8.
  // Call utf8::find_invalid beforehand if you're not sure if the string is
  // valid UTF8.
  // Note: end should be the end of a line or the end of a document. The lexer
  // will not carry over state from the last run and continue lexing from it.
  // Basically, all calls to run should extend from the beginning of a line to
  // either the end of a line or the end of a document. Stopping in the middle
  // of whitespace is also acceptable, but not a great idea. Stopping in the
  // middle of a possible token will result in errors.
  bool run(string::const_iterator begin, const string::const_iterator &end);
  bool run(const string &source);

  //
  bool has_error() const;
  const string &error(size_t *line = nullptr, size_t *column = nullptr) const;
  const tokenlist_t &tokens() const;

private:

  struct token_mark_t
  {
    size_t line, column;
    size_t token;
    uint32_t code;
    string::const_iterator place;
  };

  void set_error(const char *errlit, size_t line, size_t col);
  token_t &new_token(token_t &&token);
  void merge_tokens(size_t from, size_t to, token_kind_t newKind);
  token_mark_t current_mark() const;
  // void reset_to_mark(token_mark_t mark);
  uint32_t current() const;
  bool has_next() const;
  uint32_t read_next();
  uint32_t peek_next() const;
  void skip_whitespace();
  token_t read_base_number();
  token_t read_number();
  token_t read_word();
  token_t read_string(const uint32_t delim);
  token_t read_line_comment();
  token_t read_block_comment();

  string::const_iterator source_end_;
  token_mark_t current_;
  tokenlist_t tokens_;

  struct
  {
    size_t line;
    size_t column;
    string message;
  } error_;
};


} // namespace snow


#endif /* end __SNOW__LEXER_HH__ include guard */
