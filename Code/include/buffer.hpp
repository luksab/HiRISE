#pragma once

#include "common.hpp"

unsigned int
makeBuffer(unsigned int bufferType, GLenum usageHint, unsigned int bufferSize = 0, void* data = nullptr);
