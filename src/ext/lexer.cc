// lexer.cc -- Noel Cower -- Public Domain
// Part of the Snow engine

#include "lexer.hh"
#include <cstddef>
#include <cctype>
#include <cstring>
#include <cstdarg>
#include <array>
#include "utf8.h"


namespace snow {


static const std::vector<string> token_descriptors {
  "invalid",

  "true",
  "false",

  "null",

  ":",
  "?",
  "!",
  "!=",
  "#",
  ".",
  "..",
  "...",
  "@",
  "$",
  "%",
  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  ">",
  ">>",
  ">=",
  "<",
  "<<",
  "<=",
  "=",
  "==",
  "-",
  "+",
  "*",
  "^",
  "~",
  "`",
  "\\",
  "/",
  ",",
  ";",
  "|",
  "||",
  "&",
  "&&",
  "\\n",

  "identifier",

  "number",
  "hexnum",
  "binnum",
  "'...' string",
  "\"...\" string",

  "// comment",
  "/* comment */",
};



token_t::token_t(token_t &&token) :
  kind(token.kind),
  line(token.line),
  column(token.column),
  value_(std::move(token.value_))
{
  token.kind    = TOK_INVALID;
  token.line    = 0;
  token.column  = 0;
}



token_t &token_t::operator = (token_t &&token)
{
  if (&token != this) {
    kind          = token.kind;
    line          = token.line;
    column        = token.column;
    value_        = std::move(token.value_);

    token.kind    = TOK_INVALID;
    token.line    = 0;
    token.column  = 0;
  }
  return *this;
}



const string &token_t::value() const
{
  return value_;
}



const string &token_t::descriptor() const
{
  if (kind >= TOK_COUNT) {
    return token_descriptors[TOK_INVALID];
  } else {
    return token_descriptors[kind];
  }
}



void lexer_t::set_error(const char *errlit, size_t line, size_t col)
{
  error_.message = string(errlit);
  error_.line = line;
  error_.column = col;
}



lexer_t::lexer_t() :
  source_end_(),
  current_({1, 1, 0}),
  tokens_(),
  error_({0, 0})
{
  // nop
}



void lexer_t::reset()
{
  tokens_.clear();
  error_.message.clear();
  error_.line = 0;
  error_.column = 0;
  current_.line = 1;
  current_.column = 1;
  current_.token = 0;
}



token_t &lexer_t::new_token(token_t &&token)
{
  current_.token += 1;
  tokens_.emplace_back(std::forward<token_t &&>(token));
  return tokens_.back();
}



auto lexer_t::current_mark() const -> token_mark_t
{
  return current_;
}



uint32_t lexer_t::current() const
{
  return current_.code;
}



bool lexer_t::has_next() const
{
  return current_.place != source_end_;
}



/*==============================================================================
  lexer_next

    Reads the next character from the source into the lexer. The character can
    be gotten again later via lexer_current. Will also increment the line and
    column counters as needed.

    Note: when parsing, if you have a loop that calls lexer_next, and the loop
    consumes the last character parsed, call lexer_next once again after it
    completes - if the final character shouldn't be consumed, of course, don't.
==============================================================================*/
uint32_t lexer_t::read_next()
{
  if (current() == '\n') {
    current_.line += 1;
    current_.column = 0;
  }

  if (has_next()) {
    ++current_.column;
    current_.code = utf8::unchecked::next(current_.place);
  } else {
    current_.code = 0;
  }


  return current_.code;
}



uint32_t lexer_t::peek_next() const
{
  if (has_next()) {
    return utf8::unchecked::peek_next(current_.place);
  } else {
    return 0;
  }
}



void lexer_t::skip_whitespace()
{
  uint32_t cur;
  while ((cur = current()) != 0 &&
       (cur == ' ' || cur == '\t' || cur == '\r')) {
    read_next();
  }
}



token_t lexer_t::read_base_number()
{
  token_mark_t mark = current_mark();
  token_t token;
  token.kind = TOK_NUMBER_LIT;
  token.line = mark.line;

  auto inserter = std::back_inserter(token.value_);
  utf8::unchecked::append(current(), inserter);
  uint32_t cur = read_next();
  utf8::unchecked::append(cur, inserter);

  if (cur == 'b' || cur == 'B') { // bin
    token.kind = TOK_BIN_LIT;
    while (has_next() && ((cur = read_next()) == '0' || cur == '1')) {
      utf8::unchecked::append(cur, inserter);
    }
  } else if (cur == 'x' || cur == 'X') {  // hex
    token.kind = TOK_HEX_LIT;
    while (has_next() && isxdigit(read_next())) {
      utf8::unchecked::append(cur, inserter);
    }
  } else {
    set_error("Malformed number literal: not a base-number",
        current_.line, current_.column);
    token.kind = TOK_INVALID;
  }

  return token;
}



token_t lexer_t::read_number()
{
  uint32_t cur = current();
  token_mark_t mark = current_mark();
  bool isDec = (cur == '.');
  bool isExp = false;
  token_t token;
  token.kind = TOK_NUMBER_LIT;
  token.line = mark.line;
  token.column = mark.column;

  auto inserter = std::back_inserter(token.value_);
  utf8::unchecked::append(cur, inserter);

  while (has_next() && (cur = read_next()) != 0) {
    switch (cur) {
    case '.':
      if (!isDec) {
        utf8::unchecked::append(cur, inserter);
        isDec = true;
        continue;
      }
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      utf8::unchecked::append(cur, inserter);
      continue;

    case 'e': case 'E':
      if (isExp) {
        set_error("Malformed number literal: exponent already provided",
            current_.line, current_.column);
        token.kind = TOK_INVALID;
        return token;
      }
      isExp = true;
      utf8::unchecked::append(cur, inserter);
      utf8::unchecked::append((cur = read_next()), inserter);
      if (cur == '-' || cur == '+') {
        utf8::unchecked::append((cur = read_next()), inserter);
      }

      if (cur < '0' || cur > '9') {
        set_error("Malformed number literal: exponent expected but not found",
          current_.line, current_.column);
        token.kind = TOK_INVALID;
        return token;
      }
      continue;

      default: break;
    }
    break;
  }
  lex_number_done:
  return token;
}



token_t lexer_t::read_word()
{
  token_mark_t mark = current_mark();
  token_t token;
  token.kind = TOK_ID;
  token.line = mark.line;
  token.column = mark.column;

  auto inserter = std::back_inserter(token.value_);
  utf8::unchecked::append(current(), inserter);

  while (has_next()) {
    uint32_t cur = peek_next();
    if (cur == '_' ||
      /* is ASCII number */
      ('0' <= cur && cur <= '9') ||
      /* is ASCII letter */
      ('a' <= cur && cur <= 'z') || ('A' <= cur && cur <= 'Z') ||
      /* character is some other thing that's valid */
      cur >= 160) {
      utf8::unchecked::append(read_next(), inserter);
    } else {
      break;
    }
  }

  read_next();

  switch (token.value_.size()) {
  case 4:
    switch (token.value_.front()) {
    case 't': if (token.value_.find("rue", 1, 3) == 1) token.kind = TOK_TRUE_KW; break;
    case 'n': if (token.value_.find("ull", 1, 3) == 1) token.kind = TOK_NULL_KW; break;
    default: break;
    } break;
  case 5:
    if (token.value_ == "false") {
      token.kind = TOK_FALSE_KW;
    }
  default: break;
  }

  return token;
}



token_t lexer_t::read_string(const uint32_t delim)
{
  static uint32_t hex_lookup[16] = {
    0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9,
    0xA, 0xB, 0xC, 0xD, 0xE, 0xF
  };

  uint32_t cur = current();
  token_mark_t mark = current_mark();
  bool escape = false;
  token_t token;
  token.kind = delim == '"' ? TOK_DOUBLE_STRING_LIT : TOK_SINGLE_STRING_LIT;
  token.line = mark.line;
  token.column = mark.column;

  auto inserter = std::back_inserter(token.value_);
  utf8::unchecked::append(cur, inserter);

  while (has_next() && (cur = read_next())) {
    if (escape) {
      switch (cur) {
      case 'x': case 'X': {
        uint32_t peeked = peek_next();
        if (!isxdigit(peeked)) {
          // will not fail lexing, but will emit an error
          set_error("Malformed unicode literal in string",
            current_.line, current_.column);
          break;
        }
        size_t hexnums = cur == 'x' ? 4 : 8;
        cur = 0;
        do {
          if (peeked >= 'A') {
            peeked -= ('A' - 10);
          } else if (peeked >= 'a') {
            peeked -= ('a' - 10);
          } else {
            peeked -= '0';
          }
          cur = (cur << 4) | hex_lookup[peeked];
          --hexnums;
          read_next();
          peeked = peek_next();
        } while(isxdigit(peeked) && hexnums);
      } break;
      case 'r': cur = '\r'; break;
      case 'n': cur = '\n'; break;
      case 't': cur = '\t'; break;
      case '0': cur = '\0'; break;
      case 'b': cur = '\b'; break;
      case 'a': cur = '\a'; break;
      case 'f': cur = '\f'; break;
      case 'v': cur = '\v'; break;
      default: break;
      }
      escape = false;
    } else {
      if (cur == delim) {
        break;
      } else if (cur == '\\') {
        escape = true;
        continue;
      }
    }
    utf8::unchecked::append(cur, inserter);
  }

  if (cur == 0) {
    set_error("Unterminated string", mark.line, mark.column);
  } else {
    utf8::unchecked::append(cur, inserter);
    read_next();
  }

  return token;
}



token_t lexer_t::read_line_comment()
{
  uint32_t cur = current();
  token_mark_t mark = current_mark();
  token_t token;
  token.kind = TOK_LINE_COMMENT;
  token.line = mark.line;
  token.column = mark.column;

  auto inserter = std::back_inserter(token.value_);

  do {
    utf8::unchecked::append(cur, inserter);
    cur = read_next();
  } while(cur != 0 && cur != '\n');

  return token;
}



token_t lexer_t::read_block_comment()
{
  token_mark_t mark = current_mark();
  uint32_t cur = read_next();
  token_t token;
  token.kind = TOK_BLOCK_COMMENT;
  token.line = mark.line;
  token.column = mark.column;

  auto inserter = std::back_inserter(token.value_);
  utf8::unchecked::append(mark.code, inserter);
  utf8::unchecked::append(cur, inserter);

  while (has_next() && (cur = read_next())) {
    utf8::unchecked::append(cur, inserter);
    if (cur == '*' && peek_next() == '/') {
      cur = read_next();
      utf8::unchecked::append(cur, inserter);
      break;
    }
  }

  if (cur == 0) {
    set_error("Unterminated block comment", mark.line, mark.column);
  } else {
    read_next();
  }

  return token;
}



bool lexer_t::run(const string &source)
{
  return run(source.cbegin(), source.cend());
}



bool lexer_t::run(string::const_iterator begin, const string::const_iterator &end)
{
  current_.code = 0;
  current_.column = 0;
  current_.line = 1;
  current_.place = begin;
  source_end_ = end;
  read_next();

  token_mark_t mark;
  token_t token;
  uint32_t cur;

  while(current() != 0) {
    token.kind = TOK_INVALID;
    skip_whitespace();

    mark = current_mark();
    if ((cur = current()) == 0) {
      break;
    }

    switch (cur) {
    case '@':
      token.kind = TOK_AT;
      token.value_ = token_descriptors[TOK_AT];
      token.line = mark.line;
      token.column = mark.column;
      read_next();
      break;

    case '.':
      switch (peek_next()) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        token = std::move(read_number());
        break;
      default:
        token.kind = TOK_DOT;

        while(token.kind <= TOK_TRIPLE_DOT && read_next() == '.') {
          ++token.kind;
        }

        token.value_ = token_descriptors[token.kind];
        token.line = mark.line;
        token.column = mark.column;
      }
      break;

      // TOK_LINE_COMMENT
      // TOK_BLOCK_COMMENT
    case '/':
      switch (peek_next()) {
      case '/':
        token = std::move(read_line_comment());
        break;
      case '*':
        token = std::move(read_block_comment());
        break;
      default:
        token.kind = TOK_SLASH;
        token.value_ = token_descriptors[TOK_SLASH];
        goto lex_build_token;
      }
      break;

      // TOK_SINGLE_STRING_LIT
      // TOK_DOUBLE_STRING_LIT
    case '"':
    case '\'':
      token = std::move(read_string(cur));
      break;

    case '0':
      switch (peek_next()) {
      case 'x': case 'b':
      case 'X': case 'B':
      token = std::move(read_base_number());
      default: break;
      }
      // fall-through:
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      token = std::move(read_number());
      break;

    case '>': token.kind = TOK_GREATER_THAN;  goto lex_doubled_token;
    case '<': token.kind = TOK_LESS_THAN;     goto lex_doubled_token;
    case '&': token.kind = TOK_AMPERSAND;     goto lex_doubled_token;
    case '|': token.kind = TOK_PIPE;          goto lex_doubled_token;
    case '!': token.kind = TOK_BANG;
    lex_doubled_token:
    {
      auto inserter = std::back_inserter(token.value_);
      utf8::unchecked::append(cur, inserter);
      const uint32_t next = read_next();
      if (next == cur || (cur == '!' && next == '=')) {
        utf8::unchecked::append(next, inserter);
        ++token.kind;
        read_next();
      } else if (token.kind < TOK_AMPERSAND && next == '=') {
        utf8::unchecked::append(next, inserter);
        token.kind = (token_kind_t)((unsigned)token.kind + 2);
        read_next();
      }
    } break;

    case ':' : token.kind = TOK_COLON;         goto lex_build_token;
    case '?' : token.kind = TOK_QUESTION;      goto lex_build_token;
    case '$' : token.kind = TOK_DOLLAR;        goto lex_build_token;
    case '%' : token.kind = TOK_PERCENT;       goto lex_build_token;
    case '(' : token.kind = TOK_PAREN_OPEN;    goto lex_build_token;
    case ')' : token.kind = TOK_PAREN_CLOSE;   goto lex_build_token;
    case '[' : token.kind = TOK_BRACKET_OPEN;  goto lex_build_token;
    case ']' : token.kind = TOK_BRACKET_CLOSE; goto lex_build_token;
    case '{' : token.kind = TOK_CURL_OPEN;     goto lex_build_token;
    case '}' : token.kind = TOK_CURL_CLOSE;    goto lex_build_token;
    case '-' : token.kind = TOK_MINUS;         goto lex_build_token;
    case '+' : token.kind = TOK_PLUS;          goto lex_build_token;
    case '*' : token.kind = TOK_ASTERISK;      goto lex_build_token;
    case '^' : token.kind = TOK_CARET;         goto lex_build_token;
    case '~' : token.kind = TOK_TILDE;         goto lex_build_token;
    case '`' : token.kind = TOK_GRAVE;         goto lex_build_token;
    case '\\': token.kind = TOK_BACKSLASH;     goto lex_build_token;
    case ',' : token.kind = TOK_COMMA;         goto lex_build_token;
    case ';' : token.kind = TOK_SEMICOLON;     goto lex_build_token;
    case '\n': token.kind = TOK_NEWLINE;
      lex_build_token:
      token.value_.push_back(cur);
      read_next();
      break;

    default:
      token = std::move(read_word());
    }

    if (token.kind != TOK_INVALID) {
      new_token(std::move(token));
    } else if (!has_error()) {
      set_error("Invalid token", current_.line, current_.column);
    }

    if (has_error()) {
      return false;
    }
  }

  return true;
}



const tokenlist_t &lexer_t::tokens() const
{
  return tokens_;
}



bool lexer_t::has_error() const
{
  return !(error_.message.empty());
}



const string &lexer_t::error(size_t *line, size_t *column) const
{
  if (line) *line = error_.line;
  if (column) *column = error_.column;
  return error_.message;
}


} // namespace snow
