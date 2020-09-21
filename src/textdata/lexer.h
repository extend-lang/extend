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

#include "ast.h"
#include "text/LexerStruct.h"
#include "text/ParserTraits.h"
#include <EASTL/string.h>
#include <EASTL/unordered_set.h>

using namespace eastl;

namespace extend::text {
/**
 * Token type
 */
template<>
enum struct ParserTraits<ParserType::TEXTDATA>::TOK {
#define DECLARE_TOKEN(NAME, TYPE) NAME,
#include "token_types.h"
#undef DECLARE_TOKEN
};

template<>
constexpr bool ParserTraits<ParserType::TEXTDATA>::isIndentSupported = true;

#define DECLARE_TOKEN(NAME, TYPE)                                              \
  template<>                                                                   \
  template<>                                                                   \
  struct ParserTraits<ParserType::TEXTDATA>::UnderlyingType<                   \
    ParserTraits<ParserType::TEXTDATA>::TOK::NAME>                             \
  {                                                                            \
    using Type = TYPE;                                                         \
  };
#include "token_types.h"
#undef DECLARE_TOKEN

template<>
struct ParserTraits<ParserType::TEXTDATA>::ParsedValue
{
  using FromLexer =
    variant<
#define DECLARE_PARSED_VALUE(TYPE) TYPE,
#include "lexer.parsed_values.h"
#undef DECLARE_PARSED_VALUE
    monostate>;
  using FromParser =
    variant<
#define DECLARE_PARSED_VALUE(TYPE) ParsedExpr<TYPE>,
#include "lexer.parsed_values.h"
#undef DECLARE_PARSED_VALUE
    ParsedExpr<monostate>>;
};

}

namespace extend::textdata {
using TOK = text::ParserTraits<text::ParserType::TEXTDATA>::TOK;
}
