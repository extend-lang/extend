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

#pragma once

#include "io/FileSource.h"

#include <EASTL/string_view.h>
#include <EASTL/type_traits.h>
#include <EASTL/vector.h>

#include <sstream>

using namespace eastl;

namespace extend::textdata {

/**
 * Merge 2 chars into one int32_t.
 * Usefull for check equality of sequential chars.
 * We haven't s(c1, c2, c3, c4) becouse it could be equal to `eof = 0xffffffff`
 *
 * @param c1 first char
 * @param c2 second char
 * @returns  int = [0, 0, c1, c2]
 */
constexpr int32_t
s(char c1, char c2)
{
  return (int32_t(c1) << 8) | c2;
}

/**
 * Merge 3 chars into one int32_t.
 * Usefull for check equality of sequential chars.
 * We haven't s(c1, c2, c3, c4) becouse it could be equal to `eof = 0xffffffff`
 *
 * @param c1 first char
 * @param c2 second char
 * @param c3 third char
 * @returns  int = [0, c1, c2, c3]
 */
constexpr int32_t
s(char c1, char c2, char c3)
{
  return (int32_t(c1) << 16) | (int32_t(c2) << 8) | c3;
}

struct Reader final
{
private:
  vector<ssize_t> lines;

  enum iostate
  {
    GOOD_BIT = 0x0,
    EOF_BIT = 0x4
  } read_state;

  template<typename T>
  [[nodiscard]] T message(ssize_t pos) const
  {
    const ssize_t *it = upper_bound(lines.begin(), lines.end(), pos);
    ssize_t nextLine = it != lines.end() ? *it - 1 : source.data.length();
    --it;
    ssize_t line = it - lines.begin();
    ssize_t column = pos - *it;

    string_view str =
      string_view(source.data.c_str() + *it, nextLine - *it);
    return T(source.filename, line + 1, column + 1, str);
  }

public:
  struct ReaderNote
  {
  private:
    std::ostringstream prefix, suffix;

    ReaderNote(string_view filename,
               ssize_t line,
               ssize_t column,
               string_view input);

  public:
    friend struct Reader;
    ReaderNote(const ReaderNote&) = delete;
    ReaderNote(ReaderNote&&) = default;
    ReaderNote& operator=(const ReaderNote&) = delete;
    ReaderNote& operator=(ReaderNote&&) = default;
    ~ReaderNote() = default;
  };

  struct ReaderError final : ReaderNote
  {
  private:
    vector<ReaderNote> notes;
    ReaderError(string_view filename,
                ssize_t line,
                ssize_t column,
                string_view input)
      : ReaderNote(filename, line, column, input)
    {}

  public:
    friend struct Reader;
    ReaderError(const ReaderError&) = delete;
    ReaderError(ReaderError&&) = delete;
    ReaderError& operator=(const ReaderError&) = delete;
    ReaderError& operator=(ReaderError&&) = delete;
    ~ReaderError();

    template<typename T>
    ReaderError& operator<<(const T& value)
    {
      if (notes.empty()) {
        prefix << value;
      } else {
        notes.back().prefix << value;
      }

      return *this;
    };

    ReaderError& operator<<(ReaderNote&& note)
    {
      notes.push_back(move(note));
      return *this;
    };
  };

  io::FileSource source;
  constexpr static int32_t eof_value = -1;
  ssize_t offset = 0;
  ssize_t errors = 0;

  Reader(io::FileSource&& input);
  ~Reader() = default;

  Reader(const Reader&) = delete;
  Reader(Reader&&) = delete;
  Reader& operator=(const Reader&) = delete;
  Reader& operator=(Reader&&) = delete;

  template<ssize_t len = 1,
           enable_if_t<1 <= len, bool> = true,
           enable_if_t<len <= 3, bool> = true>
  [[nodiscard]] int32_t peek() const
  {
    if (eof()) {
      return eof_value;
    }

    char c1 = source.data[offset];
    if constexpr (len == 1) {
      return c1;
    }
    char c2 = (offset + 1 < ssize_t(source.data.length()))
                ? source.data[offset + 1]
                : char(0);
    if constexpr (len == 2) {
      return s(c1, c2);
    }
    char c3 = (offset + 2 < ssize_t(source.data.length()))
                ? source.data[offset + 2]
                : char(0);
    return s(c1, c2, c3);
  }

  template<int32_t value>
  [[nodiscard]] bool check() const
  {
    static_assert(0 < value && value <= 0xffffff);
    constexpr ssize_t len = value <= 0xff ? 1 : (value <= 0xffff ? 2 : 3);
    return peek<len>() == value;
  }

  int32_t get();
  [[nodiscard]] bool eof() const;
  [[nodiscard]] string substr(ssize_t pos, ssize_t length) const;

  [[nodiscard]] ReaderError error(ssize_t pos);
  [[nodiscard]] ReaderNote note(ssize_t pos) const;
};
}
