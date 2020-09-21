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

#include "lexer.h"
#include <catch2/catch.hpp>

namespace extend::textdata {
TEST_CASE("Equal tokens", "[lexer]")
{
  Token lhs(SYMBOL, '+');
  Token rhs(SYMBOL, '+', 0, "+");

  REQUIRE(lhs == rhs);

  Token minus(SYMBOL, '-');
  REQUIRE(lhs != minus);

  Token number(I32, 10);
  REQUIRE(lhs != number);
}

TEST_CASE("Parse sum", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("123 + 256"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{ Token(I32, 123),
                                              Token(SPACE, " "),
                                              Token(SYMBOL, '+'),
                                              Token(SPACE, " "),
                                              Token(I32, 256) });
}

TEST_CASE("Parse empty string", "[lexer]")
{
  Lexer lexer(io::FileSource::memory(""));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data.empty());
}

TEST_CASE("Parse spaces", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("  \t"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{ Token(SPACE, "  \t") });
}

TEST_CASE("Parse newline", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("\n"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{ Token(SYMBOL, '\n') });
}

TEST_CASE("Parse identifier", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("abacaba abacaba2"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{ Token(IDENTIFIER, "abacaba"),
                                              Token(SPACE, " "),
                                              Token(IDENTIFIER, "abacaba2") });
}

TEST_CASE("Parse operators", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("+-*/"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{ Token(SYMBOL, '+'),
                                              Token(SYMBOL, '-'),
                                              Token(SYMBOL, '*'),
                                              Token(SYMBOL, '/') });
}

TEST_CASE("Parse integer", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("123"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{
                          Token(I32, 123),
                        });
}

TEST_CASE("Parse keywords", "[lexer]")
{
  for (const auto& word : KEYWORDS) {
    Lexer lexer(io::FileSource::memory(word));

    REQUIRE(lexer.reader.errors == 0);
    REQUIRE(lexer.data == vector<Token>{
                            Token(KEYWORD, word),
                          });
  }
}

TEST_CASE("Parse braces", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("()"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data ==
          vector<Token>{ Token(SYMBOL, '('), Token(SYMBOL, ')') });
}

TEST_CASE("Error on unclosed braces", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("("));

  REQUIRE(lexer.reader.errors == 1);
}

TEST_CASE("Error on unopened braces", "[lexer]")
{
  Lexer lexer(io::FileSource::memory(")"));

  REQUIRE(lexer.reader.errors == 1);
}

TEST_CASE("Parse indent in first line", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("  abacaba"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data ==
          vector<Token>{ Token(SPACE, "  "),
                                Token(INDENT, monostate()),
                                Token(IDENTIFIER, "abacaba"),
                                Token(DEINDENT, monostate()) });
}

TEST_CASE("Parse indents", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("abacaba\n  abacaba2"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{
                          Token(IDENTIFIER, "abacaba"),
                          Token(SYMBOL, '\n'),
                          Token(SPACE, "  "),
                          Token(INDENT, monostate()),
                          Token(IDENTIFIER, "abacaba2"),
                          Token(DEINDENT, monostate()),
                        });
}

TEST_CASE("Parse nested indents", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("abacaba\n  abacaba2\n    abacaba3"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{
                          Token(IDENTIFIER, "abacaba"),
                          Token(SYMBOL, '\n'),
                          Token(SPACE, "  "),
                          Token(INDENT, monostate()),
                          Token(IDENTIFIER, "abacaba2"),
                          Token(SYMBOL, '\n'),
                          Token(SPACE, "    "),
                          Token(INDENT, monostate()),
                          Token(IDENTIFIER, "abacaba3"),
                          Token(DEINDENT, monostate()),
                          Token(DEINDENT, monostate()),
                        });
}

TEST_CASE("Match deindents", "[lexer]")
{
  Lexer lexer(
    io::FileSource::memory("abacaba\n  abacaba2\n    abacaba3\n  abacaba4"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data ==
          vector<Token>{ Token(IDENTIFIER, "abacaba"),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "  "),
                                Token(INDENT, monostate()),
                                Token(IDENTIFIER, "abacaba2"),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "    "),
                                Token(INDENT, monostate()),
                                Token(IDENTIFIER, "abacaba3"),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "  "),
                                Token(DEINDENT, monostate()),
                                Token(IDENTIFIER, "abacaba4"),
                                Token(DEINDENT, monostate()) });
}

