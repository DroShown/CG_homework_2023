//
// Created by 徐琢雄 on 2023/10/6.
//
// 实现的功能：
// 绘制六边形图像
// 三角形边缘高亮
// 顶点闪烁
//
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cmath>

void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow *window, bool &dotMode, bool &changeColorMode);
void rotate(float x, float y, double angle, float* des);
float genColor(float phase, float offset);
void genCircleVertex(float x, float y, float r, std::vector<float> &vertices);
GLFWwindow* init_glfw();
void init_glad();
unsigned int init_shader(const char* &vertexShaderSource, const char* &fragmentShaderSource);
void genDot(double time,std::vector<float> &vertices,std::vector<int> &index, std::vector<float> &result);
// settings
const unsigned int SCR_WIDTH = 600;
const unsigned int SCR_HEIGHT = 600;

const char *vertexShaderSource_0 ="#version 330 core\n"
                                "layout (location = 0) in vec3 aPos;\n"
                                "void main()\n"
                                "{\n"
                                "   gl_Position = vec4(aPos, 1.0);\n"
                                "}\0";

const char *fragmentShaderSource_0 = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "uniform vec4 ourColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = ourColor;\n"
                                   "}\n\0";

const char *vertexShaderSource_1 ="#version 330 core\n"
                                "layout (location = 0) in vec3 aPos;\n"
                                "layout (location = 1) in vec3 aColor;\n"
                                "out vec3 ourColor;\n"
                                "void main()\n"
                                "{\n"
                                "   gl_Position = vec4(aPos, 1.0);\n"
                                "   ourColor = aColor;\n"
                                "}\0";

const char *fragmentShaderSource_1 = "#version 330 core\n"
                                   "out vec4 FragColor;\n"
                                   "in vec3 ourColor;\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(ourColor, 1.0f);\n"
                                   "}\n\0";



