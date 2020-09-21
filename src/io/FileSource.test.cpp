/* extend - file format and program language
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

#include "FileSource.h"

#include <catch2/catch.hpp>
#include <tests/mocks.h>

namespace extend::io {
TEST_CASE("Stores a string", "[filesource]")
{
  auto source = FileSource<io::EncodingType::US_ASCII>::memory(u8"hello");
  REQUIRE(source.filename == u8"<memory>");
  REQUIRE(source.data == u8"hello");
}

TEST_CASE("Fatal on non US ASCII symbols", "[filesource]")
{
  tests::FatalMessageMock fatalRequired;
  auto source = FileSource<io::EncodingType::US_ASCII>::memory(u8"\xff");
}

TEST_CASE("Decode utf-8", "[filesource]")
{
  auto source = FileSource<io::EncodingType::UTF8>::memory(u8"Привет, мир!");
  REQUIRE(source.data == U"Привет, мир!");
}

TEST_CASE("Fatal on ill-formed utf-8", "[filesource]")
{
  tests::FatalMessageMock fatalRequired;
  auto source = FileSource<io::EncodingType::UTF8>::memory(u8"\xff");
}

TEST_CASE("Replace newlines", "[filesource]")
{
  auto source =
    FileSource<io::EncodingType::US_ASCII>::memory(u8"h\ne\r\nl\n\rl\n\no\n");
  REQUIRE(source.filename == u8"<memory>");
  REQUIRE(source.data == u8"h\ne\nl\nl\n\no\n");
}
}
