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

namespace extend::textcode {


TEST_CASE("Print dcl", "[textcode::ast]")
{
  Prog prog;

  prog.appendDcl(move(Dcl(Dcl::Type::INT, U"integer_key")));
  prog.appendDcl(move(Dcl(Dcl::Type::CHAR, U"char_key")));

  REQUIRE(prog.exportCpp() ==
    U"int integer_key;\nchar char_key;\n");
}

TEST_CASE("Print array", "[textcode::ast]")
{
  Prog prog;

  constexpr intmax_t indeces[] = {1, 3, 5};
  prog.appendDcl(move(Dcl(Dcl::Type::INT, U"array", indeces)));

  REQUIRE(prog.exportCpp() ==
    U"int array[1][3][5];\n");
}

TEST_CASE("Print func", "[textcode::ast]")
{
  Prog prog;

  prog.appendFunc(move(Func(Dcl::Type::INT, U"f1", {})));
  const ParmType parameters[] = {ParmType(Dcl::Type::INT, U"a", 3), ParmType(Dcl::Type::INT, U"b", 0)};
  prog.appendFunc(move(Func(Dcl::Type::INT, U"f2", parameters)));

  REQUIRE(prog.exportCpp() ==
    U"int f1(void) {\n}\nint f2(int a[][][], int b) {\n}\n");
}

}
