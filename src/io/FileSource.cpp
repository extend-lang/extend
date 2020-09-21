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

#include "FileSource.h"

#include <g3log/g3log.hpp>

#include <cassert>
#include <fstream>

namespace extend::io {
FileSource::FileSource(string_view _filename, string&& _data)
  : filename(_filename)
  , data(move(_data))
{}

static void
replace_newlines(string& data)
{
  ssize_t len = data.length();
  ssize_t j = 0;

  for (ssize_t i = 0; i < len; ++i, ++j) {
    char c = data[i];
    if (i + 1 < len && ((c == '\n' && data[i + 1] == '\r') ||
                        (c == '\r' && data[i + 1] == '\n'))) {
      ++i;
    }

    if (c == '\r') {
      c = '\n';
    }

    data[j] = c;
  }

  data.resize(j);
}

FileSource
FileSource::memory(string_view inp)
{
  string data(inp);
  replace_newlines(data);
  return FileSource("<memory>", move(data));
}

FileSource
FileSource::open(string_view filename)
{
  string str(filename);
  std::ifstream ifs = std::ifstream(str.c_str(),
                                    std::ifstream::in | std::ifstream::ate |
                                      std::ifstream::binary);

  if (!ifs.good()) {
    LOG(FATAL) << "Cannot open file " << filename;
  }

  ssize_t length = ifs.tellg();
  LOG(DEBUG) << "Read file " << filename << " with length " << length;
  ifs.seekg(0);

  string data = string(length, ' ');
  ifs.read(data.data(), length);
  replace_newlines(data);
  return FileSource(filename, move(data));
}
} // namespace extend::io
