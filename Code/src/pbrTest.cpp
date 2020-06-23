#include <random>
#include <unistd.h>
#include <glm/gtx/transform.hpp>
#include <stb_image.h>

#include "common.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "pbrObject.hpp"
#include "boneObject.hpp"
#include "glm/gtx/string_cast.hpp"

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

void renderQuad();

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

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const *path)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);

    int width, height, nrComponents;
    unsigned char *data = stbi_load(path, &width, &height, &nrComponents, 0);
    if (data)
    {
        GLenum format;
        if (nrComponents == 1)
            format = GL_RED;
        else if (nrComponents == 3)
            format = GL_RGB;
        else if (nrComponents == 4)
            format = GL_RGBA;

        glBindTexture(GL_TEXTURE_2D, textureID);
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        stbi_image_free(data);
    }
    else
    {
        std::cout << "Texture failed to load at path: " << path << std::endl;
        stbi_image_free(data);
    }

    return textureID;
}

std::vector<unsigned int> loadPBR(char const *path)
{
    std::vector<unsigned int> ret;
    ret.push_back(loadTexture((DATA_ROOT + path + "/" + path + "_diff_8k.jpg").c_str()));
    ret.push_back(loadTexture((DATA_ROOT + path + "/" + path + "_nor_8k.jpg").c_str()));
    if (access((DATA_ROOT + path + "/" + path + "_disp_8k.jpg").c_str(), F_OK) != -1)
    {
    }
    //ret.push_back(loadTexture((path+"/"+path+"_disp_8k.png").c_str()));
    unsigned int metallic;
    glGenTextures(1, &metallic);
    ret.push_back(metallic);
    ret.push_back(loadTexture((DATA_ROOT + path + "/" + path + "_rough_8k.jpg").c_str()));
    ret.push_back(loadTexture((DATA_ROOT + path + "/" + path + "_ao_8k.jpg").c_str()));
    ret.push_back(loadTexture((DATA_ROOT + path + "/" + path + "_disp_8k.jpg").c_str()));

    return ret;
}

struct pbrTex
{
    unsigned int hdrTexture;
    unsigned int prefilterMap;
    unsigned int irradianceMap;
    unsigned int brdfLUTTexture;
};

