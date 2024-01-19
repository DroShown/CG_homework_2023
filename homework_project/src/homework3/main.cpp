#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include "model.h"
#include "physics.h"
#include "particle_system.h"

#include <iostream>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window);
unsigned int loadTexture(const char *path);
void renderScene(Shader &shader, Model &ourModel, Physics &physics);
void renderRoom();
void renderCube();
void renderSphere();
void renderParticles(ParticleSystem & particleSystem, Shader & shader, int type);
void renderCursor();

// settings
const unsigned int SCR_WIDTH = 2800;
const unsigned int SCR_HEIGHT = 2100;
bool shadows = true;
bool shadowsKeyPressed = false;
bool grabMode = false;
bool grabModeKeyPressed = false;
bool fireBallMode = false;
bool fireBallKeyPressed = false;

// camera
Camera camera(glm::vec3(0.0f, 0.0f,10.0f));
float lastX = (float)SCR_WIDTH / 2.0;
float lastY = (float)SCR_HEIGHT / 2.0;
bool firstMouse = true;

// timing
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// const
const float r_body = 0.04936f;

// physics
Physics* physics_ptr = nullptr;

//particle system
ParticleSystem* particleSystem_ptr = nullptr;

//cursor position
glm::vec2 cursor_pos = glm::vec2(0.0f, 0.0f);

//VAO&VBO
unsigned int cubeVAO = 0;
unsigned int cubeVBO = 0;

unsigned int roomVAO = 0;
unsigned int roomVBO = 0;

unsigned int sphereVAO = 0;
unsigned int indexCount;

unsigned int cursorVAO = 0;
unsigned int cursorVBO = 0;