TEST_CASE("Dont parse indents in braces", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("(abacaba\n  abacaba2)"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{
                          Token(SYMBOL, '('),
                          Token(IDENTIFIER, "abacaba"),
                          Token(SYMBOL, '\n'),
                          Token(SPACE, "  "),
                          Token(IDENTIFIER, "abacaba2"),
                          Token(SYMBOL, ')'),
                        });
}

TEST_CASE("Indent ignore empty lines", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("  abacaba\n\n    \n\t\n  abacaba2"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data ==
          vector<Token>{ Token(SPACE, "  "),
                                Token(INDENT, monostate()),
                                Token(IDENTIFIER, "abacaba"),
                                Token(SYMBOL, '\n'),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "    "),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "\t"),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "  "),
                                Token(IDENTIFIER, "abacaba2"),
                                Token(DEINDENT, monostate()) });
}

TEST_CASE("Error on wrong deindent", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("    abacaba\n  abacaba2"));
  REQUIRE(lexer.reader.errors == 1);

  Lexer lexer2(io::FileSource::memory("  abacaba\n    abacaba2\n   abacaba3"));
  REQUIRE(lexer2.reader.errors == 1);

  Lexer lexer3(io::FileSource::memory("  abacaba\n\tabacaba2"));
  REQUIRE(lexer3.reader.errors == 1);
}

TEST_CASE("Parse comments", "[lexer]")
{
  Lexer lexer(
    io::FileSource::memory("  // comment1\nabacaba\n# comment2\nabacaba2/* "
                           "comment3 */abacaba3\nabacaba4"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data == vector<Token>{ Token(SPACE, "  "),
                                              Token(COMMENT, "// comment1"),
                                              Token(SYMBOL, '\n'),
                                              Token(IDENTIFIER, "abacaba"),
                                              Token(SYMBOL, '\n'),
                                              Token(COMMENT, "# comment2"),
                                              Token(SYMBOL, '\n'),
                                              Token(IDENTIFIER, "abacaba2"),
                                              Token(COMMENT, "/* comment3 */"),
                                              Token(IDENTIFIER, "abacaba3"),
                                              Token(SYMBOL, '\n'),
                                              Token(IDENTIFIER, "abacaba4") });
}
TEST_CASE("Parse multiline comments", "[lexer]")
{
  Lexer lexer(
    io::FileSource::memory("  /* comment1 */  \n/* comment2\n  comment3 */  \n "
                           " /* comment4 */  /* comment 5 */"));

  REQUIRE(lexer.reader.errors == 0);
  REQUIRE(lexer.data ==
          vector<Token>{ Token(SPACE, "  "),
                                Token(COMMENT, "/* comment1 */"),
                                Token(SPACE, "  "),
                                Token(SYMBOL, '\n'),
                                Token(COMMENT, "/* comment2\n  comment3 */"),
                                Token(SPACE, "  "),
                                Token(SYMBOL, '\n'),
                                Token(SPACE, "  "),
                                Token(COMMENT, "/* comment4 */"),
                                Token(SPACE, "  "),
                                Token(COMMENT, "/* comment 5 */") });
}

TEST_CASE("Error on unclosed comment", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("/* comment"));
  REQUIRE(lexer.reader.errors == 1);
}

TEST_CASE("Error on broken indents by comment", "[lexer]")
{
  Lexer lexer(io::FileSource::memory("  /* comment */  abacaba"));
  REQUIRE(lexer.reader.errors == 1);

  Lexer lexer2(io::FileSource::memory("  /* comment */abacaba"));
  REQUIRE(lexer2.reader.errors == 1);

  Lexer lexer3(io::FileSource::memory("/* comment */  abacaba"));
  REQUIRE(lexer3.reader.errors == 1);

  Lexer lexer4(io::FileSource::memory("/* comment\n*/abacaba"));
  REQUIRE(lexer4.reader.errors == 1);

  Lexer lexer5(io::FileSource::memory("/* comment\n*/  abacaba"));
  REQUIRE(lexer5.reader.errors == 1);
}
}
