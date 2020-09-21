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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "reader.h"

#include <EASTL/algorithm.h>
#include <g3log/g3log.hpp>

#include <cassert>
#include <cctype>

namespace extend::text {

template <class Char>
static void
put_spaces(std::ostringstream& stream, basic_string_view<Char> input, ssize_t column)
{
  for (ssize_t i = 0; i < column - 1; ++i) {
    if (isblank(input[i]) != 0) {
      stream.put(input[i]);
    } else {
      stream.put(' ');
    }
  }
  stream.put('^');
}

template<io::EncodingType enc>
Reader<enc>::ReaderNote::ReaderNote(
  u8string_view filename,
  ssize_t line,
  ssize_t column,
  typename Encoding::Span input,
  u8string_view level)
{
  prefix << filename << ":" << line << ":" << column << ": " << level << ": ";
  suffix << input << std::endl;
  put_spaces(suffix, input, column);
}

template<io::EncodingType enc>
Reader<enc>::ReaderError::ReaderError(
  u8string_view filename,
  ssize_t line,
  ssize_t column,
  typename Encoding::Span input)
{
  notes.emplace_back(filename, line, column, input, u8"error");
}

template<io::EncodingType enc>
Reader<enc>::ReaderError::~ReaderError()
{
  auto logCapture = INTERNAL_LOG_MESSAGE(ERROR);
  std::ostringstream& stream = logCapture.stream();
  for (const auto& note : notes) {
    stream << note.prefix.str() << std::endl << note.suffix.str() << std::endl;
  }
}

template<io::EncodingType enc>
typename Reader<enc>::ReaderError
Reader<enc>::error(ssize_t pos)
{
  if (pos == -1) {
    pos = tellp();
  }
  ++errors;
  return message<ReaderError>(pos);
}

template<io::EncodingType enc>
typename Reader<enc>::ReaderError &
Reader<enc>::ReaderError::operator<<(ReaderNote&& note)
{
  notes.push_back(move(note));
  return *this;
};

template<io::EncodingType enc>
ssize_t
Reader<enc>::errorCount() const
{
  return errors;
}

template<io::EncodingType enc>
typename Reader<enc>::ReaderNote
Reader<enc>::note(ssize_t pos) const
{
  return message<ReaderNote>(pos);
}

template<io::EncodingType enc>
Reader<enc>::Reader(io::FileSource<encoding>&& input)
  : read_state(input.data.empty() ? EOF_BIT : GOOD_BIT)
  , lines({ 0 })
  , source(move(input))
{
  for (ssize_t i = 0; i < ssize_t(source.data.length()); ++i) {
    if (source.data[i] == '\n') {
      lines.push_back(i + 1);
    }
  }
}

template<io::EncodingType enc>
typename Reader<enc>::Encoding::Triplet
Reader<enc>::peek(ssize_t size) const
{
  assert(1 <= size && size <= 3);

  if (eof()) {
    return Triplet::eof;
  }

  CodePoint c1 = source.data[pos];
  if (size == 1) {
    return Triplet(c1);
  }
  CodePoint c2 = (pos + 1 < ssize_t(source.data.length())) ? source.data[pos + 1]
                                                      : CodePoint(0);
  if (size == 2) {
    return Triplet(c1, c2);
  }
  CodePoint c3 = (pos + 2 < ssize_t(source.data.length())) ? source.data[pos + 2]
                                                      : CodePoint(0);
  return Triplet(c1, c2, c3);
}

template<io::EncodingType enc>
bool
Reader<enc>::check(Triplet value) const
{
  ssize_t size = value.size();
  return peek(size) == value;
}

template<io::EncodingType enc>
typename Reader<enc>::Encoding::Triplet
Reader<enc>::get(ssize_t size)
{
  if (eof()) {
    return Triplet::eof;
  }

  assert(read_state == GOOD_BIT);
  assert(pos + size - 1 < ssize_t(source.data.length()));

  Triplet result = peek(size);

  pos += size;

  if (pos >= ssize_t(source.data.length())) {
    read_state = EOF_BIT;
  }

  return result;
}

template<io::EncodingType enc>
bool
Reader<enc>::eof() const
{
  return (read_state & EOF_BIT) != 0;
}

template<io::EncodingType enc>
typename Reader<enc>::Encoding::Span
Reader<enc>::substr(ssize_t pos, ssize_t length) const
{
  return typename Encoding::Span(source.data.c_str() + pos, length);
}

template<io::EncodingType enc>
ssize_t
Reader<enc>::tellp() const
{
  return pos;
}

template<io::EncodingType enc>
u8string_view
Reader<enc>::filename() const
{
  return source.filename;
}

template struct Reader<io::EncodingType::US_ASCII>;
template struct Reader<io::EncodingType::UTF8>;
}
