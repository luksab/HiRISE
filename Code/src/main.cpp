#include "glm/gtx/string_cast.hpp"
#include <glm/gtx/transform.hpp>
#include <random>
#include <stb_image.h>

#include "animation.hpp"
#include "boneObject.hpp"
#include "buffer.hpp"
#include "camera.hpp"
#include "common.hpp"
#include "mesh.hpp"
#include "pbrObject.hpp"
#include "pbrTex.hpp"
#include "pngImg.hpp"
#include "shader.hpp"
#include "spline.hpp"

#include <imgui.hpp>
#include <irrKlang.h>

int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;
float FOV = 45.;
const float NEAR_VALUE = 0.1f;
const float FAR_VALUE = 100.f;
GLFWmonitor* primary;
const GLFWvidmode* mode;
std::array<int, 2> windowPos { 0, 0 };
std::array<int, 2> windowSize { 0, 0 };

unsigned int fbo = 0;
unsigned int framebuffer_tex = 0;
unsigned int depth_rbo = 0;

bool paused;
bool inCameraView;

#ifndef M_PI
#define M_PI 3.14159265359
#endif

glm::mat4 proj_matrix;

void resizeCallback(GLFWwindow* window, int width, int height);
static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

float*
load_texture_data(std::string filename, int* width, int* height)
{
    stbi_set_flip_vertically_on_load(false);
    int channels;
    unsigned char* file_data = stbi_load(filename.c_str(), width, height, &channels, 3);

    int w = *width;
    int h = *height;

    float* data = new float[4 * w * h];
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            data[j * w * 4 + i * 4 + 0] = static_cast<float>(file_data[j * w * 3 + i * 3 + 0]) / 255;
            data[j * w * 4 + i * 4 + 1] = static_cast<float>(file_data[j * w * 3 + i * 3 + 1]) / 255;
            data[j * w * 4 + i * 4 + 2] = static_cast<float>(file_data[j * w * 3 + i * 3 + 2]) / 255;
            data[j * w * 4 + i * 4 + 3] = 1.f;
        }
    }

    stbi_image_free(file_data);

    return data;
}

float*
load_pds_data(std::string filename, int* width, int* height, int* channels)
{
    FILE* fp = fopen(filename.c_str(), "r");
    char* line = NULL;
    size_t len = 0;
    int skipLen = 0;
    ssize_t read;

    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1) {
        skipLen += read;
        if (strncmp(line, "  LINES            = ", 21) == 0)// 21 = len("  LINES            = ")
        {
            *height = atoi(line + 21);
        } else if (strncmp(line, "  LINE_SAMPLES     = ", 21) == 0) {
            *width = atoi(line + 21);
        } else if ((strncmp(line, "END\n", 4) == 0 || strncmp(line, "END\r", 4) == 0) && read < 6) {
            break;
        }
    }

    printf("height: %d\n", *height);
    printf("width: %d\n", *width);

    fclose(fp);
    if (line)
        free(line);

    FILE* f = fopen(filename.c_str(), "rb");

    int w = *width;
    int h = *height;

    float* data = new float[*channels * w * h];
    long filelen = w * h * *channels;

    float* buffer = (float*)malloc(filelen * sizeof(float));// Enough memory for the file
    fread(buffer, sizeof(float), filelen, f);               // Read in the entire file
    fclose(f);
    for (int j = 0; j < h; ++j) {
        for (int i = 0; i < w; ++i) {
            for (int k = 0; k < *channels; k++) {
                data[j * w * *channels + i * *channels + k] = buffer[j * w * *channels + i * *channels + k];
                data[j * w * *channels + i * *channels + k] += 4185.03;
                if (data[j * w * *channels + i * *channels + k] < -5000 || data[j * w * *channels + i * *channels + k] > 5000) {
                    data[j * w * *channels + i * *channels + k] = -10000;
                }

                data[j * w * *channels + i * *channels + k] /= 1938.42;
            }
        }
    }

    free(buffer);

    return data;
}

unsigned int
create_texture_rgba32f(int width, int height, float* data)
{
    unsigned int handle;
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);
    glTextureStorage2D(handle, 1, GL_RGBA32F, width, height);
    glTextureSubImage2D(handle, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);

    return handle;
}

