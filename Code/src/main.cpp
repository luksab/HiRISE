#include <random>
#include <glm/gtx/transform.hpp>
#include "glm/gtx/string_cast.hpp"
#include <stb_image.h>

#include "common.hpp"
#include "shader.hpp"
#include "camera.hpp"
#include "buffer.hpp"
#include "mesh.hpp"
#include "animation.hpp"

#include <imgui.hpp>

int WINDOW_WIDTH = 1920;
int WINDOW_HEIGHT = 1080;
float FOV = 45.;
const float NEAR_VALUE = 0.1f;
const float FAR_VALUE = 100.f;

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

float *
load_pds_data(std::string filename, int *width, int *height, int *channels)
{
    FILE *fp = fopen(filename.c_str(), "r");
    char *line = NULL;
    size_t len = 0;
    int skipLen = 0;
    ssize_t read;

    if (fp == NULL)
        exit(EXIT_FAILURE);

    while ((read = getline(&line, &len, fp)) != -1)
    {
        //printf("Retrieved line of length %zu:\n", read);
        //printf("%s", line);
        skipLen += read;
        if (strncmp(line, "  LINES            = ", 21) == 0) //21 = len("  LINES            = ")
        {
            *height = atoi(line + 21);
        }
        else if (strncmp(line, "  LINE_SAMPLES     = ", 21) == 0)
        {
            *width = atoi(line + 21);
        }
        else if ((strncmp(line, "END\n", 4) == 0 || strncmp(line, "END\r", 4) == 0) && read < 6)
        {
            break;
        }
    }

    printf("height: %d\n", *height);
    printf("width: %d\n", *width);
    skipLen += getline(&line, &len, fp);
    skipLen += getline(&line, &len, fp);
    skipLen += getline(&line, &len, fp);
    printf("skipLen: %ld\n", ftell(fp));
    /*printf("%d\n",read = getline(&line, &len, fp));
    printf("%d\n",read = getline(&line, &len, fp));
    printf("%d\n",read = getline(&line, &len, fp));*/

    fclose(fp);
    if (line)
        free(line);

    FILE *f = fopen(filename.c_str(), "rb");

    int w = *width;
    int h = *height;

    //printf("channels: %d\n",*channels);

    float *data = new float[*channels * w * h];
    long filelen = w * h * *channels;
    /*fseek(f, 0, SEEK_END);         // Jump to the end of the file
    filelen = ftell(f);            // Get the current byte offset in the file
    rewind(f);                     // Jump back to the beginning of the file*/

    float *buffer = (float *)malloc(filelen * sizeof(float)); // Enough memory for the file
    //fseek(f,4348,SEEK_SET);
    fseek(f, ftell(fp), SEEK_SET);
    //fseek(f,4625*4,SEEK_SET);
    fread(buffer, sizeof(float), filelen, f); // Read in the entire file
    fclose(f);
    //float max = -10000;
    //float min = 14286578683;
    for (int j = 0; j < h; ++j)
    {
        for (int i = 0; i < w; ++i)
        {
            for (int k = 0; k < *channels; k++)
            {
                data[j * w * *channels + i * *channels + k] = buffer[j * w * *channels + i * *channels + k];
                data[j * w * *channels + i * *channels + k] += 4185.03;
                if (data[j * w * *channels + i * *channels + k] < -5000 || data[j * w * *channels + i * *channels + k] > 5000)
                {
                    data[j * w * *channels + i * *channels + k] = -10000;
                }

                data[j * w * *channels + i * *channels + k] /= 1938.42;
                /*if (j == 0)
                    printf("%lf\n", data[j * w * *channels + i * *channels + k]);*/
                //max = max>data[j * w * *channels + i * *channels + k]?max:data[j * w * *channels + i * *channels + k];
                //min = min<data[j * w * *channels + i * *channels + k]?max:data[j * w * *channels + i * *channels + k];
            }
        }
    }
    //printf("max: %lf\n",max);
    //printf("min: %lf\n",min);

    delete[] buffer;

    return data;
}

