/*

	Copyright 2011 Etay Meiri

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

#include "boneObject.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "shader.hpp"

void boneObject::setup(bones* model, const char* vertex, const char* fragment)
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

void boneObject::setup(bones* model, bool tessellation)
{
    defaultMat = true;
    object = model;
    useTessellation = tessellation;
    if (tessellation) {
        // load and compile shaders and link program
        unsigned int vertexShader = compileShader("pbr/pbrST.vert", GL_VERTEX_SHADER);
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
        unsigned int vertexShader = compileShader("pbr/pbrS.vert", GL_VERTEX_SHADER);
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

    boneMats = glGetUniformLocation(shaderProgram, "Bone");
    objMat = glm::mat4(1.);
}

void boneObject::use()
{
    glUseProgram(shaderProgram);
}

void boneObject::setInt(char const* name, int value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1i(loc, value);
}

void boneObject::setFloat(char const* name, float value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1f(loc, value);
}

void boneObject::setMat4(char const* name, glm::mat4* value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &(*value)[0][0]);
}

void boneObject::setVec3(char const* name, glm::vec3 value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, value[0], value[1], value[2]);
}

void boneObject::setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat)
{
    glUseProgram(shaderProgram);
    view_matrix = view_mat;
    proj_matrix = proj_mat;
}

void boneObject::scale(float scale)
{
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scale, scale, scale));
    objMat = scaleMat*objMat;
}

void boneObject::move(float x, float y, float z)
{
    objMat = glm::translate(objMat, glm::vec3(x, y, z));
}

void boneObject::render(double currentTime)
{
    glUseProgram(shaderProgram);
    if (defaultMat) {
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &(*view_matrix)[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &(*proj_matrix)[0][0]);
    }

    uint index = (uint)(currentTime * 24.) % object->boneTransform.size();

    float* mats = (float*)malloc(sizeof(float) * 16 * object->NumBones);
    for (uint i = 0; i < object->NumBones; i++) {
        for (uint j = 0; j < 4; j++) {
            for (uint k = 0; k < 4; k++) {
                mats[i * 16 + j * 4 + k] = object->boneTransform[index][i][j][k];
            }
        }
    }

    glUniformMatrix4fv(boneMats, object->NumBones, GL_FALSE, mats);
    free(mats);

    //setMat4("model_mat",&objMat);
    glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(objMat)[0][0]);
    //glUniformMatrix4fv(boneMats, object->NumBones, GL_FALSE, &(object->boneTransform[index][0][0][0]));

    //glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(*object).matrixAt(currentTime)[0][0]);

    (*object).bind();
    if (useTessellation) {
        glDrawElements(GL_PATCHES, (*object).vertex_count, GL_UNSIGNED_INT, (void*)0);
    } else {
        glDrawElements(GL_TRIANGLES, (*object).vertex_count, GL_UNSIGNED_INT, (void*)0);
    }
}