#include <buffer.hpp>

unsigned int
makeBuffer(unsigned int bufferType, GLenum usageHint, unsigned int bufferSize, void* data) {
    unsigned int handle;
    glGenBuffers(1, &handle);
    if (data) {
        glBindBuffer(bufferType, handle);
        glBufferData(bufferType, bufferSize, data, usageHint);
    }

    return handle;
}
