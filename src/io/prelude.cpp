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

#include "prelude.h"

#include <sstream>
#include <cinttypes>
#include <cstddef>

void* __cdecl
operator new[](
  size_t size,
  const char* /*pName*/,
  int /*flags*/,
  unsigned /*debugFlags*/,
  const char* /*file*/,
  int /*line*/)
{
  return new uint8_t[size];
}

void* __cdecl
operator new[](
  size_t size,
  size_t /*alignment*/,
  size_t /*alignmentOffset*/,
  const char* /*pName*/,
  int /*flags*/,
  unsigned /*debugFlags*/,
  const char* /*file*/,
  int /*line*/)

{
  return new uint8_t[size];
}

std::stringstream&
eastl::operator>>(std::stringstream& input, eastl::string& str)
{
  std::streambuf* pbuf = input.rdbuf();
  std::streamsize size = pbuf->pubseekoff(0, input.end);
  pbuf->pubseekoff(0, input.beg); // rewind
  str.resize(size);
  pbuf->sgetn(str.data(), size);
  input.setstate(std::stringstream::eofbit);
  return input;
}