int main()
{
    /// ================================================ ///
    /// --------------------- 初始化 -------------------- ///
    /// ================================================ ///


    /// ------------ glfw初始化 ------------ ///
    // --------------------------------------//
    glfwInit();
    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    #ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif


    /// ------------ 创建window和捕获鼠标位置 ------------ ///
    // -------------------------------------------------- //
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH/2, SCR_HEIGHT/2, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // tell GLFW to capture our mouse
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);


    /// ------------ glad初始化 ------------ ///
    // ---------------------------------------//
    // glad: load all OpenGL function pointers
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    /// ------------ 配置global状态 ------------ ///
    // ---------------------------------------//
    // configure global opengl state
    // -----------------------------
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glEnable(GL_MULTISAMPLE);


    /// ------------ 初始化shader------------ ///
    // ---------------------------------------//
    // build and compile shaders
    // -------------------------
    Shader shader("point_shadows.vs", "point_shadows.fs");
    Shader simpleDepthShader("point_shadows_depth.vs", "point_shadows_depth.fs", "point_shadows_depth.gs");
    Shader lightShader("light.vs", "light.fs");
    Shader cursorShader("cursor.vs", "cursor.fs");


    /// ------------ 加载纹理 ------------ ///
    // ---------------------------------------//
    // load textures
    // -------------
    unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/toy_box_diffuse.png").c_str());
    //unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/marble.jpg").c_str());
    //unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/marble_concat.jpeg").c_str());
    //unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/marble_dark.jpeg").c_str());
    //unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/marble_white.jpg").c_str());
    //unsigned int woodTexture = loadTexture(FileSystem::getPath("resources/textures/carpet2.jpg").c_str());
    unsigned int particleTexture = loadTexture(FileSystem::getPath("resources/textures/marble_dark.jpeg").c_str());
    unsigned int fireBallTexture = loadTexture(FileSystem::getPath("resources/textures/image_filled.png").c_str());
    unsigned int circleTexture = loadTexture(FileSystem::getPath("resources/textures/circle2.png").c_str());


    /// ------------ 建立深度缓冲 ------------ ///
    // ---------------------------------------//
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


    /// ------------ 加载模型,初始化物理系统和粒子系统 ------------ ///
    // ---------------------------------------------------------//
    Model dollModel(FileSystem::getPath("resources/objects/tumbler/tumbler.obj"));
    Model ballModel(FileSystem::getPath("resources/objects/sphere/sphere.obj"));

    Physics physics(5,30);
    physics_ptr = &physics;
    physics.init();
    //physics.dolls[0].model = glm::rotate(physics.dolls[0].model, glm::radians(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
    physics.dolls[1].position += glm::vec3(3.0f, 0.0f, 0.0f);
    physics.dolls[2].position += glm::vec3(0.0f, 0.0f, 3.0f);
    physics.dolls[3].position += glm::vec3(-3.0f, 0.0f, 0.0f);
    physics.dolls[4].position += glm::vec3(0.0f, 0.0f, -3.0f);

    ParticleSystem particleSystem;
    particleSystem_ptr = &particleSystem;


    /// ------------ 配置shader ------------ ///
    // ---------------------------------------//
    // shader configuration
    lightShader.use();
    lightShader.setInt("texture_diffuse1", 0);
    lightShader.setInt("texture_diffuse2", 1);
    lightShader.setInt("texture_diffuse3", 2);

    shader.use();
    shader.setInt("diffuseTexture", 0);
    shader.setInt("depthMap", 1);

    // lighting info
    glm::vec3 lightPos(0.0f, 4.7f, 0.0f);
    glm::vec3 lightModelPos(0.0f, 5.0f, 0.0f);



    /// ================================================ ///
    /// -------------------- 渲染循环 ------------------- ///
    /// ================================================ ///
    while (!glfwWindowShouldClose(window))
    {
        /// ------------ 时间间隔计算 ------------ ///
        // ---------------------------------------//
        float currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;


        /// ------------ 处理输入事件 ------------ ///
        // ---------------------------------------//
        processInput(window);


        /// ------------ 更新物理系统和粒子系统 ------------ ///
        // ------------------------------------------------//
        physics.update(deltaTime, particleSystem);
        particleSystem.Update(deltaTime, physics.balls[0].position, physics.balls[0].velocity);

        /// ------------- 开始渲染 ------------- ///
        // ---------------------------------------//
        glClearColor(0.1f, 0.13f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);


        /// ------------ 0.创建深度立方体映射变换矩阵 ------------ ///
        // ----------------------------------------------------//
        float near_plane = 1.0f;
        float far_plane = 25.0f;
        glm::mat4 shadowProj = glm::perspective(glm::radians(90.0f), (float)SHADOW_WIDTH / (float)SHADOW_HEIGHT, near_plane, far_plane);
        std::vector<glm::mat4> shadowTransforms;
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));
        shadowTransforms.push_back(shadowProj * glm::lookAt(lightPos, lightPos + glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f)));


        /// --------------- 1.渲染场景到深度贴图 --------------- ///
        // ----------------------------------------------------//
        glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFBO);
        glClear(GL_DEPTH_BUFFER_BIT);
        simpleDepthShader.use();
        for (unsigned int i = 0; i < 6; ++i)
            simpleDepthShader.setMat4("shadowMatrices[" + std::to_string(i) + "]", shadowTransforms[i]);
        simpleDepthShader.setFloat("far_plane", far_plane);
        simpleDepthShader.setVec3("lightPos", lightPos);
        renderScene(simpleDepthShader, dollModel, physics);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);


        /// --------------- 2.正常渲染场景 --------------- ///
        // ------------------------------------------------//
        glViewport(0, 0, SCR_WIDTH, SCR_HEIGHT);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        shader.use();
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        shader.setMat4("projection", projection);
        shader.setMat4("view", view);
        // set lighting uniforms
        shader.setVec3("lightPos", lightPos);
        shader.setVec3("viewPos", camera.Position);
        shader.setInt("shadows", shadows); // enable/disable shadows by pressing 'SPACE'
        shader.setFloat("far_plane", far_plane);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, woodTexture);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_CUBE_MAP, depthCubemap);
        renderScene(shader, dollModel, physics);

        /// --------------- 3.渲染不参与阴影计算的物体 --------------- ///
        // ----------------------------------------------------------//
        // 渲染光源
        lightShader.use();
        lightShader.setMat4("projection", projection);
        lightShader.setMat4("view", view);
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, lightModelPos);
        model = glm::scale(model, glm::vec3(0.2f)); // a smaller cube
        lightShader.setMat4("model", model);
        lightShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
        lightShader.setInt("mode", 0);
        ballModel.Draw(lightShader);

        if(fireBallMode)
        {
            // 渲染立方体粒子
            //active particle texture
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, particleTexture);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, fireBallTexture);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, circleTexture);
            // enable additive blending
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            lightShader.setVec3("lightColor", glm::vec3(1.7f, 2.0f, 2.6f));
            lightShader.setInt("mode", 1);
            renderParticles(particleSystem, lightShader, 1);

            // 渲染火球

            //核心
            lightShader.setInt("mode", 1);
            lightShader.setVec3("lightColor", glm::vec3(1.4f, 2.1f, 2.6f));
            auto scale = 0.1f / physics.fireball.scale[0];
            for (int i = -1; i < 3; i += 2)
                for (int j = -1; j < 3; j += 2)
                    for (int k = -1; k < 3; k += 2) {
                        lightShader.setFloat("alpha", 0.9f);
                        lightShader.setMat4("model", glm::scale(
                                glm::translate(physics.fireball.Model(), glm::vec3(i * scale, j * scale, k * scale)),
                                glm::vec3(0.6f * scale)));
                        renderCube();
                    }
            // 内层
            lightShader.setVec3("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
            lightShader.setInt("mode", 2);
            lightShader.setMat4("model", physics.fireball.Model());
            lightShader.setFloat("alpha", physics.fireball.alpha);
            renderCube();
            // 外层
            lightShader.setVec3("lightColor", glm::vec3(0.9f, 0.95f, 1.2f));
            lightShader.setInt("mode", 3);
            lightShader.setMat4("model", glm::scale(physics.fireball.Model(), glm::vec3(1.4f)));
            lightShader.setFloat("alpha", physics.fireball.alpha * 0.7f);
            renderCube();
        }

        lightShader.setVec3("lightColor", glm::vec3(1.8f, 2.1f, 2.8f));
        lightShader.setInt("mode", 1);
        renderParticles(particleSystem, lightShader, 3);

        glDisable(GL_BLEND);

        // 渲染鼠标
        if(grabMode)
        {
            cursorShader.use();
            std::cout << "cursor position: " << cursor_pos.x << ' ' << cursor_pos.y << std::endl;
            cursorShader.setVec4("cursorPos", glm::vec4(cursor_pos.x, cursor_pos.y, 0.0f, 1.0f));
            renderCursor();
        }


        /// --------------- 交换帧缓存 --------------- ///
        // --------------------------------------------//
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}




