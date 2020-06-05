#include <shader.hpp>

#include <fstream>

const char*
loadShaderFile(const char* filename) {
    std::string actualFile = SHADER_ROOT + filename;
    std::ifstream in(actualFile.c_str());
    std::string str((std::istreambuf_iterator<char>(in)),
                     std::istreambuf_iterator<char>());
    char* code = new char[str.size()+1];
    code[str.size()] = '\0';
    str.copy(code, str.size());
    return code;
}

unsigned int
compileShader(const char* filename, unsigned int type) {
    const char* shaderSource = loadShaderFile(filename);

    // create shader object
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &shaderSource, NULL);
    // try to compile
    glCompileShader(shader);
    // source code is no longer needed
    delete [] shaderSource;

    // check if compilation succeeded
    int  success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(shader, 512, NULL, infoLog);
        std::cerr << "Shader compilation failed\n" << infoLog << std::endl;
        return 0;
    }

    return shader;
}

unsigned int
linkProgram(unsigned int computeShader) {
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, computeShader);
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Linking program failed\n" << infoLog << std::endl;
        return 0;
    }

    return shaderProgram;
}

unsigned int
linkProgram(unsigned int vertexShader, unsigned int fragmentShader) {
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Linking program failed\n" << infoLog << std::endl;
        return 0;
    }

    return shaderProgram;
}

unsigned int
linkProgram(unsigned int vertexShader, unsigned int fragmentShader, unsigned int tesselationShader, unsigned int tesselationEShader) {
    unsigned int shaderProgram;
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glAttachShader(shaderProgram, tesselationShader);
    glAttachShader(shaderProgram, tesselationEShader);
    glLinkProgram(shaderProgram);
    int success;
    char infoLog[512];
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if(!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Linking program failed\n" << infoLog << std::endl;
        return 0;
    }

    return shaderProgram;
}
