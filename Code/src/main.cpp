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
#include <thread>

int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;
float FOV = 45.;
const float NEAR_VALUE = 0.1f;
const float FAR_VALUE = 1000.f;
GLFWmonitor* primary;
const GLFWvidmode* mode;
ImGuiIO* io;
std::array<int, 2> windowPos { 0, 0 };
std::array<int, 2> windowSize { 0, 0 };

unsigned int fbo = 0;
unsigned int framebuffer_tex = 0;
unsigned int depth_rbo = 0;

bool playing;
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

unsigned int setupComputeShader()
{
    // load and compile shaders and link program
    unsigned int computeShader = compileShader("compute/computeMars.cs", GL_COMPUTE_SHADER);
    unsigned int shaderProgram = linkProgram(computeShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(computeShader);
    return shaderProgram;
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
    d = dot(norm, p0);
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

struct ImageThreadCall {
    unsigned char** data;
    unsigned char* dataP;
    std::string path0;
    int* widthP;
    int* heightP;
    int* nrComponentsP;
    int width;
    int height;
    int nrComponents;
    std::thread t;
};

int main(void)
{
    irrklang::ISoundEngine* SoundEngine = irrklang::createIrrKlangDevice();
    if (!SoundEngine) {
        printf("Could not start irrklang");// error starting up the engine
        assert(0);
    }
    irrklang::ISound* music = SoundEngine->play2D((DATA_ROOT + "audio/breakout.ogg").c_str(), true, false, true);
    music->setVolume(0.);

    Plane p;
    p.FromTriangle(glm::vec3(5.2, 0, 0), glm::vec3(5.2, 1, 0), glm::vec3(5.2, 0, 1));

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

    spline<vector<float>> CamPosSpline(7);
    {
        vector<float> pos;
        pos.push_back(cam.position()[0]);
        pos.push_back(cam.position()[1]);
        pos.push_back(cam.position()[2]);
        pos.push_back(state->radius);
        pos.push_back(state->phi);
        pos.push_back(state->theta);
        pos.push_back(1.);
        CamPosSpline.addPoint(0., pos);
    }
    vector<float> keyFrames;
    keyFrames.push_back(0.);
    CamPosSpline.loadFrom(DATA_ROOT + "camPos");
    //CamPosSpline.print();

    init_imgui(window);
    io = &ImGui::GetIO();
    (void)io;

    printf("loading models\n");
    geometry model = loadMesh("hiresUV.obj", false, glm::vec4(0.f, 0.f, 0.f, 1.f));
    pbrObject mars = {};
    animated marsAnim = toAnimated(model);
    mars.setup(&marsAnim, "main.vert", "mainSimple.frag", "main.tess", "main.tesse");
    glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(1000., 1000., 1000.));
    scaleMat = glm::translate(scaleMat, glm::vec3(0.0, -0.13, 0.0));
    marsAnim.transform[0] = scaleMat;
    mars.defaultMat = true;
    mars.useTessellation = true;

    animated glass = loadMeshAnim("HiRISE/oneGlass.dae", 1., true);
    pbrObject glassObj = {};
    glassObj.setup(&glass, "glass/glass.vert", "glass/glass.frag");
    scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(3.3, 3.3, 3.3));
    scaleMat = glm::translate(scaleMat, glm::vec3(0.0, -8.41, 0.0));
    scaleMat = glm::rotate(scaleMat, (float)M_PI, glm::vec3(0., 1., 0.));
    glass.transform[0] = scaleMat;
    glassObj.defaultMat = true;

    animated textModel = loadMeshAnim("HiRISE/text.dae", 1., true);
    pbrObject textObj = {};
    textObj.setup(&textModel, "text/text.vert", "text/text.frag");
    scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(3.3, 3.3, 3.3));
    scaleMat = glm::translate(scaleMat, glm::vec3(0.0, -8.41, 0.0));
    scaleMat = glm::rotate(scaleMat, (float)-M_PI_2, glm::vec3(0., 1., 0.));
    textModel.transform[0] = scaleMat;
    textObj.defaultMat = true;

    animated hirise = loadMeshAnim("HiRISE/HiRISE.dae", 1., true);
    pbrObject hiriseObj = {};
    //hiriseObj.setup(&hirise, "simple/simple.vert", "simple/simple.frag");
    hiriseObj.setup(&hirise, "shadowMap/point_shadows.vs", "shadowMap/point_shadows.fs");
    hiriseObj.setInt("diffuseTexture", 0);
    hiriseObj.setInt("depthMap", 1);
    hiriseObj.setInt("levels", 5);
    scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(3.3, 3.3, 3.3));
    scaleMat = glm::translate(scaleMat, glm::vec3(0.0, -8.41, 0.0));
    scaleMat = glm::rotate(scaleMat, (float)M_PI, glm::vec3(0., 1., 0.));
    hirise.transform[0] = scaleMat;
    hiriseObj.defaultMat = true;

    animated chair = loadMeshAnim("HiRISE/chair.dae", true);//toAnimated(loadMesh("chair.dae", false, glm::vec4(0.f, 0.f, 0.f, 1.f)));
    pbrObject chairObj = {};
    // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.26, 0.26, 0.26));
    // scaleMat = glm::translate(scaleMat, glm::vec3(0., 0.49, -0.12));
    // chair.transform[0] = scaleMat;
    // chairObj.setup(&chair, "simple/simple.vert", "simple/simple.frag");
    chairObj.setup(&chair, "shadowMap/point_shadows.vs", "shadowMap/point_shadows.fs");
    chairObj.setInt("diffuseTexture", 0);
    chairObj.setInt("depthMap", 1);
    chairObj.setInt("levels", 5);
    chair.transform[0] = glm::mat4(1);
    chairObj.defaultMat = true;

    animated monitor = loadMeshAnim("HiRISE/monitor.dae", true);//toAnimated(loadMesh("chair.dae", false, glm::vec4(0.f, 0.f, 0.f, 1.f)));
    pbrObject monitorObj = {};
    // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.26, 0.26, 0.26));
    // scaleMat = glm::translate(scaleMat, glm::vec3(0., 0.49, -0.12));
    // chair.transform[0] = scaleMat;
    monitorObj.setup(&monitor, "simple/simple.vert", "simple/simple.frag");
    monitor.transform[0] = glm::mat4(1);
    monitorObj.defaultMat = true;

    animated pbr = loadMeshAnim("cube.dae", true);

    pbrObject renderCube = {};
    renderCube.setup(&pbr, "cubeMap/hdr.vert", "cubeMap/hdr.frag");
    renderCube.defaultMat = true;
    renderCube.setInt("environmentMap", 0);

    bones human = loadMeshBone("Lowpolymesh_Eliber_start.dae", false);
    boneObject humanObj = {};
    humanObj.setup(&human, false);
    humanObj.use();
    humanObj.scale(0.16);
    humanObj.move(0., 5.96, -0.29);

    // animated table = loadMeshAnim("table.dae", true);
    // pbrObject tableObj = {};
    // // scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.47, 0.47, 0.47));
    // // scaleMat = glm::translate(scaleMat, glm::vec3(0.0, 0.25, 0.86));
    // // table.transform[0] = scaleMat;
    // tableObj.setup(&table, true);
    // tableObj.setInt("heightMap", 5);
    // tableObj.setFloat("displacementFactor", 0.);
    std::vector<mapTexture> hiriseTex;
    hiriseTex.resize(1);
    hiriseTex[0].type = GL_TEXTURE_2D;
    hiriseTex[0].spot = 0;
    hiriseTex[0].texture = loadTexture((DATA_ROOT + "hirise.png").c_str());

    std::vector<mapTexture> indoorTex;
    indoorTex.resize(1);
    indoorTex[0].type = GL_TEXTURE_2D;
    indoorTex[0].spot = 0;
    indoorTex[0].texture = loadTexture((DATA_ROOT + "Atlas.png").c_str());

    printf("loading model textures\n");
    auto start = glfwGetTime();
    const char* path = "rock_ground";

    ImageThreadCall diff = {};
    diff.data = &diff.dataP;
    diff.path0 = DATA_ROOT + path + "/" + path + "_diff_8k.jpg";
    diff.widthP = &diff.width;
    diff.heightP = &diff.height;
    diff.nrComponentsP = &diff.nrComponents;
    diff.t = std::thread(loadTextureData, diff.data, diff.path0.c_str(), diff.widthP, diff.heightP, diff.nrComponentsP);

    ImageThreadCall nor = {};
    nor.data = &nor.dataP;
    nor.path0 = DATA_ROOT + path + "/" + path + "_nor_8k.jpg";
    nor.widthP = &nor.width;
    nor.heightP = &nor.height;
    nor.nrComponentsP = &nor.nrComponents;
    nor.t = std::thread(loadTextureData, nor.data, nor.path0.c_str(), nor.widthP, nor.heightP, nor.nrComponentsP);

    ImageThreadCall rough = {};
    rough.data = &rough.dataP;
    rough.path0 = DATA_ROOT + path + "/" + path + "_rough_8k.jpg";
    rough.widthP = &rough.width;
    rough.heightP = &rough.height;
    rough.nrComponentsP = &rough.nrComponents;
    rough.t = std::thread(loadTextureData, rough.data, rough.path0.c_str(), rough.widthP, rough.heightP, rough.nrComponentsP);

    ImageThreadCall ao = {};
    ao.data = &ao.dataP;
    ao.path0 = DATA_ROOT + path + "/" + path + "_ao_8k.jpg";
    ao.widthP = &ao.width;
    ao.heightP = &ao.height;
    ao.nrComponentsP = &ao.nrComponents;
    ao.t = std::thread(loadTextureData, ao.data, ao.path0.c_str(), ao.widthP, ao.heightP, ao.nrComponentsP);

    ImageThreadCall disp = {};
    disp.data = &disp.dataP;
    disp.path0 = DATA_ROOT + path + "/" + path + "_disp_8k.jpg";
    disp.widthP = &disp.width;
    disp.heightP = &disp.height;
    disp.nrComponentsP = &disp.nrComponents;
    disp.t = std::thread(loadTextureData, disp.data, disp.path0.c_str(), disp.widthP, disp.heightP, disp.nrComponentsP);

    diff.t.join();
    nor.t.join();
    rough.t.join();
    ao.t.join();
    disp.t.join();
    auto stop = glfwGetTime();
    auto duration = stop - start;
    cout << "loading textures took " << duration << "s" << endl;
    start = glfwGetTime();
    std::vector<mapTexture> rockTex;
    rockTex.resize(10);
    rockTex[0].type = GL_TEXTURE_2D;
    rockTex[0].spot = 3;
    rockTex[0].texture = loadTexture(diff.dataP, diff.width, diff.height, diff.nrComponents);
    rockTex[1].type = GL_TEXTURE_2D;
    rockTex[1].spot = 4;
    rockTex[1].texture = loadTexture(nor.dataP, nor.width, nor.height, nor.nrComponents);
    rockTex[2].type = GL_TEXTURE_2D;
    rockTex[2].spot = 5;
    glGenTextures(1, &rockTex[2].texture);
    rockTex[3].type = GL_TEXTURE_2D;
    rockTex[3].spot = 6;
    rockTex[3].texture = loadTexture(rough.dataP, rough.width, rough.height, rough.nrComponents);
    rockTex[4].type = GL_TEXTURE_2D;
    rockTex[4].spot = 7;
    rockTex[4].texture = loadTexture(ao.dataP, ao.width, ao.height, ao.nrComponents);
    rockTex[5].type = GL_TEXTURE_2D;
    rockTex[5].spot = 8;
    rockTex[5].texture = loadTexture(disp.dataP, disp.width, disp.height, disp.nrComponents);
    stop = glfwGetTime();
    duration = stop - start;
    cout << "loading textures to GPU took " << duration << "s" << endl;

    start = glfwGetTime();
    pbrTex envtex = setupPBR(&pbr, "HDRI-II.hdr");
    rockTex[6].type = GL_TEXTURE_CUBE_MAP;
    rockTex[6].spot = 0;
    rockTex[6].texture = envtex.irradianceMap;
    rockTex[7].type = GL_TEXTURE_CUBE_MAP;
    rockTex[7].spot = 1;
    rockTex[7].texture = envtex.prefilterMap;
    rockTex[8].type = GL_TEXTURE_CUBE_MAP;
    rockTex[8].spot = 2;
    rockTex[8].texture = envtex.brdfLUTTexture;
    stop = glfwGetTime();
    duration = stop - start;
    cout << "loading hdri textures took " << duration << "s" << endl;

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    //glEnable(GL_CULL_FACE);
    proj_matrix = glm::perspective(FOV, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);

    // configure depth map FBO
    // -----------------------
    const unsigned int SHADOW_WIDTH = 1024, SHADOW_HEIGHT = 1024;
    unsigned int depthMapFBO;
    glGenFramebuffers(1, &depthMapFBO);
    // create depth cubemap texture
    unsigned int depthCubemap;
    glGenTextures(1, &depthCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
    for (unsigned int i = 0; i < 6; ++i)
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    // attach depth texture as FBO's depth buffer
    glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
    glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, depthCubemap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // build and compile shaders
    // -------------------------
    pbrObject shader = {};
    shader.setup(&pbr, "shadowMap/point_shadows.vs", "shadowMap/point_shadows.fs");
    shader.setInt("diffuseTexture", 0);
    shader.setInt("depthMap", 1);
    pbrObject simpleDepthShader = {};
    simpleDepthShader.setup(&pbr, "shadowMap/point_shadows_depth.vs", "shadowMap/point_shadows_depth.fs", "shadowMap/point_shadows_depth.gs");
    pbrObject skinningDepthShader = {};
    skinningDepthShader.setup(&pbr, "shadowMap/pbrS.vs", "shadowMap/point_shadows_depth.fs", "shadowMap/point_shadows_depth.gs");
    // lighting info
    // -------------
    glm::vec3 lightPos(0.0f, 3.0f, 0.0f);

    glm::mat4 ident = glm::mat4(1.);

    printf("loading mars data\n");
    start = glfwGetTime();
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

    glBindTextureUnit(0, image_tex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTextureParameteri(image_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
    glBindTextureUnit(1, pds_tex);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTextureParameteri(pds_tex, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);

    int tex_w = pds_width, tex_h = pds_height;
    GLuint tex_output;
    glGenTextures(1, &tex_output);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, tex_output);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, tex_w, tex_h, 0, GL_RGBA, GL_FLOAT,
        NULL);
    glBindImageTexture(0, tex_output, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

    glBindTextureUnit(1, image_tex);
    glBindTextureUnit(2, pds_tex);

    unsigned int computeProgram = setupComputeShader();
    glUseProgram(computeProgram);

    int computeImage_loc = glGetUniformLocation(computeProgram, "tex");
    int computeHeight_loc = glGetUniformLocation(computeProgram, "height");

    glUniform1i(computeImage_loc, 1);
    glUniform1i(computeHeight_loc, 2);

    glDispatchCompute((GLuint)tex_w, (GLuint)tex_h, 1);

    glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

    mars.setInt("height", 1);
    stop = glfwGetTime();
    duration = stop - start;
    cout << "loading hdri textures took " << duration << "s" << endl;

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

    glm::vec4 textColor = glm::vec4(1.11,1.26,1.68,1.0);
    textObj.setVec4("color", textColor);

    float irradianceR = 0.815;
    float irradianceG = 1.;
    float irradianceB = 1.;

    float tessFactor = 5;
    float discardFactor = 1.055;
    float h = 3.0;

    float glass_power = 2.0;
    float glass_factor = 1.0;

    float volume = 0.;

    float bias = 3.460;

    bool vSync = true;
    bool rotate = false;
    bool Framerate = true;
    bool lineRendering = false;
    bool recordFrames = false;
    uint frameNumber = 0;
    bool Camera = false;
    bool CameraMove = false;
    bool Color = false;
    bool Music = false;
    bool Draw = false;

    bool drawObjs[5] = { true, false, true, true, true };

    glm::vec3 translateVec(0., 0., 0.);
    float scaleChair = 0;

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
    float playbackRate = 1.;
    float maxTime = 1.;
    playing = false;
    cout << "Total loading time: " << glfwGetTime() << "s" << endl;
    // rendering loop
    while (glfwWindowShouldClose(window) == false) {
        dt = glfwGetTime() - lastGLTime;
        lastGLTime = glfwGetTime();
        currentTime += (recordFrames ? 0.0166666666 : dt) * timeScale * playbackRate * playing;
        maxTime = max(maxTime, CamPosSpline.points.back().first);
        currentTime = fmod(currentTime, maxTime + 1e-3);
        nbFrames++;
        if (glfwGetTime() - lastTime >= 1.0) {// If last prinf() was more than 1 sec ago
            // reset timer
            fps = nbFrames;
            frameTime = 1000.0 / double(nbFrames);
            nbFrames = 0;
            lastTime += 1.0;
        }

        // define UI
        glfwPollEvents();
        imgui_new_frame(400, 200);
        ImGui::Begin("General");
        ImGui::Checkbox("Framerate", &Framerate);
        ImGui::Checkbox("Camera", &Camera);
        ImGui::Checkbox("CameraControl", &CameraMove);
        ImGui::Checkbox("Color", &Color);
        ImGui::Checkbox("Draw", &Draw);
        ImGui::Checkbox("Music", &Music);
        ImGui::DragFloat3("translate", &(lightPos[0]));
        // ImGui::DragFloat("Scale", &scaleChair);
        ImGui::End();
        if (Framerate) {
            ImGui::Begin("Framerate");
            ImGui::Text("FPs: %04d", fps);
            ImGui::Text("avg. frametime: %04f", frameTime);
            ImGui::Checkbox("V-Sync", &vSync);
            ImGui::DragFloat("timeScale", &timeScale);
            ImGui::End();
        }
        if (Camera) {
            ImGui::Begin("Camera");
            ImGui::SliderFloat("float", &FOV, 10.0f, 90.0f);
            ImGui::Checkbox("rotate", &rotate);
            ImGui::SliderFloat("tessFactor", &tessFactor, 0.0f, 20.0f);
            ImGui::Checkbox("render using lines", &lineRendering);
            if (ImGui::Checkbox("record", &recordFrames) && recordFrames) {
                printf("record!\n");
                currentTime = 0;
                frameNumber = 0;
            }

            ImGui::End();
        }
        if (CameraMove) {
            ImGui::Begin("Camera Keyframes");
            ImGui::Checkbox("ViewCam", &inCameraView);
            ImGui::SameLine();
            ImGui::SliderFloat("Camera Time", &currentTime, 0.0f, maxTime);
            ImGui::SameLine();
            ImGui::PushItemWidth(100);
            ImGui::DragFloat("end", &maxTime, 1.f, 0.f, 100.f, "%5.3f", 1.f);

            static uint selectedPos = 0;
            {
                ImGui::BeginChild("left pane", ImVec2(150, 0), true);
                for (uint i = 0; i < CamPosSpline.points.size(); i++) {
                    char label[128];
                    sprintf(label, "Keyframe %d", i);
                    if (ImGui::Selectable(label, selectedPos == i)) {
                        selectedPos = i;
                        currentTime = CamPosSpline.points[i].first;
                        //printf("Clicked on %d t:%3.2lf\n", selectedPos, currentTime);
                    }
                }
                ImGui::EndChild();
            }
            if (playing) {
                selectedPos = CamPosSpline.getIndex(currentTime);
            }
            ImGui::SameLine();
            {
                ImGui::BeginGroup();
                ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));// Leave room for 1 line below us
                ImGui::Text("Keyframe: %d", selectedPos);
                ImGui::Separator();
                if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
                    if (ImGui::BeginTabItem("Description")) {
                        ImGui::TextWrapped("Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. ");
                        ImGui::EndTabItem();
                    }
                    if (ImGui::BeginTabItem("Details")) {
                        ImGui::Text("ID: 0123456789");

                        ImGui::DragFloat4("Camera Position", (float*)&(CamPosSpline.points[selectedPos].second[0]), 0.1f, -100.f, 100.f, "%5.3f", 1.f);
                        ImGui::DragFloat2("Camera Rotation", (float*)&(CamPosSpline.points[selectedPos].second[4]), 0.1f, -100.f, 100.f, "%5.3f", 1.f);
                        ImGui::DragFloat("Ingame TimeScale", (float*)&(CamPosSpline.points[selectedPos].second[6]), 0.1f, 0.f, 100.f, "%5.3f", 1.f);
                        if (ImGui::DragFloat("Camera Time", (float*)&(CamPosSpline.points[selectedPos].first), 0.1f, 0.f, 100.f, "%5.3f", 1.f)) {
                            double whatTime = CamPosSpline.points[selectedPos].first;
                            CamPosSpline.sort();
                            selectedPos = CamPosSpline.getIndex(whatTime);
                        }
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::EndChild();
                if (ImGui::Button("Delete")) {
                    ImGui::OpenPopup("sure");
                }
                ImGui::SameLine();
                if (ImGui::Button("Add new")) {
                    vector<float> pos;
                    pos.push_back(cam.position()[0]);
                    pos.push_back(cam.position()[1]);
                    pos.push_back(cam.position()[2]);
                    pos.push_back(state->radius);
                    pos.push_back(state->phi);
                    pos.push_back(state->theta);
                    pos.push_back(playbackRate);
                    CamPosSpline.addPoint(currentTime, pos);
                }
                ImGui::SameLine();
                if (ImGui::Button("Set to current")) {
                    vector<float> pos;
                    pos.push_back(cam.position()[0]);
                    pos.push_back(cam.position()[1]);
                    pos.push_back(cam.position()[2]);
                    pos.push_back(state->radius);
                    pos.push_back(state->phi);
                    pos.push_back(state->theta);
                    pos.push_back(playbackRate);
                    CamPosSpline.setCurrentPoint(selectedPos, pos);
                }
                ImGui::SameLine();
                if (ImGui::Button("Save to File")) {
                    CamPosSpline.storeTo(DATA_ROOT + "camPos");
                }
                ImGui::SameLine();
                if (ImGui::Button("Load from File")) {
                    CamPosSpline.loadFrom(DATA_ROOT + "camPos");
                }
                ImGui::EndGroup();
            }
            if (ImGui::BeginPopup("sure")) {// Delete the current Keyframe
                ImGui::Text("Are you sure?");
                if (ImGui::Button("yes!")) {
                    CamPosSpline.removePoint(selectedPos);
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("no"))
                    ImGui::CloseCurrentPopup();
                ImGui::EndPopup();
            }
            ImGui::End();
        }
        if (Color) {
            ImGui::Begin("Color");
            ImGui::DragFloat("bias", &bias);
            ImGui::DragFloat4("textColor", &(textColor[0]));
            textObj.setVec4("color", textColor);
            // ImGui::SliderFloat("colaH", &colaH, 0.0f, 1.0f);
            // ImGui::SliderFloat("colaS", &colaS, 0.0f, 1.0f);
            // ImGui::SliderFloat("colaV", &colaV, 0.0f, 1.0f);
            // ImGui::SliderFloat("colbH", &colbH, 0.0f, 1.0f);
            // ImGui::SliderFloat("colbS", &colbS, 0.0f, 1.0f);
            // ImGui::SliderFloat("colbV", &colbV, 0.0f, 1.0f);
            ImGui::SliderFloat("irradianceR", &irradianceR, 0.0f, 1.0f);
            ImGui::SliderFloat("irradianceG", &irradianceG, 0.0f, 1.0f);
            ImGui::SliderFloat("irradianceB", &irradianceB, 0.0f, 1.0f);

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
        if (Music) {
            ImGui::Begin("Music");
            ImGui::SliderFloat("Volume", &volume, 0.f, 1.0f);
            music->setVolume(volume);
            ImGui::End();
        }

        mars.setVec3("colorA", colaH, colaS, colaV);
        mars.setVec3("colorB", colbH, colbS, colbV);
        mars.setVec3("irradiance", irradianceR, irradianceG, irradianceB);

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

        if (inCameraView) {// animate the camera using keyframes
            vector<float> current = CamPosSpline.eval(fmod(currentTime, maxTime + 1e-3));
            state->look_at = glm::vec3(current[0], current[1], current[2]);
            state->radius = current[3];
            state->phi = current[4];
            state->theta = current[5];
            playbackRate = current[6];
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
        glm::mat4 view_matrix_new;

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glClearColor(0.5f, 0.5f, 0.5f, 0.0f);
        glStencilMask(0xFF);
        glClear(GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_TRUE);
        glDisable(GL_STENCIL_TEST);
        //Draw normal scene
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);// Do draw any pixels on the back buffer

        // move light position over time
        // lightPos.z = sin(glfwGetTime() * 0.5) * 3.0;

        // 0. create depth cubemap transformation matrices
        // -----------------------------------------------
        float near_plane = NEAR_VALUE;
        float far_plane = FAR_VALUE;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));

        // 1. render scene to depth cubemap
        // --------------------------------
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        simpleDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            simpleDepthShader.setMat4(("shadowMatrices[" + std::to_string(i) + "]").c_str(), &(shadowTransforms[i]));
        simpleDepthShader.setFloat("far_plane", far_plane);
        simpleDepthShader.setVec3("lightPos", lightPos);
        //renderScene(simpleDepthShader);
        simpleDepthShader.setMat4("model_mat", &(ident));
        simpleDepthShader.setVec3("camPos", cam.position());
        chairObj.render(currentTime, simpleDepthShader.shaderProgram);
        monitorObj.render(currentTime, simpleDepthShader.shaderProgram);

        skinningDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            skinningDepthShader.setMat4(("shadowMatrices[" + std::to_string(i) + "]").c_str(), &(shadowTransforms[i]));
        skinningDepthShader.setFloat("far_plane", far_plane);
        skinningDepthShader.setVec3("lightPos", lightPos);
        //renderScene(skinningDepthShader);
        skinningDepthShader.setMat4("model_mat", &(ident));
        skinningDepthShader.setVec3("camPos", cam.position());
        humanObj.render(currentTime, skinningDepthShader);

        glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

        if (drawObjs[0]) {// render human
            // bind pre-computed IBL data
            bindTextures(rockTex);

            glShadeModel(GL_SMOOTH);
            // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleChair, scaleChair, scaleChair));
            // humanObj.objMat = glm::translate(scaleMat, translateVec);
            humanObj.setVec3("lightPos", lightPos);
            humanObj.setFloat("displacementFactor", 0.);
            humanObj.setFloat("bias", bias);
            humanObj.setMaticies(&view_matrix, &proj_matrix);
            humanObj.setVec3("camPos", cam.position());
            humanObj.render(currentTime);
        }
        // if (drawObjs[4]) {//render table
        //     bindTextures(rockTex);
        //     tableObj.setMaticies(&view_matrix, &proj_matrix);
        //     tableObj.setVec3("camPos", cam.position());
        //     tableObj.render(ident);
        // }

        // bindTextures(rockTex);
        // tableObj.setMaticies(&view_matrix, &proj_matrix);
        // tableObj.setVec3("camPos", cam.position());
        // tableObj.render(0);

        bindTextures(hiriseTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleChair, scaleChair, scaleChair));
        // scaleMat = glm::translate(scaleMat, translateVec);
        hiriseObj.setMaticies(&view_matrix, &proj_matrix);
        hiriseObj.setVec3("lightPos", lightPos);
        hiriseObj.setVec3("viewPos", cam.position());
        hiriseObj.setFloat("far_plane", far_plane);
        hiriseObj.render(0);
        //hiriseObj.render(0, humanObj.shaderProgram);

        bindTextures(indoorTex);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        chairObj.setMaticies(&view_matrix, &proj_matrix);
        chairObj.setVec3("lightPos", lightPos);
        chairObj.setVec3("viewPos", cam.position());
        chairObj.setFloat("far_plane", far_plane);
        chairObj.render(0);

        bindTextures(indoorTex);
        monitorObj.setMaticies(&view_matrix, &proj_matrix);
        monitorObj.render(ident);

        if (drawObjs[2]) {// render mars
            glBindTextureUnit(0, tex_output);
            glBindTextureUnit(1, pds_tex);
            mars.setMaticies(&view_matrix, &proj_matrix);
            mars.render(0);
        }

        if (drawObjs[3]) {// reflexion
            //Draw in stencil first
            glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);// Do not draw any pixels on the back buffer
            glEnable(GL_STENCIL_TEST);
            glStencilFunc(GL_ALWAYS, 1, 0xFF);        // Set any stencil to 1
            glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);// Only write when both tests pass
            glDepthMask(GL_FALSE);                    // Don't write to depth buffer
            glFrontFace(GL_CW);                       //invert normals
            glassObj.setMaticies(&view_matrix, &proj_matrix);
            glassObj.render(currentTime);

            // render mirrored version
            glStencilFunc(GL_EQUAL, 1, 0xFF);      // only draw when there is reflection
            glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);// dont change stencil
            glDepthMask(GL_TRUE);                  // enable depth test
            glEnable(GL_DEPTH_TEST);

            glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);// draw pixels on the back buffer
            view_matrix_new = view_matrix * r;

            bindTextures(indoorTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
            chairObj.setMaticies(&view_matrix_new, &proj_matrix);
            chairObj.setVec3("lightPos", lightPos);
            chairObj.setVec3("viewPos", cam.position());
            chairObj.setFloat("far_plane", far_plane);
            chairObj.render(0);

            bindTextures(indoorTex);
            monitorObj.setMaticies(&view_matrix_new, &proj_matrix);
            monitorObj.render(ident);

            bindTextures(rockTex);
            // tableObj.setMaticies(&view_matrix_new, &proj_matrix);
            // tableObj.setVec3("camPos", cam.position());
            // tableObj.render(ident);

            humanObj.setMaticies(&view_matrix_new, &proj_matrix);
            humanObj.setVec3("camPos", cam.position());
            humanObj.render(currentTime);

            bindTextures(hiriseTex);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
            // glm::mat4 scaleMat = glm::scale(glm::mat4(1.0f), glm::vec3(scaleChair, scaleChair, scaleChair));
            // scaleMat = glm::translate(scaleMat, translateVec);
            hiriseObj.setMaticies(&view_matrix_new, &proj_matrix);
            hiriseObj.setVec3("lightPos", lightPos);
            hiriseObj.setVec3("viewPos", cam.position());
            hiriseObj.setFloat("far_plane", far_plane);
            hiriseObj.render(0);
            glFrontFace(GL_CCW);//uninvert normals
        }

        glStencilFunc(GL_ALWAYS, 1, 0x00);     // Always pass stencil
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);// dont change stencil

        // render Background
        glDisable(GL_CULL_FACE);
        glDepthFunc(GL_LEQUAL);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.hdrTexture);
        glUseProgram(renderCube.shaderProgram);
        renderCube.setMaticies(&view_matrix, &proj_matrix);
        renderCube.render(0);
        glDepthFunc(GL_LESS);
        glEnable(GL_CULL_FACE);

        // render transparency last
        if (drawObjs[1]) {// render glass
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.hdrTexture);
            glassObj.setMaticies(&view_matrix, &proj_matrix);
            glassObj.setFloat("factor", glass_factor);
            glassObj.setFloat("power", glass_power);
            glassObj.render(currentTime);
        }
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, envtex.hdrTexture);
        textObj.setMaticies(&view_matrix, &proj_matrix);
        textObj.setFloat("factor", glass_factor);
        textObj.setFloat("power", glass_power);
        textObj.render(currentTime);

        // COMPOSE PASS
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_STENCIL_TEST);
        glUseProgram(shaderProgramCompose);
        glBindVertexArray(quad);
        glBindTextureUnit(0, framebuffer_tex);
        glUniform1i(tex_loc, 0);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void*)0);

        if (recordFrames) {
            playing = true;
            inCameraView = true;
            if (currentTime > maxTime - 0.03) {
                playing = false;
                recordFrames = false;
            }
            frameNumber++;
            char str[7];
            snprintf(str, 7, "%06d", frameNumber);
            screenShotPNG((DATA_ROOT + "screenshots/s" + str + ".png").c_str(), WINDOW_WIDTH, WINDOW_HEIGHT);
        }

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
    if (io->WantCaptureKeyboard) {
        return;
    }
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
            playing = !playing;
            break;
        case GLFW_KEY_KP_0:
            inCameraView = !inCameraView;
            break;
        default:
            break;
        }
}