pbrTex setupPBR(animated *pbr)
{
    pbrObject hdrCube = {};
    hdrCube.setup(pbr, "equirectangular/main.vert", "equirectangular/main.frag");
    hdrCube.defaultMat = true;
    unsigned int equirectangularMap_loc = glGetUniformLocation(hdrCube.shaderProgram, "equirectangularMap");

    pbrObject irradianceCube = {};
    irradianceCube.setup(pbr, "equirectangular/main.vert", "cubeMap/conv.frag");
    irradianceCube.defaultMat = true;
    unsigned int equirectangularMap_loc_irra = glGetUniformLocation(irradianceCube.shaderProgram, "environmentMap");

    pbrObject filterCube = {};
    filterCube.setup(pbr, "equirectangular/main.vert", "cubeMap/rough.frag");
    filterCube.defaultMat = true;
    unsigned int equirectangularMap_locF_irra = glGetUniformLocation(filterCube.shaderProgram, "environmentMap");
    filterCube.setInt("environmentMap", 0);

    pbrObject brdfCube = {};
    brdfCube.setup(pbr, "brdf/brdf.vs", "brdf/brdf.fs");

    printf("loading hdri\n");
    stbi_set_flip_vertically_on_load(true);
    int width, height, nrComponents;
    float *data = stbi_loadf((DATA_ROOT + "construction_yard_8k.hdr").c_str(), &width, &height, &nrComponents, 0);
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
    int resolution = 4096;

    printf("converting hdri to environment map\n");
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

    printf("converting environment map to irradiance map\n");
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
    //glBindTexture(GL_TEXTURE_CUBE_MAP, irradianceMap);

    printf("converting environment map to roughness map\n");
    unsigned int prefilterMap;
    glGenTextures(1, &prefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, prefilterMap);
    for (unsigned int i = 0; i < 6; ++i)
    {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F, 128, 128, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    filterCube.use();
    glUseProgram(filterCube.shaderProgram);
    filterCube.setInt("environmentMap", 0);
    filterCube.proj_matrix = &captureProjection;
    glUniform1i(equirectangularMap_locF_irra, 0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);

    unsigned int maxMipLevels = 5;
    for (unsigned int mip = 0; mip < maxMipLevels; ++mip)
    {
        // reisze framebuffer according to mip-level size.
        unsigned int mipWidth = 128 * std::pow(0.5, mip);
        unsigned int mipHeight = 128 * std::pow(0.5, mip);
        glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipWidth, mipHeight);
        glViewport(0, 0, mipWidth, mipHeight);

        float roughness = (float)mip / (float)(maxMipLevels - 1);
        filterCube.setFloat("roughness", roughness);
        for (unsigned int i = 0; i < 6; ++i)
        {
            filterCube.view_matrix = &captureViews[i];
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                                   GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, prefilterMap, mip);

            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            filterCube.render(0);
        }
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // pbr: generate a 2D LUT from the BRDF equations used.
    // ----------------------------------------------------
    printf("precomputing brdf\n");
    unsigned int brdfLUTTexture;
    glGenTextures(1, &brdfLUTTexture);

    // pre-allocate enough memory for the LUT texture.
    glBindTexture(GL_TEXTURE_2D, brdfLUTTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, 512, 512, 0, GL_RG, GL_FLOAT, 0);
    // be sure to set wrapping mode to GL_CLAMP_TO_EDGE
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // then re-configure capture framebuffer object and render screen-space quad with BRDF shader.
    glBindFramebuffer(GL_FRAMEBUFFER, captureFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, captureRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, 512, 512);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, brdfLUTTexture, 0);

    glViewport(0, 0, 512, 512);
    brdfCube.use();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    renderQuad();

    pbrTex returnTex = {};
    returnTex.prefilterMap = prefilterMap;
    returnTex.irradianceMap = irradianceMap;
    returnTex.brdfLUTTexture = brdfLUTTexture;
    returnTex.hdrTexture = envCubemap;

    return returnTex;
}

