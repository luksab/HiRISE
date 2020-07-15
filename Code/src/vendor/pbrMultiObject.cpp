/*

	Copyright 2011 Etay Meiri
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

#include "pbrMultiObject.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "shader.hpp"

void pbrMultiObject::setup(std::vector<animated> model, const char* vertex, const char* fragment)
{
    defaultMat = false;
    object = model;
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

void pbrMultiObject::setup(std::vector<animated> model, const char* vertex, const char* fragment, const char* geometry)
{
    defaultMat = false;
    object = model;
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

void pbrMultiObject::setup(std::vector<animated> model, const char* vertex, const char* fragment, const char* tess, const char* tesse)
{
    defaultMat = false;
    object = model;
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

void pbrMultiObject::setup(std::vector<animated> model, bool tessellation)
{
    defaultMat = true;
    object = model;
    useTessellation = tessellation;
    if (tessellation) {
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

void pbrMultiObject::setup(std::vector<animated> model, std::string vertex, std::string fragment)
{
    setup(model, vertex.c_str(), fragment.c_str());
}

void pbrMultiObject::setup(std::vector<animated> model, std::string vertex, std::string fragment, std::string geometry)
{
    setup(model, vertex.c_str(), fragment.c_str(), geometry.c_str());
}

void setup(std::vector<animated> model, std::string vertex, std::string tess, std::string tesse, std::string fragment)
{
    setup(model, vertex.c_str(), tess.c_str(), tesse.c_str(), fragment.c_str());
}

void pbrMultiObject::use()
{
    glUseProgram(shaderProgram);
}

void pbrMultiObject::setInt(char const* name, int value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1i(loc, value);
}

void pbrMultiObject::setFloat(char const* name, float value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1f(loc, value);
}

void pbrMultiObject::setMat4(char const* name, glm::mat4* value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &(*value)[0][0]);
}

void pbrMultiObject::setVec3(char const* name, glm::vec3 value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, value[0], value[1], value[2]);
}

void pbrMultiObject::setVec3(char const* name, float x, float y, float z)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, x, y, z);
}

void pbrMultiObject::setVec4(char const* name, glm::vec4 value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform4f(loc, value[0], value[1], value[2], value[3]);
}

void pbrMultiObject::setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat)
{
    glUseProgram(shaderProgram);
    view_matrix = view_mat;
    proj_matrix = proj_mat;
}

void pbrMultiObject::render(double currentTime)
{
    glUseProgram(shaderProgram);
    if (defaultMat) {
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &(*view_matrix)[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &(*proj_matrix)[0][0]);
    }

    for (uint i = 0; i < object.size(); i++) {
        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(object[i].matrixAt(currentTime))[0][0]);
        object[i].bind();
        if (useTessellation) {
            glDrawElements(GL_PATCHES, object[i].vertex_count, GL_UNSIGNED_INT, (void*)0);
        } else {
            glDrawElements(GL_TRIANGLES, object[i].vertex_count, GL_UNSIGNED_INT, (void*)0);
        }
    }
}

void pbrMultiObject::render(double currentTime, unsigned int shaderProg)
{
    glUseProgram(shaderProg);

    for (uint i = 0; i < object.size(); i++) {
        object[i].bind();
        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(object[i].matrixAt(currentTime))[0][0]);
        if (useTessellation) {
            glDrawElements(GL_PATCHES, object[i].vertex_count, GL_UNSIGNED_INT, (void*)0);
        } else {
            glDrawElements(GL_TRIANGLES, object[i].vertex_count, GL_UNSIGNED_INT, (void*)0);
        }
    }
}
