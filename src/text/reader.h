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
#include <io/prelude.h>

#include "io/FileSource.h"

#include <EASTL/optional.h>
#include <EASTL/string_view.h>
#include <EASTL/type_traits.h>
#include <EASTL/vector.h>

#include <sstream>

using namespace eastl;

namespace extend::text {

/**
 * Reader for text file
 *
 * Internal used by Lexer
 */
template<io::EncodingType enc>
struct Reader final
{
  constexpr static io::EncodingType encoding = enc;
  using Encoding = io::EncodingTraits<encoding>;
  using CodePoint = typename Encoding::CodePoint;
  using Triplet = typename Encoding::Triplet;

private:
  enum iostate
  {
    GOOD_BIT = 0x0,
    EOF_BIT = 0x4
  };

  iostate read_state;    //!< Or-merge of iostate bits.
  vector<ssize_t> lines; //!< Source lines.
  io::FileSource<encoding> source; //!< Input source.
  ssize_t pos = 0;                                   //!< Current position.
  ssize_t errors = 0;                                //!< Current error count.

  /**
   * Create an message: filename, line, column, code string.
   *
   * Usage:
   * \code{.cpp}
   *   reader.error(indent_pos) << "unexpected indent"
   *     << reader.note(prev_indent) << "to match this indent";
   * \endcode
   *
   * \tparam T message class: ReaderNote or ReaderError
   */
  template<typename T>
  [[nodiscard]] T message(ssize_t pos) const
  {
    const ssize_t* it = upper_bound(lines.begin(), lines.end(), pos);
    ssize_t nextLine =
      it != lines.end() ? *it - 1 : ssize_t(source.data.length());
    --it;
    ssize_t line = it - lines.begin();
    ssize_t column = pos - *it;

    auto substr = typename Encoding::Span(source.data.c_str() + *it, nextLine - *it);
    return T(source.filename, line + 1, column + 1, substr);
  }

public:
  struct ReaderNote final
  {
    std::ostringstream prefix, suffix;

    ReaderNote(u8string_view filename,
               ssize_t line,
               ssize_t column,
               typename Encoding::Span input,
               u8string_view level = u8"note");

    friend struct Reader;
    ReaderNote(const ReaderNote&) = delete;
    ReaderNote(ReaderNote&&) noexcept = default;
    ReaderNote& operator=(const ReaderNote&) = delete;
    ReaderNote& operator=(ReaderNote&&) noexcept = default;
    ~ReaderNote() = default;
  };

  struct ReaderError final
  {
    vector<ReaderNote> notes;

    ReaderError(u8string_view filename,
                ssize_t line,
                ssize_t column,
                typename Encoding::Span input);

    ReaderError(const ReaderError&) = delete;
    ReaderError(ReaderError&&) noexcept = delete;
    ReaderError& operator=(const ReaderError&) = delete;
    ReaderError& operator=(ReaderError&&) noexcept = delete;
    ~ReaderError();

    template<typename T>
    ReaderError& operator<<(const T& value)
    {
      notes.back().prefix << value;
      return *this;
    };

    ReaderError& operator<<(ReaderNote&& note);
    friend struct Reader;
  };

  Reader(io::FileSource<encoding>&& input);
  ~Reader() = default;

  Reader(const Reader&) = delete;
  Reader(Reader&&) = delete;
  Reader& operator=(const Reader&) = delete;
  Reader& operator=(Reader&&) = delete;

  /**
   * Peek 1, 2 or 3 chars from input
   *
   * \tparam size count of char to peek
   * \return these chars merged in Triplet
   */
  [[nodiscard]] Triplet peek(ssize_t size = 1) const;

  [[nodiscard]] bool check(Triplet value) const;

  /**
   * Get current position
   */
  [[nodiscard]] ssize_t tellp() const;

  /**
   * Peek several chars and increase position
   *
   * \tparam size count of char to peek
   * \return these chars merged in Triplet
   */
  typename Encoding::Triplet get(ssize_t size = 1);

  /**
   * Is eof reached?
   */
  [[nodiscard]] bool eof() const;

  /**
   * Get substring from source
   */
  [[nodiscard]] typename Encoding::Span substr(ssize_t pos, ssize_t length) const;

  /**
   * Print error and increase errorCount()
   *
   * Usage:
   * \code{.cpp}
   *   reader.error(indent_pos) << "unexpected indent"
   *     << reader.note(prev_indent) << "to match this indent";
   * \endcode
   *
   * \param pos position of error; if omit, error on current line
   *
   * \return ostream for error text
   */
  [[nodiscard]] ReaderError error(ssize_t pos = -1);

  /**
   * Print note
   *
   * Usage:
   * \code{.cpp}
   *   reader.error(indent_pos) << "unexpected indent"
   *     << reader.note(prev_indent) << "to match this indent";
   * \endcode
   *
   * \param pos position of note
   */
  [[nodiscard]] ReaderNote note(ssize_t pos) const;

  /**
   * Get count of errors
   */
  [[nodiscard]] ssize_t errorCount() const;

  /**
   * Get the file name. Call to underlying source.
   */
  [[nodiscard]] u8string_view filename() const;
};
}
