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

#include <catch2/catch.hpp>
#include <g3log/g3log.hpp>

namespace extend::tests {

struct FatalMessageMock
{
  enum struct Require
  {
    DENY,
    ALLOWED,
    REQUIRED
  };

  Require requireFatal;

  bool wasFatal = false;
  std::function<void(g3::FatalMessagePtr)> prev_handler;

  FatalMessageMock(FatalMessageMock&&) = delete;
  FatalMessageMock(const FatalMessageMock&) = delete;
  FatalMessageMock& operator=(FatalMessageMock&&) = delete;
  FatalMessageMock& operator=(const FatalMessageMock&) = delete;

  FatalMessageMock(Require require_fatal = Require::REQUIRED)
    : requireFatal(require_fatal)
  {
    prev_handler = g3::setFatalExitHandler([this](auto /*unused*/) {
      REQUIRE(requireFatal != Require::DENY);
      wasFatal = true;
    });
  }

  ~FatalMessageMock()
  {
    if (requireFatal == Require::REQUIRED) {
      REQUIRE(wasFatal);
    }
    g3::setFatalExitHandler(prev_handler);
  }
};

}
