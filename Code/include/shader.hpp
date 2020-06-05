#pragma once

#include "common.hpp"

const char*
loadShaderFile(const char* filename);

// loads a shader source file, tries to compile the shader
// and checks for compilation errors
unsigned int
compileShader(const char* filename, unsigned int type);

unsigned int
linkProgram(unsigned int computeShader);
unsigned int
linkProgram(unsigned int vertexShader, unsigned int fragmentShader);
unsigned int
linkProgram(unsigned int vertexShader, unsigned int fragmentShader, unsigned int tesselationShader, unsigned int tesselationEShader);