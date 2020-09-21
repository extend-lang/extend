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

#include <io/log.h>
#include <textdata/lexer.h>
#include <textdata/reader.h>

#include <EASTL/string.h>
#include <g3log/g3log.hpp>
#include <iostream>
#include <lyra/lyra.hpp>

using namespace eastl;

namespace extend::pretty {

static string input;
static string output;

static int
parse_args(int argc, const char** argv)
{
  bool show_help = false;

  auto cli = lyra::help(show_help)("Prettify extend text data") |
             lyra::arg(input, "input")("Source file, - for stdin").required() |
             lyra::opt(output, "output")["-o"]["--output"](
               "Ouput file, stdout for default");
  auto res = cli.parse({ argc, argv });

  if (!res) {
    LOG(ERROR) << res.errorMessage();
    LOG(INFO) << cli;
    return 1;
  }

  if (show_help) {
    LOG(INFO) << cli;
  }

  return 0;
}

} // namespace extend::pretty

int
main(int argc, const char** argv)
{
  using namespace extend;
  using namespace extend::pretty;

  io::Logger logger(false);

  int ret_code = parse_args(argc, argv);
  if (ret_code != 0) {
    return ret_code;
  }

  io::FileSource source = io::FileSource::open(input);
  textdata::Lexer lexer(move(source));

  if (lexer.reader.errors != 0) {
    LOG(FATAL) << "Found " << lexer.reader.errors
               << " while lexical analysis of " << lexer.reader.source.filename;
  }

  return ret_code;
}
