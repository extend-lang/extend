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

#include "lexer.h"

#include <EASTL/utility.h>
#include <g3log/g3log.hpp>

#include <cassert>
#include <cctype>

namespace extend::textdata {

const char*
Token::typeAsStr() const
{
  switch (this->type) {
#define DECLARE_TOKEN(NAME, VALUE_TYPE)                                        \
  case TOK::NAME:                                                              \
    return #NAME;
#include "tokens.h"
#undef DECLARE_TOKEN
  }
}

Token::Token(TOK type_, TokenValue&& value_)
  : type(type_)
  , value(move(value_))
{}

Token::Token(TOK type_,
             TokenValue&& value_,
             ssize_t pos_,
             string&& input_)
  : type(type_)
  , value(move(value_))
  , pos(pos_)
  , input(move(input_))
{}

bool
Token::operator==(const Token& rhs) const
{
  return (this->type == rhs.type) && (this->value == rhs.value);
}

bool
Token::operator!=(const Token& rhs) const
{
  return !(*this == rhs);
}

Lexer::Indent::Indent(string&& input_, ssize_t pos_)
  : input(input_)
  , pos(pos_)
{}

void
Lexer::emplaceDeindent()
{
  data.emplace_back(TOK::DEINDENT, monostate(), reader.offset, "");
  indents.pop_back();
}

bool
Lexer::emplaceIndent(const string &input)
{
  data.emplace_back(TOK::INDENT, monostate(), reader.offset, "");
  indents.emplace_back(string(input), reader.offset);
  return true;
}

bool
Lexer::unexpectedIndent()
{
  assert(!indents.empty());
  reader.error(reader.offset)
    << "unexpected indent" << reader.note(indents.back().pos)
    << "to match this indent";
  return false;
}

// parse spaces in new line
bool
Lexer::parseIndent(const string &input)
{
  // close indents on eof
  if (reader.eof()) {
    while (!indents.empty()) {
      emplaceDeindent();
    }
    return true;
  }

  // ignore empty lines: \n after spaces
  if (reader.check<'\n'>()) {
    return true;
  }

  // Real line, don't ignore

  // No current indents
  if (indents.empty()) {
    if (!input.empty()) {
      return emplaceIndent(input);
    }
    return true;
  }

  // We already have several current indents
  const string& lastIndent = indents.back().input;

  // indent
  if (input.size() > lastIndent.size()) {
    if (input.substr(0, lastIndent.size()) != lastIndent) {
      return unexpectedIndent();
    }

    return emplaceIndent(input);
  }

  // deindent
  vector<Indent>::reverse_iterator it;
  for (it = indents.rbegin(); it != indents.rend(); ++it) {
    if (it->input == input) {
      break;
    }
    if (it->input.size() < input.size() ||
        it->input.substr(0, input.size()) != input) {
      return unexpectedIndent();
    }
  }

  if (it == indents.rend() && !input.empty()) {
    return unexpectedIndent();
  }

  ssize_t count = it - indents.rbegin();
  for (ssize_t i = 0; i < count; ++i) {
    emplaceDeindent();
  }

  return true;
}

Lexer::Lexer(io::FileSource&& source)
  : reader(move(source))
{
  optional<Token> token;
  vector<pair<ssize_t, int32_t>> braces;
  bool isIndentParsing = true;
  optional<ssize_t> indentBrokenBy;
  string currentIndent;

  while (!reader.eof()) {
    char c = reader.peek();

    if (c == '\n') {
      if (braces.empty()) {
        isIndentParsing = true;
        indentBrokenBy.reset();
        currentIndent = "";
      }
      data.emplace_back(TOK::SYMBOL,
                        reader.get(),
                        reader.offset,
                        reader.substr(reader.offset, 1));
      continue;
    }

    if ((token = sequence(isblank)).has_value()) {
      token->type = TOK::SPACE;
      token->value = token->input;
      data.push_back(*move(token));

      if (isIndentParsing && currentIndent.empty()) {
        currentIndent = data.back().input;
      }
      continue;
    }

    if (reader.check<s('/', '*')>()) {
      ssize_t pos = reader.offset;
      token = until<s('*', '/'), true, 2, true>();
      if (!token.has_value()) {
        reader.error(reader.offset) << "unexpected eof, expected `*/`"
                                    << reader.note(pos) << "to match this `/*`";
        return;
      }
      token->type = TOK::COMMENT;
      token->value = token->input;
      data.push_back(*move(token));
      if (isIndentParsing) {
        indentBrokenBy = pos;
      }
      continue;
    }

    if (c == '#' || reader.check<s('/', '/')>()) {
      token = until<'\n'>();
      assert(token.has_value());
      token->type = TOK::COMMENT;
      token->value = token->input;
      data.push_back(*move(token));
      continue;
    }

    // Real code: not comment or new line
    if (isIndentParsing) {
      if (indentBrokenBy.has_value()) {
        reader.error(indentBrokenBy.value())
          << "indent broken by multuline comment";
        return;
      }
      if (!parseIndent(currentIndent)) {
        return; // error occured
      }
      isIndentParsing = false;
      continue;
    }

    if (string_view("+-*/()").find(c) != string_view::npos) {
      if (c == '(') {
        braces.emplace_back(reader.offset, c);
      } else if (c == ')') {
        if (!braces.empty() && braces.back().second == '(') {
          braces.pop_back();
        } else {
          reader.error(reader.offset)
            << "Unexpected closing brace `" << c << "`";
        }
      }
      data.emplace_back(TOK::SYMBOL,
                        reader.get(),
                        reader.offset,
                        reader.substr(reader.offset, 1));
      continue;
    }

    if ((token = sequence(isdigit)).has_value()) {
      token->type = TOK::I32;
      token->value = atoi(token->input.c_str());
      data.push_back(*move(token));
      continue;
    }

    if (isalpha(c) != 0) {
      token = sequence(isalnum);
      assert(token.has_value());

      string lower = token->input;
      lower.make_lower();
      if (KEYWORDS.find(lower) != KEYWORDS.end()) {
        token->type = TOK::KEYWORD;
        token->value = lower;
      } else {
        token->type = TOK::IDENTIFIER;
        token->value = token->input;
      }

      data.push_back(*move(token));
      continue;
    }

    reader.error(reader.offset) << "Unexpected char `" << c << "`";
    reader.get();
  }

  if (!braces.empty()) {
    reader.error(reader.offset)
      << "Unexpected eof, expected close brace"
      << reader.note(braces.back().first) << "to match this `"
      << char(braces.back().second) << "`";
  }

  while (!indents.empty()) {
    emplaceDeindent();
  }
}
}
