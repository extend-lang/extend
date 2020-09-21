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

#include "reader.h"

#include <catch2/catch.hpp>

namespace extend::text {
using namespace extend::io;

TEST_CASE("Reader::get returns a chars", "[reader]")
{
  Reader<EncodingType::US_ASCII> reader(FileSource<EncodingType::US_ASCII>::memory(u8"hello"));
  using Triplet = Reader<EncodingType::US_ASCII>::Triplet;

  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == Triplet('h'));
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == Triplet('e'));
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == Triplet('l'));
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == Triplet('l'));
  REQUIRE(!reader.eof());
  REQUIRE(reader.get() == Triplet('o'));
  REQUIRE(reader.eof());
  REQUIRE(reader.get() == Reader<io::EncodingType::US_ASCII>::Triplet::eof);
}

TEST_CASE("Reader::get captures enters", "[reader]")
{
  Reader<EncodingType::US_ASCII> reader(FileSource<EncodingType::US_ASCII>::memory(u8"h\ne\rl\n\rl\r\no"));
  using Triplet = Reader<EncodingType::US_ASCII>::Triplet;

  REQUIRE(reader.get() == Triplet('h'));
  REQUIRE(reader.get() == Triplet('\n'));
  REQUIRE(reader.get() == Triplet('e'));
  REQUIRE(reader.get() == Triplet('\n'));
  REQUIRE(reader.get() == Triplet('l'));
  REQUIRE(reader.get() == Triplet('\n'));
  REQUIRE(reader.get() == Triplet('l'));
  REQUIRE(reader.get() == Triplet('\n'));
  REQUIRE(reader.get() == Triplet('o'));
  REQUIRE(reader.get() == Reader<EncodingType::US_ASCII>::Triplet::eof);
}
}
