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

namespace extend::textdata {

using namespace io;
using Encoding = Parser::Encoding;
using Container = Parser::Container;
using Span = Parser::Span;

TEST_CASE("Parse assignment of int", "[textdata::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"val = 123"));

  REQUIRE(parser.doc().exportCpp() == U"int32_t val = 123;\n");
}

TEST_CASE("Parse assignment of string", "[textdata::parser]")
{
  Parser parser(FileSource<Parser::encoding>::memory(u8"val = \"hello\""));

  REQUIRE(parser.doc().exportCpp() == U"eastl::u8string val = u8\"hello\";\n");
}

TEST_CASE("Error on wrong format", "[textdata::parser]")
{
  {
    tests::FatalMessageMock fatalRequired;
    Parser parser(FileSource<Parser::encoding>::memory(u8"val ="));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Parser parser(FileSource<Parser::encoding>::memory(u8"val = 123 123"));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Parser parser(FileSource<Parser::encoding>::memory(u8"val"));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Parser parser(FileSource<Parser::encoding>::memory(u8"val val"));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Parser parser(FileSource<Parser::encoding>::memory(u8"= 123"));
  }
  {
    tests::FatalMessageMock fatalRequired;
    Parser parser(FileSource<Parser::encoding>::memory(u8"val = 123\nval = 123"));
  }
}

}
