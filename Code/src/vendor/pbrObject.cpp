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

#include "pbrObject.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "shader.hpp"

void pbrObject::setup(animated* model, const char* vertex, const char* fragment)
{
    object = model;
    shaderProgram.setup(vertex, fragment);
}

void pbrObject::setup(animated* model, const char* vertex, const char* fragment, const char* geometry)
{
    object = model;
    shaderProgram.setup(vertex, fragment, geometry);
}

void pbrObject::setup(animated* model, const char* vertex, const char* fragment, const char* tess, const char* tesse)
{
    object = model;
    shaderProgram.setup(vertex, fragment, tess, tesse);
}

void pbrObject::setup(animated* model, bool tessellation)
{
    object = model;
    shaderProgram.setup(tessellation);

    setInt("irradianceMap", 0);
    setInt("prefilterMap", 1);
    setInt("brdfLUT", 2);
    setInt("albedoMap", 3);
    setInt("normalMap", 4);
    setInt("metallicMap", 5);
    setInt("roughnessMap", 6);
    setInt("aoMap", 7);
    setInt("heightMap", 8);

    setFloat("displacementFactor", 0.);
}

void pbrObject::setup(animated* model, std::string vertex, std::string fragment)
{
    setup(model, vertex.c_str(), fragment.c_str());
}

void pbrObject::setup(animated* model, std::string vertex, std::string fragment, std::string geometry)
{
    setup(model, vertex.c_str(), fragment.c_str(), geometry.c_str());
}

void setup(animated* model, std::string vertex, std::string tess, std::string tesse, std::string fragment)
{
    setup(model, vertex.c_str(), tess.c_str(), tesse.c_str(), fragment.c_str());
}

void pbrObject::reload(){
    shaderProgram.reload();
}

void pbrObject::reloadCheck(){
    shaderProgram.reloadCheck();
}

bool pbrObject::checkReload(){
    return shaderProgram.checkReload();
}

void pbrObject::use()
{
    shaderProgram.use();
}

void pbrObject::setInt(char const* name, int value)
{
    shaderProgram.setInt(name, value);
}

void pbrObject::setFloat(char const* name, float value)
{
    shaderProgram.setFloat(name, value);
}

void pbrObject::setMat4(char const* name, glm::mat4* value)
{
    shaderProgram.setMat4(name, *value);
}

void pbrObject::setVec3(char const* name, glm::vec3 value)
{
    shaderProgram.setVec3(name, value);
}

void pbrObject::setVec3(char const* name, float x, float y, float z)
{
    shaderProgram.setVec3(name, x, y, z);
}

void pbrObject::setVec4(char const* name, glm::vec4 value)
{
    shaderProgram.setVec4(name, value);
}

void pbrObject::setMaticies(glm::mat4* view_mat, glm::mat4* proj_mat)
{
    shaderProgram.setMaticies(view_mat, proj_mat);
}

void pbrObject::render(double currentTime)
{
    (*object).bind();
    glm::mat4 modelMat = (*object).matrixAt(currentTime);
    shaderProgram.render(modelMat, object->vertex_count);
}

void pbrObject::render(double currentTime, shaderObject shaderProg)
{
    (*object).bind();
    glm::mat4 modelMat = (*object).matrixAt(currentTime);
    shaderProg.render(modelMat, object->vertex_count);
}

void pbrObject::render(glm::mat4& matrix)
{
    (*object).bind();
    shaderProgram.render(matrix, object->vertex_count);
}

void pbrObject::renderRotated(float rotation, float t)
{
    glm::mat4 trans = (*object).matrixAt(t);
    trans = glm::rotate(trans, glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    double mat[4][4] = {
        { cos(t), sin(t), 0., 0. },
        { -sin(t), cos(t), 0., 0. },
        { 0., 0., 1., 0. },
        { 0., 0., 0., 1. }
    };
    trans = glm::mat4(1);
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            trans[j][i] = mat[i][j];
        }
    }

    (*object).bind();
    shaderProgram.render(trans, object->vertex_count);
}