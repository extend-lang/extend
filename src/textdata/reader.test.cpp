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

#include "reader.h"

#include <catch2/catch.hpp>

namespace extend::textdata {
TEST_CASE("Reader::get returns a chars", "[reader]")
{
  textdata::Reader reader(io::FileSource::memory("hello"));

  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == 'h');
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == 'e');
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == 'l');
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == 'l');
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == 'o');
  REQUIRE(reader.eof());
  REQUIRE(reader.get() == reader.eof_value);
}

TEST_CASE("Reader::get captures enters", "[reader]")
{
  textdata::Reader reader(io::FileSource::memory("h\ne\rl\n\rl\r\no"));

  REQUIRE(reader.get() == 'h');
  REQUIRE(reader.get() == '\n');
  REQUIRE(reader.get() == 'e');
  REQUIRE(reader.get() == '\n');
  REQUIRE(reader.get() == 'l');
  REQUIRE(reader.get() == '\n');
  REQUIRE(reader.get() == 'l');
  REQUIRE(reader.get() == '\n');
  REQUIRE(reader.get() == 'o');
  REQUIRE(reader.get() == reader.eof_value);
}
}
