/* extend - file format and program language
 * Copyright (C) 2020 Vladimir Liutov vs@lutov.net
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Affero General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Affero General Public License for more details.
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include "textdata/reader.h"

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/type_traits.h>
#include <EASTL/unordered_set.h>
#include <EASTL/variant.h>
#include <EASTL/vector.h>

using namespace eastl;

namespace extend::textdata {

/**
 * Token type
 */
enum TOK
{
#define DECLARE_TOKEN(NAME, VALUE_TYPE) NAME,
#include "tokens.h"
#undef DECLARE_TOKEN
};

/**
 * Info about one Token.
 */
struct Token
{
  using TokenValue =
    variant<monostate, int32_t, string>; /**< Token value type */

  TOK type;
  TokenValue value;
  ssize_t pos = 0;
  string input = "";

  /**
   * Create token without binding to source code.
   * For test purpose only.
   */
  Token(TOK type, TokenValue&& value);
  /**
   * Create token from source code.
   */
  Token(TOK type, TokenValue&& value, ssize_t pos, string&& input);

  /**
   * Check for equal Token::type and Token::value
   */
  bool operator==(const Token& rhs) const;

  /**
   * Check for not equal Token::type and Token::value
   */
  bool operator!=(const Token& rhs) const;

  /**
   * Get string name of Token::type.
   *
   * @return     human readable representation
   */
  [[nodiscard]] const char* typeAsStr() const;
};

/**
 * Print token to ostream
 */
template<class Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& out, const Token& token)
{
  out << "Token(" << token.typeAsStr() << ", ";
  if (holds_alternative<int32_t>(token.value)) {
    uint32_t value = get<int32_t>(token.value);
    if (token.type == TOK::SYMBOL) {
      out << "'";
      bool started = false;
      for (ssize_t i = 3; i >= 0; --i) {
        unsigned char c = (value >> (i * 8)) & 0xff;
        started |= bool(c);
        if (started) {
          if (c == '\n') {
            out << "\\n";
          } else if (c == '\t') {
            out << "\\t";
          } else {
            out << c;
          }
        }
      }
      out << "'";
    } else {
      out << value;
    }
  } else if (holds_alternative<string>(token.value)) {
    out << '"';
    for (char c : get<string>(token.value)) {
      if (c == '\n') {
        out << "\\n";
      } else if (c == '\t') {
        out << "\\t";
      } else {
        out << c;
      }
    }
    out << '"';
  } else if (holds_alternative<monostate>(token.value)) {
    out << "monostate()";
  } else {
    out << "unexpected";
  }
  out << ")";
  return out;
}

struct Lexer
{
private:
  template<ssize_t peek_len = 1,
           bool close_required = false,
           ssize_t begin_capture = 0,
           bool end_capture = false>
  optional<Token> sequence(int predicate(int))
  {
    if ((predicate(reader.peek<peek_len>()) != 0) || (begin_capture > 0)) {
      ssize_t pos = reader.offset;
      for (ssize_t i = 0; i < begin_capture; ++i) {
        reader.get();
      }

      while (!reader.eof() && predicate(reader.peek<peek_len>()) != 0) {
        reader.get();
      }

      if (close_required && reader.eof()) {
        return optional<Token>();
      }

      if (end_capture) {
        for (ssize_t i = 0; i < peek_len; ++i) {
          reader.get();
        }
      }

      string input = reader.substr(pos, reader.offset - pos);

      return make_optional<Token>(
        TOK::EMPTY, monostate(), pos, move(input));
    }

    return optional<Token>();
  }

  template<int32_t value,
           bool close_required = false,
           ssize_t begin_capture = 0,
           bool end_capture = false>
  optional<Token> until()
  {
    static_assert(0 < value && value <= 0xffffff);
    auto isContinue = [](int c) -> int { return int(c != value); };
    if constexpr (value <= 0xff) {
      return sequence<1, close_required, begin_capture, end_capture>(
        isContinue);
    }
    if constexpr (value <= 0xffff) {
      return sequence<2, close_required, begin_capture, end_capture>(
        isContinue);
    }
    return sequence<3, close_required, begin_capture, end_capture>(isContinue);
  }

  struct Indent final
  {
    string input;
    ssize_t pos;
    Indent(string&& input, ssize_t pos);
  };
  vector<Indent> indents;

  void emplaceDeindent();
  bool emplaceIndent(const string &input);
  bool unexpectedIndent();
  bool parseIndent(const string &input);

public:
  Reader reader;
  vector<Token> data;
  ssize_t errors = 0;

  Lexer(io::FileSource&& source);
  ~Lexer() = default;

  Lexer(const Lexer&) = delete;
  Lexer(Lexer&&) = delete;
  Lexer& operator=(const Lexer&) = delete;
  Lexer& operator=(Lexer&&) = delete;
};

static unordered_set<string> KEYWORDS{ "i32" };

}