unsigned int
create_texture_rgba32f(int width, int height)
{
    unsigned int handle;
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);
    glTextureStorage2D(handle, 1, GL_RGBA32F, width, height);

    return handle;
}

void build_framebuffer(int width, int height)
{
    if (framebuffer_tex) {
        glDeleteTextures(1, &framebuffer_tex);
    }

    if (depth_rbo) {
        glDeleteRenderbuffers(1, &depth_rbo);
    }

    if (fbo) {
        glDeleteFramebuffers(1, &fbo);
    }

    framebuffer_tex = create_texture_rgba32f(width, height);
    glCreateRenderbuffers(1, &depth_rbo);
    glNamedRenderbufferStorage(depth_rbo, GL_DEPTH24_STENCIL8, width, height);

    glCreateFramebuffers(1, &fbo);
    glNamedFramebufferTexture(fbo, GL_COLOR_ATTACHMENT0, framebuffer_tex, 0);
    glNamedFramebufferRenderbuffer(fbo, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depth_rbo);
    if (glCheckNamedFramebufferStatus(fbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("Incomplete FBO!");
        std::terminate();
    }
}

unsigned int
setup_fullscreen_quad()
{
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f
    };

    unsigned int indices[] = {
        0, 1, 2, 2, 3, 0
    };

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO = makeBuffer(GL_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(vertices), vertices);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int IBO = makeBuffer(GL_ELEMENT_ARRAY_BUFFER, GL_STATIC_DRAW, sizeof(indices), indices);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, IBO);

    return VAO;
}

unsigned int
create_texture_r32f(int width, int height, float* data)
{
    unsigned int handle;
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);
    glTextureStorage2D(handle, 1, GL_R32F, width, height);
    glTextureSubImage2D(handle, 0, 0, 0, width, height, GL_RED, GL_FLOAT, data);

    return handle;
}

void set_texture_wrap_mode(unsigned int texture, GLenum mode)
{
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, mode);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, mode);
}

struct Plane {
public:
    glm::vec3 norm;
    float d;

    Plane() {}
    Plane(float a, float b, float c, float d)
        : norm(a, b, c)
        , d(d)
    {
    }

    void FromTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2);
    void Normalize();
    glm::mat4 MakeReflectionMatrix();
};

void Plane::FromTriangle(const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2)
{
    norm = cross(p1 - p0, p2 - p0);
    norm = glm::normalize(norm);
    d = -dot(norm, p0);
}

void Plane::Normalize()
{
    float len = norm.x * norm.x + norm.y * norm.y + norm.z * norm.z;
    if (len > 0.00001f) {
        len = 1.0f / len;
        norm.x *= len;
        norm.y *= len;
        norm.z *= len;
        d *= len;
        return;
    }
    norm.y = 1;
}

glm::mat4 Plane::MakeReflectionMatrix()
{
    //Normalize(); // let's expect it to be already normalized
    glm::mat4 m;
    m[0][0] = -2 * norm.x * norm.x + 1;
    m[0][1] = -2 * norm.y * norm.x;
    m[0][2] = -2 * norm.z * norm.x;
    m[0][3] = 0;

    m[1][0] = -2 * norm.x * norm.y;
    m[1][1] = -2 * norm.y * norm.y + 1;
    m[1][2] = -2 * norm.z * norm.y;
    m[1][3] = 0;

    m[2][0] = -2 * norm.x * norm.z;
    m[2][1] = -2 * norm.y * norm.z;
    m[2][2] = -2 * norm.z * norm.z + 1;
    m[2][3] = 0;

    m[3][0] = -2 * norm.x * d;
    m[3][1] = -2 * norm.y * d;
    m[3][2] = -2 * norm.z * d;
    m[3][3] = 1;
    return m;
}

