/* extend - file format and program language
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

#include "log.h"

#include <g3log/g3log.hpp>
#include <g3log/logworker.hpp>
#include <iostream>
#include <memory>

namespace extend::io {

static std::string
consoleFormatting(const g3::LogMessage& msg)
{
  if (msg._level.value == INFO.value) {
    return "";
  }

  return msg.letter();
}

ConsoleSink::ConsoleSink(std::ostream& out_)
  : out(out_)
{}

int
ConsoleSink::GetColor(const LEVELS& level)
{
  if (level.value == DEBUG.value) {
    return GREEN;
  }
  if (level.value == INFO.value) {
    return CYAN;
  }
  if (level.value == WARNING.value) {
    return YELLOW;
  }
  if (level.value == ERROR.value) {
    return RED;
  }
  if (g3::internal::wasFatal(level)) {
    return RED;
  }

  return RESET;
}

void
ConsoleSink::ReceiveLogMessage(g3::LogMessageMover logEntry) const
{
  auto level = logEntry.get()._level;
  auto color = GetColor(level);

  out << "\033[" << color << "m" << logEntry.get().toString(consoleFormatting)
      << "\033[m";
}

Logger::Logger(bool is_capture)
  : worker(g3::LogWorker::createLogWorker())
  , handle(worker->addSink(
      std::make_unique<ConsoleSink>(is_capture ? capture : std::cerr),
      &ConsoleSink::ReceiveLogMessage))
{
  std::ios_base::sync_with_stdio(false);
  std::cin.tie(nullptr);

  g3::initializeLogging(worker.get());
}

Logger::~Logger()
{
  g3::internal::shutDownLogging();
}
}
