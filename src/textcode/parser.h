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
#include "lexer.h"
#include "text/ParserStruct.h"
#include "text/ParserTraits.h"
#include "ast.h"

#include <EASTL/map.h>

using namespace eastl;

namespace extend::textcode {
using Parser = text::ParserStruct<text::ParserType::TEXTCODE>;
}

namespace extend::text {
template<>
struct ParserTraits<ParserType::TEXTCODE>::Document
{
  using Type = textcode::Prog;
};
}
