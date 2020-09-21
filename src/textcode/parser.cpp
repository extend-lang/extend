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

namespace extend::textcode {

using Encoding = io::EncodingTraits<text::ParserTraits<text::ParserType::TEXTCODE>::encoding>;
using Triplet = Encoding::Triplet;

#define REQUIRE(X)                                     \
  if (!(X)) {                                          \
    return {};                                         \
  }
#define UNWRAP(NAME, VALUE)                            \
  const auto _TOKEN_##NAME##__LINE__ = (VALUE);        \
  REQUIRE(_TOKEN_##NAME##__LINE__);                    \
  const auto&(NAME) = _TOKEN_##NAME##__LINE__.value();

static text::ParsedExpr<Dcl::Type>
type(Parser& parser)
{
  auto typ = parser.token<TOK::KEYWORD>();
  REQUIRE(typ);

  if (typ.value() == U"int") {
    return {Dcl::Type::INT, typ.index()};
  }

  if (typ.value() == U"char") {
    return {Dcl::Type::CHAR, typ.index()};
  }

  return {};
}

static text::ParsedExpr<VarDecl>
var_decl(Parser& parser)
{
  auto id = parser.token<TOK::IDENTIFIER>();
  REQUIRE(id);

  vector<intmax_t> sizes;
  if (parser.symbol(Triplet('['))) {
    UNWRAP(size, parser.token<TOK::INTEGER_LITERAL>());
    sizes.push_back(size);
    REQUIRE(parser.symbol(Triplet(']')));
  }

  return {VarDecl(id.value(), sizes), id.index()};
}

static text::ParsedExpr<monostate>
dcl(Parser& parser)
{
  auto typ = parser.rule(type);
  REQUIRE(type);

  auto consumeVarDecl = [&]() -> bool {
    auto varDecl = parser.rule(var_decl);
    if (!varDecl)
      return false;

    parser.action([&](Prog& prog) {
      if (prog.haveGlobalVar(get<0>(varDecl.value()))) {
        parser.error(varDecl.index()) << "Duplicate global id";
      }

      prog.appendDcl(Dcl(typ.value(), get<0>(varDecl.value()), get<1>(varDecl.value())));
    });

    return true;
  };

  REQUIRE(consumeVarDecl());

  while (parser.symbol(Triplet(','))) {
    REQUIRE(consumeVarDecl());
  }

  REQUIRE(parser.symbol(Triplet(';')));

  return { monostate(), typ.index() };
}

static text::ParsedExpr<ParmTypes>
parm_types(Parser& parser)
{
  if (auto voidKw = parser.keyword(U"void")) {
    return {{}, voidKw.index()};
  }

  ParmTypes parameters;

  ssize_t startIndex = parser.idx();
  auto consumeParameter = [&]() -> bool {
    auto typ = parser.rule(type);
    if (!typ)
      return false;

    auto id = parser.token<TOK::IDENTIFIER>();
    if (!id)
      return false;

    intmax_t bracesCounter = 0;
    if (parser.symbol(Triplet('['))) {
      if (!parser.symbol(Triplet(']'))) {
        return false;
      }
      ++ bracesCounter;
    }

    parameters.emplace_back(typ.value(), id.value(), bracesCounter);
    return true;
  };

  if (consumeParameter()) {
    while (parser.symbol(Triplet(','))) {
      REQUIRE(consumeParameter());
    }

    return {move(parameters), startIndex};
  }

  return {};
}

static text::ParsedExpr<monostate>
func(Parser& parser)
{
  ssize_t startIndex = parser.idx();

  Dcl::Type retType = Dcl::Type::VOID;
  if (parser.keyword(U"void")) {
    // do nothing
  } else if (auto typ = parser.rule(type)) {
    retType = typ.value();
  } else {
    return {};
  }

  auto id = parser.token<TOK::IDENTIFIER>();
  REQUIRE(id);

  REQUIRE(parser.symbol(Triplet('(')));

  auto parameters = parser.rule(parm_types);
  REQUIRE(parameters);

  REQUIRE(parser.symbol(Triplet(')')));

  parser.action([&](Prog& prog) {
    if (prog.haveGlobalVar(id.value())) {
      parser.error(id.index()) << "Duplicate global id";
    }

    prog.appendFunc(move(Func(retType, id.value(), parameters.value())));
  });

  REQUIRE(parser.symbol(Triplet('{')));

  while (true) {
    auto typ = parser.rule(type);
    if (!typ) {
      break;
    }

    auto consumeVarDecl = [&]() -> bool {
      auto varDecl = parser.rule(var_decl);
      if (!varDecl)
        return false;

      parser.action([&](Prog& prog) {
        if (prog.currentFunc().haveVar(get<0>(varDecl.value()))) {
          parser.error(varDecl.index()) << "Duplicate local id";
        }

        prog.currentFunc().appendDcl(Dcl(typ.value(), get<0>(varDecl.value()), get<1>(varDecl.value())));
      });

      return true;
    };

    REQUIRE(consumeVarDecl());

    while (parser.symbol(Triplet(','))) {
      REQUIRE(consumeVarDecl());
    }

    REQUIRE(parser.symbol(Triplet(';')));
  }

  REQUIRE(parser.symbol(Triplet('}')));

  return {monostate(), startIndex};
}

static text::ParsedExpr<monostate>
prog(Parser& parser)
{
  while (parser.rule(dcl) || parser.rule(func)) {
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
ParserTraits<ParserType::TEXTCODE>::document(
  ParserStruct<ParserType::TEXTCODE>& parser)
{
  return parser.rule(textcode::prog);
}
}
