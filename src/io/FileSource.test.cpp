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

#include "FileSource.h"

#include <catch2/catch.hpp>

namespace extend::io {
TEST_CASE("FileSource stores a string", "[filesource]")
{
  FileSource source = FileSource::memory("hello");
  REQUIRE(source.filename == "<memory>");
  REQUIRE(source.data == "hello");
}

TEST_CASE("FileSource replace newlines", "[filesource]")
{
  FileSource source = FileSource::memory("h\ne\r\nl\n\rl\n\no\n");
  REQUIRE(source.filename == "<memory>");
  REQUIRE(source.data == "h\ne\nl\nl\n\no\n");
}
}
