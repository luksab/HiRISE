/*

	Copyright 2011 Etay Meiri
    2020 Lukas Sabatschus
        2020 Lukas Sabatschus

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "shaderObject.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "shader.hpp"
#include <chrono>
#include <sys/stat.h>
#include <thread>

time_t shader_getModTime(const char* file)
{
    struct stat fileInfo;
    std::string actualFile = SHADER_ROOT + file;
    if (stat(actualFile.c_str(), &fileInfo) != 0) {// Use stat() to get the info
        std::cerr << "Error: " << strerror(errno) << '\n';
        abort();
    }
    return fileInfo.st_mtime;
}

void shaderObject::setup(const char* _vertex, const char* _fragment)
{
    type = 1;
    vertex = strdup(_vertex);
    fragment = strdup(_fragment);
    std::cout << "new shader:" << vertex << "  " << fragment << "\n";
    vertexFileTime = shader_getModTime(vertex);
    fragmentFileTime = shader_getModTime(fragment);
    defaultMat = false;
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
    shaderProgram = linkProgram(vertexShader, fragmentShader);
    //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glUseProgram(shaderProgram);
    model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
}

void shaderObject::setup(const char* _vertex, const char* _fragment, const char* _geometry)
{
    type = 2;
    vertex = strdup(_vertex);
    fragment = strdup(_fragment);
    geometry = strdup(_geometry);
    vertexFileTime = shader_getModTime(vertex);
    fragmentFileTime = shader_getModTime(fragment);
    geometryFileTime = shader_getModTime(geometry);
    std::cout << "new shader:" << vertex << "  " << fragment << "  " << geometry << "\n";
    defaultMat = false;
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
    unsigned int geometryShader = compileShader(geometry, GL_GEOMETRY_SHADER);
    shaderProgram = linkProgram(vertexShader, fragmentShader, geometryShader);
    //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(geometryShader);

    glUseProgram(shaderProgram);
    model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
}

void shaderObject::setup(const char* _vertex, const char* _fragment, const char* _tess, const char* _tesse)
{
    type = 3;
    vertex = strdup(_vertex);
    fragment = strdup(_fragment);
    tess = strdup(_tess);
    tesse = strdup(_tesse);
    vertexFileTime = shader_getModTime(vertex);
    fragmentFileTime = shader_getModTime(fragment);
    tessFileTime = shader_getModTime(tess);
    tesseFileTime = shader_getModTime(tesse);
    std::cout << "new shader:" << vertex << "  " << fragment << "  " << tess << "  " << tesse << "\n";
    defaultMat = false;
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
    unsigned int tessellationShader = compileShader(tess, GL_TESS_CONTROL_SHADER);
    unsigned int tessellationEShader = compileShader(tesse, GL_TESS_EVALUATION_SHADER);
    shaderProgram = linkProgram(vertexShader, fragmentShader, tessellationShader, tessellationEShader);
    //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(tessellationShader);
    glDeleteShader(tessellationEShader);

    glUseProgram(shaderProgram);
    model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
}

void shaderObject::setup(bool tessellation)
{
    defaultMat = true;
    useTessellation = tessellation;
    if (tessellation) {
        type = 3;
        vertex = "pbr/pbrT.vert";
        fragment = "pbr/pbrT.frag";
        tess = "pbr/pbrT.tess";
        tesse = "pbr/pbrT.tesse";
        vertexFileTime = shader_getModTime(vertex);
        fragmentFileTime = shader_getModTime(fragment);
        tessFileTime = shader_getModTime(tess);
        tesseFileTime = shader_getModTime(tesse);
        std::cout << "new shader:" << vertex << "  " << fragment << "  " << tess << "  " << tesse << "\n";
        // load and compile shaders and link program
        unsigned int vertexShader = compileShader("pbr/pbrT.vert", GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader("pbr/pbrT.frag", GL_FRAGMENT_SHADER);
        unsigned int tessellationShader = compileShader("pbr/pbrT.tess", GL_TESS_CONTROL_SHADER);
        unsigned int tessellationEShader = compileShader("pbr/pbrT.tesse", GL_TESS_EVALUATION_SHADER);
        shaderProgram = linkProgram(vertexShader, fragmentShader, tessellationShader, tessellationEShader);
        //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
        // after linking the program the shader objects are no longer needed
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(tessellationShader);
        glDeleteShader(tessellationEShader);
    } else {
        type = 1;
        vertex = "pbr/pbr.vert";
        fragment = "pbr/pbr.frag";
        vertexFileTime = shader_getModTime(vertex);
        fragmentFileTime = shader_getModTime(fragment);
        std::cout << "new shader:" << vertex << "  " << fragment << "\n";
        // load and compile shaders and link program
        unsigned int vertexShader = compileShader("pbr/pbr.vert", GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader("pbr/pbr.frag", GL_FRAGMENT_SHADER);
        shaderProgram = linkProgram(vertexShader, fragmentShader);
        //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
        // after linking the program the shader objects are no longer needed
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
    }

    setInt("irradianceMap", 0);
    setInt("prefilterMap", 1);
    setInt("brdfLUT", 2);
    setInt("albedoMap", 3);
    setInt("normalMap", 4);
    setInt("metallicMap", 5);
    setInt("roughnessMap", 6);
    setInt("aoMap", 7);
    setInt("heightMap", 8);

    glUseProgram(shaderProgram);
    model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");

    setFloat("displacementFactor", 0.);
}

void shaderObject::setup(std::string vertex, std::string fragment)
{
    setup(vertex.c_str(), fragment.c_str());
}

void shaderObject::setup(std::string vertex, std::string fragment, std::string geometry)
{
    setup(vertex.c_str(), fragment.c_str(), geometry.c_str());
}

void setup(std::string vertex, std::string tess, std::string tesse, std::string fragment)
{
    setup(vertex.c_str(), tess.c_str(), tesse.c_str(), fragment.c_str());
}

void shaderObject::reload()
{
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int tessellationShader;
    unsigned int tessellationEShader;
    unsigned int geometryShader;

    switch (type) {
    case 1:
        glDeleteProgram(shaderProgram);
        // load and compile shaders and link program
        vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
        fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
        shaderProgram = linkProgram(vertexShader, fragmentShader);
        vertexFileTime = shader_getModTime(vertex);
        fragmentFileTime = shader_getModTime(fragment);
        //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
        // after linking the program the shader objects are no longer needed
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);

        glUseProgram(shaderProgram);
        model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
        view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
        proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
        break;
    case 2:
        glDeleteProgram(shaderProgram);
        // load and compile shaders and link program
        vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
        fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
        geometryShader = compileShader(geometry, GL_GEOMETRY_SHADER);
        vertexFileTime = shader_getModTime(vertex);
        fragmentFileTime = shader_getModTime(fragment);
        geometryFileTime = shader_getModTime(geometry);
        shaderProgram = linkProgram(vertexShader, fragmentShader, geometryShader);
        //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
        // after linking the program the shader objects are no longer needed
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(geometryShader);

        glUseProgram(shaderProgram);
        model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
        view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
        proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
        break;
    case 3:
        glDeleteProgram(shaderProgram);
        // load and compile shaders and link program
        vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
        fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
        tessellationShader = compileShader(tess, GL_TESS_CONTROL_SHADER);
        tessellationEShader = compileShader(tesse, GL_TESS_EVALUATION_SHADER);
        vertexFileTime = shader_getModTime(vertex);
        fragmentFileTime = shader_getModTime(fragment);
        tessFileTime = shader_getModTime(tess);
        tesseFileTime = shader_getModTime(tesse);
        shaderProgram = linkProgram(vertexShader, fragmentShader, tessellationShader, tessellationEShader);
        //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
        // after linking the program the shader objects are no longer needed
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(tessellationShader);
        glDeleteShader(tessellationEShader);

        glUseProgram(shaderProgram);
        model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
        view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
        proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
        break;
    default:
        break;
    }

    for (const auto& value : ints) {
        setInt(value.first, value.second);
    }

    for (const auto& value : floats) {
        setFloat(value.first, value.second);
    }

    for (const auto& value : vec3s) {
        setVec3(value.first, value.second);
    }

    for (const auto& value : vec4s) {
        setVec4(value.first, value.second);
    }

    for (const auto& value : mat4s) {
        glm::mat4 val = value.second;
        setMat4(value.first, val);
    }
}

bool shaderObject::checkReload()
{
    switch (type) {
    case 1:
        if (shader_getModTime(vertex) > vertexFileTime && loadShaderFile(vertex)[0] != '\0') {
            return true;
        }
        if (shader_getModTime(fragment) > fragmentFileTime && loadShaderFile(vertex)[0] != '\0') {
            return true;
        }
        break;
    case 2:
        if (shader_getModTime(vertex) > vertexFileTime && loadShaderFile(vertex)[0] != '\0') {
            return true;
        }
        if (shader_getModTime(fragment) > fragmentFileTime && loadShaderFile(fragment)[0] != '\0') {
            return true;
        }
        if (shader_getModTime(geometry) > geometryFileTime && loadShaderFile(geometry)[0] != '\0') {
            return true;
        }
        break;
    case 3:
        if (shader_getModTime(vertex) > vertexFileTime && loadShaderFile(vertex)[0] != '\0') {
            return true;
        }
        if (shader_getModTime(fragment) > fragmentFileTime && loadShaderFile(fragment)[0] != '\0') {
            return true;
        }
        if (shader_getModTime(tess) > tessFileTime && loadShaderFile(tess)[0] != '\0') {
            return true;
        }
        if (shader_getModTime(tesse) > tesseFileTime && loadShaderFile(tesse)[0] != '\0') {
            return true;
        }
        break;
    }
    return false;
}

void shaderObject::reloadCheck()
{
    bool rel = checkReload();
    if (rel) {
        std::chrono::milliseconds timespan(20);// or whatever
        std::this_thread::sleep_for(timespan);
        reload();
    }
}

void shaderObject::use()
{
    glUseProgram(shaderProgram);
}

void shaderObject::setInt(char const* name, int value)
{
    ints[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1i(loc, value);
}

void shaderObject::setFloat(char const* name, float value)
{
    floats[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1f(loc, value);
}

void shaderObject::setMat4(char const* name, glm::mat4& value)
{
    mat4s[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void shaderObject::setVec3(char const* name, glm::vec3 value)
{
    vec3s[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, value[0], value[1], value[2]);
}

void shaderObject::setVec3(char const* name, float x, float y, float z)
{
    vec3s[name] = glm::vec3(x, y, z);
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, x, y, z);
}

void shaderObject::setVec4(char const* name, glm::vec4 value)
{
    vec4s[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform4f(loc, value[0], value[1], value[2], value[3]);
}

void shaderObject::setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat)
{
    glUseProgram(shaderProgram);
    view_matrix = view_mat;
    proj_matrix = proj_mat;
}

void shaderObject::render(glm::mat4& matrix, uint vertex_count)
{
    glUseProgram(shaderProgram);
    if (defaultMat) {
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &(*view_matrix)[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &(*proj_matrix)[0][0]);
    }

    glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(matrix[0][0]));

    if (useTessellation) {
        glDrawElements(GL_PATCHES, vertex_count, GL_UNSIGNED_INT, (void*)0);
    } else {
        glDrawElements(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, (void*)0);
    }
}

void shaderObject::render(uint vertex_count)
{
    glUseProgram(shaderProgram);

    if (useTessellation) {
        glDrawElements(GL_PATCHES, vertex_count, GL_UNSIGNED_INT, (void*)0);
    } else {
        glDrawElements(GL_TRIANGLES, vertex_count, GL_UNSIGNED_INT, (void*)0);
    }
}
