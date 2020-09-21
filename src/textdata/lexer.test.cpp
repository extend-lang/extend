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

namespace extend::textdata {

using namespace text;
using namespace io;

using Lexer = LexerStruct<ParserType::TEXTDATA>;
using Token = Lexer::Token;
using Encoding = Lexer::Encoding;
using Triplet = Encoding::Triplet;
using Container = Encoding::Container;

TEST_CASE("Equal tokens", "[textdata::lexer]")
{
  Token lhs(TOK::SYMBOL, Triplet('+'));
  Token rhs(TOK::SYMBOL, Triplet('+'), 0, Encoding::Span(U"+"));

  REQUIRE(lhs == rhs);

  Token minus(TOK::SYMBOL, Triplet('-'));
  REQUIRE(lhs != minus);

  Token number(TOK::INTEGER_LITERAL, 10L);
  REQUIRE(lhs != number);
}

TEST_CASE("Parse dot", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"a . b"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::IDENTIFIER, Container(U"a")),
                                           Token(TOK::SPACE, Container(U" ")),
                                           Token(TOK::SYMBOL, Triplet('.')),
                                           Token(TOK::SPACE, Container(U" ")),
                                           Token(TOK::IDENTIFIER, Container(U"b")) });
}

TEST_CASE("Parse empty string", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8""));

  REQUIRE(lexer.tokens().empty());
}

TEST_CASE("Parse spaces", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  \t"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SPACE, Container(U"  \t")) });
}

TEST_CASE("Parse newline", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"\n"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SYMBOL, Triplet('\n')) });
}

TEST_CASE("Parse identifier", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"abacaba abacaba2"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::IDENTIFIER, Container(U"abacaba")),
                         Token(TOK::SPACE, Container(U" ")),
                         Token(TOK::IDENTIFIER, Container(U"abacaba2")) });
}

TEST_CASE("Parse operators", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8".="));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::SYMBOL, Triplet('.')), Token(TOK::SYMBOL, Triplet('=')) });
}

TEST_CASE("Parse integer", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"123"));

  REQUIRE(lexer.tokens() == vector<Token>{
                              Token(TOK::INTEGER_LITERAL, 123L),
                            });
}

TEST_CASE("Parse keywords", "[textdata::lexer]")
{
    REQUIRE(Lexer(FileSource<Lexer::encoding>::memory(u8"i32")).tokens() == vector<Token>{
                                Token(TOK::KEYWORD, Container(U"i32")),
                              });
}

TEST_CASE("Parse braces", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"()"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::SYMBOL, Triplet('(')), Token(TOK::SYMBOL, Triplet(')')) });
}

TEST_CASE("Error on unclosed braces", "[textdata::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"("));
}

TEST_CASE("Error on unopened braces", "[textdata::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8")"));
}

TEST_CASE("Parse indent in first line", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  abacaba"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::INDENT, monostate()),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba")),
                                           Token(TOK::DEINDENT, monostate()) });
}

TEST_CASE("Parse indent with eof", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  abacaba\n  "));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::INDENT, monostate()),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::DEINDENT, monostate()) });

  Lexer lexer2(FileSource<Lexer::encoding>::memory(u8"  abacaba\n"));

  REQUIRE(lexer2.tokens() ==
          vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::INDENT, monostate()),
                         Token(TOK::IDENTIFIER, Container(U"abacaba")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::DEINDENT, monostate()) });
}

TEST_CASE("Parse indents", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"abacaba\n  abacaba2"));

  REQUIRE(lexer.tokens() == vector<Token>{
                              Token(TOK::IDENTIFIER, Container(U"abacaba")),
                              Token(TOK::SYMBOL, Triplet('\n')),
                              Token(TOK::SPACE, Container(U"  ")),
                              Token(TOK::INDENT, monostate()),
                              Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                              Token(TOK::DEINDENT, monostate()),
                            });
}

TEST_CASE("Parse nested indents", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"abacaba\n  abacaba2\n    abacaba3"));

  REQUIRE(lexer.tokens() == vector<Token>{
                              Token(TOK::IDENTIFIER, Container(U"abacaba")),
                              Token(TOK::SYMBOL, Triplet('\n')),
                              Token(TOK::SPACE, Container(U"  ")),
                              Token(TOK::INDENT, monostate()),
                              Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                              Token(TOK::SYMBOL, Triplet('\n')),
                              Token(TOK::SPACE, Container(U"    ")),
                              Token(TOK::INDENT, monostate()),
                              Token(TOK::IDENTIFIER, Container(U"abacaba3")),
                              Token(TOK::DEINDENT, monostate()),
                              Token(TOK::DEINDENT, monostate()),
                            });
}