int main(int, char *argv[])
{
    GLFWwindow *window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    camera cam(window);

    init_imgui(window);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    printf("loading textures\n");
    std::vector<unsigned int> pbrImgs = loadPBR("rock_ground");
    unsigned int albedo = pbrImgs[0]; //loadTexture((DATA_ROOT + "book/book_pattern_col2_8k.png").c_str());
    unsigned int normal = pbrImgs[1]; //vloadTexture((DATA_ROOT + "book/book_pattern_nor_8k.png").c_str());
    //unsigned int metallic = loadTexture((DATA_ROOT + "book/book_pattern_disp_8k.png").c_str());
    unsigned int metallic = pbrImgs[0];
    //glGenTextures(1, &metallic);
    unsigned int roughness = pbrImgs[3]; //loadTexture((DATA_ROOT + "book/book_pattern_rough_8k.png").c_str());
    unsigned int ao = pbrImgs[4];        //loadTexture((DATA_ROOT + "book/book_pattern_AO_8k.png").c_str());
    //unsigned int disp = loadTexture((DATA_ROOT + "SphereDisplacement.png").c_str());//pbrImgs[5];
    unsigned int disp = pbrImgs[5];

    printf("loading meshes\n");

    animated pbr = loadMeshAnim("cube.dae", true);
    pbrTex envtex = setupPBR(&pbr);

    pbrObject renderCube = {};
    renderCube.setup(&pbr, "cubeMap/hdr.vert", "cubeMap/hdr.frag");
    renderCube.defaultMat = true;
    renderCube.setInt("environmentMap", 0);

    //bones human = loadMeshBone("Lowpolymesh_Eliber2.dae", false);
    //animated human = loadMeshAnim("hiresSphere.dae", true);
    animated human = toAnimated(loadMesh("sphere_fine.obj", true));
    // std::cout << "main: " << glm::to_string(human.boneTransform[0][5]) << "\n";
    // for (int i = 0; i < 51*16; i++)
    // {
    //     if(i%16==0)printf("\n");
    //     std::cout << *((&(human.boneTransform[5][0][0][0]))+i) << ",";
    // }

    // for (size_t i = 0; i < human.boneWeight.size(); i++)
    // {
    //     std::cout << glm::to_string(human.boneWeight[i]) << ", " << glm::to_string(human.boneIndex[i]) << "\n";
    // }

    pbrObject boneObj = {};
    boneObj.setup(&human, false);
    boneObj.use();
    boneObj.setInt("irradianceMap", 0);
    boneObj.setInt("prefilterMap", 1);
    boneObj.setInt("brdfLUT", 2);
    boneObj.setInt("albedoMap", 3);
    boneObj.setInt("normalMap", 4);
    boneObj.setInt("metallicMap", 5);
    boneObj.setInt("roughnessMap", 6);
    boneObj.setInt("aoMap", 7);
    boneObj.setInt("heightMap", 8);

    // load PBR material textures
    // --------------------------
    // unsigned int albedo = loadTexture((DATA_ROOT + "rust/albedo.png").c_str());
    // unsigned int normal = loadTexture((DATA_ROOT + "rust/normal.png").c_str());
    // unsigned int metallic = loadTexture((DATA_ROOT + "rust/metallic.png").c_str());
    // unsigned int roughness = loadTexture((DATA_ROOT + "rust/roughness.png").c_str());
    // unsigned int ao = loadTexture((DATA_ROOT + "rust/ao.png").c_str());

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    proj_matrix = glm::perspective(FOV, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool vSync = true;
    bool Framerate = true;
    bool Camera = false;

    const char *BG_Textures[] = {"HDRI", "irradiance", "prefilter"};
    int whichBGTexture = 0;

    float bgLoD = 2.25;

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
            ImGui::SliderFloat("FOV", &FOV, 90.0f, 5.0f);
            ImGui::SliderFloat("LoD for HDRI", &bgLoD, 0.0f, 6.0f);
            ImGui::Combo("BG Texture", &whichBGTexture, BG_Textures, 3);
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
        glActiveTexture(GL_TEXTURE0);
        switch (whichBGTexture)
        {
        case 0:
            glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.hdrTexture);
            break;
        case 1:
            glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.irradianceMap);
            break;
        case 2:
        glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.prefilterMap);
            break;
        default:
            break;
        }
        
        renderCube.setFloat("lod", bgLoD);
        glUseProgram(renderCube.shaderProgram);
        unsigned int lodLoc = glGetUniformLocation(renderCube.shaderProgram, "lod");
        glUniform1f(lodLoc, bgLoD);
        renderCube.setMaticies(&view_matrix, &proj_matrix);
        renderCube.render(0);

        // bind pre-computed IBL data
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.irradianceMap);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.prefilterMap);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, envtex.brdfLUTTexture);

        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, albedo);
        glActiveTexture(GL_TEXTURE4);
        glBindTexture(GL_TEXTURE_2D, normal);
        glActiveTexture(GL_TEXTURE5);
        glBindTexture(GL_TEXTURE_2D, metallic);
        glActiveTexture(GL_TEXTURE6);
        glBindTexture(GL_TEXTURE_2D, roughness);
        glActiveTexture(GL_TEXTURE7);
        glBindTexture(GL_TEXTURE_2D, ao);
        glActiveTexture(GL_TEXTURE8);
        glBindTexture(GL_TEXTURE_2D, disp);

        boneObj.setMaticies(&view_matrix, &proj_matrix);
        boneObj.setVec3("camPos", cam.position());
        boneObj.renderRotated(currentTime, 7.);

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

unsigned int quadVAO = 0;
unsigned int quadVBO;
void renderQuad()
{
    if (quadVAO == 0)
    {
        float quadVertices[] = {
            // positions        // texture Coords
            -1.0f,
            1.0f,
            0.0f,
            0.0f,
            1.0f,
            -1.0f,
            -1.0f,
            0.0f,
            0.0f,
            0.0f,
            1.0f,
            1.0f,
            0.0f,
            1.0f,
            1.0f,
            1.0f,
            -1.0f,
            0.0f,
            1.0f,
            0.0f,
        };
        // setup plane VAO
        glGenVertexArrays(1, &quadVAO);
        glGenBuffers(1, &quadVBO);
        glBindVertexArray(quadVAO);
        glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
    }
    glBindVertexArray(quadVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
}
