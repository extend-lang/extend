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

/** \file
 * In this file declared all tokens in textcode parser.
 *
 * If you need add more tokens, add the new call to DECLARE_TOKEN below.
 */

DECLARE_TOKEN(EMPTY, monostate)
DECLARE_TOKEN(PARSER_ERROR, monostate)
DECLARE_TOKEN(SPACE, io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Container)
DECLARE_TOKEN(IDENTIFIER, io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Container)
DECLARE_TOKEN(KEYWORD, io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Container)
DECLARE_TOKEN(SYMBOL, io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Triplet)
DECLARE_TOKEN(COMMENT, io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Container)
DECLARE_TOKEN(INDENT, monostate)
DECLARE_TOKEN(DEINDENT, monostate)
DECLARE_TOKEN(INTEGER_LITERAL, intmax_t)
DECLARE_TOKEN(STRING, io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Container)
