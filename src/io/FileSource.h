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

#include <EASTL/string.h>
#include <EASTL/string_view.h>

using namespace eastl;

namespace extend::io {

class FileSource final
{
private:
  FileSource(string_view _filename, string&& _data);

public:
  string_view filename;
  string data;

  ~FileSource() = default;

  FileSource(FileSource&&) = default;
  FileSource(const FileSource&) = delete;
  FileSource& operator=(FileSource&&) = default;
  FileSource& operator=(const FileSource&) = delete;

  static FileSource memory(string_view inp);
  static FileSource open(string_view filename);
};
}
