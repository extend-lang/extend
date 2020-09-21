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

#include "parser.h"
#include <catch2/catch.hpp>
#include <tests/mocks.h>

namespace extend::textcode {

using namespace io;
using Encoding = Parser::Encoding;
using Container = Parser::Container;
using Span = Parser::Span;

TEST_CASE("Parse declare of int", "[textcode::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"int val;"));

  REQUIRE(parser.doc().exportCpp() == U"int val;\n");
}

TEST_CASE("Parse declare of several vars", "[textcode::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"int val, val2;"));

  REQUIRE(parser.doc().exportCpp() == U"int val;\nint val2;\n");
}

TEST_CASE("Parse declare of arrays", "[textcode::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"int val[15];"));

  REQUIRE(parser.doc().exportCpp() == U"int val[15];\n");
}

TEST_CASE("Parse functions", "[textcode::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"int f(void) {} void g(int a[], char b) { }"));

  REQUIRE(parser.doc().exportCpp() == U"int f(void) {\n}\nvoid g(int a[], char b) {\n}\n");
}

TEST_CASE("Parse functions with variables", "[textcode::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"int f(void) { char c; }"));

  REQUIRE(parser.doc().exportCpp() == U"int f(void) {\n  char c;\n}\n");
}

}
