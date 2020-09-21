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

#include <EASTL/optional.h>
#include <EASTL/string.h>
#include <EASTL/variant.h>

#include "io/FileSource.h"

using namespace eastl;

namespace extend::text {

/**
 * Full list of all parser types
 */
enum struct ParserType
{
#define DECLARE_PARSER(NAME) NAME,
#include "parser_types.h"
#undef DECLARE_PARSER
};

template<ParserType t>
struct TokenRecord;

template<ParserType t>
struct BraceRecord;

template<ParserType t>
struct LexerStruct;

template<ParserType t>
struct ParserStruct;

template<typename T>
struct ParsedExpr;

template<ParserType t>
struct ParserTraits
{
  static constexpr ParserType parser_type = t;
  static constexpr io::EncodingType encoding = io::EncodingType::UTF8;

  enum struct TOK;
  struct ParsedValue
  {
    using FromLexer = variant<monostate>;
    using FromParser = variant<ParsedExpr<monostate>>;
  };

  template<TOK type>
  struct UnderlyingType
  {
    using Type = void;
  };

  using Token = TokenRecord<parser_type>;
  using Brace = BraceRecord<parser_type>;
  using Lexer = LexerStruct<parser_type>;
  using Parser = ParserStruct<parser_type>;

  /**
   * Check if word is a keyword.
   *
   * \param word possible keyword
   * \return true or false
   */
  static bool isKeyword(typename io::EncodingTraits<encoding>::Span word);

  /**
   * Get string name of TOK.
   *
   * \param ttype token type
   * \return human readable representation
   */
  static const char* typeAsStr(TOK ttype);

  /**
   * Parse a comment.
   *
   * \param lexer text lexer
   * \return nullopt if we haven't a comment under cursor
   * \return COMMENT if we have a comment under cursor
   * \return PARSER_ERROR if an error has been occured while read the comment
   */
  static optional<Token> parseComment(Lexer& lexer);

  /**
   * Parse a symbol.
   *
   * \param lexer text lexer
   * \return nullopt if we haven't a symbol under cursor
   * \return SYMBOL if we have a symbol under cursor
   * \return PARSER_ERROR if an error has been occured while read the symbol
   */
  static optional<Token> parsePlainSymbol(Lexer& lexer);

  /**
   * Parse a brace.
   *
   * \param lexer current reader
   * \return nullopt if we haven't brace under cursor
   * \return brace if we have
   */
  static optional<Brace> parseBrace(Lexer& lexer);

  static bool ignore(const Token& tok);

  static constexpr bool isIndentSupported =
    false; //!< override in trait implementation

  /**
   * Parsed document type.
   */
  struct Document;

  /**
   * Root parse rule.
   */
  static ParsedExpr<monostate> document(Parser& parser);
};

}
