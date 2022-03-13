/* extend - expansible programming language
 * Copyright (C) 2022 Vladimir Liutov vs@lutov.net
 * Copyright (C) 2015 THL A29 Limited, a Tencent company, and Milo Yip.
 * Source: https://github.com/Tencent/rapidjson
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

/** This is a C++ header-only implementation of Grisu2 algorithm from the
 * publication: Loitsch, Florian. "Printing floating-point numbers quickly and
 * accurately with integers." ACM Sigplan Notices 45.6 (2010): 233-243.
 */

#pragma once

#include <cinttypes>

namespace extend::log {

/** Prettify print.
 * For example convert 1234e-2 -> 12.34
 *
 * @param  buffer   String should be at least 24 char length.
 *                  Contain mantissa in form 1234
 * @param  length   Length of mantissa.
 * @param  exponent Exponent, could be positive and negative.
 * @return          Count of rewritten chars.
 */
int16_t
dtoa_prettify(char8_t* buffer, int16_t length, int16_t exponent);

/** Write float representation into string.
 * @param  x      Floating point value.
 * @param  buffer String should be at least 24 char length.
 * @return        Count of written chars.
 */
int
dtoa(double x, char8_t* buffer);

/** Write float representation into string.
 * @param  x      Floating point value.
 * @param  buffer String should be at least 24 char length.
 * @return        Count of written chars.
 */
int
ftoa(float x, char8_t* buffer);

} // namespace extend::log
