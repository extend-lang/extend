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
 *
 * You should have received a copy of the GNU Affero General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "lexer.h"
#include "text/LexerStruct.h"
#include "text/ParserTraits.h"

#include <unordered_map.hpp>

namespace extend::text {

using Encoding = io::EncodingTraits<ParserTraits<ParserType::TEXTDATA>::encoding>;
using Triplet = Encoding::Triplet;
using Span = Encoding::Span;
using Container = Encoding::Container;

template<>
const char*
ParserTraits<ParserType::TEXTDATA>::typeAsStr(TOK ttype)
{
  switch (ttype) {
#define DECLARE_TOKEN(NAME, VALUE_TYPE)                                        \
  case TOK::NAME:                                                              \
    return #NAME;
#include "token_types.h"
#undef DECLARE_TOKEN
  }
}

template<>
bool
ParserTraits<ParserType::TEXTDATA>::isKeyword(Span word)
{
  using Span = Span;
  static const ska::unordered_set<Span, hash<Span>> keywords = {
    Span(U"i8"),
    Span(U"i16"),
    Span(U"i32"),
    Span(U"i64"),
    Span(U"u8"),
    Span(U"u16"),
    Span(U"u32"),
    Span(U"u64"),
    Span(U"f16"),
    Span(U"f32"),
    Span(U"f64"),
    Span(U"str"),
    Span(U"template"),
  };
  return keywords.find(word) != keywords.end();
}

template<>
optional<TokenRecord<ParserType::TEXTDATA>>
ParserTraits<ParserType::TEXTDATA>::parseComment(
  LexerStruct<ParserType::TEXTDATA>& lexer)
{
  ssize_t pos = lexer.r().tellp();

  if (lexer.r().check(Triplet('/', '*'))) {
    auto token = lexer.until<true, 2, true>(Triplet('*', '/'));
    if (!token.has_value()) {
      lexer.r().error() << "unexpected eof, expected `*/`"
                        << lexer.r().note(pos) << "to match this `/*`";
      return Token(TOK::PARSER_ERROR, monostate());
    }
    token->type = TOK::COMMENT;
    token->value = Container(token->input);
    return token;
  }

  if (lexer.r().check(Triplet('#')) || lexer.r().check(Triplet('/', '/'))) {
    auto token = lexer.until(Triplet('\n'));
    assert(token.has_value());
    token->type = TOK::COMMENT;
    token->value = Container(token->input);
    return token;
  }

  return nullopt;
}

template<>
optional<LexerStruct<ParserType::TEXTDATA>::Token>
ParserTraits<ParserType::TEXTDATA>::parsePlainSymbol(
  LexerStruct<ParserType::TEXTDATA>& lexer)
{
  (void)lexer;
  ssize_t pos = lexer.r().tellp();

  if (Span(U".=").find(lexer.r().peek()[0]) != Span::npos) {
    auto token =
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1));
    return token;
  }

  if (lexer.r().check(Triplet('"'))) {
    lexer.r().get();
    Container stringLiteral;
    while (!lexer.r().check(Triplet('"'))) {
      auto c = lexer.r().get()[0];
      if (lexer.r().eof()) {
        lexer.r().error()
          << "Unexpected eof, expected closing quotes for a string"
          << lexer.r().note(pos) << "the string started here";
        return Token(TOK::PARSER_ERROR, monostate());
      }
      if (c == '\\') {
        c = lexer.r().get()[0];
        switch (c) {
          case 't':
            c = '\t';
            break;
          case 'n':
            c = '\n';
          case '"':
          case '\\':
            break;
          default:
            lexer.r().error(lexer.r().tellp() - 2)
              << "Unexpected escape sequence `"
              << lexer.r().substr(lexer.r().tellp() - 2, 2) << "`";
            return Token(TOK::PARSER_ERROR, monostate());
        }
      }
      stringLiteral.push_back(c);
    }
    lexer.r().get();

    return Token(
      TOK::STRING, stringLiteral, pos, lexer.r().substr(pos, lexer.r().tellp() - pos));
  }

  return nullopt;
}

template<>
optional<BraceRecord<ParserType::TEXTDATA>>
ParserTraits<ParserType::TEXTDATA>::parseBrace(
  LexerStruct<ParserType::TEXTDATA>& lexer)
{
  Encoding::CodePoint c = lexer.r().peek()[0];
  ssize_t pos = lexer.r().tellp();

  if (c == '(') {
    return Brace(
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1)),
      true,
      nullopt);
  }
  if (c == ')') {
    return Brace(
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1)),
      false,
      Token(TOK::SYMBOL, Triplet('(')));
  }
  if (c == '[') {
    return Brace(
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1)),
      true,
      nullopt);
  }
  if (c == ']') {
    return Brace(
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1)),
      false,
      Token(TOK::SYMBOL, Triplet('[')));
  }

  return nullopt;
}

template<>
bool
ParserTraits<ParserType::TEXTDATA>::ignore(const Token& tok)
{
  return tok.type == TOK::SPACE || tok.type == TOK::COMMENT;
}

}
