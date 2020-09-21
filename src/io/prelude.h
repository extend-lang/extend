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

#pragma once

#include <EASTL/string.h>
#include <EASTL/string_view.h>
#include <EASTL/vector.h>
#include <llvm/Support/ConvertUTF.h>

#include <sstream>
#include <cassert>

#include "GetTypeName.h"
#include <type_traits>

namespace eastl {
template<typename Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& output, const eastl::basic_string_view<char> &str)
{
  output.write(str.data(), str.size());
  return output;
}

template<typename Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& output, const eastl::basic_string_view<char8_t> &str)
{
  output.write(reinterpret_cast<const char *>(str.data()), str.size());
  return output;
}

template<typename Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& output, const eastl::basic_string_view<char32_t> &str)
{
  eastl::vector<llvm::UTF8> storage(str.size() * 4);
  llvm::UTF8 *storage_ptr = storage.data();
  const auto *source_ptr = reinterpret_cast<const llvm::UTF32*>(str.data());
  llvm::ConversionResult errorCode = llvm::ConvertUTF32toUTF8(
      &source_ptr, reinterpret_cast<const llvm::UTF32*>(str.end()), &storage_ptr, storage.end(),
      llvm::ConversionFlags::strictConversion);
  assert(errorCode == llvm::ConversionResult::conversionOK);
  output.write(reinterpret_cast<char *>(storage.begin()), storage_ptr - storage.data());
  return output;
}

template<class Traits>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& output, const eastl::string& str)
{
  output.write(str.data(), str.size());
  return output;
}

std::stringstream&
operator>>(std::stringstream& input, eastl::string& str);

template <
  typename T,
  typename Allocator,
  typename OStream,
  typename = eastl::enable_if_t<eastl::is_base_of_v<std::ostream, OStream>>
>
OStream&
operator<<(OStream &output, const eastl::vector<T, Allocator> &c)
{
  output << "eastl::vector<" << GetTypeName<T>() << "> {";
  for (const T &e : c) {
    output << e << ", ";
  }
  output << "}";
  return output;
}
}
