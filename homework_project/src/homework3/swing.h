//
// Created by 徐琢雄 on 2023/12/16.
//

#ifndef LEARNOPENGL_SWING_H
#define LEARNOPENGL_SWING_H
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
class Swing
{
public:
    //转动轴
    glm::vec3 rotationAxis;
    //转动角度
    float rotationAngle;
    //转动速度
    float rotationSpeed;
    //最大速度
    float maxSpeed;
    //摆动方向
    int direction;
    //时间
    float time;

    glm::mat4 swing_model;

public:
    Swing()
    {
        rotationAxis = glm::vec3(0.0, 0.0, 1.0);
        rotationAngle = 0.0f;
        maxSpeed = 20.0f;
        time = ((float)std::rand())/1000000000.0f;
        std::cout << "time:" << time << std::endl;
        rotationSpeed = maxSpeed;
        direction = 1;
        swing_model = glm::mat4(1.0f);
    }
    void swing(float deltaTime)
    {
        time += deltaTime;
        rotationAngle = glm::radians(std::sin(3 * time) * 30.0f);

        swing_model = glm::rotate(glm::mat4(1.0f), rotationAngle, glm::normalize(rotationAxis));
        swing_model = glm::rotate(glm::mat4(1.0f), time, glm::vec3(0.0f, 1.0f, 0.0f))*swing_model;

    }
    glm::mat4 model()
    {
        return swing_model;
    }
};

#endif //LEARNOPENGL_SWING_H
