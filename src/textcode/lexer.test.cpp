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

#include "lexer.h"
#include "tests/mocks.h"
#include <catch2/catch.hpp>

namespace extend::textcode {

using namespace text;
using namespace io;

using Lexer = LexerStruct<ParserType::TEXTCODE>;
using Token = Lexer::Token;
using Encoding = Lexer::Encoding;
using Triplet = Encoding::Triplet;
using Container = Encoding::Container;

TEST_CASE("Equal tokens in code", "[textcode::lexer]")
{
  Token lhs(TOK::SYMBOL, Triplet('+'));
  Token rhs(TOK::SYMBOL, Triplet('+'), 0, Encoding::Span(U"+"));

  REQUIRE(lhs == rhs);

  Token minus(TOK::SYMBOL, Triplet('-'));
  REQUIRE(lhs != minus);

  Token number(TOK::INTEGER_LITERAL, 10L);
  REQUIRE(lhs != number);
}

TEST_CASE("Parse dot in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"a . b"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::IDENTIFIER, Container(U"a")),
                                           Token(TOK::SPACE, Container(U" ")),
                                           Token(TOK::SYMBOL, Triplet('.')),
                                           Token(TOK::SPACE, Container(U" ")),
                                           Token(TOK::IDENTIFIER, Container(U"b")) });
}

TEST_CASE("Parse empty string in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8""));

  REQUIRE(lexer.tokens().empty());
}

TEST_CASE("Parse spaces in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  \t"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SPACE, Container(U"  \t")) });
}

TEST_CASE("Parse newline in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"\n"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SYMBOL, Triplet('\n')) });
}

TEST_CASE("Parse identifier in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"abacaba abacaba2"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::IDENTIFIER, Container(U"abacaba")),
                         Token(TOK::SPACE, Container(U" ")),
                         Token(TOK::IDENTIFIER, Container(U"abacaba2")) });
}

TEST_CASE("Parse operators in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"+ - * / = . > < ! <= >= == != && ||"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{
            Token(TOK::SYMBOL, Triplet('+')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('-')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('*')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('/')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('=')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('.')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('>')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('<')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('!')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('<', '=')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('>', '=')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('=', '=')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('!', '=')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('&', '&')),
            Token(TOK::SPACE, Container(U" ")),
            Token(TOK::SYMBOL, Triplet('|', '|')),
          });
}

TEST_CASE("Parse integer in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"123"));

  REQUIRE(lexer.tokens() == vector<Token>{
                              Token(TOK::INTEGER_LITERAL, 123L),
                            });
}

TEST_CASE("Parse keywords in code", "[textcode::lexer]")
{
  for (const auto *kwd : {u8"char", u8"int", u8"void", u8"if", u8"else", u8"while", u8"for", u8"return"}) {
    REQUIRE(Lexer(FileSource<Lexer::encoding>::memory(kwd)).tokens() == vector<Token>{
      Token(TOK::KEYWORD, Container(Encoding::decode(u8"<memory>", kwd))),
    });
  }
}

TEST_CASE("Parse braces in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"()"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::SYMBOL, Triplet('(')), Token(TOK::SYMBOL, Triplet(')')) });
}

TEST_CASE("Error on unclosed braces in code", "[textcode::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"("));
}

TEST_CASE("Error on unopened braces in code", "[textcode::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8")"));
}

TEST_CASE("Parse comments in code", "[textcode::lexer]")
{
  Lexer lexer(
    FileSource<Lexer::encoding>::memory(u8"  // comment1\nabacaba\nabacaba2/* "
                             "comment3 */abacaba3\nabacaba4"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::COMMENT, Container(U"// comment1")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::IDENTIFIER, Container(U"abacaba")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                         Token(TOK::COMMENT, Container(U"/* comment3 */")),
                         Token(TOK::IDENTIFIER, Container(U"abacaba3")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::IDENTIFIER, Container(U"abacaba4"))
                       });
}

TEST_CASE("Error on python comments", "[textcode::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"# hello"));
}

TEST_CASE("Parse multiline comments in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(
    u8"  /* comment1 */  \n/* comment2\n  comment3 */  \n  /* comment4 */  /* comment 5 */"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::COMMENT, Container(U"/* comment1 */")),
                         Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::COMMENT, Container(U"/* comment2\n  comment3 */")),
                         Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::COMMENT, Container(U"/* comment4 */")),
                         Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::COMMENT, Container(U"/* comment 5 */"))
                       });
}

TEST_CASE("Error on unclosed comment in code", "[textcode::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"/* comment"));
}

TEST_CASE("Parse a plain string in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"("hello")"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::STRING, Container(U"hello")) });
}

TEST_CASE("Parse escape sequence in a string in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"("\t\n\\")"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::STRING, Container(U"\t\n\\")) });
}

TEST_CASE("Parse quotes in a string in code", "[textcode::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"\"\\\"'\""));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::STRING, Container(U"\"'")) });
}

TEST_CASE("Error on unclosed string in code", "[textcode::lexer]")
{
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"(")"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"("\)"));
  }
}

TEST_CASE("Error on unexpected escape sequence in code", "[textcode::lexer]")
{
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"(\d)"));
  }
}

TEST_CASE("Parse charcon", "[textcode::lexer]")
{
  {
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"('x')"));
    REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::INTEGER_LITERAL, intmax_t('x')) });
  }
  {
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"('\n')"));
    REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::INTEGER_LITERAL, intmax_t('\n')) });
  }
  {
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"('\0')"));
    REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::INTEGER_LITERAL, intmax_t(0)) });
  }
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"('he')"));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"('\h')"));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"('\'x)"));
  }
}
}
