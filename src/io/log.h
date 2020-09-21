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

#include <EASTL/functional.h>
#include <g3log/logmessage.hpp>
#include <sstream>

using namespace eastl;

namespace g3 {
class LogWorker;

template<typename Sink>
class SinkHandle;
} // namespace g3

namespace extend::io {

enum COLORS
{
  // Linux xterm color
  // https://stackoverflow.com/questions/2616906/how-do-i-output-coloured-text-to-a-linux-terminal
  RESET = 0,
  RED = 31,
  GREEN = 32,
  YELLOW = 33,
  BLUE = 34,
  MAGENTA = 35,
  CYAN = 36,
  WHITE = 37
};

/**
 * Console sing to stderr.
 * For internal use.
 *
 * \internal
 */
struct ConsoleSink final
{
  std::ostream& out;
  ConsoleSink(std::ostream& out);

  static int GetColor(const LEVELS& level);
  void ReceiveLogMessage(g3::LogMessageMover logEntry) const;
};

struct Logger final
{
  std::stringstream capture;
  std::unique_ptr<g3::LogWorker> worker;
  std::unique_ptr<g3::SinkHandle<ConsoleSink>> handle;

  Logger(bool is_capture = false);
  ~Logger();

  Logger(const Logger&) = delete;
  Logger(Logger&&) = delete;
  Logger& operator=(const Logger&) = delete;
  Logger& operator=(Logger&&) = delete;
};
} // namespace extend::io
