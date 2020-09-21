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

#include "ast.h"
#include <catch2/catch.hpp>

namespace extend::textdata {


TEST_CASE("Print simple document", "[textdata::ast]")
{
  Document doc;

  doc.appendField(Field::Type::I32, U"integer_key", 123L);
  doc.appendField(Field::Type::STRING, U"message", U"Hello, World");

  REQUIRE(doc.exportCpp() ==
    U"int32_t integer_key = 123;\neastl::u8string message = u8\"Hello, World\";\n");
}

}