/// ================================================ ///
/// -------------------- 工具函数 ------------------- ///
/// ================================================ ///

// renders the 3D scene
// --------------------
void renderScene(Shader &shader, Model &ourModel, Physics &physics)
{
    // room cube
    auto scale_room = 5.0f;
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::scale(model, glm::vec3(scale_room));
    shader.setMat4("model", model);
    glDisable(GL_CULL_FACE); // note that we disable culling here since we render 'inside' the cube instead of the usual 'outside' which throws off the normal culling methods.
    shader.setInt("reverse_normals", 1); // A small little hack to invert normals when drawing cube from the inside so lighting still works.
    shader.setInt("ballColor",-1);
    renderRoom();
    shader.setInt("reverse_normals", 0); // and of course disable it
    glEnable(GL_CULL_FACE);

    auto& balls = physics.balls;
    for(int i = 1; i < balls.size(); ++i)
    {
        if(!balls[i].exist) continue;
        shader.setInt("ballColor",balls[i].color);
        model = glm::scale(glm::translate(glm::mat4(1.0f), balls[i].position), glm::vec3(balls[i].radius,balls[i].radius,balls[i].radius));
        shader.setMat4("model",model);
        renderSphere();
    }

    shader.setInt("ballColor",0);
    for(auto &doll: physics.dolls)
    {
        model = doll.Model();
        shader.setMat4("model",model);
        ourModel.Draw(shader);
    }

}

