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

#include "common.hpp"
#include "shader.hpp"
#include "pbrObject.hpp"

void pbrObject::setup(animated *model, const char *vertex, const char *fragment)
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

void pbrObject::setup(animated *model, bool tessellation)
{
    defaultMat = true;
    object = model;
    useTessellation = tessellation;
    if (tessellation)
    {
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
    }
    else
    {
        // load and compile shaders and link program
        unsigned int vertexShader = compileShader("pbr/pbr.vert", GL_VERTEX_SHADER);
        unsigned int fragmentShader = compileShader("pbr/pbr.frag", GL_FRAGMENT_SHADER);
        shaderProgram = linkProgram(vertexShader, fragmentShader);
        //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
        // after linking the program the shader objects are no longer needed
        glDeleteShader(fragmentShader);
        glDeleteShader(vertexShader);
    }

    glUseProgram(shaderProgram);
    model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
    unsigned int light_dir_loc = glGetUniformLocation(shaderProgram, "light_dir");
    unsigned int roughness_loc = glGetUniformLocation(shaderProgram, "roughness");
    unsigned int ref_index_loc = glGetUniformLocation(shaderProgram, "refractionIndex");
    unsigned int diffuse_loc = glGetUniformLocation(shaderProgram, "diffuse");
    unsigned int specular_loc = glGetUniformLocation(shaderProgram, "specular");
    float light_phi = 0.6f;
    float light_theta = 1.f;
    glm::vec3 light_dir(std::cos(light_phi) * std::sin(light_theta),
                        std::cos(light_theta),
                        std::sin(light_phi) * std::sin(light_theta));
    glUniform3f(light_dir_loc, light_dir.x, light_dir.y, light_dir.z);

    glm::vec4 diffuse_color(0.7f, 0.7f, 0.7f, 1.f);
    glm::vec4 specular_color(1.0f, 1.0f, 1.0f, 1.f);

    glUniform4f(diffuse_loc, diffuse_color.x, diffuse_color.y, diffuse_color.z, diffuse_color.w);
    glUniform4f(specular_loc, specular_color.x, specular_color.y, specular_color.z, specular_color.w);

    float roughness = 0.4f;
    float refraction_index = 0.4f;
    glUniform1f(roughness_loc, roughness);
    glUniform1f(ref_index_loc, refraction_index);
}

void pbrObject::use()
{
    glUseProgram(shaderProgram);
}

void pbrObject::setInt(char const *name, int value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1i(loc, value);
}

void pbrObject::setFloat(char const *name, float value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform1f(loc, value);
}

void pbrObject::setMat4(char const *name, glm::mat4* value)
{
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniformMatrix4fv(loc, 1, GL_FALSE, &(*value)[0][0]);
}

void pbrObject::setVec3(char const * name, glm::vec3 value){
    glUseProgram(shaderProgram);
    unsigned int loc = glGetUniformLocation(shaderProgram, name);
    glUniform3f(loc, value[0], value[1], value[2]);
}


void pbrObject::setMaticies(glm::mat4 *view_mat, glm::mat4 *proj_mat)
{
    glUseProgram(shaderProgram);
    view_matrix = view_mat;
    proj_matrix = proj_mat;
}

void pbrObject::render(double currentTime)
{
    glUseProgram(shaderProgram);
    if (defaultMat)
    {
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &(*view_matrix)[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &(*proj_matrix)[0][0]);
    }
    glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &(*object).matrixAt(currentTime)[0][0]);

    (*object).bind();
    if (useTessellation)
    {
        glDrawElements(GL_PATCHES, (*object).vertex_count, GL_UNSIGNED_INT, (void *)0);
    }
    else
    {
        glDrawElements(GL_TRIANGLES, (*object).vertex_count, GL_UNSIGNED_INT, (void *)0);
    }
}