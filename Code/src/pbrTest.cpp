#include <glm/gtx/transform.hpp>
#include <random>
#include <stb_image.h>
#include <unistd.h>

#include "boneObject.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "common.hpp"
#include "glm/gtx/string_cast.hpp"
#include "mesh.hpp"
#include "pbrObject.hpp"
#include "pngImg.hpp"
#include "shader.hpp"
#include "pbrTex.hpp"

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

void resizeCallback(GLFWwindow* window, int width, int height);

int main(int, char* argv[])
{
    GLFWwindow* window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    camera cam(window);

    init_imgui(window);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    printf("loading meshes\n");

    animated pbr = loadMeshAnim("cube.dae", true);

    pbrObject renderCube = {};
    renderCube.setup(&pbr, "cubeMap/hdr.vert", "cubeMap/hdr.frag");
    renderCube.shaderProgram.defaultMat = true;
    renderCube.setInt("environmentMap", 0);

    //bones human = loadMeshBone("cylinderBones.dae", false);
    bones human = loadMeshBone("Lowpolymesh_Eliber.dae", false);
    //animated human = loadMeshAnim("hiresSphereRot.dae", false);
    //animated human = toAnimated(loadMesh("hiresSphereRot.dae", true));
    // std::cout << "main: " << glm::to_string(human.boneTransform[0][5]) << "\n";
    // for (int i = 0; i < 51*16; i++)
    // {
    //     if(i%4==0)printf("\n");
    //     if(i%16==0)printf("%d\n",i/16);
    //     std::cout << *((&(human.boneTransform[0][0][0][0]))+i) << ",";
    // }printf("\n");

    // for (size_t i = 0; i < human.boneWeight.size(); i++)
    // {
    //     std::cout << glm::to_string(human.boneWeight[i]) << ", " << glm::to_string(human.boneIndex[i]) << "\n";
    // }

    boneObject boneObj = {};
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

    printf("loading textures\n");
    std::vector<unsigned int> pbrImgs = loadPBR("rock_ground");
    unsigned int albedo = pbrImgs[0];//loadTexture((DATA_ROOT + "book/book_pattern_col2_8k.png").c_str());
    unsigned int normal = pbrImgs[1];//vloadTexture((DATA_ROOT + "book/book_pattern_nor_8k.png").c_str());
    //unsigned int metallic = loadTexture((DATA_ROOT + "book/book_pattern_disp_8k.png").c_str());
    unsigned int metallic = pbrImgs[0];
    //glGenTextures(1, &metallic);
    unsigned int roughness = pbrImgs[3];//loadTexture((DATA_ROOT + "book/book_pattern_rough_8k.png").c_str());
    unsigned int ao = pbrImgs[4];       //loadTexture((DATA_ROOT + "book/book_pattern_AO_8k.png").c_str());
    //unsigned int disp = loadTexture((DATA_ROOT + "SphereDisplacement.png").c_str());//pbrImgs[5];
    unsigned int disp = pbrImgs[5];

    // load PBR material textures
    // --------------------------
    // unsigned int albedo = loadTexture((DATA_ROOT + "rust/albedo.png").c_str());
    // unsigned int normal = loadTexture((DATA_ROOT + "rust/normal.png").c_str());
    // unsigned int metallic = loadTexture((DATA_ROOT + "rust/metallic.png").c_str());
    // unsigned int roughness = loadTexture((DATA_ROOT + "rust/roughness.png").c_str());
    // unsigned int ao = loadTexture((DATA_ROOT + "rust/ao.png").c_str());
    pbrTex envtex = setupPBR(&pbr, "HDRI-II.hdr");

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    proj_matrix = glm::perspective(FOV, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool vSync = true;
    bool Framerate = true;
    bool Camera = false;

    const char* BG_Textures[] = { "HDRI", "irradiance", "prefilter" };
    int whichBGTexture = 0;

    float bgLoD = 2.25;
    float dispFac = 0.;

    bool recordFrames = false;
    uint frameNumber = 0;

    //fuer fps
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    int fps = 0;
    double frameTime = 0.;
    // rendering loop
    double currentTime = 0.;
    while (glfwWindowShouldClose(window) == false) {
        if (!recordFrames) {
            currentTime = glfwGetTime();
        }else{
            currentTime += 0.01666666666;
        }
        nbFrames++;
        if (currentTime - lastTime >= 1.0) {// If last prinf() was more than 1 sec ago
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
        ImGui::Checkbox("Record", &recordFrames);
        ImGui::End();
        if (Framerate) {
            ImGui::Begin("Framerate");
            ImGui::Text("FPs: %04d", fps);
            ImGui::Text("avg. frametime: %04f", frameTime);
            ImGui::Checkbox("V-Sync", &vSync);
            ImGui::End();
        }
        if (Camera) {
            ImGui::Begin("Camera");
            ImGui::SliderFloat("FOV", &FOV, 90.0f, 5.0f);
            ImGui::SliderFloat("LoD for HDRI", &bgLoD, 0.0f, 6.0f);
            ImGui::SliderFloat("Displacement Factor", &dispFac, 0.0f, 1.0f);
            ImGui::Combo("BG Texture", &whichBGTexture, BG_Textures, 3);
            ImGui::End();
        }

        if (vSync) {
            glfwSwapInterval(1);
        } else {
            glfwSwapInterval(0);
        }

        proj_matrix = glm::perspective(glm::radians(FOV), static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

        glm::mat4 view_matrix = cam.view_matrix();

        glDepthFunc(GL_LEQUAL);
        glActiveTexture(GL_TEXTURE0);
        switch (whichBGTexture) {
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

        glShadeModel(GL_SMOOTH);
        boneObj.setFloat("displacementFactor", dispFac);
        boneObj.setMaticies(&view_matrix, &proj_matrix);
        boneObj.setVec3("camPos", cam.position());
        boneObj.render(currentTime);
        //boneObj.renderRotated(currentTime, dispFac);


        if (recordFrames) {
            frameNumber++;
            char str[7];
            snprintf (str, 7, "%06d", frameNumber);
            screenShotPNG((DATA_ROOT + "screenshots/s" + str + ".png").c_str(), WINDOW_WIDTH, WINDOW_HEIGHT);
        }

        // render UI
        imgui_render();

        glfwSwapBuffers(window);
    }

    cleanup_imgui();
    glfwTerminate();
}

void resizeCallback(GLFWwindow*, int width, int height)
{
    // set new width and height as viewport size
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);
}