// renderRoom()
// -------------------------------------------------
void renderRoom()
{
    // initialize (if necessary)
    if (roomVAO == 0)
    {
        float vertices[] = {
            // back face
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 6.0f, 0.0f, // bottom-left
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 6.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 6.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 6.0f, 1.0f, // top-right
            -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 6.0f, 0.0f, // bottom-left
            -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 6.0f, 1.0f, // top-left
            // front face
//            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
//             1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
//             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
//             1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
//            -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
//            -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
            // left face
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 2.0f, 0.0f, // top-right
            -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 2.0f, 1.0f, // top-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 2.0f, 1.0f, // bottom-left
            -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 2.0f, 1.0f, // bottom-left
            -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 2.0f, 0.0f, // bottom-right
            -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 2.0f, 0.0f, // top-right
            // right face
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 3.0f, 0.0f, // top-left
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 3.0f, 1.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 3.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 3.0f, 1.0f, // bottom-right
             1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 3.0f, 0.0f, // top-left
             1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 3.0f, 0.0f, // bottom-left
            // bottom face
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
             1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
             1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
            -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
            -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
            // top face
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 5.0f, 1.0f, // top-left
             1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 5.0f, 0.0f, // bottom-right
             1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 5.0f, 1.0f, // top-right
             1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 5.0f, 0.0f, // bottom-right
            -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 5.0f, 1.0f, // top-left
            -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 5.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &roomVAO);
        glGenBuffers(1, &roomVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, roomVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(roomVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Room
    glBindVertexArray(roomVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderCube()
// -------------------------------------------------
void renderCube()
{
    // initialize (if necessary)
    if (cubeVAO == 0)
    {
        float vertices[] = {
                // back face
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 1.0f, 1.0f, // top-right
                -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 0.0f, // bottom-left
                -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, -1.0f, 0.0f, 1.0f, // top-left
                // front face
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f, 1.0f, // top-right
                -1.0f,  1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 1.0f, // top-left
                -1.0f, -1.0f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f, 0.0f, // bottom-left
                // left face
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                -1.0f,  1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f, -1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-left
                -1.0f, -1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f,  1.0f,  1.0f, -1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-right
                // right face
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 1.0f, // bottom-right
                1.0f,  1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 1.0f, 0.0f, // top-left
                1.0f, -1.0f,  1.0f,  1.0f,  0.0f,  0.0f, 0.0f, 0.0f, // bottom-left
                // bottom face
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 1.0f, // top-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 1.0f, 0.0f, // bottom-left
                -1.0f, -1.0f,  1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 0.0f, // bottom-right
                -1.0f, -1.0f, -1.0f,  0.0f, -1.0f,  0.0f, 0.0f, 1.0f, // top-right
                // top face
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                1.0f,  1.0f , 1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 1.0f, // top-right
                1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 1.0f, 0.0f, // bottom-right
                -1.0f,  1.0f, -1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 1.0f, // top-left
                -1.0f,  1.0f,  1.0f,  0.0f,  1.0f,  0.0f, 0.0f, 0.0f  // bottom-left
        };
        glGenVertexArrays(1, &cubeVAO);
        glGenBuffers(1, &cubeVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cubeVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);
    }
    // render Cube
    glBindVertexArray(cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

// renderParticles()
// -------------------------------------------------
void renderParticles(ParticleSystem & particleSystem, Shader & shader, int type)
{
    unsigned int i = particleSystem.Head;
    std::cout << "head,tail = " << particleSystem.Head << ' ' << particleSystem.Tail << std::endl;
    if(type != 3)
    {
        while (i != particleSystem.Tail) {
            if(particleSystem.CubicParticles[i].type != 3)
            {
                auto model = particleSystem.CubicParticles[i].Model();
                shader.setMat4("model", model);
                shader.setFloat("alpha", particleSystem.CubicParticles[i].alpha);
                renderCube();
            }
            i = (i + 1) % particleSystem.CubicParticles.size();
        }
    }
    else
    {
        while (i != particleSystem.Tail) {
            if (particleSystem.CubicParticles[i].type == 3) {
                auto model = particleSystem.CubicParticles[i].Model();
                shader.setMat4("model", model);
                shader.setFloat("alpha", particleSystem.CubicParticles[i].alpha);
                renderCube();
            }
            i = (i + 1) % particleSystem.CubicParticles.size();
        }
    }
}

// renderSphere()
// -------------------------------------------------
void renderSphere()
{
    if (sphereVAO == 0)
    {
        glGenVertexArrays(1, &sphereVAO);

        unsigned int vbo, ebo;
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        std::vector<glm::vec3> positions;
        std::vector<glm::vec2> uv;
        std::vector<glm::vec3> normals;
        std::vector<unsigned int> indices;

        const unsigned int X_SEGMENTS = 64;
        const unsigned int Y_SEGMENTS = 64;
        const float PI = 3.14159265359f;
        for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
        {
            for (unsigned int y = 0; y <= Y_SEGMENTS; ++y)
            {
                float xSegment = (float)x / (float)X_SEGMENTS;
                float ySegment = (float)y / (float)Y_SEGMENTS;
                float xPos = std::cos(xSegment * 2.0f * PI) * std::sin(ySegment * PI);
                float yPos = std::cos(ySegment * PI);
                float zPos = std::sin(xSegment * 2.0f * PI) * std::sin(ySegment * PI);

                positions.push_back(glm::vec3(xPos, yPos, zPos));
                uv.push_back(glm::vec2(xSegment, ySegment));
                normals.push_back(glm::vec3(xPos, yPos, zPos));
            }
        }

        bool oddRow = false;
        for (unsigned int y = 0; y < Y_SEGMENTS; ++y)
        {
            if (!oddRow) // even rows: y == 0, y == 2; and so on
            {
                for (unsigned int x = 0; x <= X_SEGMENTS; ++x)
                {
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                }
            }
            else
            {
                for (int x = X_SEGMENTS; x >= 0; --x)
                {
                    indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
                    indices.push_back(y * (X_SEGMENTS + 1) + x);
                }
            }
            oddRow = !oddRow;
        }
        indexCount = static_cast<unsigned int>(indices.size());

        std::vector<float> data;
        for (unsigned int i = 0; i < positions.size(); ++i)
        {
            data.push_back(positions[i].x);
            data.push_back(positions[i].y);
            data.push_back(positions[i].z);
            if (normals.size() > 0)
            {
                data.push_back(normals[i].x);
                data.push_back(normals[i].y);
                data.push_back(normals[i].z);
            }
            if (uv.size() > 0)
            {
                data.push_back(uv[i].y);
                data.push_back(uv[i].x);
            }
        }
        glBindVertexArray(sphereVAO);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(float), &data[0], GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
        unsigned int stride = (3 + 2 + 3) * sizeof(float);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    }

    glBindVertexArray(sphereVAO);
    glDrawElements(GL_TRIANGLE_STRIP, indexCount, GL_UNSIGNED_INT, 0);
}

// renderCursor()
// -------------------------------------------------
void renderCursor()
{
    if(cursorVAO == 0)
    {
        ///render a single triangle as cursor
        float vertices[] = {
                0.0f, 0.0f, 0.0f,
                0.01f*0.75f,  -0.02f, 0.0f,
                0.02f*0.75f, -0.01f, 0.0f
        };
        glGenVertexArrays(1, &cursorVAO);
        glGenBuffers(1, &cursorVBO);
        // fill buffer
        glBindBuffer(GL_ARRAY_BUFFER, cursorVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
        // link vertex attributes
        glBindVertexArray(cursorVAO);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    }
    ///render cursor
    ///关闭深度测试
    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(cursorVAO);
    //设置线宽且线为白色
    glLineWidth(20.0f);

    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    ///打开深度测试
    glEnable(GL_DEPTH_TEST);
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        camera.ProcessKeyboard(RIGHT, deltaTime);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS && !shadowsKeyPressed)
    {
        shadows = !shadows;
        shadowsKeyPressed = true;
    }
    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_RELEASE)
    {
        shadowsKeyPressed = false;
    }

    if(glfwGetKey(window, GLFW_KEY_G) == GLFW_PRESS && !grabModeKeyPressed)
    {
        grabMode = !grabMode;
        grabModeKeyPressed = true;
    }
    if(glfwGetKey(window, GLFW_KEY_G) == GLFW_RELEASE)
    {
        grabModeKeyPressed = false;
    }

    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_PRESS && !fireBallKeyPressed)
    {
        fireBallMode = !fireBallMode;
        physics_ptr->fireball.ball_->exist = fireBallMode;
        fireBallKeyPressed = true;
    }
    if(glfwGetKey(window, GLFW_KEY_F) == GLFW_RELEASE)
    {
        fireBallKeyPressed = false;
    }

    if(glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS)
    {
        physics_ptr->init();
        //physics.dolls[0].model = glm::rotate(physics.dolls[0].model, glm::radians(1.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        physics_ptr->dolls[1].position += glm::vec3(3.0f, 0.0f, 0.0f);
        physics_ptr->dolls[2].position += glm::vec3(0.0f, 0.0f, 3.0f);
        physics_ptr->dolls[3].position += glm::vec3(-3.0f, 0.0f, 0.0f);
        physics_ptr->dolls[4].position += glm::vec3(0.0f, 0.0f, -3.0f);
    }
    if(glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
    {
        physics_ptr->freeze = 0.1f;
        particleSystem_ptr->freeze = 0.1f;
    }
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

// glfw: whenever the mouse moves, this callback is called
// -------------------------------------------------------
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    auto xpos = static_cast<float>(xposIn);
    auto ypos = static_cast<float>(yposIn);
    cursor_pos = glm::vec2(xpos/SCR_WIDTH, -ypos/SCR_HEIGHT);
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // reversed since y-coordinates go from bottom to top

    ///处理鼠标抓取
    if(grabMode)
    {
        //cout << "grabMode: cursor position: " << xpos << ' ' << ypos << endl;
        bool cursorClicked = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
        auto projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);
        //processGrab
        if(physics_ptr) physics_ptr->processGrab(projection, camera.GetViewMatrix(), cursor_pos, glm::vec2((xoffset+0.0f)/SCR_WIDTH, (yoffset+0.0f)/SCR_HEIGHT), cursorClicked);
        //glfwSetCursorPos(window, xpos, ypos);
    }
    ///处理鼠标移动
    else
    {
        camera.ProcessMouseMovement(xoffset, yoffset);
    }

    lastX = xpos;
    lastY = ypos;
}

// glfw: whenever the mouse scroll wheel scrolls, this callback is called
// ----------------------------------------------------------------------
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

// utility function for loading a 2D texture from file
// ---------------------------------------------------
unsigned int loadTexture(char const * path)
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

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT); // for this tutorial: use GL_CLAMP_TO_EDGE to prevent semi-transparent borders. Due to interpolation it takes texels from next repeat
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, format == GL_RGBA ? GL_CLAMP_TO_EDGE : GL_REPEAT);
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


