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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "reader.h"

#include <EASTL/algorithm.h>
#include <g3log/g3log.hpp>

#include <cassert>
#include <cctype>

namespace extend::textdata {

static void
put_spaces(std::ostringstream& stream, string_view input, ssize_t column)
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

Reader::ReaderNote::ReaderNote(string_view filename,
                               ssize_t line,
                               ssize_t column,
                               string_view input)
{
  prefix << filename << ":" << line << ":" << column << ": note: ";
  suffix << input << std::endl;
  put_spaces(suffix, input, column);
}

Reader::ReaderError::~ReaderError()
{
  auto logCapture = INTERNAL_LOG_MESSAGE(ERROR);
  std::ostringstream& stream = logCapture.stream();
  stream << prefix.str() << std::endl << suffix.str();
  for (const auto& note : notes) {
    stream << std::endl << note.prefix.str() << std::endl << note.suffix.str();
  }
}

Reader::ReaderError
Reader::error(ssize_t pos)
{
  ++errors;
  return message<ReaderError>(pos);
}

Reader::ReaderNote
Reader::note(ssize_t pos) const
{
  return message<ReaderNote>(pos);
}

Reader::Reader(io::FileSource&& input)
  : lines({ 0 })
  , read_state(input.data.empty() ? EOF_BIT : GOOD_BIT)
  , source(move(input))
{
  for (ssize_t i = 0; i < ssize_t(source.data.length()); ++i) {
    if (source.data[i] == '\n') {
      lines.push_back(i + 1);
    }
  }
}

int32_t
Reader::get()
{
  if (eof()) {
    return eof_value;
  }

  assert(read_state == GOOD_BIT);

  ++offset;

  if (offset >= ssize_t(source.data.length())) {
    read_state = EOF_BIT;
  }

  return source.data[offset - 1];
}

bool
Reader::eof() const
{
  return (read_state & EOF_BIT) != 0;
}

string
Reader::substr(ssize_t pos, ssize_t length) const
{
  return string(source.data.c_str() + pos, length);
}
}
