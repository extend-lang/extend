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

DECLARE_PARSED_VALUE(typename io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Triplet)
DECLARE_PARSED_VALUE(typename io::EncodingTraits<ParserTraits<ParserType::TEXTCODE>::encoding>::Container)
DECLARE_PARSED_VALUE(intmax_t)
DECLARE_PARSED_VALUE(uintmax_t)
DECLARE_PARSED_VALUE(textcode::Dcl::Type)
DECLARE_PARSED_VALUE(textcode::VarDecl)
DECLARE_PARSED_VALUE(textcode::ParmTypes)
