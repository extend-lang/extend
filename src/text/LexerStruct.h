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

#include "ParserTraits.h"
#include "reader.h"

#include <cassert>

using namespace eastl;

namespace extend::text {

/**
 * Record about one Token.
 */
template<ParserType t>
struct TokenRecord
{
  static constexpr ParserType parser_type = t;

  using TOK = typename ParserTraits<parser_type>::TOK;
  using FromLexer = typename ParserTraits<parser_type>::ParsedValue::FromLexer;
  using Encoding = io::EncodingTraits<ParserTraits<parser_type>::encoding>;

  TOK type;
  FromLexer value;
  ssize_t pos = 0;
  typename Encoding::Span input = Encoding::zeroString;

  /**
   * Create token without binding to source code.
   * For internal purpose only.
   */
  TokenRecord(TOK type_, FromLexer&& value_);

  /**
   * Create token from source file.
   */
  TokenRecord(TOK type_, FromLexer&& value_, ssize_t pos_, typename Encoding::Span input_);

  /**
   * Check for equal TokenRecord::type and TokenRecord::value
   */
  bool operator==(const TokenRecord& rhs) const;

  /**
   * Check for not equal TokenRecord::type and TokenRecord::value
   */
  bool operator!=(const TokenRecord& rhs) const;
};

/**
 * Record about one brace.
 */
template<ParserType t>
struct BraceRecord
{
  static constexpr ParserType parser_type = t;

  using TOK = typename ParserTraits<parser_type>::TOK;
  using Token = TokenRecord<parser_type>;

  BraceRecord(Token&& token_,
              bool openingBrace_,
              optional<Token>&& closingBrace_);

  Token token;
  bool openingBrace;
  optional<Token> closingBrace;
};

/**
 * Print token to ostream
 */
template<typename Traits, ParserType t>
std::basic_ostream<char, Traits>&
operator<<(std::basic_ostream<char, Traits>& out, const TokenRecord<t>& token);

/**
 * Universal lexer
 *
 * Usage example:
 * \code{.cpp}
 *   auto source = FileSource<>::open("somefile");
 *   LexerStruct<ParserType::TEXTDATA> lexer(move(source));
 * \endcode
 */
template<ParserType t>
struct LexerStruct
{
  static constexpr ParserType parser_type = t;

  using TOK = typename ParserTraits<parser_type>::TOK;
  using Token = TokenRecord<parser_type>;
  using Brace = BraceRecord<parser_type>;
  using Lexer = LexerStruct<parser_type>;
  using Traits = ParserTraits<parser_type>;
  constexpr static io::EncodingType encoding = Traits::encoding;
  using Encoding = io::EncodingTraits<encoding>;
  using Container = typename Encoding::Container;
  using Span = typename Encoding::Span;
  using CodePoint = typename Encoding::CodePoint;
  using Triplet = typename Encoding::Triplet;

  /**
   * Capture Token with all contents until predicate is true.
   *
   * \param predicate Predicate to check if we should capture the next char
   *
   * \param peek_len How many chars need for predicate.
   *
   * \tparam close_required Should panic if eof happens before predicate false?
   *
   * \tparam begin_capture How much symbols at start should be captured without
   * any checks?
   *
   * \tparam end_capture Should capture end_marker?
   */
  template<bool close_required = false,
           ssize_t begin_capture = 0,
           bool end_capture = false,
           typename Predicate>
  optional<Token> sequence(Predicate predicate, ssize_t peek_len = 1)
  {
    if (predicate(reader.peek(peek_len)) || (begin_capture > 0)) {
      ssize_t pos = reader.tellp();
      for (ssize_t i = 0; i < begin_capture; ++i) {
        reader.get();
      }

      while (!reader.eof() && predicate(reader.peek(peek_len))) {
        reader.get();
      }

      if (close_required && reader.eof()) {
        return optional<Token>();
      }

      if (end_capture) {
        for (ssize_t i = 0; i < peek_len; ++i) {
          reader.get();
        }
      }

      return make_optional<Token>(
        TOK::EMPTY, monostate(), pos, reader.substr(pos, reader.tellp() - pos));
    }

    return optional<Token>();
  }

  /**
   * Capture Token with all contents until the end_marker.
   *
   * \tparam end_marker Where we should stop creating a token. Can be either
   * char, or result of function s().
   *
   * \tparam close_required Should panic if eof happens before end_marker?
   *
   * \tparam begin_capture How much symbols at start should be captured without
   * any checks?
   *
   * \tparam end_capture Should capture end_marker?
   */
  template<bool close_required = false,
           ssize_t begin_capture = 0,
           bool end_capture = false>
  optional<Token> until(Triplet end_marker)
  {
    assert(end_marker != Triplet::eof && end_marker.size() > 0);
    auto isContinue = [end_marker](Triplet c) -> bool { return c != end_marker; };
    return sequence<close_required, begin_capture, end_capture>(isContinue, end_marker.size());
  }

  /**
   * Get the reader. Usefull for get the chars.
   *
   * \returns internal reader
   */
  [[nodiscard]] Reader<encoding>& r();
  [[nodiscard]] typename Reader<encoding>::ReaderError error(ssize_t index);
  [[nodiscard]] typename Reader<encoding>::ReaderNote note(ssize_t index) const;
  [[nodiscard]] ssize_t errorCount() const;

  /**
   * Parse source file.
   *
   * After construct you should check errorCount(). If it is zero, then you can
   * use tokens() vector.
   *
   * \param source file source
   */
  LexerStruct(io::FileSource<encoding>&& source);

  LexerStruct(const LexerStruct&) = delete;
  LexerStruct(LexerStruct&&) = delete;
  LexerStruct& operator=(const LexerStruct&) = delete;
  LexerStruct& operator=(LexerStruct&&) = delete;
  ~LexerStruct() = default;

  /**
   * Get vector of captured tokens.
   */
  [[nodiscard]] const vector<Token>& tokens() const { return data; }

  void removeIgnored();

  const Token& operator[](ssize_t index) const;

private:
  struct Indent final
  {
    Span input;
    ssize_t pos;

    Indent(Span input_, ssize_t pos_);
  };

  Reader<encoding> reader;
  vector<Token> data = {};
  vector<Indent> indents = { Indent(Encoding::zeroString, 0) };

  /**
   * Emplace indent and add token.
   *
   *  \param currentIndent spaces in the begin of a line
   */
  bool emplaceIndent(Span currentIndent);

  /**
   * Emplace idendent and add token.
   */
  void emplaceDeindent();

  /**
   * Print Reader<>::error() about unexpected indent
   */
  bool unexpectedIndent();

  /**
   * Parse indent in a new line.
   *
   * \param currentIndent spaces in the begin of a line
   */
  bool parseIndent(Span currentIndent);
};
}
