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
#include <catch2/catch_test_macros.hpp>
#include <utils/eastl_io.h>

using namespace extend::log;

TEST_CASE("Pipe should log integer consts", "log")
{
  init_default_pipe<BufferPipe<>>();
  debug << 10;
  BufferPipe<>* pipe = get_default_pipe<BufferPipe<>>();
  REQUIRE(pipe != nullptr);
  REQUIRE(pipe->buffer == u8"10\n");
}

TEST_CASE("Pipe should log different levels", "log")
{
  init_default_pipe<BufferPipe<true>>();
  debug << 10;
  info << 20;
  warning << 30;
  error << 40;
  fatal << 50;
  BufferPipe<true>* pipe = get_default_pipe<BufferPipe<true>>();
  REQUIRE(pipe != nullptr);
  REQUIRE(pipe->buffer == u8"[D] 10\n[I] 20\n[W] 30\n[E] 40\n[F] 50\n");
}

struct ExpectLog
{
  decltype(BufferPipe<>::buffer) expected;
  ExpectLog(decltype(BufferPipe<>::buffer)&& str)
    : expected(eastl::move(str))
  {
    init_default_pipe<BufferPipe<>>();
  }

  ~ExpectLog()
  {
    BufferPipe<>* pipe = get_default_pipe<BufferPipe<>>();
    REQUIRE(pipe != nullptr);
    REQUIRE(pipe->buffer == expected);
  }
};

TEST_CASE("Integer limits", "log")
{
  {
    ExpectLog log(u8"-128\n");
    debug << int8_t(-128);
  }

  {
    ExpectLog log(u8"127\n");
    debug << int8_t(127);
  }

  {
    ExpectLog log(u8"255\n");
    debug << uint8_t(255);
  }

  {
    ExpectLog log(u8"32767\n");
    debug << int16_t(32767);
  }

  {
    ExpectLog log(u8"-32768\n");
    debug << int16_t(-32768);
  }

  {
    ExpectLog log(u8"65535\n");
    debug << uint16_t(65535);
  }

  {
    ExpectLog log(u8"2147483647\n");
    debug << int32_t(2147483647);
  }

  {
    ExpectLog log(u8"-2147483648\n");
    debug << int32_t(-2147483648);
  }

  {
    ExpectLog log(u8"4294967295\n");
    debug << uint32_t(4294967295);
  }

  {
    ExpectLog log(u8"9223372036854775807\n");
    debug << int64_t(9223372036854775807LL);
  }

  {
    ExpectLog log(u8"-9223372036854775808\n");
    // compiler cannot store full literal
    debug << int64_t(-9223372036854775807LL - 1);
  }

  {
    ExpectLog log(u8"18446744073709551615\n");
    debug << uint64_t(18446744073709551615LLu);
  }
}

TEST_CASE("Log char", "log")
{
  {
    ExpectLog log(u8"a\n");
    debug << u8'a';
  }

  {
    ExpectLog log(u8"b\n");
    debug << u'b';
  }

  {
    ExpectLog log(u8"c\n");
    debug << U'c';
  }

  {
    ExpectLog log(u8"u8string\n");
    debug << u8"u8string";
  }

  {
    ExpectLog log(u8"u16string\n");
    debug << u"u16string";
  }

  {
    ExpectLog log(u8"u32string\n");
    debug << U"u32string";
  }
}

TEST_CASE("Log double", "log")
{
  {
    ExpectLog log(u8"0.0\n");
    debug << 0.0;
  }
  {
    ExpectLog log(u8"123.0\n");
    debug << 123.0;
  }
  {
    ExpectLog log(u8"12.34\n");
    debug << 12.34;
  }
  {
    ExpectLog log(u8"1.2345678901234568e+20\n");
    debug << 1.2345678901234568e+20;
  }
  {
    ExpectLog log(u8"1.2345678901234568e+21\n");
    debug << 1.2345678901234568e21;
  }
  {
    ExpectLog log(u8"0.0012345\n");
    debug << 0.0012345;
  }
  {
    ExpectLog log(u8"0.00012345\n");
    debug << 0.00012345;
  }
  {
    ExpectLog log(u8"1.2345e-5\n");
    debug << 1.2345e-5;
  }
  {
    ExpectLog log(u8"12345000.0\n");
    debug << 12345000.0;
  }
  {
    ExpectLog log(u8"1.2345e+8\n");
    debug << 1.2345e+8;
  }
  {
    ExpectLog log(u8"1.2345e+18\n");
    debug << 1.2345e+18;
  }
  {
    ExpectLog log(u8"123456790.0\n");
    debug << 123456790.0;
  }
  {
    ExpectLog log(u8"nan\n");
    debug << 0.0 / 0.0;
  }
  {
    ExpectLog log(u8"Infinity\n");
    debug << 1.0 / 0.0;
  }
  {
    ExpectLog log(u8"-Infinity\n");
    debug << -1.0 / 0.0;
  }

  {
    ExpectLog log(u8"1.0e+30\n");
    debug << 1.0e+30;
  }

  {
    ExpectLog log(u8"2.0e+10\n");
    debug << 2.0e+10;
  }

  {
    ExpectLog log(u8"2.2250738585072014e-308\n");
    debug << DBL_MIN;
  }

  {
    ExpectLog log(u8"5.0e-324\n");
    debug << DBL_TRUE_MIN;
  }

  {
    ExpectLog log(u8"1.0e+9\n");
    debug << 1.0e+9;
  }
  {
    ExpectLog log(u8"1.0e+99\n");
    debug << 1.0e+99;
  }
  {
    ExpectLog log(u8"1.0e+100\n");
    debug << 1.0e+100;
  }
}

