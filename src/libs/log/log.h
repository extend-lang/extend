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

#pragma once

#include <EASTL/fixed_string.h>
#include <EASTL/internal/functional_base.h>
#include <EASTL/string.h>
#include <EASTL/variant.h>

namespace extend::log {

enum struct LEVEL
{
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL
};

/** Write to file or stream
 */
struct IPipe
{
  IPipe() = default;
  virtual ~IPipe() = default;
  IPipe(IPipe&&) = delete;
  IPipe& operator=(IPipe&&) = delete;
  IPipe(const IPipe&) = delete;
  IPipe& operator=(const IPipe&) = delete;

  /** Write one line to file or stream.
   */
  virtual void log(LEVEL level, eastl::u8string_view str) = 0;
  static eastl::u8string linePrefix(LEVEL level);
};

/** Info to create an output stream
 */
struct OStreamFactory
{
  LEVEL level;
  eastl::reference_wrapper<IPipe> pipe;

  OStreamFactory(LEVEL l, IPipe& p)
    : level(l)
    , pipe(p)
  {}
  OStreamFactory(OStreamFactory&& s) = default;
  OStreamFactory& operator=(OStreamFactory&& s) = default;
  OStreamFactory(const OStreamFactory&) = delete;
  OStreamFactory& operator=(const OStreamFactory&) = delete;
};

/** Output stream.
 */
struct OStream
{
  LEVEL level = LEVEL::DEBUG;
  eastl::reference_wrapper<IPipe> pipe;
  bool enable = false;
  eastl::fixed_string<char8_t, 256> line;

  /** Build stream from factory.
   *
   * Factory creates stream in OStreamFactory::operator<<
   */
  OStream(OStreamFactory& f);

  /** Move stream.
   *
   * Factory moves out stream from OStreamFactory::operator<<
   */
  OStream(OStream&& s);

  /** Write message to log.
   */
  ~OStream();

  OStream& operator=(OStream&& s) = delete;
  OStream(const OStream&) = delete;
  OStream& operator=(const OStream&) = delete;

  OStream& operator<<(char8_t c);
  OStream& operator<<(char16_t c);
  OStream& operator<<(char32_t c);

  OStream& operator<<(eastl::u8string_view str);
  OStream& operator<<(eastl::u16string_view str);
  OStream& operator<<(eastl::u32string_view str);

  OStream& operator<<(int8_t x);
  OStream& operator<<(int16_t x);
  OStream& operator<<(int32_t x);
  OStream& operator<<(int64_t x);

  OStream& operator<<(uint8_t x);
  OStream& operator<<(uint16_t x);
  OStream& operator<<(uint32_t x);
  OStream& operator<<(uint64_t x);

  OStream& operator<<(float x);
  OStream& operator<<(double x);
  OStream& operator<<(long double x);
};

template<typename T>
OStream
operator<<(OStreamFactory& factory, const T& x)
{
  return eastl::move(OStream(factory) << x);
}

extern OStreamFactory debug;
extern OStreamFactory info;
extern OStreamFactory warning;
extern OStreamFactory error;
extern OStreamFactory fatal;

/** Ignore logs completely.
 */
struct NullPipe : IPipe
{
  virtual void log(LEVEL level, eastl::u8string_view str) override;
};

/** Print logs to cerr. Default pipe.
 */
struct CErrPipe : IPipe
{
  virtual void log(LEVEL level, eastl::u8string_view str) override;
};

/** Print logs to buffer.
 * Usefull for tests.
 */
template<bool enable_prefix = false>
struct BufferPipe : IPipe
{
  static constexpr bool ENABLE_PREFIX = enable_prefix;

  virtual void log(LEVEL level, eastl::u8string_view str) override;
  eastl::fixed_string<char8_t, 4096> buffer;
};

template<typename T>
void
init_default_pipe();

template<typename T>
T*
get_default_pipe();
}
