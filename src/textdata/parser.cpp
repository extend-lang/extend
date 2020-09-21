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

#include "parser.h"
#include "text/ParserTraits.h"
#include <EASTL/optional.h>
#include <EASTL/string_view.h>

namespace extend::textdata {

using Encoding = io::EncodingTraits<text::ParserTraits<text::ParserType::TEXTDATA>::encoding>;
using Triplet = Encoding::Triplet;

#define REQUIRE(X)                                                             \
  if (!(X)) {                                                                  \
    return {};                                                                 \
  }
#define UNWRAP(NAME, VALUE)                                                    \
  const auto* _TOKEN_##NAME##__LINE__ = (VALUE);                               \
  REQUIRE(_TOKEN_##NAME##__LINE__);                                            \
  const auto&(NAME) = *_TOKEN_##NAME##__LINE__;

static text::ParsedExpr<Field::VariantConst>
rvalue(Parser& parser)
{
  text::ParsedExpr<intmax_t> i32 = parser.token<TOK::INTEGER_LITERAL>();
  if (i32) {
    return { Field::VariantConst(i32.value()), i32.index() };
  }

  text::ParsedExpr<Encoding::Container> s = parser.token<TOK::STRING>();
  if (s) {
    return { Field::VariantConst(s.value()), s.index() };
  }

  return {};
}

static text::ParsedExpr<monostate>
assignment(Parser& parser)
{
  text::ParsedExpr<Encoding::Container> id = parser.token<TOK::IDENTIFIER>();
  REQUIRE(id);
  parser.action([&](Document& doc) {
    if (doc.contains(id.value())) {
      parser.error(id.index()) << "Duplicate id";
    }
  });

  REQUIRE(parser.symbol(Triplet('=')));

  text::ParsedExpr<Field::VariantConst> val = parser.rule(rvalue);
  REQUIRE(val);
  parser.action([&](Document& doc) { 
    Field::VariantConst fieldValue = val.value();
    if (holds_alternative<Encoding::Container>(fieldValue)) {
      doc.appendField(Field::Type::STRING, id.value(), move(fieldValue));
    } else if (holds_alternative<intmax_t>(fieldValue)) {
      doc.appendField(Field::Type::I32, id.value(), move(fieldValue));
    } else if (holds_alternative<uintmax_t>(fieldValue)) {
      doc.appendField(Field::Type::U32, id.value(), move(fieldValue));
    } else {
      LOG(FATAL) << "unexpected variant type";
    }
  });

  return { monostate(), id.index() };
}

static text::ParsedExpr<monostate>
document(
  text::ParserStruct<text::ParserType::TEXTDATA>& parser)
{
  while (parser.rule(textdata::assignment)) {
    // do nothing
  }

  return { monostate(), 0 };
}

#undef REQUIRE
#undef UNWRAP
}

namespace extend::text {
template<>
ParsedExpr<monostate>
ParserTraits<ParserType::TEXTDATA>::document(
  ParserStruct<ParserType::TEXTDATA>& parser)
{
  return parser.rule(textdata::document);
}
}
