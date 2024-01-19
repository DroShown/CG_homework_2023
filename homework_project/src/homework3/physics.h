//
// Created by 徐琢雄 on 2023/12/20.
//

#ifndef LEARNOPENGL_PHYSICS_H
#define LEARNOPENGL_PHYSICS_H

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <random>
#include "particle_system.h"
struct doll{
    float radius;
    float scale;
    float head_radius;
    float body_length;
    glm::vec3 position;
    glm::vec2 rotate_axis;
    glm::vec3 velocity;
    glm::vec3 angular_velocity;
    std::vector<glm::vec2> crash_balls;//vec2.x是y方向的高度，vec2.y是半径
    float m_bias;
    float I;
    glm::mat4 model;
    bool is_checked;
    [[nodiscard]] glm::mat4 Model() const{
        auto check_scale = is_checked? scale+0.5f: scale;
        return glm::scale(glm::translate(glm::mat4(1.0f), position) * model, glm::vec3(check_scale, check_scale, check_scale));
    }
};
struct ball{
    bool exist;
    unsigned int idx;
    float radius;
    glm::vec3 position;
    glm::vec3 velocity;
    int color;
};

struct fireBall{
    glm::mat4 model;
    glm::vec3 angular_velocity;
    glm::vec3 scale;
    float alpha;
    ball* ball_;
    fireBall(ball* ball_ = nullptr) : ball_(ball_), model(1.0f), angular_velocity(0.0f), scale(0.2f) {}
    [[nodiscard]] glm::mat4 Model() const {return glm::scale(glm::translate(glm::mat4(1.0f), ball_->position) * model, scale);}
};