int main()
{
    auto window = init_glfw();
    init_glad();
    auto shaderProgram = init_shader(vertexShaderSource_0, fragmentShaderSource_0);
    auto shaderProgram_color = init_shader(vertexShaderSource_1, fragmentShaderSource_1);
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float central_points[] = {
            0.26f, 0.11f, 0.0f,
            -0.29f, 0.08f, 0.0f,
            0.03f, -0.33f, 0.0f
    };
    std::vector<float> vertices(9 * 3);
    for (int i = 0; i < 6; ++ i)
    {
        rotate(0,0.5,M_PI / 3 * i,vertices.data() + 3 * i);
        vertices[3 * i + 2] = 0.0f;
    }
    for (int i = 0;i < 9; ++ i) vertices[i + 6 * 3] = central_points[i];

    for(int i = 0; i < 9; ++i) std::cout << vertices[3 * i] << ","<< vertices[3 * i + 1] << ","<< vertices[3 * i + 2] << std::endl;

    unsigned int indices[] = {  // note that we start from 0!
            0,1,7,
            1,7,2,
            0,7,6,
            7,2,8,
            6,7,8,
            2,8,3,
            5,6,0,
            4,5,6,
            4,8,6,
            8,3,4,
    };

    std::vector<int> line_index = {  // note that we start from 0!
            0,1,
            1,7,
            7,2,
            7,6,
            2,8,
            7,8,
            6,7,
            2,3,
            8,3,
            5,6,
            0,5,
            0,6,
            4,5,
            4,6,
            4,8,
            8,6,
            3,4,
            8,3
    };

    //data for points;
    std::vector<float> vertices_point;
    float r_arr[9] = {0.009,0.016,0.012,0.011,0.005,0.014,0.009,0.017,0.019};
    for(int i = 0; i < 9; ++ i)
    {
        genCircleVertex(vertices[3 * i], vertices[3 * i + 1], r_arr[i],vertices_point);
    }



    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    unsigned int VBO_point, VAO_point;
    glGenVertexArrays(1, &VAO_point);
    glGenBuffers(1,&VBO_point);
    glBindVertexArray(VAO_point);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_point);
    //glBufferData(GL_ARRAY_BUFFER, vertices_point.size() * sizeof(float), vertices_point.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);


    unsigned int VBO_dot, VAO_dot;
    glGenVertexArrays(1, &VAO_dot);
    glGenBuffers(1, &VBO_dot);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO_dot);

    glBindBuffer(GL_ARRAY_BUFFER, VBO_dot);


    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);



    int vertexColorLocation = glGetUniformLocation(shaderProgram, "ourColor");

    // render loop
    // -----------
    bool changeColorMode = 0;
    bool dotMode = 0;
    double time_offset = 0;
    while (!glfwWindowShouldClose(window))
    {
        double  timeValue = glfwGetTime();
        // input
        // -----
        processInput(window,dotMode,changeColorMode);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // be sure to activate the shader before any calls to glUniform
        glUseProgram(shaderProgram);

        // render triangles
        glBindVertexArray(VAO);


        for(int i = 0; i < 10; ++ i){

            auto r = genColor(M_PI/2  *time_offset +  0.05 * i + 0.05,0.95f-0.08f * i);
            auto g = genColor(M_PI/1.3  *time_offset +  0.07 * i + 0.3,0.95f-0.08f * i);
            auto b = genColor(M_PI/2.9  *time_offset +  0.1 * i + 0.5,0.95f-0.08f * i);
            glUniform4f(vertexColorLocation, r, g, b, 1.0f);
            glDrawElements(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(3 * i * sizeof(GLuint)));
        }

        //render dots and edges
        if(dotMode)
        {
            glUseProgram(shaderProgram_color);
            std::vector<float> line_dot(0);
            glBindVertexArray(VAO_dot);
            glBindBuffer(GL_ARRAY_BUFFER,VBO_dot);
            genDot(timeValue, vertices, line_index, line_dot);
            glBufferData(GL_ARRAY_BUFFER, line_dot.size() * sizeof(float), line_dot.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINES, 0, line_dot.size());
            //printf("size = %zu", line_dot.size());

            glUseProgram(shaderProgram);

            glBindVertexArray(VAO_point);
            vertices_point.clear();
            vertices_point.resize(0);

            for (int i = 0; i < 9; ++i) {
                auto lit = (0.4 * sin((0.4 * i + 0.6) * timeValue + 2 * i) + 0.6);
                if(i == 0)printf("lit:%f\n",2 * r_arr[i] * lit);
                genCircleVertex(vertices[3 * i], vertices[3 * i + 1], 2 * r_arr[i] * (0.6 + 0.2 * lit), vertices_point);
            }
            printf("size:%i\n",vertices_point.size());
            glBindBuffer(GL_ARRAY_BUFFER,VBO_point);
            glBufferData(GL_ARRAY_BUFFER, vertices_point.size() * sizeof(float), vertices_point.data(), GL_STATIC_DRAW);


            for (int i = 0; i < 9; ++i) {
                auto lit = (0.2 * sin((0.4 * i + 0.6) * timeValue + 2 * i) + 0.8);
                auto r = genColor(M_PI / 2, lit);
                auto g = genColor(M_PI / 2, lit);
                auto b = genColor(M_PI / 2, lit);
                glUniform4f(vertexColorLocation, r, g, b, 1.0f);
                glDrawArrays(GL_TRIANGLE_FAN, i * 20, 20);
            }
            glBindVertexArray(0);


        }

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(window);
        glfwPollEvents();
        if(changeColorMode) time_offset += 0.01;
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteBuffers(1,&VBO_point);
    glDeleteBuffers(1,&VBO_dot);
    glDeleteProgram(shaderProgram);
    glDeleteProgram(shaderProgram_color);


    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

// process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
// ---------------------------------------------------------------------------------------------------------
void processInput(GLFWwindow *window, bool &dotMode, bool &changeColorMode)
{
    static bool S_press = false;
    static bool C_press = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if(glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS && !S_press)
    {
        dotMode = !dotMode;
        S_press = true;
    }
    if(glfwGetKey(window, GLFW_KEY_S) != GLFW_PRESS && S_press)
    {
        S_press = false;
    }

    if(glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS && !C_press)
    {
        changeColorMode = !changeColorMode;
        C_press = true;
    }
    if(glfwGetKey(window, GLFW_KEY_C) != GLFW_PRESS && C_press)
    {
        C_press = false;
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

//旋转：将旋转后的坐标存储到指定位置
void rotate(float x, float y, double angle, float* des)
{
    des[0] = cos(angle) * x - sin(angle) * y;
    des[1] = cos(angle) * y + sin(angle) * x;
}

float genColor(float phase, float range){
    return static_cast<float>(sin(phase) / 5 + 0.8 ) * range;
}

void genCircleVertex(float x, float y, float r, std::vector<float> &vertices)
{
    int n = 20;
    //vertices.insert(vertices.end(), {x, y, 0});
    float tmp[3] = {0,0,0};
    for(int i = 0;i < n; ++ i)
    {
        rotate(0,r,M_PI / 10 * i, tmp);
        tmp[0] += x;
        tmp[1] += y;
        vertices.insert(vertices.end(), {tmp[0], tmp[1], 0});
    }
}

GLFWwindow* init_glfw(){
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // glfw window creation
    // --------------------
    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "LearnOpenGL", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        exit(-1);
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    return window;
}

void init_glad(){
    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        exit(-1);
    }
}

unsigned int init_shader(const char* &vertexShaderSource, const char* &fragmentShaderSource)
{
    // build and compile our shader program
    // ------------------------------------
    // vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    // check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    // check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
    // link shaders
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    // check for linking errors
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return shaderProgram;
}

float mean(float x1, float x2, double offset)
{
    return x1 * offset + x2 * (1 - offset);
}

void genLineDot(float x1, float y1, float x2, float y2, float time, std::vector<float> &result)
{
    double length = 1;
    double offset = time - (int)time;
    auto left_bound = offset - length > 0? offset - length : 0;
    auto right_bound = offset + length < 1? offset + length : 1;
    auto x_center = mean(x1, x2, offset);
    auto y_center = mean(y1, y2, offset);
    auto x_left = mean(x1, x2, left_bound);
    auto y_left = mean(y1,y2, left_bound);
    auto x_right = mean(x1, x2, right_bound);
    auto y_right = mean(y1, y2, right_bound);
    result.insert(result.end(),{
        x1,y1,            0.0f,  0.5f,0.5f,0.5f,
        x_center,y_center,0.0f,  1.0f,1.0f,1.0f,
        x_center,y_center,0.0f,  1.0f,1.0f,1.0f,
        x2,y2,            0.0f,  0.5f,0.5f,0.5f
    });
}

void genDot(double time,std::vector<float> &vertices,std::vector<int> &index, std::vector<float> &result)
{
    for(int i = 0;i < index.size();i += 2)
    {
        //printf("x = %f, y = %f \n",vertices[3 * index[i]], vertices[3 * index[i]+1]);
        genLineDot(vertices[3 * index[i]],vertices[3 * index[i]+1],vertices[3 * index[i + 1]],vertices[3 * index[i + 1] + 1], 0.5 * sin(2 * time + 0.1 * i) + 0.5, result);
    }
}