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

#include "boneObject.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "shader.hpp"
#include <chrono>
#include <sys/stat.h>
#include <thread>

time_t bone_getModTime(const char* file)
{
    struct stat fileInfo;
    std::string actualFile = SHADER_ROOT + file;
    if (stat(actualFile.c_str(), &fileInfo) != 0) {// Use stat() to get the info
        std::cerr << "Error: " << strerror(errno) << '\n';
        abort();
    }
    return fileInfo.st_mtime;
}

void boneObject::setup(bones* model, const char* _vertex, const char* _fragment)
{
    defaultMat = false;
    object = model;
    vertex = strdup(_vertex);
    fragment = strdup(_fragment);
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
    vertexFileTime = bone_getModTime(vertex);
    fragmentFileTime = bone_getModTime(fragment);
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
        vertex = strdup("pbr/pbrS.vert");
        fragment = strdup("pbr/pbr.frag");
        vertexFileTime = bone_getModTime(vertex);
        fragmentFileTime = bone_getModTime(fragment);
        unsigned int vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
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

bool boneObject::checkReload()
{
    if (bone_getModTime(vertex) > vertexFileTime && loadShaderFile(vertex)[0] != '\0') {
        return true;
    }
    if (bone_getModTime(fragment) > fragmentFileTime && loadShaderFile(vertex)[0] != '\0') {
        return true;
    }
    return false;
}

void boneObject::reload()
{
    unsigned int vertexShader;
    unsigned int fragmentShader;
    unsigned int tessellationShader;
    unsigned int tessellationEShader;
    unsigned int geometryShader;

    glDeleteProgram(shaderProgram);
    // load and compile shaders and link program
    vertexShader = compileShader(vertex, GL_VERTEX_SHADER);
    fragmentShader = compileShader(fragment, GL_FRAGMENT_SHADER);
    shaderProgram = linkProgram(vertexShader, fragmentShader);
    vertexFileTime = bone_getModTime(vertex);
    fragmentFileTime = bone_getModTime(fragment);
    //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);

    glUseProgram(shaderProgram);
    model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");

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

void boneObject::reloadCheck()
{
    bool rel = checkReload();
    if (rel) {
        std::chrono::milliseconds timespan(20);// or whatever
        std::this_thread::sleep_for(timespan);
        reload();
    }
}

void boneObject::setInt(char const* name, int value)
{
    ints[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1i(loc, value);
}

void boneObject::setFloat(char const* name, float value)
{
    floats[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1f(loc, value);
}

void boneObject::setMat4(char const* name, glm::mat4& value)
{
    mat4s[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &value[0][0]);
}

void boneObject::setVec3(char const* name, glm::vec3 value)
{
    vec3s[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, value[0], value[1], value[2]);
}

void boneObject::setVec3(char const* name, float x, float y, float z)
{
    vec3s[name] = glm::vec3(x, y, z);
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, x, y, z);
}

void boneObject::setVec4(char const* name, glm::vec4 value)
{
    vec4s[name] = value;
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform4f(loc, value[0], value[1], value[2], value[3]);
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
    objMat = scaleMat * objMat;
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

    float* mats = (float*)malloc(sizeof(float) * 16 * object->NumBones);
    for (uint i = 0; i < object->NumBones; i++) {
        uint PositionIndex = (uint)(currentTime / object->timePerFrame) % object->boneTransform.size();//FindPosition(AnimationTime, pNodeAnim);
        uint NextPositionIndex = (PositionIndex + 1) % object->boneTransform.size();
        float DeltaTime = (float)(object->timePerFrame);
        float Factor = fmod(currentTime, object->timePerFrame) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        glm::mat4 Start = object->boneTransform[PositionIndex][i];
        glm::mat4 End = object->boneTransform[NextPositionIndex][i];
        glm::mat4 Delta = End - Start;
        glm::mat4 Out = Start + Factor * Delta;
        for (uint j = 0; j < 4; j++) {
            for (uint k = 0; k < 4; k++) {
                mats[i * 16 + j * 4 + k] = Out[j][k];//object->boneTransform[index][i][j][k];
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

void boneObject::render(double currentTime, pbrObject shaderProg)
{
    glUseProgram(shaderProg.shaderProgram);
    // shaderProg.setMat4("view_mat", view_matrix);
    // shaderProg.setMat4("proj_mat", proj_matrix);

    float* mats = (float*)malloc(sizeof(float) * 16 * object->NumBones);
    for (uint i = 0; i < object->NumBones; i++) {
        uint PositionIndex = (uint)(currentTime / object->timePerFrame) % object->boneTransform.size();//FindPosition(AnimationTime, pNodeAnim);
        uint NextPositionIndex = (PositionIndex + 1) % object->boneTransform.size();
        float DeltaTime = (float)(object->timePerFrame);
        float Factor = fmod(currentTime, object->timePerFrame) / DeltaTime;
        assert(Factor >= 0.0f && Factor <= 1.0f);
        glm::mat4 Start = object->boneTransform[PositionIndex][i];
        glm::mat4 End = object->boneTransform[NextPositionIndex][i];
        glm::mat4 Delta = End - Start;
        glm::mat4 Out = Start + Factor * Delta;
        for (uint j = 0; j < 4; j++) {
            for (uint k = 0; k < 4; k++) {
                mats[i * 16 + j * 4 + k] = Out[j][k];//object->boneTransform[index][i][j][k];
            }
        }
    }

    unsigned int boneMs = glGetUniformLocation(shaderProg.shaderProgram, "Bone");
    glUniformMatrix4fv(boneMs, object->NumBones, GL_FALSE, mats);
    free(mats);

    shaderProg.setMat4("model_mat", &objMat);
    //glUniformMatrix4fv(boneMats, object->NumBones, GL_FALSE, &(object->boneTransform[index][0][0][0]));

    //glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(*object).matrixAt(currentTime)[0][0]);

    (*object).bind();
    if (useTessellation) {
        glDrawElements(GL_PATCHES, (*object).vertex_count, GL_UNSIGNED_INT, (void*)0);
    } else {
        glDrawElements(GL_TRIANGLES, (*object).vertex_count, GL_UNSIGNED_INT, (void*)0);
    }
}