int main(void)
{
    irrklang::ISoundEngine* SoundEngine = irrklang::createIrrKlangDevice();
    if (!SoundEngine) {
        printf("Could not start irrklang");// error starting up the engine
        assert(0);
    }
    irrklang::ISound* music = SoundEngine->play2D((DATA_ROOT + "audio/breakout.ogg").c_str(), true, false, true);
    music->setVolume(0.1);

    Plane p;
    p.FromTriangle(glm::vec3(0, 0.5, 0), glm::vec3(1, 0.7, 0), glm::vec3(0, 7, 0));

    glm::mat4 r = p.MakeReflectionMatrix();

    GLFWwindow* window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, "HiRISE");
    primary = glfwGetPrimaryMonitor();
    mode = glfwGetVideoMode(primary);
    glfwSetKeyCallback(window, key_callback);

    glfwSetFramebufferSizeCallback(window, resizeCallback);
    GLint tessLvl;
    glGetIntegerv(GL_MAX_TESS_GEN_LEVEL, &tessLvl);
    printf("max tess lvl: %d\n", tessLvl);

    camera cam(window);
    camera_state* state = cam.getState();

    spline CamPosSpline;
    CamPosSpline.addPoint((splinePoint) { 0., glm::vec3(0., 0.5, 0.) });
    CamPosSpline.addPoint((splinePoint) { 1., glm::vec3(1., 0., 0.) });
    CamPosSpline.addPoint((splinePoint) { 2., glm::vec3(2., 1., 0.) });
    CamPosSpline.addPoint((splinePoint) { 3., glm::vec3(3., 0., 0.) });
    CamPosSpline.addPoint((splinePoint) { 4., glm::vec3(4., 0., 3.) });
    CamPosSpline.addPoint((splinePoint) { 5., glm::vec3(5., 0., 0.) });
    CamPosSpline.addPoint((splinePoint) { 6., glm::vec3(6., 0., 0.) });

    init_imgui(window);

    printf("loading models\n");
    geometry model = loadMesh("hiresUV.obj", false, glm::vec4(0.f, 0.f, 0.f, 1.f));
    pbrObject mars = {};
    animated marsAnim = toAnimated(model);
    mars.setup(&marsAnim, "main.vert", "main.frag", "main.tess", "main.tesse");
    mars.defaultMat = true;
    mars.useTessellation = true;

    animated glass = loadMeshAnim("shard.dae", 1., true);
    pbrObject glassObj = {};
    glassObj.setup(&glass, "glass/glass.vert", "glass/glass.frag");
    glassObj.defaultMat = true;

    animated pbr = loadMeshAnim("cube.dae", true);

    pbrObject renderCube = {};
    renderCube.setup(&pbr, "cubeMap/hdr.vert", "cubeMap/hdr.frag");
    renderCube.defaultMat = true;
    renderCube.setInt("environmentMap", 0);

    bones human = loadMeshBone("Lowpolymesh_Eliber.dae", false);
    boneObject humanObj = {};
    humanObj.setup(&human, false);
    humanObj.use();
    humanObj.scale(0.1);
    humanObj.move(0., 7., 0.);

    animated table = loadMeshAnim("table.dae", true);
    pbrObject tableObj = {};
    tableObj.setup(&table, true);
    tableObj.setInt("heightMap", 5);
    tableObj.setFloat("displacementFactor", 0.);

    printf("loading model textures\n");
    char* path = "rock_ground";
    std::vector<mapTexture> rockTex;
    rockTex.resize(9);
    rockTex[0].type = GL_TEXTURE_2D;
    rockTex[0].spot = 3;
    rockTex[0].texture = loadTexture((DATA_ROOT + path + "/" + path + "_diff_8k.jpg").c_str());
    rockTex[1].type = GL_TEXTURE_2D;
    rockTex[1].spot = 4;
    rockTex[1].texture = loadTexture((DATA_ROOT + path + "/" + path + "_nor_8k.jpg").c_str());
    rockTex[2].type = GL_TEXTURE_2D;
    rockTex[2].spot = 5;
    glGenTextures(1, &rockTex[2].texture);
    rockTex[3].type = GL_TEXTURE_2D;
    rockTex[3].spot = 6;
    rockTex[3].texture = loadTexture((DATA_ROOT + path + "/" + path + "_rough_8k.jpg").c_str());
    rockTex[4].type = GL_TEXTURE_2D;
    rockTex[4].spot = 7;
    rockTex[4].texture = loadTexture((DATA_ROOT + path + "/" + path + "_ao_8k.jpg").c_str());
    rockTex[5].type = GL_TEXTURE_2D;
    rockTex[5].spot = 8;
    rockTex[5].texture = loadTexture((DATA_ROOT + path + "/" + path + "_disp_8k.jpg").c_str());

    pbrTex envtex = setupPBR(&pbr);
    rockTex[6].type = GL_TEXTURE_2D;
    rockTex[6].spot = 0;
    rockTex[6].texture = envtex.irradianceMap;
    rockTex[7].type = GL_TEXTURE_2D;
    rockTex[7].spot = 0;
    rockTex[7].texture = envtex.prefilterMap;
    rockTex[8].type = GL_TEXTURE_2D;
    rockTex[8].spot = 0;
    rockTex[8].texture = envtex.brdfLUTTexture;

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    proj_matrix = glm::perspective(FOV, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

    glm::mat4 ident = glm::mat4(1.);

    int image_width, image_height;
    float* image_tex_data = load_texture_data(DATA_ROOT + "ESP_041121_1725_RED_A_01_ORTHO_quarter.jpg", &image_width, &image_height);
    int pds_width;
    int pds_height;
    int pds_channels = 1;
    float* pds_tex_data = load_pds_data(DATA_ROOT + "DTEEC_048136_1725_041121_1725_A01.img", &pds_width, &pds_height, &pds_channels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned int image_tex = create_texture_rgba32f(image_width, image_height, image_tex_data);
    set_texture_wrap_mode(image_tex, GL_CLAMP_TO_BORDER);
    unsigned int pds_tex = create_texture_r32f(pds_width, pds_height, pds_tex_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTextureUnit(0, image_tex);
    glBindTextureUnit(1, pds_tex);
    glTextureParameteri(image_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    mars.setInt("height", 1);

    delete[] image_tex_data;
    delete[] pds_tex_data;

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // UI variables
    float colaH = 0.094;
    float colaS = 0.263;
    float colaV = 0.636;
    float colbH = 0.162;
    float colbS = 0.122;
    float colbV = 0.606;
    float tessFactor = 5;
    float discardFactor = 1.055;
    float h = 3.0;

    float glass_power = 2.0;
    float glass_factor = 1.0;

    float volume = 0.5;

    bool vSync = true;
    bool rotate = false;
    bool Framerate = true;
    bool lineRendering = false;
    bool Camera = false;
    bool CameraMove = false;
    bool Color = false;
    bool Music = false;
    bool Draw = false;
    bool mirror = false;

    bool drawObjs[5] = { false, false, true, false, false };

    //for deferred rendering
    build_framebuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
    unsigned int quad = setup_fullscreen_quad();
    unsigned int vertexShaderCompose = compileShader("deferred/deferred_compose.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShaderCompose = compileShader("deferred/deferred_compose.frag", GL_FRAGMENT_SHADER);
    unsigned int shaderProgramCompose = linkProgram(vertexShaderCompose, fragmentShaderCompose);
    glUseProgram(shaderProgramCompose);
    // bind framebuffer texture to shader variable
    int tex_loc = glGetUniformLocation(shaderProgramCompose, "tex");

    // timing variables
    double lastTime = glfwGetTime();
    double lastGLTime = lastTime;
    int nbFrames = 0;
    int fps = 0;
    double frameTime = 0.;
    double dt = 0;
    float currentTime = 0;
    float timeScale = 1.0;
    paused = false;
    // rendering loop
    while (glfwWindowShouldClose(window) == false) {
        dt = glfwGetTime() - lastGLTime;
        lastGLTime = glfwGetTime();
        currentTime += dt * timeScale * paused;
        nbFrames++;
        if (glfwGetTime() - lastTime >= 1.0) {// If last prinf() was more than 1 sec ago
            // reset timer
            fps = nbFrames;
            frameTime = 1000.0 / double(nbFrames);
            nbFrames = 0;
            lastTime += 1.0;
        }

        // define UI
        imgui_new_frame(400, 200);
        ImGui::Begin("General");
        ImGui::Checkbox("Framerate", &Framerate);
        ImGui::Checkbox("Camera", &Camera);
        ImGui::Checkbox("CameraControl", &CameraMove);
        ImGui::Checkbox("Color", &Color);
        ImGui::Checkbox("Draw", &Draw);
        ImGui::Checkbox("Music", &Music);
        ImGui::End();
        if (Framerate) {
            ImGui::Begin("Framerate");
            ImGui::Text("FPs: %04d", fps);
            ImGui::Text("avg. frametime: %04f", frameTime);
            ImGui::Checkbox("V-Sync", &vSync);
            ImGui::SliderFloat("timeScale", &timeScale, 0.0f, 2.0f);
            ImGui::End();
        }
        if (Camera) {
            ImGui::Begin("Camera");
            ImGui::SliderFloat("float", &FOV, 10.0f, 90.0f);
            ImGui::Checkbox("rotate", &rotate);
            ImGui::SliderFloat("tessFactor", &tessFactor, 0.0f, 20.0f);
            ImGui::Checkbox("render using lines", &lineRendering);
            ImGui::Checkbox("mirror", &mirror);
            ImGui::End();
        }
        if (CameraMove) {
            ImGui::Begin("Camera");
            ImGui::Checkbox("toggle", &inCameraView);
            ImGui::SliderFloat("time", &currentTime, 0.0f, 20.0f);
            ImGui::End();
        }
        if (Color) {
            ImGui::Begin("Color");
            // ImGui::SliderFloat("colaH", &colaH, 0.0f, 1.0f);
            // ImGui::SliderFloat("colaS", &colaS, 0.0f, 1.0f);
            // ImGui::SliderFloat("colaV", &colaV, 0.0f, 1.0f);
            // ImGui::SliderFloat("colbH", &colbH, 0.0f, 1.0f);
            // ImGui::SliderFloat("colbS", &colbS, 0.0f, 1.0f);
            // ImGui::SliderFloat("colbV", &colbV, 0.0f, 1.0f);
            ImGui::SliderFloat("glass_power", &glass_power, 0.5f, 4.0f);
            ImGui::SliderFloat("glass_factor", &glass_factor, 0.0f, 5.0f);
            ImGui::SliderFloat("discardFactor", &discardFactor, 1.f, 1.1f);
            ImGui::SliderFloat("h", &h, 1.f, 7.f);
            ImGui::End();
        }
        if (Draw) {
            ImGui::Begin("Draw");
            ImGui::Checkbox("human", &(drawObjs[0]));
            ImGui::Checkbox("glass", &(drawObjs[1]));
            ImGui::Checkbox("mars", &(drawObjs[2]));
            ImGui::Checkbox("reflexion", &(drawObjs[3]));
            ImGui::Checkbox("table", &(drawObjs[4]));
            ImGui::End();
        }
        if(Music){
            ImGui::Begin("Music");
            ImGui::SliderFloat("Volume", &volume, 0.f, 1.0f);
            ImGui::End();
        }
        music->setVolume(volume);

        mars.setVec3("colorA", colaH, colaS, colaV);
        mars.setVec3("colorB", colbH, colbS, colbV);

        mars.setFloat("tessFactor", tessFactor);
        mars.setFloat("discardFactor", discardFactor);
        mars.setFloat("h", pow(10., -h));

        if (lineRendering) {
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        } else {
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
        }

        if (rotate) {// let the camera rotate slowly
            state->phi += 0.1 * dt;
            //state->look_at.x -= 0.01;
            cam.update();
        }

        if (inCameraView) {                                                              // animate the camera using keyframes
            state->look_at = CamPosSpline.eval(fmod(currentTime, CamPosSpline.length()));//spline(fmod(currentTime, (cameraPositions.back().time + 1)), cameraPositions);
            printf("vec3(%lf,%lf,%lf)\n", state->look_at[0], state->look_at[1], state->look_at[2]);
            //state->phi += 0.1 * dt;
            //state->look_at.x -= 0.01;
            cam.update();
        }

        if (vSync) {// turn vSync on
            glfwSwapInterval(1);
        } else {// or off
            glfwSwapInterval(0);
        }

        proj_matrix = glm::perspective(glm::radians(FOV), static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);
        //printf("%s\n",glm::to_string(proj_matrix).c_str());

        glm::mat4 view_matrix = cam.view_matrix();

        glfwPollEvents();
        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
        glStencilMask(0xFF);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_STENCIL_TEST);
        //Draw normal scene
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);// Do draw any pixels on the back buffer

        if (drawObjs[2]) {// render mars
            glBindTextureUnit(0, image_tex);
            glBindTextureUnit(1, pds_tex);
            mars.setMaticies(&view_matrix, &proj_matrix);
            mars.render(0);
        }

        if (drawObjs[0]) {// render human
            // bind pre-computed IBL data
            bindTextures(rockTex);

            glShadeModel(GL_SMOOTH);
            humanObj.setFloat("displacementFactor", 0.);
            humanObj.setMaticies(&view_matrix, &proj_matrix);
            humanObj.setVec3("camPos", cam.position());
            humanObj.render(currentTime);
        }
        if (drawObjs[4]) {//render table
            bindTextures(rockTex);
            tableObj.setMaticies(&view_matrix, &proj_matrix);
            tableObj.setVec3("camPos", cam.position());
            tableObj.render(ident);
        }

        if (drawObjs[3]) {
            //Draw in stencil first
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);// Do not draw any pixels on the back buffer
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);        // Set any stencil to 1
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);// Only write when both tests pass
            glDepthMask(GL_FALSE);                    // Don't write to depth buffer
            glassObj.setMaticies(&view_matrix, &proj_matrix);
            glassObj.render(currentTime);

            // render mirrored version
            glStencilFunc(GL_EQUAL, 1, 0xFF);      // only draw when there is reflection
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);// dont change stencil
            glDepthMask(GL_TRUE);                  // enable depth test
            glEnable(GL_DEPTH_TEST);

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);// draw pixels on the back buffer
            view_matrix = cam.view_matrix() * (glass.matrixAt(currentTime)) * r;

            glBindTextureUnit(0, image_tex);
            glBindTextureUnit(1, pds_tex);
            mars.setMaticies(&view_matrix, &proj_matrix);
            mars.render(0);

            bindTextures(rockTex);
            tableObj.setMaticies(&view_matrix, &proj_matrix);
            tableObj.setVec3("camPos", cam.position());
            tableObj.render(ident);

            humanObj.setMaticies(&view_matrix, &proj_matrix);
            humanObj.setVec3("camPos", cam.position());
            humanObj.render(currentTime);
        }

        glStencilFunc(GL_ALWAYS, 1, 0x00);     // Always pass stencil
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);// dont change stencil

        // render Background
        view_matrix = cam.view_matrix();
        glDepthFunc(GL_LEQUAL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.hdrTexture);
        glUseProgram(renderCube.shaderProgram);
        renderCube.setMaticies(&view_matrix, &proj_matrix);
        renderCube.render(0);
        glDepthFunc(GL_LESS);

        // render transparency last
        if (drawObjs[1]) {// render glass
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.hdrTexture);
            glassObj.setMaticies(&view_matrix, &proj_matrix);
            glassObj.setFloat("factor", glass_factor);
            glassObj.setFloat("power", glass_power);
            glassObj.render(currentTime);
        }

        // COMPOSE PASS
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glUseProgram(shaderProgramCompose);
        glBindVertexArray(quad);
        glBindTextureUnit(0, framebuffer_tex);
        glUniform1i(tex_loc, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        // render UI
        imgui_render();

        glfwSwapBuffers(window);
    }

    SoundEngine->drop();
    cleanup_imgui();
    glfwTerminate();
}

void resizeCallback(GLFWwindow*, int width, int height)
{
    // set new width and height as viewport size
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);

    build_framebuffer(WINDOW_WIDTH, WINDOW_HEIGHT);
}

bool IsFullscreen(GLFWwindow* window)
{
    return glfwGetWindowMonitor(window) != nullptr;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    if (action == GLFW_PRESS)
        switch (key) {
        case GLFW_KEY_ESCAPE:
            glfwSetWindowShouldClose(window, GLFW_TRUE);
            break;
        case GLFW_KEY_F11:
            if (IsFullscreen(window)) {
                glfwSetWindowMonitor(window, nullptr, windowPos[0], windowPos[1], windowSize[0], windowSize[1], 60);
            } else {
                glfwGetWindowPos(window, &windowPos[0], &windowPos[1]);
                glfwGetWindowSize(window, &windowSize[0], &windowSize[1]);
                glfwSetWindowMonitor(window, primary, 0, 0, mode->width, mode->height, 60);
            }
            break;
        case GLFW_KEY_SPACE:
            paused = !paused;
            break;
        case GLFW_KEY_KP_0:
            inCameraView = !inCameraView;
            break;
        default:
            break;
        }
}