unsigned int
create_texture_rgba32f(int width, int height, float *data)
{
    unsigned int handle;
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);
    glTextureStorage2D(handle, 1, GL_RGBA32F, width, height);
    glTextureSubImage2D(handle, 0, 0, 0, width, height, GL_RGBA, GL_FLOAT, data);

    return handle;
}

unsigned int loadCubemap(std::vector<std::string> faces)
{
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

    int width, height, nrChannels;
    for (unsigned int i = 0; i < faces.size(); i++)
    {
        unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
        if (data)
        {
            glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                         0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return textureID;
}

unsigned int
create_texture_r32f(int width, int height, float *data)
{
    unsigned int handle;
    glCreateTextures(GL_TEXTURE_2D, 1, &handle);
    glTextureStorage2D(handle, 1, GL_R32F, width, height);
    glTextureSubImage2D(handle, 0, 0, 0, width, height, GL_RED, GL_FLOAT, data);

    return handle;
}

void set_texture_filter_mode(unsigned int texture, GLenum mode)
{
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, mode);
}

void set_texture_wrap_mode(unsigned int texture, GLenum mode)
{
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, mode);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, mode);
}

unsigned int setupShader()
{
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader("main.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("main.frag", GL_FRAGMENT_SHADER);
    unsigned int tesselationShader = compileShader("main.tess", GL_TESS_CONTROL_SHADER);
    unsigned int tesselationEShader = compileShader("main.tesse", GL_TESS_EVALUATION_SHADER);
    unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader, tesselationShader, tesselationEShader);
    //unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(tesselationShader);
    glDeleteShader(tesselationEShader);
    return shaderProgram;
}

