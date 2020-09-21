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

DECLARE_TOKEN(EMPTY, monostate)
DECLARE_TOKEN(SPACE, string)
DECLARE_TOKEN(IDENTIFIER, string)
DECLARE_TOKEN(KEYWORD, string)
DECLARE_TOKEN(SYMBOL, int32_t)
DECLARE_TOKEN(COMMENT, string)
DECLARE_TOKEN(INDENT, monostate)
DECLARE_TOKEN(DEINDENT, monostate)
DECLARE_TOKEN(I32, int32_t)