TEST_CASE("Log long double", "log")
{
  {
    ExpectLog log(u8"0.0\n");
    debug << 0.0L;
  }
  {
    ExpectLog log(u8"123.0\n");
    debug << 123.0L;
  }
  {
    ExpectLog log(u8"12.34\n");
    debug << 12.34L;
  }
  {
    ExpectLog log(u8"1.2345678901234568e+20\n");
    debug << 1.2345678901234568e+20L;
  }
  {
    ExpectLog log(u8"1.2345678901234568e+21\n");
    debug << 1.2345678901234568e21L;
  }
  {
    ExpectLog log(u8"0.0012345\n");
    debug << 0.0012345L;
  }
  {
    ExpectLog log(u8"0.00012345\n");
    debug << 0.00012345L;
  }
  {
    ExpectLog log(u8"1.2345e-5\n");
    debug << 1.2345e-5L;
  }
  {
    ExpectLog log(u8"12345000\n");
    debug << 12345000L;
  }
  {
    ExpectLog log(u8"1.2345e+8\n");
    debug << 1.2345e+8L;
  }
  {
    ExpectLog log(u8"1.2345e+18\n");
    debug << 1.2345e+18L;
  }
  {
    ExpectLog log(u8"123456790.0\n");
    debug << 123456790.0L;
  }
  {
    ExpectLog log(u8"nan\n");
    debug << 0.0L / 0.0L;
  }
  {
    ExpectLog log(u8"Infinity\n");
    debug << 1.0L / 0.0L;
  }
  {
    ExpectLog log(u8"-Infinity\n");
    debug << -1.0L / 0.0L;
  }

  {
    ExpectLog log(u8"1.0e+30\n");
    debug << 1.0e+30L;
  }

  {
    ExpectLog log(u8"2.0e+10\n");
    debug << 2.0e+10L;
  }
  {
    ExpectLog log(u8"2.2250738585072014e-308\n");
    debug << (long double)DBL_MIN;
  }
  {
    ExpectLog log(u8"5.0e-324\n");
    debug << (long double)DBL_TRUE_MIN;
  }
  {
    ExpectLog log(u8"1.0e+9\n");
    debug << 1.0e+9L;
  }
  {
    ExpectLog log(u8"1.0e+99\n");
    debug << 1.0e+99L;
  }
  {
    ExpectLog log(u8"1.0e+100\n");
    debug << 1.0e+100L;
  }
}

TEST_CASE("Log float", "log")
{
  {
    ExpectLog log(u8"0.0\n");
    debug << 0.0f;
  }
  {
    ExpectLog log(u8"123.0\n");
    debug << 123.0f;
  }
  {
    ExpectLog log(u8"12.34\n");
    debug << 12.34f;
  }
  {
    ExpectLog log(u8"1.0\n");
    debug << 1.0f;
  }
  {
    ExpectLog log(u8"12.0\n");
    debug << 12.0f;
  }
  {
    ExpectLog log(u8"123.0\n");
    debug << 123.0f;
  }
  {
    ExpectLog log(u8"1234.0\n");
    debug << 1234.0f;
  }
  {
    ExpectLog log(u8"12345.0\n");
    debug << 12345.0f;
  }
  {
    ExpectLog log(u8"123456.0\n");
    debug << 123456.0f;
  }
  {
    ExpectLog log(u8"1234567.0\n");
    debug << 1234567.0f;
  }
  {
    ExpectLog log(u8"12345678.0\n");
    debug << 12345678.0f;
  }
  {
    ExpectLog log(u8"1.2345678e+20\n");
    debug << 1.2345678e+20f;
  }
  {
    ExpectLog log(u8"1.2345678e+21\n");
    debug << 1.2345678e21f;
  }
  {
    ExpectLog log(u8"0.0012345\n");
    debug << 0.0012345f;
  }
  {
    ExpectLog log(u8"0.00012345\n");
    debug << 0.00012345f;
  }
  {
    ExpectLog log(u8"1.2345e-5\n");
    debug << 1.2345e-5f;
  }
  {
    ExpectLog log(u8"12345000.0\n");
    debug << 12345000.0f;
  }
  {
    ExpectLog log(u8"1.2345e+8\n");
    debug << 1.2345e+8f;
  }
  {
    ExpectLog log(u8"1.2345e+18\n");
    debug << 1.2345e+18f;
  }
  {
    ExpectLog log(u8"123456790.0\n");
    debug << 123456790.0f;
  }
  {
    ExpectLog log(u8"nan\n");
    debug << 0.0f / 0.0f;
  }
  {
    ExpectLog log(u8"Infinity\n");
    debug << 1.0f / 0.0f;
  }
  {
    ExpectLog log(u8"-Infinity\n");
    debug << -1.0f / 0.0f;
  }

  {
    ExpectLog log(u8"1.0e+30\n");
    debug << 1.0e+30f;
  }

  {
    ExpectLog log(u8"2.0e+10\n");
    debug << 2.0e+10f;
  }
  {
    ExpectLog log(u8"1.1754944e-38\n");
    debug << FLT_MIN;
  }
  {
    ExpectLog log(u8"1.0e-45\n");
    debug << FLT_TRUE_MIN;
  }
  {
    ExpectLog log(u8"1.0e+9\n");
    debug << 1.0e+9f;
  }
  {
    ExpectLog log(u8"1.0e+10\n");
    debug << 1.0e+10f;
  }
}
