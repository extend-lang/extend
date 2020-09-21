/* extend - expansible programming language
 * Copyright (C) 2021 Vladimir Liutov vs@lutov.net
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

#include "text/LexerStruct.h"
#include "text/ParserTraits.h"
#include "textdata/lexer.h"
#include "textcode/lexer.h"
#include <g3log/g3log.hpp>
#include <string>

namespace extend::text {

template<ParserType t>
TokenRecord<t>::TokenRecord(TOK type_, FromLexer&& value_)
  : type(type_)
  , value(move(value_))
{}

template<ParserType t>
TokenRecord<t>::TokenRecord(TOK type_,
                            FromLexer&& value_,
                            ssize_t pos_,
                            typename Encoding::Span input_)
  : type(type_)
  , value(move(value_))
  , pos(pos_)
  , input(move(input_))
{}

template<ParserType t>
bool
TokenRecord<t>::operator==(const TokenRecord& rhs) const
{
  return (this->type == rhs.type) && (this->value == rhs.value);
}

template<ParserType t>
bool
TokenRecord<t>::operator!=(const TokenRecord& rhs) const
{
  return !(*this == rhs);
}

template<ParserType t>
BraceRecord<t>::BraceRecord(Token&& token_,
                            bool openingBrace_,
                            optional<Token>&& closingBrace_)
  : token(move(token_))
  , openingBrace(openingBrace_)
  , closingBrace(closingBrace_)
{}

template<typename Traits, ParserType t>
std::basic_ostream<char, Traits>&
operator<<( // NOLINT(readability-function-cognitive-complexity)
  std::basic_ostream<char, Traits>& out,
  const TokenRecord<t>& token)
{
  using Triplet = typename io::EncodingTraits<ParserTraits<t>::encoding>::Triplet;
  using Container = typename io::EncodingTraits<ParserTraits<t>::encoding>::Container;
  out << "Token(" << ParserTraits<t>::typeAsStr(token.type) << ", ";
  visit([&out](const auto &value) {
    using T = decay_t<decltype(value)>;
    if constexpr (is_same_v<T, Triplet>) {
      out << value;
    } else if constexpr (is_same_v<T, Container>) {
      out << '"';
      for (char32_t c : value) {
        out << io::printChar(c).begin();
      }
      out << '"';
    } else if constexpr (is_same_v<T, monostate>) {
      out << "monostate()";
    } else {
      out << "Unexpected";
      // static_assert(0);
    }
  }, token.value);
  out << ')';
  return out;
}

template<ParserType t>
Reader<LexerStruct<t>::encoding>&
LexerStruct<t>::r()
{
  return reader;
}

template<ParserType t>
ssize_t
LexerStruct<t>::errorCount() const
{
  return reader.errorCount();
}

static void
parseError()
{
  LOG(FATAL) << "Parsing failed because of previous errors";
}

template<typename Encoding>
static bool
is_ascii(const typename Encoding::Triplet &tri) {
  return tri.size() == 1 && tri[0] <= 0x7f;
}

template<typename Encoding>
static bool
is_blank(const typename Encoding::Triplet &tri) {
  return is_ascii<Encoding>(tri)
    && isblank(tri[0]);
}


template<typename Encoding>
static bool
is_digit(const typename Encoding::Triplet &tri) {
  return is_ascii<Encoding>(tri)
    && isdigit(tri[0]);
}

template<typename Encoding>
static bool
is_alnum(const typename Encoding::Triplet &tri) {
  return is_ascii<Encoding>(tri)
    && isalnum(tri[0]);
}

// FIXME: split into several functions
template<ParserType t>
LexerStruct<t>::LexerStruct(
  io::FileSource<encoding>&& source)
  : reader(move(source))
{
  optional<Token> token = {};
  vector<Token> braces = {};
  bool isIndentParsing = true;
  ssize_t indentBrokenBy = -1;
  Span currentIndent = Encoding::zeroString;

  while (!reader.eof()) {
    ssize_t pos = reader.tellp();
    CodePoint c = reader.peek()[0];

    if (c == '\n') {
      if constexpr (Traits::isIndentSupported) {
        if (braces.empty()) {
          isIndentParsing = true;
          indentBrokenBy = -1;
          currentIndent = Encoding::zeroString;
        }
      }
      data.emplace_back(TOK::SYMBOL, reader.get(), pos, reader.substr(pos, 1));
      continue;
    }

    if ((token = sequence(is_blank<Encoding>)).has_value()) {
      token->type = TOK::SPACE;
      token->value = Container(token->input);
      data.push_back(*move(token));

      if constexpr (Traits::isIndentSupported) {
        if (isIndentParsing && currentIndent.empty()) {
          currentIndent = data.back().input;
        }
      }
      continue;
    }

    if ((token = Traits::parseComment(*this)).has_value()) {
      if (token->type == TOK::PARSER_ERROR) {
        parseError();
        return;
      }

      data.push_back(*move(token));
      if constexpr (Traits::isIndentSupported) {
        if (isIndentParsing) {
          indentBrokenBy = pos;
        }
      }
      continue;
    }

    if constexpr (Traits::isIndentSupported) {
      // Real code: not comment or new line
      if (isIndentParsing) {
        if (indentBrokenBy != -1) {
          reader.error(indentBrokenBy) << "indent broken by multiline comment";
          parseError();
          return;
        }
        if (!parseIndent(currentIndent)) {
          parseError();
          return; // error occured
        }
        isIndentParsing = false;
        continue;
      }
    }

    optional<Brace> brace;
    if ((brace = Traits::parseBrace(*this)).has_value()) {
      if (brace->token.type == TOK::PARSER_ERROR) {
        parseError();
        return;
      }
      if (brace->closingBrace) {
        if (!braces.empty() && braces.back() == brace->closingBrace) {
          braces.pop_back();
        } else {
          reader.error() << "Unexpected closing brace `" << brace->token.input
                         << "`";
        }
      }
      if (brace->openingBrace) {
        braces.push_back(brace->token);
      }
      data.push_back(move(brace->token));
      continue;
    }

    if ((token = Traits::parsePlainSymbol(*this)).has_value()) {
      if (token->type == TOK::PARSER_ERROR) {
        parseError();
        return;
      }
      data.push_back(move(*token));
      continue;
    }

    if ((token = sequence(is_digit<Encoding>)).has_value()) {
      token->type = TOK::INTEGER_LITERAL;
      u8string encodedStr = Encoding::encode(token->input);
      const char8_t *beginPtr = encodedStr.begin();
      char8_t *endPtr = nullptr;
      token->value = strtoimax(reinterpret_cast<const char *>(beginPtr), reinterpret_cast<char **>(&endPtr), 0);
      if (errno) {
        reader.error(token->pos) << "Error while parse an integer: " << std::strerror(errno);
      } else if (endPtr != encodedStr.end()) {
        reader.error(token->pos + (endPtr - beginPtr)) << "Unexpected char in integer literal";
      }
      data.push_back(*move(token));
      continue;
    }

    if (isalpha(c) != 0) {
      token = sequence(is_alnum<Encoding>);
      assert(token.has_value());

      Container lower = Container(token->input);
      lower.make_lower();
      if (Traits::isKeyword(lower)) {
        token->type = TOK::KEYWORD;
        token->value = lower;
      } else {
        token->type = TOK::IDENTIFIER;
        token->value = Container(token->input);
      }

      data.push_back(*move(token));
      continue;
    }

    reader.error() << "Unexpected char `" << c << "`";
    reader.get();
  }

  if (!braces.empty()) {
    reader.error() << "Unexpected eof, expected close brace"
                   << reader.note(braces.back().pos) << "to match this `"
                   << braces.back().input << "`";
  }

  // last one is a margin empty string ""
  while (indents.size() != 1) {
    emplaceDeindent();
  }

  if (errorCount()) {
    parseError();
  }
}

template<ParserType t>
void
LexerStruct<t>::removeIgnored()
{
  data.erase(remove_if(data.begin(), data.end(), Traits::ignore), data.end());
}

template<ParserType t>
LexerStruct<t>::Indent::Indent(Span input_, ssize_t pos_)
  : input(input_)
  , pos(pos_)
{}

template<ParserType t>
bool
LexerStruct<t>::emplaceIndent(Span currentIndent)
{
  data.emplace_back(TOK::INDENT, monostate(), reader.tellp(), Encoding::zeroString);
  indents.emplace_back(currentIndent, reader.tellp());
  return true;
}

template<ParserType t>
void
LexerStruct<t>::emplaceDeindent()
{
  data.emplace_back(TOK::DEINDENT, monostate(), reader.tellp(), Encoding::zeroString);
  indents.pop_back();
}

template<ParserType t>
bool
LexerStruct<t>::unexpectedIndent()
{
  assert(!indents.empty());
  reader.error() << "unexpected indent" << reader.note(indents.back().pos)
                 << "to match this indent";
  return false;
}

template<ParserType t>
bool
LexerStruct<t>::parseIndent(Span currentIndent)
{
  // ignore empty lines: \n after spaces
  if (reader.check(Triplet('\n'))) {
    return true;
  }

  // Real line, don't ignore
  // We always have a margin indent empty string ""
  Span lastIndent = indents.back().input;

  // indent
  if (currentIndent.size() > lastIndent.size()) {
    if (currentIndent.substr(0, lastIndent.size()) != lastIndent) {
      return unexpectedIndent();
    }

    return emplaceIndent(currentIndent);
  }

  // deindent
  for (auto it = indents.rbegin(); it != indents.rend(); ++it) {
    // We always have empty indent. So correct deindent have to be parsed
    // here.
    if (it->input == currentIndent) {
      ssize_t count = it - indents.rbegin();
      for (ssize_t i = 0; i < count; ++i) {
        emplaceDeindent();
      }

      return true;
    }

    if (it->input.size() < currentIndent.size() ||
        it->input.substr(0, currentIndent.size()) != currentIndent) {
      return unexpectedIndent();
    }
  }

  return unexpectedIndent();
}

template<ParserType t>
const typename LexerStruct<t>::Token&
LexerStruct<t>::operator[](ssize_t index) const
{
  return data[index];
}

template<ParserType t>
typename Reader<LexerStruct<t>::encoding>::ReaderError
LexerStruct<t>::error(ssize_t index)
{
  return reader.error(data[index].pos);
}

template<ParserType t>
typename Reader<LexerStruct<t>::encoding>::ReaderNote
LexerStruct<t>::note(ssize_t index) const
{
  return reader.note(data[index].pos);
}

#define DECLARE_PARSER(NAME)                                                   \
  template struct TokenRecord<ParserType::NAME>;                               \
  template struct BraceRecord<ParserType::NAME>;                               \
  template struct LexerStruct<ParserType::NAME>;                               \
  template std::basic_ostream<char, std::char_traits<char>>& operator<<(       \
    std::basic_ostream<char, std::char_traits<char>>& out,                     \
    const TokenRecord<ParserType::NAME>& token);
#include "parser_types.h"
#undef DECLARE_PARSER

}
