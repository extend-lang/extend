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

#include "text/ParserStruct.h"
#include "textdata/parser.h"
#include "textcode/parser.h"
#include <EASTL/unique_ptr.h>
#include <g3log/g3log.hpp>

namespace extend::text {

template<ParserType t>
ParserStruct<t>::ParserStruct(
  io::FileSource<encoding>&& source)
  : lexer(move(source))
{
  lexer.removeIgnored();

  // Evaluating the document
  if (!rule(Traits::document)) {
    error(index) << "cannot parse document";
  }

  if (!eof()) {
    error(index) << "unexpected token, expected EOF";
  }

  // Actual parsing
  if (!errorCount()) {
    mode = Mode::PARSE;
    index = 0;
    rule(Traits::document);
  }

  if (errorCount()) {
    LOG(FATAL) << "Parsing failed because of previous errors";
  }
}

template<ParserType t>
typename ParserStruct<t>::Document&
ParserStruct<t>::doc()
{
  return document;
}

template<ParserType t>
typename Reader<ParserStruct<t>::encoding>::ReaderError
ParserStruct<t>::error(ssize_t idx)
{
  return lexer.error(idx);
}

template<ParserType t>
typename Reader<ParserStruct<t>::encoding>::ReaderNote
ParserStruct<t>::note(ssize_t idx) const
{
  return lexer.note(idx);
}

template<ParserType t>
ssize_t
ParserStruct<t>::errorCount() const
{
  return lexer.errorCount();
}

template<ParserType t>
bool
ParserStruct<t>::eof() const
{
  return lexer.tokens().size() == size_t(index);
}

#define DECLARE_PARSER(NAME) template struct ParserStruct<ParserType::NAME>;
#include "parser_types.h"
#undef DECLARE_PARSER
}
