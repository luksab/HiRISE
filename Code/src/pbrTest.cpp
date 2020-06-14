#include <random>
#include <glm/gtx/transform.hpp>
#include <stb_image.h>

#include "common.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "pbrObject.hpp"

#include <imgui.hpp>

const int WINDOW_WIDTH = 1920;
const int WINDOW_HEIGHT = 1080;
float FOV = 45.;
const float NEAR_VALUE = 0.1f;
const float FAR_VALUE = 100.f;
const int TEXTURE_WIDTH = 20;
const int TEXTURE_HEIGHT = 20;

#ifndef M_PI
#define M_PI 3.14159265359
#endif

glm::mat4 proj_matrix;

void resizeCallback(GLFWwindow *window, int width, int height);

float *
load_texture_data(std::string filename, int *width, int *height)
{
    int channels;
    unsigned char *file_data = stbi_load(filename.c_str(), width, height, &channels, 3);

    int w = *width;
    int h = *height;

    float *data = new float[4 * w * h];
    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            data[j * w * 4 + i * 4 + 0] = static_cast<float>(file_data[j * w * 3 + i * 3 + 0]) / 255;
            data[j * w * 4 + i * 4 + 1] = static_cast<float>(file_data[j * w * 3 + i * 3 + 1]) / 255;
            data[j * w * 4 + i * 4 + 2] = static_cast<float>(file_data[j * w * 3 + i * 3 + 2]) / 255;
            data[j * w * 4 + i * 4 + 3] = 1.f;
        }
    }

    delete[] file_data;

    return data;
}

int main(int, char *argv[])
{
    GLFWwindow *window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    camera cam(window);

    init_imgui(window);

    animated pbr = loadMeshAnim("cube.dae", true);
    pbrObject hdrCube = {};
    hdrCube.setup(&pbr, "equirectangular/main.vert", "equirectangular/main.frag");
    unsigned int equirectangularMap_loc = glGetUniformLocation(hdrCube.shaderProgram, "equirectangularMap");

    pbrObject renderCube = {};
    renderCube.setup(&pbr, "cubeMap/hdr.vert", "cubeMap/hdr.frag");

    pbrObject irradianceCube = {};
    irradianceCube.setup(&pbr, "equirectangular/main.vert", "cubeMap/conv.frag");
    unsigned int equirectangularMap_loc_irra = glGetUniformLocation(irradianceCube.shaderProgram, "environmentMap");

    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf((DATA_ROOT + "moonless_golf_4k.hdr").c_str(), &width, &height, &nrComponents, 0);
    unsigned int hdrTexture;
    if (data)
    {
        glGenTextures(1, &hdrTexture);
        glBindTexture(GL_TEXTURE_2D, hdrTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, width, height, 0, GL_RGB, GL_FLOAT, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Failed to load HDR image." << std::endl;
    }
    int resolution = 512;

    unsigned int captureFBO, captureRBO;
    glGenFramebuffers(1, &captureFBO);
    glGenRenderbuffers(1, &captureRBO);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, captureRBO);

    unsigned int envCubemap;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        // note that we store each face with 16 bit floating point values
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
                     resolution, resolution, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glm::mat4 captureProjection = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 captureViews[] =
        {
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
            glm::lookAt(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))};

    // convert HDR equirectangular environment map to cubemap equivalent
    glUseProgram(hdrCube.shaderProgram);
    //equirectangularToCubemapShader.setInt("equirectangularMap", 0);
    hdrCube.proj_matrix = &captureProjection;
    glUniform1i(equirectangularMap_loc, 0);
    //equirectangularToCubemapShader.setMat4("projection", captureProjection);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTexture);

    glViewport(0, 0, resolution, resolution); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        hdrCube.view_matrix = &captureViews[i];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, envCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        hdrCube.render(0); // renders a 1x1 cube
    }
    glGenerateMipmap(envCubemap);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //conv code
    resolution = 32;
    unsigned int irradianceMap;
    glGenTextures(1, &irradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, resolution, resolution, 0,
                     GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, resolution, resolution);

    glUseProgram(irradianceCube.shaderProgram);
    //irradianceShader.setInt("environmentMap", 0);
    //irradianceShader.setMat4("projection", captureProjection);
    irradianceCube.proj_matrix = &captureProjection;
    glUniform1i(equirectangularMap_loc_irra, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glViewport(0, 0, resolution, resolution); // don't forget to configure the viewport to the capture dimensions.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    for (unsigned int i = 0; i < 6; ++i)
    {
        irradianceCube.view_matrix = &captureViews[i];
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                               GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, irradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        irradianceCube.render(0);
    }
    glGenerateMipmap(irradianceMap);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    //glUseProgram(renderCube.shaderProgram);
    //unsigned int environmentMap_loc_render = glGetUniformLocation(renderCube.shaderProgram, "environmentMap");
    // glUniform1i(environmentMap_loc_render, 0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    proj_matrix = glm::perspective(FOV, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool vSync = true;
    bool Framerate = true;
    bool Camera = false;

    //fuer fps
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    int fps = 0;
    double frameTime = 0.;
    // rendering loop
    while (glfwWindowShouldClose(window) == false)
    {
        double currentTime = glfwGetTime();
        nbFrames++;
        if (currentTime - lastTime >= 1.0)
        { // If last prinf() was more than 1 sec ago
            // printf and reset timer
            fps = nbFrames;
            frameTime = 1000.0 / double(nbFrames);
            //printf("%d fps, %f ms/frame\n", nbFrames, 1000.0 / double(nbFrames));
            nbFrames = 0;
            lastTime += 1.0;
        }

        glfwPollEvents();
        glClearColor(0.25f, 0.25f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // define UI
        imgui_new_frame(400, 200);
        ImGui::Begin("General");
        ImGui::Checkbox("Framerate", &Framerate);
        ImGui::Checkbox("Camera", &Camera);
        ImGui::End();
        if (Framerate)
        {
            ImGui::Begin("Framerate");
            ImGui::Text("FPs: %04d", fps);
            ImGui::Text("avg. frametime: %04f", frameTime);
            ImGui::Checkbox("V-Sync", &vSync);
            ImGui::End();
        }
        if (Camera)
        {
            ImGui::Begin("Camera");
            ImGui::SliderFloat("float", &FOV, 90.0f, 5.0f);
            ImGui::End();
        }

        if (vSync)
        {
            glfwSwapInterval(1);
        }
        else
        {
            glfwSwapInterval(0);
        }

        proj_matrix = glm::perspective(glm::radians(FOV), static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

        glm::mat4 view_matrix = cam.view_matrix();

        glDepthFunc(GL_LEQUAL);
        renderCube.setMaticies(&view_matrix, &proj_matrix);
        renderCube.render(0);

        // render UI
        imgui_render();

        glfwSwapBuffers(window);
    }

    cleanup_imgui();
    glfwTerminate();
}

void resizeCallback(GLFWwindow *, int width, int height)
{
    // set new width and height as viewport size
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);
}
