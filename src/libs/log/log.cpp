/* extend - expansible programming language
 * Copyright (C) 2022 Vladimir Liutov vs@lutov.net
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

#include "log.h"

#include <EASTL/fixed_string.h>
#include <cassert>
#include <iostream>
#include <llvm/Support/ConvertUTF.h>

#include "dtoa.h"

namespace extend::log {

using Pipe =
  eastl::variant<CErrPipe, NullPipe, BufferPipe<true>, BufferPipe<false>>;
Pipe default_pipe{};

OStreamFactory debug = eastl::visit(
  [](auto&& pipe) -> OStreamFactory {
    return OStreamFactory(LEVEL::DEBUG, pipe);
  },
  default_pipe);
OStreamFactory info = OStreamFactory(LEVEL::INFO, eastl::get<0>(default_pipe));
OStreamFactory warning =
  OStreamFactory(LEVEL::WARNING, eastl::get<0>(default_pipe));
OStreamFactory error =
  OStreamFactory(LEVEL::ERROR, eastl::get<0>(default_pipe));
OStreamFactory fatal =
  OStreamFactory(LEVEL::FATAL, eastl::get<0>(default_pipe));

eastl::u8string
IPipe::linePrefix(LEVEL level)
{
  switch (level) {
    case LEVEL::DEBUG:
      return u8"[D] ";
    case LEVEL::INFO:
      return u8"[I] ";
    case LEVEL::WARNING:
      return u8"[W] ";
    case LEVEL::ERROR:
      return u8"[E] ";
    case LEVEL::FATAL:
      return u8"[F] ";
    default:;
  }
  return u8"";
}

void
NullPipe::log(LEVEL, eastl::u8string_view)
{}
void
CErrPipe::log(LEVEL level, eastl::u8string_view str)
{
  eastl::u8string prefix = linePrefix(level);
  std::cerr.write(reinterpret_cast<const char*>(prefix.c_str()), prefix.size());
  std::cerr.write(reinterpret_cast<const char*>(str.data()), str.size());
  std::cerr << std::endl;
}

template<bool enable_prefix>
void
BufferPipe<enable_prefix>::log(LEVEL level, eastl::u8string_view str)
{
  if constexpr (enable_prefix) {
    eastl::u8string prefix = linePrefix(level);
    buffer.append(prefix.data(), prefix.size());
  }
  buffer.append(str.data(), str.size());
  buffer.append(u8"\n", 1);
}

template<typename T>
void
init_default_pipe()
{
  default_pipe.emplace<T>();
  eastl::visit(
    [](auto&& pipe) {
      debug = OStreamFactory(LEVEL::DEBUG, pipe);
      info = OStreamFactory(LEVEL::INFO, pipe);
      warning = OStreamFactory(LEVEL::WARNING, pipe);
      error = OStreamFactory(LEVEL::ERROR, pipe);
      fatal = OStreamFactory(LEVEL::FATAL, pipe);
    },
    default_pipe);
}

template<typename T>
T*
get_default_pipe()
{
  return eastl::get_if<T>(&default_pipe);
}

template void
init_default_pipe<NullPipe>();
template void
init_default_pipe<CErrPipe>();
template void
init_default_pipe<BufferPipe<false>>();
template void
init_default_pipe<BufferPipe<true>>();

template NullPipe*
get_default_pipe<NullPipe>();
template CErrPipe*
get_default_pipe<CErrPipe>();
template BufferPipe<false>*
get_default_pipe<BufferPipe<false>>();
template BufferPipe<true>*
get_default_pipe<BufferPipe<true>>();

OStream&
OStream::operator<<(int8_t x)
{
  line.append_sprintf(u8"%I8d", x);
  return *this;
}

OStream&
OStream::operator<<(uint8_t x)
{
  line.append_sprintf(u8"%I8u", x);
  return *this;
}

OStream&
OStream::operator<<(int16_t x)
{
  line.append_sprintf(u8"%I16d", x);
  return *this;
}

OStream&
OStream::operator<<(uint16_t x)
{
  line.append_sprintf(u8"%I16u", x);
  return *this;
}

OStream&
OStream::operator<<(uint32_t x)
{
  line.append_sprintf(u8"%I32u", x);
  return *this;
}

OStream&
OStream::operator<<(int32_t x)
{
  line.append_sprintf(u8"%I32d", x);
  return *this;
}

OStream&
OStream::operator<<(uint64_t x)
{
  line.append_sprintf(u8"%I64u", x);
  return *this;
}

OStream&
OStream::operator<<(int64_t x)
{
  line.append_sprintf(u8"%I64d", x);
  return *this;
}

OStream&
OStream::operator<<(double x)
{
  ssize_t oldSize = line.size();
  line.resize(oldSize + 25);
  line.resize(dtoa(x, line.begin() + oldSize) + oldSize);
  return *this;
}

OStream&
OStream::operator<<(float x)
{
  ssize_t oldSize = line.size();
  line.resize(oldSize + 16);
  line.resize(ftoa(x, line.begin() + oldSize) + oldSize);
  return *this;
}

OStream&
OStream::operator<<(long double x)
{
  return operator<<(static_cast<double>(x));
}

OStream&
OStream::operator<<(char8_t c)
{
  line.append_sprintf(u8"%c", c);
  return *this;
}

template<typename LLVMCharType, typename F, F convert, typename T>
static inline void
append_string(eastl::fixed_string<char8_t, 256>& line, T str)
{
  ssize_t old_size = line.size();
  line.resize(line.size() + str.size() * UNI_MAX_UTF8_BYTES_PER_CODE_POINT);
  llvm::UTF8* storage_ptr =
    reinterpret_cast<llvm::UTF8*>(line.data() + old_size);
  const LLVMCharType* source_ptr =
    reinterpret_cast<const LLVMCharType*>(str.data());
  llvm::ConversionResult errorCode =
    convert(&source_ptr,
            reinterpret_cast<const LLVMCharType*>(str.end()),
            &storage_ptr,
            reinterpret_cast<llvm::UTF8*>(line.end()),
            llvm::ConversionFlags::strictConversion);
  (void)errorCode;
  assert(errorCode == llvm::ConversionResult::conversionOK);
  line.resize(reinterpret_cast<char8_t*>(storage_ptr) - line.begin());
}

OStream&
OStream::operator<<(char16_t c)
{
  eastl::array<char16_t, 1> str = { c };
  append_string<llvm::UTF16,
                decltype(llvm::ConvertUTF16toUTF8),
                llvm::ConvertUTF16toUTF8>(line, str);
  return *this;
}

OStream&
OStream::operator<<(char32_t c)
{
  eastl::array<char32_t, 1> str = { c };
  append_string<llvm::UTF32,
                decltype(llvm::ConvertUTF32toUTF8),
                llvm::ConvertUTF32toUTF8>(line, str);
  return *this;
}

OStream&
OStream::operator<<(eastl::u8string_view str)
{
  line.append(str.data(), str.size());
  return *this;
}

OStream&
OStream::operator<<(eastl::u16string_view str)
{
  append_string<llvm::UTF16,
                decltype(llvm::ConvertUTF16toUTF8),
                llvm::ConvertUTF16toUTF8>(line, str);
  return *this;
}

OStream&
OStream::operator<<(eastl::u32string_view str)
{
  append_string<llvm::UTF32,
                decltype(llvm::ConvertUTF32toUTF8),
                llvm::ConvertUTF32toUTF8>(line, str);
  return *this;
}

OStream::OStream(OStreamFactory& f)
  : level(f.level)
  , pipe(f.pipe)
  , enable(true)
{}

OStream::OStream(OStream&& s)
  : level(s.level)
  , pipe(s.pipe)
  , enable(true)
  , line(eastl::move(s.line))
{
  s.enable = false;
}

OStream::~OStream()
{
  if (enable) {
    pipe.get().log(level, line);
  }
}
}