unsigned int setupCubeShader()
{
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader("cubeMap/vert.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("cubeMap/frag.frag", GL_FRAGMENT_SHADER);
    unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    return shaderProgram;
}

unsigned int setupGlassShader()
{
    // load and compile shaders and link program
    unsigned int vertexShader = compileShader("glass/glass.vert", GL_VERTEX_SHADER);
    unsigned int fragmentShader = compileShader("glass/glass.frag", GL_FRAGMENT_SHADER);
    unsigned int shaderProgram = linkProgram(vertexShader, fragmentShader);
    // after linking the program the shader objects are no longer needed
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
    return shaderProgram;
}

int main(int, char *argv[])
{
    GLFWwindow *window = initOpenGL(WINDOW_WIDTH, WINDOW_HEIGHT, argv[0]);
    glfwSetFramebufferSizeCallback(window, resizeCallback);

    camera cam(window);

    init_imgui(window);

    unsigned int shaderProgram = setupShader();

    geometry model = loadMesh("hiresUV.obj", false, glm::vec4(0.f, 0.f, 0.f, 1.f));
    if (model.vertex_count == 0)
    {
        exit(1);
    }

    animated glass = loadMeshAnim("shard.dae", true);
    if (glass.vertex_count == 0)
    {
        exit(1);
    }
    printf("Animation dt: %lf\n",glass.timePerFrame);

    glUseProgram(shaderProgram);
    int model_mat_loc = glGetUniformLocation(shaderProgram, "model_mat");
    int view_mat_loc = glGetUniformLocation(shaderProgram, "view_mat");
    int proj_mat_loc = glGetUniformLocation(shaderProgram, "proj_mat");
    int height_loc = glGetUniformLocation(shaderProgram, "height");

    int cola_loc = glGetUniformLocation(shaderProgram, "colorA");
    int colb_loc = glGetUniformLocation(shaderProgram, "colorB");
    float colaH = 0.094;
    float colaS = 0.263;
    float colaV = 0.636;
    float colbH = 0.162;
    float colbS = 0.122;
    float colbV = 0.606;
    int tessFactor_loc = glGetUniformLocation(shaderProgram, "tessFactor");
    float tessFactor = 5;
    int discardFactor_loc = glGetUniformLocation(shaderProgram, "discardFactor");
    float discardFactor = 1.055;
    int h_loc = glGetUniformLocation(shaderProgram, "h");
    float h = 3.0;

    proj_matrix = glm::perspective(FOV, static_cast<float>(WINDOW_WIDTH) / WINDOW_HEIGHT, NEAR_VALUE, FAR_VALUE);
    glm::mat4 model_matrix = glm::identity<glm::mat4>();

    int image_width, image_height;
    //float *image_tex_data = load_texture_data(DATA_ROOT + "ESP_048136_1725_MRGB_quarter.jpg", &image_width, &image_height);
    float *image_tex_data = load_texture_data(DATA_ROOT + "ESP_041121_1725_RED_A_01_ORTHO_quarter.jpg", &image_width, &image_height);
    int pds_width = 5712;
    int pds_height = 11580;
    int pds_channels = 1;
    float *pds_tex_data = load_pds_data(DATA_ROOT + "DTEEC_048136_1725_041121_1725_A01.img", &pds_width, &pds_height, &pds_channels);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    unsigned int image_tex = create_texture_rgba32f(image_width, image_height, image_tex_data);
    set_texture_wrap_mode(image_tex, GL_CLAMP_TO_BORDER);
    unsigned int pds_tex = create_texture_r32f(pds_width, pds_height, pds_tex_data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glBindTextureUnit(0, image_tex);
    glBindTextureUnit(1, pds_tex);
    glTextureParameteri(image_tex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glUniform1i(height_loc, 1);

    delete[] image_tex_data;
    delete[] pds_tex_data;

    //Cubemap loading
    std::vector<std::string> faces = {
        DATA_ROOT + "Cubemap/" + "px.png",
        DATA_ROOT + "Cubemap/" + "nx.png",
        DATA_ROOT + "Cubemap/" + "py.png",
        DATA_ROOT + "Cubemap/" + "ny.png",
        DATA_ROOT + "Cubemap/" + "pz.png",
        DATA_ROOT + "Cubemap/" + "nz.png"};
    unsigned int cubemapTexture = loadCubemap(faces);

    float skyboxVertices[] = {
        // positions
        -1.0f, 1.0f, -1.0f,
        -1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, -1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, -1.0f, 1.0f,
        -1.0f, -1.0f, 1.0f,

        -1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, -1.0f,
        1.0f, 1.0f, 1.0f,
        1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, 1.0f,
        -1.0f, 1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, -1.0f,
        1.0f, -1.0f, -1.0f,
        -1.0f, -1.0f, 1.0f,
        1.0f, -1.0f, 1.0f};

    unsigned int skyboxVAO, skyboxVBO;
    glGenVertexArrays(1, &skyboxVAO);
    glGenBuffers(1, &skyboxVBO);
    glBindVertexArray(skyboxVAO);
    glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    unsigned int cubeShder = setupCubeShader();
    glUseProgram(cubeShder);
    int view_mat_loc_cube = glGetUniformLocation(cubeShder, "view_mat");
    int proj_mat_loc_cube = glGetUniformLocation(cubeShder, "proj_mat");
    glBindTextureUnit(0, cubemapTexture);

    unsigned int glassShder = setupGlassShader();
    glUseProgram(glassShder);
    int model_mat_loc_glass = glGetUniformLocation(glassShder, "model_mat");
    int view_mat_loc_glass = glGetUniformLocation(glassShder, "view_mat");
    int proj_mat_loc_glass = glGetUniformLocation(glassShder, "proj_mat");
    int glass_power_loc = glGetUniformLocation(glassShder, "power");
    float glass_power = 2.0;
    int glass_factor_loc = glGetUniformLocation(glassShder, "factor");
    float glass_factor = 1.0;
    glBindTextureUnit(0, cubemapTexture);


    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    bool vSync = true;
    bool rotate = false;
    bool Framerate = true;
    bool Camera = false;
    bool Color = false;

    //fuer fps
    double lastTime = glfwGetTime();
    int nbFrames = 0;
    int fps = 0;
    double frameTime = 0.;
    double dt = 0;
    double currentTime = 0;
    int animationFrame = 0;
    // rendering loop
    while (glfwWindowShouldClose(window) == false)
    {
        dt = glfwGetTime() - currentTime;
        currentTime += dt;
        nbFrames++;
        if (glfwGetTime() - lastTime >= 1.0)
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
        ImGui::Checkbox("Color", &Color);
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
            ImGui::SliderFloat("float", &FOV, 10.0f, 90.0f);
            ImGui::Checkbox("rotate", &rotate);
            ImGui::SliderFloat("tessFactor", &tessFactor, 0.0f, 20.0f);
            ImGui::End();
        }
        if (Color)
        {
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
        glUseProgram(shaderProgram);
        glUniform3f(cola_loc, colaH, colaS, colaV);
        glUniform3f(colb_loc, colbH, colbS, colbV);

        glUniform1f(tessFactor_loc, tessFactor);
        glUniform1f(discardFactor_loc, discardFactor);
        //printf("%lf\n",pow(10.,-h));
        glUniform1f(h_loc, pow(10., -h));

        if (rotate)
        {
            camera_state *state = cam.getState();
            state->phi += 0.1 * dt;
            //state->look_at.x -= 0.01;
            cam.update();
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
        //printf("%s\n",glm::to_string(proj_matrix).c_str());

        glm::mat4 view_matrix = cam.view_matrix();
        glUniformMatrix4fv(view_mat_loc, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(proj_mat_loc, 1, GL_FALSE, &proj_matrix[0][0]);

        glUniformMatrix4fv(model_mat_loc, 1, GL_FALSE, &model_matrix[0][0]);
        model.bind();
        //glDrawElements(GL_TRIANGLES, model.vertex_count, GL_UNSIGNED_INT, (void *)0);
        glDrawElements(GL_PATCHES, model.vertex_count, GL_UNSIGNED_INT, (void *)0);


        glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
        glUseProgram(cubeShder);
        glUniformMatrix4fv(view_mat_loc_cube, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(proj_mat_loc_cube, 1, GL_FALSE, &proj_matrix[0][0]);
        // skybox cube
        glBindVertexArray(skyboxVAO);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        glBindVertexArray(0);
        glDepthFunc(GL_LESS); // set depth function back to default

        //render transparency last
        glUseProgram(glassShder);
        glUniformMatrix4fv(view_mat_loc_glass, 1, GL_FALSE, &view_matrix[0][0]);
        glUniformMatrix4fv(proj_mat_loc_glass, 1, GL_FALSE, &proj_matrix[0][0]);
        //glUniformMatrix4fv(model_mat_loc_glass, 1, GL_FALSE, &glass.transform[animationFrame++%glass.transform.size()][0][0]);
        //glUniformMatrix4fv(model_mat_loc_glass, 1, GL_FALSE, &glass.transform[0][0][0]);
        //glUniformMatrix4fv(model_mat_loc_glass, 1, GL_FALSE, &model_matrix[0][0]);
        glUniformMatrix4fv(model_mat_loc_glass, 1, GL_FALSE, &glass.matrixAt(currentTime)[0][0]);
        glUniform1f(glass_factor_loc, glass_factor);
        glUniform1f(glass_power_loc, glass_power);
        glass.bind();
        glDrawElements(GL_TRIANGLES, glass.vertex_count, GL_UNSIGNED_INT, (void *)0);

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
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
    glViewport(0, 0, width, height);
    proj_matrix = glm::perspective(FOV, static_cast<float>(width) / height, NEAR_VALUE, FAR_VALUE);
}