TEST_CASE("Match deindents", "[textdata::lexer]")
{
  Lexer lexer(
    FileSource<Lexer::encoding>::memory(u8"abacaba\n  abacaba2\n    abacaba3\n  abacaba4"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::IDENTIFIER, Container(U"abacaba")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::INDENT, monostate()),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"    ")),
                                           Token(TOK::INDENT, monostate()),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba3")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::DEINDENT, monostate()),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba4")),
                                           Token(TOK::DEINDENT, monostate()) });
}

TEST_CASE("Dont parse indents in braces", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"(abacaba\n  abacaba2)"));

  REQUIRE(lexer.tokens() == vector<Token>{
                              Token(TOK::SYMBOL, Triplet('(')),
                              Token(TOK::IDENTIFIER, Container(U"abacaba")),
                              Token(TOK::SYMBOL, Triplet('\n')),
                              Token(TOK::SPACE, Container(U"  ")),
                              Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                              Token(TOK::SYMBOL, Triplet(')')),
                            });
}

TEST_CASE("Indent ignore empty lines", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  abacaba\n\n    \n\t\n  abacaba2"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::INDENT, monostate()),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"    ")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"\t")),
                                           Token(TOK::SYMBOL, Triplet('\n')),
                                           Token(TOK::SPACE, Container(U"  ")),
                                           Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                                           Token(TOK::DEINDENT, monostate()) });
}

TEST_CASE("Error on wrong deindent", "[textdata::lexer]")
{
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"    abacaba\n  abacaba2"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(
      FileSource<Lexer::encoding>::memory(u8"  abacaba\n    abacaba2\n   abacaba3"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  abacaba\n\tabacaba2"));
  }
}

TEST_CASE("Parse comments", "[textdata::lexer]")
{
  Lexer lexer(
    FileSource<Lexer::encoding>::memory(u8"  // comment1\nabacaba\n# comment2\nabacaba2/* "
                             "comment3 */abacaba3\nabacaba4"));

  REQUIRE(lexer.tokens() ==
          vector<Token>{ Token(TOK::SPACE, Container(U"  ")),
                         Token(TOK::COMMENT, Container(U"// comment1")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::IDENTIFIER, Container(U"abacaba")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::COMMENT, Container(U"# comment2")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::IDENTIFIER, Container(U"abacaba2")),
                         Token(TOK::COMMENT, Container(U"/* comment3 */")),
                         Token(TOK::IDENTIFIER, Container(U"abacaba3")),
                         Token(TOK::SYMBOL, Triplet('\n')),
                         Token(TOK::IDENTIFIER, Container(U"abacaba4"))
                       });
}
TEST_CASE("Parse multiline comments", "[textdata::lexer]")
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

TEST_CASE("Error on unclosed comment", "[textdata::lexer]")
{
  tests::FatalMessageMock fatalRequired;
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"/* comment"));
}

TEST_CASE("Error on broken indents by comment", "[textdata::lexer]")
{
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  /* comment */  abacaba"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"  /* comment */abacaba"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"/* comment */  abacaba"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"/* comment\n*/abacaba"));
  }

  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8"/* comment\n*/  abacaba"));
  }
}

TEST_CASE("Parse a plain string", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"("hello")"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::STRING, Container(U"hello")) });
}

TEST_CASE("Parse escape sequence in a string", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"("\t\n\\")"));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::STRING, Container(U"\t\n\\")) });
}

TEST_CASE("Parse quotes in a string", "[textdata::lexer]")
{
  Lexer lexer(FileSource<Lexer::encoding>::memory(u8"\"\\\"'\""));

  REQUIRE(lexer.tokens() == vector<Token>{ Token(TOK::STRING, Container(U"\"'")) });
}

TEST_CASE("Error on unclosed string", "[textdata::lexer]")
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

TEST_CASE("Error on unexpected escape sequence", "[textdata::lexer]")
{
  {
    tests::FatalMessageMock fatalRequired;
    Lexer lexer(FileSource<Lexer::encoding>::memory(u8R"(\d)"));
  }
}
}
