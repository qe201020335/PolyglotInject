// Include guard - https://en.wikipedia.org/wiki/Pragma_once
#pragma once

// Include the logging framework
#include "paper2_scotland2/shared/logger.hpp"

// Define a logger that will associate all messages with our mod
constexpr auto logger = Paper::ConstLoggerContext(MOD_ID);
