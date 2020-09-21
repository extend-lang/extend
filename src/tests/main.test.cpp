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

#define CATCH_CONFIG_RUNNER
#include <catch2/catch.hpp>

#include "io/log.h"

#include <EASTL/optional.h>
#include <cstdlib>
#include <exception>
#include <iostream>

using namespace eastl;
using namespace extend;

static optional<io::Logger> logger;

static void
terminate_test()
{
  std::cerr << logger->capture.str();
  abort();
}

int
main(int argc, char** argv)
{
  logger.emplace(true);
  std::set_terminate(terminate_test);

  int returnCode = Catch::Session().run(argc, argv);

  std::set_terminate(abort);
  logger.reset();

  return returnCode;
}