class Physics
{
public:
    std::vector<doll> dolls;
    std::vector<ball> balls;
    fireBall fireball;
    float gravity;
    double time;
    float deltaTime;
    float half_cube_height;
    float freeze;
    doll* grab_doll;
    float decay;
    explicit Physics(int doll_num=5, int ball_num=30){
        dolls.resize(doll_num);
        balls.resize(ball_num);
        gravity = 9.8f;
        time = 0;
        deltaTime = 0;
        half_cube_height = 5.0f;
        freeze = 0.0f;
        grab_doll = nullptr;
        decay = 0.98f;
    }
    void init(){
        std::random_device e;
        std::uniform_real_distribution<double> uniform(-1*half_cube_height,half_cube_height);//随机数分布对象
        for(auto & doll : dolls){
            doll.scale = 10.0f;
            doll.radius = 0.04936f * doll.scale;
            doll.head_radius = 0.02450f * doll.scale;
            doll.body_length = (0.10753f-0.0244f) * doll.scale;
            doll.position = glm::vec3(0.0f, doll.radius - 5.0f, 0.0f);
            doll.rotate_axis = glm::vec3(0.0f, 0.0f, 1.0f);
            doll.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            doll.angular_velocity = glm::vec3(0.0f, 0.0f, 0.0f);
            doll.m_bias = doll.radius * 0.3f;
            doll.I = doll.radius * doll.radius * 0.4f;
            doll.model = glm::mat4(1.0f);
            doll.crash_balls.emplace_back(doll.body_length, doll.head_radius);
            doll.is_checked = false;
            //生成碰撞球
            for(int j = 0; j < 4; ++j)
            {
                int max = 5;
                int min = 1;
                float k = (min + j + 0.0f)/ (max + 0.0f);
                float radius = k*doll.radius + (1-k)*doll.head_radius;
                float height = (1-k)*doll.body_length;
                //std::cout << "r,h = " << radius << "," << height << std::endl;
                doll.crash_balls.emplace_back(height, radius);
            }
            doll.crash_balls.emplace_back(0.0f, doll.radius);
        }
        ///初始化球的位置和速度///
        int idx = 0;
        for(auto & ball : balls){
            ball.exist = true;
            ball.idx = idx;
            idx++;
            ball.radius = 0.2f;
            ball.position = glm::vec3(uniform(e),uniform(e),uniform(e));
            ball.velocity = glm::vec3(uniform(e),uniform(e),uniform(e));
            ball.color = 0;
        }
        balls[0].exist = false;
        balls[0].velocity = glm::normalize(balls[0].velocity) * 5.0f;
        fireball.ball_ = &balls[0];
        fireball.angular_velocity = glm::normalize(glm::vec3(uniform(e),uniform(e),uniform(e))) * 2.0f;
        fireball.scale = glm::vec3(0.5f, 0.5f, 0.5f);
    }
    void update(float deltaTime, ParticleSystem& particleSystem)
    {
        if(freeze > 0.0f) {

            freeze -= deltaTime;
            return;
            //deltaTime = deltaTime * 0.1;
        }


        this->deltaTime = deltaTime;
        std::cout << "//////////////////NEW FRAME////////////////////\n";
        std::cout << "fps:" << 1/deltaTime << std::endl;
        time += deltaTime;

        //update fireball
        fireball.model = glm::rotate(fireball.model, (float)glm::length(fireball.angular_velocity)*deltaTime, fireball.angular_velocity);
        fireball.scale = (float)(0.1*std::sin(5.0f * time) + 1.0f) * glm::vec3(0.3f, 0.3f, 0.3f);
        fireball.alpha = (float)(0.2*std::sin(2.5f * time) + 1.0f) * 1.2f;
        balls[0].velocity = glm::normalize(balls[0].velocity) * 3.0f * (float)(0.6 * sin(2.0f * time) + 1.5f);

        for(auto & doll : dolls){
            ///计算重力矩的影响///
            auto r = doll.model * glm::vec4(0.0f, -doll.m_bias, 0.0f, 1.0f);
            //std::cout << glm::length(doll.model[0]) << std::endl;
            auto G = glm::vec3(0.0f, -1 * gravity * 1.0f, 0.0f);
            auto M = cross(r, G);
            //计算角加速度
            auto angular_acceleration = M / doll.I;
            //计算角速度
            doll.angular_velocity += angular_acceleration * deltaTime;//角速度是世界空间中的角速度
            //计算姿态变化
            if(glm::length(doll.angular_velocity) > 0.0001f) doll.model = glm::rotate(glm::mat4(1.0f), (float)glm::length(doll.angular_velocity)*deltaTime, doll.angular_velocity) * doll.model;
            //角动量衰减
            doll.angular_velocity *= (1.0 - deltaTime*0.25);
            //计算位移
            auto move = cross( doll.angular_velocity, glm::vec3(0.0f,1.0f,0.0f)) * deltaTime + doll.velocity * deltaTime;
            doll.velocity *= (1.0 - deltaTime*0.3);
            doll.position += move;
        }
        ///球的运动///
        for(auto & ball : balls)
        {

            if(!ball.exist) continue;
            //第一个球不计算重力
            if(ball.idx != 0)
            {
                if (ball.velocity.y > 0.01f || ball.velocity.y < -0.01f ||
                    ball.position.y > -1 * half_cube_height + ball.radius + 0.01f)
                    ball.velocity.y -= gravity * deltaTime;
                else {
                    ball.velocity.y = 0.0f;
                    ball.velocity *= decay;
                }
            }
            ball.position += ball.velocity * deltaTime;
            for(int j = 0; j < 3; ++j)
            {
                if(ball.position[j] < -1*half_cube_height + ball.radius && ball.velocity[j] < 0.0f)
                {
                    if((j == 0 || j == 2)&&(ball.idx == 0)) particleSystem.addHitWall(ball.position, j);
                    ball.position[j] = -1*half_cube_height + ball.radius;
                    ball.velocity[j] *= -1.0f;
                    ball.color = 2*j + 2;
                    ball.velocity *= decay;
                }
                else if(ball.position[j] > half_cube_height - ball.radius && ball.velocity[j] > 0.0f)
                {
                    if((j == 0 )&&(ball.idx == 0)) particleSystem.addHitWall(ball.position, j);
                    ball.position[j] = half_cube_height - ball.radius;
                    ball.velocity[j] *= -1.0f;
                    ball.color = 2*j + 3;
                    ball.velocity *= decay;
                }
            }


        }


        ///球与球碰撞///
        for (int i = 0; i < balls.size(); ++i)
        {
            if(!balls[i].exist) continue;
            for (int j = i + 1; j < balls.size(); ++j)
            {
                if(!balls[j].exist) continue;
                if (ballHit(balls[i].position, balls[i].idx == 0 ? balls[i].radius * 3.0f : balls[i].radius, balls[j].position, balls[j].radius))
                {
                    auto v1 = balls[i].velocity;
                    auto v2 = balls[j].velocity;
                    auto m1 = 1.0f;
                    auto m2 = 1.0f;
                    auto x1 = balls[i].position;
                    auto x2 = balls[j].position;
                    if(glm::dot(v1 - v2, x1 - x2) <= -0.0001f)
                    {
                        auto v1_new = v1 - (2.01f * m2 / (m1 + m2)) * glm::dot(v1 - v2, x1 - x2) / glm::dot(x1 - x2, x1 - x2) * (x1 - x2);
                        auto v2_new = v2 - (2.01f * m1 / (m1 + m2)) * glm::dot(v2 - v1, x2 - x1) / glm::dot(x2 - x1, x2 - x1) * (x2 - x1);
                        balls[i].velocity = v1_new;
                        if(balls[i].idx != 0) balls[i].velocity *= decay;
                        balls[j].velocity = decay * v2_new;
                    }
                    if(balls[i].idx == 0) {
                        particleSystem.addHitBall(balls[j].position);
                        balls[j].exist = false;
                    }

                }
            }
        }
        ///球与不倒翁碰撞///
        for(auto& doll: dolls)
        {
            glm::vec3 crash_ball_pos;
            for(auto& ball: balls)
            {
                if(!ball.exist) continue;
                if(ballDollHit(ball.position, ball.radius,doll, crash_ball_pos))
                {
                    ball.color = 8;
                    //return;
                    //通过角速度和crash_ball_pos计算碰撞点的速度
                    auto v1 = ball.velocity;
                    auto v2 = cross(doll.angular_velocity, crash_ball_pos - doll.position);
                    auto m1 = 1.0f;
                    auto m2 = m1 * (doll.body_length + doll.radius) / doll.radius;
                    auto x1 = ball.position;
                    auto x2 = crash_ball_pos;
                    auto xv_dorection = glm::dot(v1 - v2, x1 - x2);
                    if(xv_dorection < -0.0001f) {
                        //freeze = 1.0f;
                        auto delta_v1 = 2.0f * xv_dorection / glm::dot(x1 - x2, x1 - x2) * (x1 - x2);
                        ball.velocity -= delta_v1;
                        auto L = 0.015f * cross(x2 - doll.position , delta_v1);
                        doll.angular_velocity += L / doll.I;
                    }
                }
            }
        }
        //std::cout << "dolls[0].position: " << dolls[0].position.x << ' ' << dolls[0].position.y << ' ' << dolls[0].position.z << std::endl;
        //std::cout << "dolls[0].angular_velocity: " << dolls[0].angular_velocity.x << ' ' << dolls[0].angular_velocity.y << ' ' << dolls[0].angular_velocity.z << std::endl;
    }
//    glm::vec2 rotate2d(glm::vec2 vec, float angle){
//        auto x = vec.x;
//        auto y = vec.y;
//        vec.x = x * std::cos(angle) - y * std::sin(angle);
//        vec.y = x * std::sin(angle) + y * std::cos(angle);
//        return vec;
//    }
    static glm::vec3 cross(glm::vec3 a, glm::vec3 b){
        return {a.y*b.z - a.z*b.y, a.z*b.x - a.x*b.z, a.x*b.y - a.y*b.x};
    }
    static bool ballHit(glm::vec3 pos1, float radius1, glm::vec3 pos2, float radius2){
        auto distance = glm::length(pos1 - pos2);
        if(distance > radius1 + radius2 + 0.001f) return false;
        //如果距离小于半径和，但是速度方向相反，也不会碰撞
//        auto v1 = balls[0].velocity;
//        auto v2 = balls[1].velocity;
//        auto v = v1 - v2;
//        auto x = pos1 - pos2;
//        if(glm::dot(v,x) >= 0) return false;
        else return true;
    }
    static bool ballDollHit(glm::vec3 ball_pos, float ball_radius, doll &doll, glm::vec3 &crash_ball_pos_ret){
        auto distance = doll.head_radius + doll.body_length + ball_radius;

        for(int i = 0; i < 3; ++i)
        {
            if(std::abs(ball_pos[i] - doll.position[i]) > distance) return false;
        }


        //auto second_pos = doll.Model() * glm::vec4(0.0f, doll.crash_balls[4].x, 0.0f, 1.0f);


        for(auto &crash_ball : doll.crash_balls)
        {
            auto height = crash_ball.x;
            //std::cout << "height:" << height << " radius:" << crash_ball.y << std::endl;
            auto radius = crash_ball.y;
            auto crash_ball_pos = doll.model * glm::vec4(0.0f, height, 0.0f, 1.0f) + glm::vec4(doll.position, 0.0f);
            //cout << "crash_ball_pos:" << crash_ball_pos.x << ' ' << crash_ball_pos.y << ' ' << crash_ball_pos.z << endl;
            auto r = glm::length(crash_ball_pos - glm::vec4(ball_pos, 1.0f));
            if(r <= radius + ball_radius)
            {
                //if(height <= 0.0001f) return false;//crash_ball_pos_ret = glm::vec3(second_pos.x, second_pos.y, second_pos.z);
                //else {
                    crash_ball_pos_ret = glm::vec3(crash_ball_pos.x, crash_ball_pos.y, crash_ball_pos.z);
                    std::cout << "hit!" << std::endl;
                    std::cout << "r:" << r << std::endl;
                    std::cout << "radius:" << radius << std::endl;
                    std::cout << "height:" << height << std::endl;
                    std::cout << "ball_pos:" << ball_pos.x << " " << ball_pos.y << " " << ball_pos.z << std::endl;
                    std::cout << "crash_ball_pos:" << crash_ball_pos.x << " " << crash_ball_pos.y << " " << crash_ball_pos.z << " " << crash_ball_pos.w << std::endl;
                //}

                return true;
            }
        }
        return false;
    }

