// lexer.cc -- Noel Cower -- Public Domain
// Part of the Snow engine

#include "lexer.hh"
#include <cstddef>
#include <cctype>
#include <cstring>
#include <cstdarg>
#include <array>
#include <sstream>
#include "utf8/unchecked.h"


namespace snow {


#define MIN_TOKEN_STORAGE         (128)
#define MIN_STRING_STORAGE        (64)


static const std::vector<string> token_descriptors {
  "invalid",

  "true",
  "false",

  "null",

  ".",
  "..",
  "...",

  "!",
  "!=",
  "?",
  "#",
  "@",
  "$",
  "%",
  "(",
  ")",
  "[",
  "]",
  "{",
  "}",
  "^",
  "~",
  "`",
  "\\",
  "/",
  ",",
  ";",
  ">",
  ">>",
  ">=",
  "<",
  "<<",
  "<=",
  "=",
  "==",
  "|",
  "||",
  "&",
  "&&",
  ":",
  "::",
  "-",
  "--",
  "->",
  "+",
  "++",
  "*",
  "**",
  "\\n",

  "identifier",

  "integer",
  "float",
  "integer exp",
  "float exp",
  "hexnum lit",
  "binary lit",
  "'...' string",
  "\"...\" string",

  "// comment",
  "/* comment */",
};

static const string g_newline_str = "\n";



std::ostream &operator << (std::ostream &out, const lexer_pos_t &in)
{
  std::stringstream outstr;
  outstr << '[' << in.line << ':' << in.column << ']';
  return out << outstr.str();
}



token_t::token_t(token_t &&token) :
  kind(token.kind),
  pos(token.pos),
  from(std::move(token.from)),
  to(std::move(token.to)),
  value(std::move(token.value))
{
  token.kind        = TOK_INVALID;
  token.pos.line    = 0;
  token.pos.column  = 0;
}



token_t &token_t::operator = (token_t &&token)
{
  if (&token != this) {
    kind          = token.kind;
    value        = std::move(token.value);

    token.kind    = TOK_INVALID;
  }
  return *this;
}



const string &token_t::descriptor() const
{
  if (kind >= TOK_COUNT) {
    return token_descriptors[TOK_INVALID];
  } else {
    return token_descriptors[kind];
  }
}



void lexer_t::set_error(const char *errlit, lexer_error_t code, const lexer_pos_t &pos)
{
  error_.code = code;
  error_.message = string(errlit);
  error_.pos = pos;
}



lexer_t::lexer_t() :
  source_end_(),
  current_({0, 0, { 1, 1 }}),
  tokens_(),
  error_({LEXER_FINISHED, { 0, 0 } })
{
  tokens_.reserve(MIN_TOKEN_STORAGE);
}



void lexer_t::clear()
{
  tokens_.clear();
  error_.message.clear();
  error_.pos = { 0, 0 };
  current_.token = 0;
}



void lexer_t::reset()
{
  error_.message.clear();
  error_.pos = { 0, 0 };
  current_.pos = { 1, 1 };
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
  if (current_.code == '\n') {
    current_.pos.line += 1;
    current_.pos.column = 0;
  }

  if (current_.place != source_end_) {
    ++current_.pos.column;
    current_.code = utf8::unchecked::next(current_.place);
  } else {
    current_.code = 0;
  }


  return current_.code;
}



uint32_t lexer_t::peek_next() const
{
  if (current_.place != source_end_) {
    return utf8::unchecked::peek_next(current_.place);
  } else {
    return 0;
  }
}



void lexer_t::skip_whitespace()
{
  uint32_t cur;
  while ((cur = current_.code) != 0 &&
       (cur == ' ' || cur == '\t' || cur == '\r')) {
    read_next();
  }
}



void lexer_t::read_base_number(token_t &token)
{
  token_mark_t mark = current_mark();
  token.kind = TOK_INVALID;

  uint32_t cur = read_next();

  if (cur == 'b' || cur == 'B') { // bin
    token.kind = TOK_BIN_LIT;
    while (((cur = peek_next()) == '0' || cur == '1')) read_next();
  } else if (cur == 'x' || cur == 'X') {  // hex
    token.kind = TOK_HEX_LIT;
    while (isxdigit(cur = peek_next())) read_next();
  } else {
    set_error("Malformed number literal: not a base-number",
        LEXER_MALFORMED_BASENUM, current_.pos);
    return;
  }

  token.value = string(token.from, current_.place);
}



void lexer_t::read_number(token_t &token)
{
  uint32_t cur = current_.code;
  bool isDec = (cur == '.');
  bool isExp = false;
  token.kind = TOK_INTEGER_LIT;

  while ((cur = peek_next()) != 0) {
    switch (cur) {
    case '.':
      if (!isDec) {
        token.kind = TOK_FLOAT_LIT;
        isDec = true;
        read_next();
        continue;
      }
      break;

    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      read_next();
      continue;

    case 'e': case 'E':
      if (isExp) {
        set_error("Malformed number literal: exponent already provided",
            LEXER_MULTIPLE_EXPONENT, current_.pos);
        token.kind = TOK_INVALID;
        return;
      }
      isExp = true;
      token.kind = token_kind_t(unsigned(token.kind) + 2);
      cur = read_next();
      if (cur == '-' || cur == '+') {
        cur = read_next();
      }

      if (cur < '0' || cur > '9') {
        set_error("Malformed number literal: exponent expected but not found",
          LEXER_NO_EXPONENT, current_.pos);
        token.kind = TOK_INVALID;
        return;
      }
      continue;

      default: break;
    }
    break;
  }

  token.value = string(token.from, current_.place);
}



void lexer_t::read_word(token_t &token)
{
  token_mark_t mark = current_mark();
  token.kind = TOK_ID;

  uint32_t cur;
  while ((cur = peek_next())) {
    if (!(cur == '_' ||
          /* is ASCII number */
          ('0' <= cur && cur <= '9') ||
          /* is ASCII letter */
          ('a' <= cur && cur <= 'z') || ('A' <= cur && cur <= 'Z') ||
          /* character is some other thing that's valid */
          cur >= 160)) {
      break;
    }
    read_next();
  }

  token.value = string(token.from, current_.place);

  switch (token.value.size()) {
  case 4:
    switch (token.value.front()) {
    case 't': if (token.value.has_suffix("rue", 3)) token.kind = TOK_TRUE_KW; break;
    case 'n': if (token.value.has_suffix("ull", 3)) token.kind = TOK_NULL_KW; break;
    default: break;
    } break;
  case 5:
    if (token.value == "false") {
      token.kind = TOK_FALSE_KW;
    }
  default: break;
  }
}



void lexer_t::read_string(token_t &token, const uint32_t delim)
{
  uint32_t cur = current_.code;
  token_mark_t mark = current_mark();
  bool escape = false;
  token.kind = delim == '"' ? TOK_DOUBLE_STRING_LIT : TOK_SINGLE_STRING_LIT;
  token.value.reserve(MIN_STRING_STORAGE);

  auto inserter = std::back_inserter(token.value);
  // uncomment if skipping quotes isn't worthwhile (note: quote still included in from/to)
  // utf8::unchecked::append(cur, inserter);

  while ((cur = read_next())) {
    if (escape) {
      switch (cur) {
      case 'x': case 'X': {
        uint32_t peeked = peek_next();
        if (!isxdigit(peeked)) {
          // will not fail lexing, but will emit an error
          set_error("Malformed unicode literal in string",
            LEXER_MALFORMED_UNICODE, current_.pos);
          break;
        }
        size_t hexnums = (cur == 'x' ? 4 : 8);
        cur = 0;
        do {
          peeked -= (peeked >= 'A'
            ? 'A' - 10
            : ( peeked >= 'a'
              ? 'a' - 10
              : '0' ));
          cur = (cur << 4) | peeked;
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
    set_error("Unterminated string", LEXER_UNTERMINATED_STRING,
      mark.pos);
  // uncomment to not skip quotes in value
  // } else {
    // utf8::unchecked::append(cur, inserter);
  }
}



void lexer_t::read_line_comment(token_t &token)
{
  uint32_t cur;
  token.kind = TOK_LINE_COMMENT;

  while ((cur = peek_next()) && cur != '\n') {
    read_next();
  }

  if (!skip_comments_) {
    token.value = string(token.from, current_.place);
  }
}



void lexer_t::read_block_comment(token_t &token)
{
  const token_mark_t mark = current_mark();
  uint32_t cur = read_next();
  token.kind = TOK_BLOCK_COMMENT;

  while ((cur = read_next())) {
    if (cur == '*' && (cur = peek_next() == '/')) {
      read_next();
      break;
    }
  }

  if (cur == 0) {
    set_error("Unterminated block comment", LEXER_UNTERMINATED_COMMENT,
      mark.pos);
  } else {
    if (!skip_comments_) {
      token.value = string(token.from, current_.place);
    }
  }
}



lexer_error_t lexer_t::run(const string &source)
{
  auto iter = source.cbegin();
  return run(iter, source.cend());
}



lexer_error_t lexer_t::run(string::const_iterator &begin, const string::const_iterator &end,
  token_kind_t until, size_t count)
{
  at_end_ = false;

  if (begin == end) {
    return LEXER_FINISHED;
  } else if (has_error()) {
    return error_.code;
  }

  const bool skip_com = skip_comments_;
  const bool skip_nl = skip_newlines_;
  bool result = true;

  current_.code = 0;
  current_.place = begin;
  source_end_ = end;
  read_next();

  token_mark_t mark;
  token_t token;
  uint32_t cur;
  uint32_t next; // temp variable for peeking/reading

  token_kind_t last_kind = TOK_INVALID;

  while(current_.code != 0 && count) {
    token.kind = TOK_INVALID;
    skip_whitespace();

    mark = current_mark();
    if ((cur = current_.code) == 0) {
      break;
    }

    token.from = current_.place - 1;
    token.pos = current_.pos;

    switch (cur) {

      token.kind = TOK_AT;
      token.value = token_descriptors[TOK_AT];
      break;

      // TOK_DOT, TOK_DOUBLE_DOT, TOK_TRIPLE_DOT
    case '.':
      switch (peek_next()) {
      case '0': case '1': case '2': case '3': case '4':
      case '5': case '6': case '7': case '8': case '9':
        read_number(token);
        break;
      default:
        token.kind = TOK_DOT;

        while(token.kind < TOK_TRIPLE_DOT && peek_next() == '.') {
          read_next();
          ++token.kind;
        }

        token.value = token_descriptors[token.kind];
      }
      break;

      // TOK_LINE_COMMENT
      // TOK_BLOCK_COMMENT
    case '/':
      switch (peek_next()) {
      case '/':
        read_line_comment(token);
        break;
      case '*':
        read_block_comment(token);
        break;
      default:
        token.kind = TOK_SLASH;
        token.value = token_descriptors[TOK_SLASH];
        goto lex_build_token;
      }
      break;

      // TOK_SINGLE_STRING_LIT
      // TOK_DOUBLE_STRING_LIT
    case '"':
    case '\'':
      read_string(token, cur);
      break;

    case '0':
      switch (peek_next()) {
      case 'x': case 'b':
      case 'X': case 'B':
      // TOK_HEX_LIT
      // TOK_BIN_LIT
      read_base_number(token);
      goto lex_exit_early;
      default: break;
      }
      // fall-through:
      // TOK_INTEGER_LIT, TOK_FLOAT_LIT, and EXP variants
    case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      read_number(token);
      break;

      // TOK_MINUS, TOK_DOUBLE_MINUS, and TOK_ARROW (special case)
    case '-': {
      token.kind = TOK_MINUS;
      next = peek_next();
      if (next == '>') {
        read_next();
        token.kind = TOK_ARROW;
        token.value = token_descriptors[TOK_ARROW];
        break;
      }
      goto lex_doubled_token_noread;
      // Punctuation with combos (TOK_AND, TOK_OR, TOK_GREATER_EQUAL, etc.)
    case ':': token.kind = TOK_COLON;         goto lex_doubled_token_read;
    case '=': token.kind = TOK_EQUALS;        goto lex_doubled_token_read;
    case '>': token.kind = TOK_GREATER_THAN;  goto lex_doubled_token_read;
    case '<': token.kind = TOK_LESS_THAN;     goto lex_doubled_token_read;
    case '&': token.kind = TOK_AMPERSAND;     goto lex_doubled_token_read;
    case '|': token.kind = TOK_PIPE;          goto lex_doubled_token_read;
    case '+': token.kind = TOK_PLUS;          goto lex_doubled_token_read;
    case '*': token.kind = TOK_ASTERISK;      goto lex_doubled_token_read;
    case '!': token.kind = TOK_BANG;
lex_doubled_token_read:
      next = peek_next();
lex_doubled_token_noread:
      if (next == cur || (cur == '!' && next == '=')) {
        ++token.kind;
        read_next();
      } else if (token.kind < TOK_PIPE && next == '=') {
        token.kind = (token_kind_t)((unsigned)token.kind + 2);
        read_next();
      }
      token.value = token_descriptors[token.kind];
    } break;

    // Punctuation tokens
    case '?' : token.kind = TOK_QUESTION;      goto lex_build_token;
    case '#' : token.kind = TOK_HASH;          goto lex_build_token;
    case '@' : token.kind = TOK_AT;            goto lex_build_token;
    case '$' : token.kind = TOK_DOLLAR;        goto lex_build_token;
    case '%' : token.kind = TOK_PERCENT;       goto lex_build_token;
    case '(' : token.kind = TOK_PAREN_OPEN;    goto lex_build_token;
    case ')' : token.kind = TOK_PAREN_CLOSE;   goto lex_build_token;
    case '[' : token.kind = TOK_BRACKET_OPEN;  goto lex_build_token;
    case ']' : token.kind = TOK_BRACKET_CLOSE; goto lex_build_token;
    case '{' : token.kind = TOK_CURL_OPEN;     goto lex_build_token;
    case '}' : token.kind = TOK_CURL_CLOSE;    goto lex_build_token;
    case '^' : token.kind = TOK_CARET;         goto lex_build_token;
    case '~' : token.kind = TOK_TILDE;         goto lex_build_token;
    case '`' : token.kind = TOK_GRAVE;         goto lex_build_token;
    case '\\': token.kind = TOK_BACKSLASH;     goto lex_build_token;
    case ',' : token.kind = TOK_COMMA;         goto lex_build_token;
    case ';' : token.kind = TOK_SEMICOLON;     goto lex_build_token;
lex_build_token:
      token.value = token_descriptors[token.kind];
      break;

    case '\n':
      token.kind = TOK_NEWLINE;
      token.value = g_newline_str;
      break;

      // TOK_ID, TOK_TRUE_KW, TOK_FALSE_KW, and TOK_NULL_KW
    default:
      read_word(token);
    }

    token.to = current_.place;
    read_next();

lex_exit_early:
    last_kind = token.kind;
    if (!(skip_com && last_kind >= TOK_LINE_COMMENT) && !(skip_nl && last_kind == TOK_NEWLINE)) {
      if (last_kind != TOK_INVALID) {
        new_token(std::move(token));
      } else if (!has_error()) {
        set_error("Invalid token", LEXER_INVALID_TOKEN, current_.pos);
      }
    }

    if (until != TOK_INVALID && last_kind == until) {
      break;
    }

    if (has_error()) {
      break;
    }

    --count;
  }

  begin = current_.place;
  return error_.code;
}



const tokenlist_t &lexer_t::tokens() const
{
  return tokens_;
}



bool lexer_t::skip_comments() const
{
  return skip_comments_;
}



void lexer_t::set_skip_comments(bool skip)
{
  skip_comments_ = skip;
}



bool lexer_t::skip_newlines() const
{
  return skip_newlines_;
}



void lexer_t::set_skip_newlines(bool skip)
{
  skip_newlines_ = skip;
}



bool lexer_t::has_error() const
{
  return error_.code != LEXER_FINISHED;
}



lexer_error_t lexer_t::error_code() const
{
  return error_.code;
}



const lexer_pos_t &lexer_t::error_position() const
{
  return error_.pos;
}



const string &lexer_t::error_message() const
{
  return error_.message;
}


} // namespace snow
