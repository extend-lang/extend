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

using Encoding = io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>;
using Triplet = Encoding::Triplet;
using Span = Encoding::Span;
using Container = Encoding::Container;

template<>
const char*
ParserTraits<ParserType::TEXTCODE>::typeAsStr(TOK ttype)
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
ParserTraits<ParserType::TEXTCODE>::isKeyword(Span word)
{
  using Span = Span;
  static const ska::unordered_set<Span, hash<Span>> keywords = {
    Span(U"int"),
    Span(U"char"),
    Span(U"void"),
    Span(U"for"),
    Span(U"while"),
    Span(U"return"),
    Span(U"if"),
    Span(U"else"),
  };
  return keywords.find(word) != keywords.end();
}

template<>
optional<TokenRecord<ParserType::TEXTCODE>>
ParserTraits<ParserType::TEXTCODE>::parseComment(
  LexerStruct<ParserType::TEXTCODE>& lexer)
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

  if (lexer.r().check(Triplet('/', '/'))) {
    auto token = lexer.until(Triplet('\n'));
    assert(token.has_value());
    token->type = TOK::COMMENT;
    token->value = Container(token->input);
    return token;
  }

  return nullopt;
}

template<>
optional<LexerStruct<ParserType::TEXTCODE>::Token>
ParserTraits<ParserType::TEXTCODE>::parsePlainSymbol(
  LexerStruct<ParserType::TEXTCODE>& lexer)
{
  (void)lexer;
  ssize_t pos = lexer.r().tellp();

  static ska::unordered_set<Triplet, hash<Triplet>> two_char_symbol {
    Triplet('<', '='),
    Triplet('>', '='),
    Triplet('=', '='),
    Triplet('!', '='),
    Triplet('&', '&'),
    Triplet('|', '|'),
  };
  if (two_char_symbol.find(lexer.r().peek(2)) != two_char_symbol.end()) {
    return Token(TOK::SYMBOL, lexer.r().get(2), pos, lexer.r().substr(pos, 2));
  }

  static ska::unordered_set<Triplet, hash<Triplet>> one_char_symbol {
    Triplet('.'),
    Triplet('='),
    Triplet('>'),
    Triplet('<'),
    Triplet('+'),
    Triplet('-'),
    Triplet('*'),
    Triplet('/'),
    Triplet('!'),
    Triplet(';'),
    Triplet(','),
  };

  if (one_char_symbol.find(lexer.r().peek()) != one_char_symbol.end()) {
    return Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1));
  }

  if (lexer.r().check(Triplet('\''))) {
    lexer.r().get(); // consume begin '

    if (lexer.r().check(Triplet('\\', '\'', '\''))) {
      lexer.r().get(3);
      return Token(
        TOK::INTEGER_LITERAL, intmax_t('\''), pos, lexer.r().substr(pos, lexer.r().tellp() - pos));
    }
    if (lexer.r().check(Triplet('\\', 'n', '\''))) {
      lexer.r().get(3);
      return Token(
        TOK::INTEGER_LITERAL, intmax_t('\n'), pos, lexer.r().substr(pos, lexer.r().tellp() - pos));
    }
    if (lexer.r().check(Triplet('\\', 'r', '\''))) {
      lexer.r().get(3);
      return Token(
        TOK::INTEGER_LITERAL, intmax_t('\r'), pos, lexer.r().substr(pos, lexer.r().tellp() - pos));
    }
    if (lexer.r().check(Triplet('\\', '0', '\''))) {
      lexer.r().get(3);
      return Token(
        TOK::INTEGER_LITERAL, intmax_t(0), pos, lexer.r().substr(pos, lexer.r().tellp() - pos));
    }
    if (lexer.r().check(Triplet('\\'))) {
      lexer.r().error(lexer.r().tellp())
        << "Unexpected escape sequence `"
        << lexer.r().substr(lexer.r().tellp(), 2) << "`";
      return Token(TOK::PARSER_ERROR, monostate());
    }

    intmax_t symbol = lexer.r().get()[0];

    if (!lexer.r().check(Triplet('\''))) {
      lexer.r().error(lexer.r().tellp())
        << "Unexpected char `"
        << lexer.r().substr(lexer.r().tellp(), 1) << "` expected end of charcon `'`";
      return Token(TOK::PARSER_ERROR, monostate());
    }
    lexer.r().get(); // consume end '

    return Token(
      TOK::INTEGER_LITERAL, symbol, pos, lexer.r().substr(pos, lexer.r().tellp() - pos));
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
          // TODO: add \0, have to check all using of str and view for char32_t *
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
optional<BraceRecord<ParserType::TEXTCODE>>
ParserTraits<ParserType::TEXTCODE>::parseBrace(
  LexerStruct<ParserType::TEXTCODE>& lexer)
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
  if (c == '{') {
    return Brace(
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1)),
      true,
      nullopt);
  }
  if (c == '}') {
    return Brace(
      Token(TOK::SYMBOL, lexer.r().get(), pos, lexer.r().substr(pos, 1)),
      false,
      Token(TOK::SYMBOL, Triplet('{')));
  }

  return nullopt;
}

template<>
bool
ParserTraits<ParserType::TEXTCODE>::ignore(const Token& tok)
{
  return tok.type == TOK::SPACE || tok.type == TOK::COMMENT ||
    (tok.type == TOK::SYMBOL && get<Triplet>(tok.value) == Triplet('\n'));
}

}