    void processGrab(glm::mat4 projection, glm::mat4 view, glm::vec2 cursorPose, glm::vec2 cursorDir, bool cursorClick)
    {
        grab_doll = nullptr;
        glm::vec4 grab_screen_pos;
        for(auto& doll: dolls) doll.is_checked = false;
        auto inverse_view = glm::inverse(view);
        auto inverse_projection = glm::inverse(projection);
        if(cursorClick)
        {
            for(auto& doll: dolls)
            {

                auto world_pos = doll.Model() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                auto screen_pos = projection * view * world_pos / doll.scale;
                auto distance = glm::distance(glm::vec2(screen_pos.x, screen_pos.y), cursorPose);
                std::cout << "distance:" << distance << std::endl;
                if(distance < 0.2f && (!grab_doll || (grab_doll && screen_pos.z < grab_screen_pos.z)))
                {
                    grab_doll = &doll;
                    grab_screen_pos = screen_pos;
                    //break;


                }
                cout << "screen_pos:" << screen_pos.x << ' ' << screen_pos.y << ' ' << screen_pos.z << endl;
            }
            if(grab_doll) {
                grab_doll->is_checked = true;
                auto force = glm::normalize(inverse_view * inverse_projection * glm::vec4(cursorDir, 0.0f, 0.0f))*glm::length(cursorDir);
                std::cout << "force:" << force.x << ' ' << force.y << ' ' << force.z << std::endl;
                force *= 5.0f;
                force.y = cursorPose.y - grab_screen_pos.y;
                if(force.y < 0.0f)
                {
                    grab_doll->position += glm::vec3(force.x, 0.0f, force.z);
                }
                else
                {
                    auto L = 0.1f * cross(glm::vec3(0.0f, grab_doll->crash_balls[0].x, 0.0f), force);
                    grab_doll->angular_velocity += L / grab_doll->I;
                }

            }
        }
    }
};

#endif //LEARNOPENGL_PHYSICS_H
