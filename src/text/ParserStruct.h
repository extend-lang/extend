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
#include <EASTL/internal/functional_base.h>
#include <EASTL/variant.h>
#include <io/prelude.h>

#include "reader.h"
#include "text/LexerStruct.h"

#include <unordered_map.hpp>
#include <cassert>

namespace extend::text {

template<typename T>
struct ParsedExpr
{
private:
  optional<const T> val;
  ssize_t idx = -1;

public:
  ParsedExpr() = default;
  ParsedExpr(T &&value, ssize_t index)
    : val(eastl::move(value))
    , idx(index)
  {}

  operator bool() const { return idx >= 0; }

  [[nodiscard]] const T& value() const
  {
    assert(idx >= 0);
    assert(val.has_value());
    return val.value();
  }

  [[nodiscard]] ssize_t index() const
  {
    assert(idx >= 0);
    assert(val.has_value());
    return idx;
  }
};

/**
 * Parser.
 *
 * It get info from lexer and make packrat-based parsing.
 */
template<ParserType t>
struct ParserStruct
{
  static constexpr ParserType parser_type = t;

  using TOK = typename ParserTraits<parser_type>::TOK;
  using Lexer = LexerStruct<parser_type>;
  using Parser = ParserStruct<parser_type>;
  using Token = typename Lexer::Token;
  using Traits = ParserTraits<parser_type>;
  using Document = typename Traits::Document::Type;
  using FromParser = typename Traits::ParsedValue::FromParser;

  static constexpr io::EncodingType encoding = Traits::encoding;
  using Encoding = io::EncodingTraits<encoding>;
  using Triplet = typename Encoding::Triplet;
  using Container = typename Encoding::Container;
  using Span = typename Encoding::Span;

  ParserStruct(const ParserStruct&) = delete;
  ParserStruct(ParserStruct&&) = delete;
  ParserStruct& operator=(const ParserStruct&) = delete;
  ParserStruct& operator=(ParserStruct&&) = delete;
  ~ParserStruct() = default;

  ParserStruct(io::FileSource<encoding>&& source);
  [[nodiscard]] Document& doc();

  [[nodiscard]] typename Reader<encoding>::ReaderError error(ssize_t idx);
  [[nodiscard]] typename Reader<encoding>::ReaderNote note(ssize_t idx) const;
  [[nodiscard]] ssize_t errorCount() const;

  [[nodiscard]] bool eof() const;

  // Lean calculation (PACKRAT)
  template<typename F>
  auto rule(F func)
  {
    using InvokeRes = invoke_result_t<F, decltype(*this)>;

    // Try to find a value in history
    ParsingCoord currentCoord {reinterpret_cast<void *>(func), index};
    auto previousRes = parsingHistory.find(currentCoord);

    if (previousRes != parsingHistory.end()) { // We found it
      if (mode == Mode::CHECK ||               // If we are in check mode
        !visit([](const auto &expr) -> bool {  // or failed
          return bool(expr);
        }, get<0>(previousRes->second))
      ) {
        assert(holds_alternative<InvokeRes>(get<0>(previousRes->second)));
        index = get<1>(previousRes->second);
        return get<InvokeRes>(get<0>(previousRes->second)); // return value from history
      }
    }

    auto res = func(*this); // Else try to parse with function
    if (!res) {
      index = currentCoord.second; // Revert index on fail
    }
    parsingHistory.insert({currentCoord, move(tuple<FromParser, ssize_t>(move(FromParser(res)), index))}); // Save the result in history

    return res;
  }

  template<TOK type>
  ParsedExpr<typename Traits::template UnderlyingType<type>::Type> token()
  {
    using Type = typename Traits::template UnderlyingType<type>::Type;
    if (eof()) {
      return {};
    }

    if (lexer[index].type == type) {
      assert(holds_alternative<Type>(lexer[index].value));
      ssize_t currentIndex = index++;
      auto value = get<Type>(lexer[currentIndex].value);
      return { move(value), currentIndex };
    }
    return {};
  }

  template<TOK tok_type = TOK::SYMBOL>
  ParsedExpr<Triplet>
  symbol(Triplet sym)
  {
    if (eof()) {
      return {};
    }

    if (lexer[index].type == tok_type &&
        get<Triplet>(lexer[index].value) == sym) {
      return { move(sym), index++ };
    }
    return {};
  }

  template<TOK tok_type = TOK::KEYWORD>
  ParsedExpr<Container>
  keyword(Span kwd)
  {
    if (eof()) {
      return {};
    }

    if (lexer[index].type == tok_type &&
        get<Container>(lexer[index].value) == kwd) {
      return { move(Container(kwd)), index++ };
    }

    return {};
  }

  template<typename T>
  void action(T function)
  {
    if (mode == Mode::PARSE) {
      function(document);
    }
  }

  [[nodiscard]] ssize_t idx() const
  {
    return index;
  }

private:
  Lexer lexer;
  Document document = {};
  ssize_t index = 0;

  enum struct Mode {
    CHECK,
    PARSE
  };

  Mode mode = Mode::CHECK;
  using ParsingCoord = pair<void *, ssize_t>;
  struct CoordHash {
    size_t operator()(const ParsingCoord &coords) const {
      return size_t(coords.first) ^ size_t(coords.second);
    }
  };
  ska::unordered_map<ParsingCoord, tuple<FromParser, ssize_t>, CoordHash> parsingHistory;
};
}
