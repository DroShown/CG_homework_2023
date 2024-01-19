//
// Created by 徐琢雄 on 2024/1/5.
//

#ifndef LEARNOPENGL_PARTICLE_SYSTEM_H
#define LEARNOPENGL_PARTICLE_SYSTEM_H
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <stb_image.h>
#include <deque>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <learnopengl/filesystem.h>
#include <learnopengl/shader.h>
#include <learnopengl/camera.h>
#include "model.h"
#include <iostream>
struct CubicParticle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    glm::vec3 AngularVelocity;
    glm::mat4 model;
    float alpha;
    float k_scale;
    float k_alpha;
    float scale;
    float Life;
    int type;
    CubicParticle() : Position(0.0f), Velocity(0.0f), AngularVelocity(0.0f), alpha(0.0f), scale(0.0f), Life(0.0f), model(1.0f), type(1) {
        //model = glm::scale(glm::mat4(1.0f), glm::vec3(1 + (rand()%10 - 5.0f)/20, 1 + (rand()%10 - 5.0f)/20, 1 + (rand()%10 - 5.0f)/20));
    }
    [[nodiscard]] glm::mat4 Model() const {return glm::scale(glm::translate(glm::mat4(1.0f), Position), glm::vec3(scale));}
};

struct SpriteParticle {
    glm::vec3 Position;
    glm::vec3 Velocity;
    float alpha;
    float scale;
    float Life;
    SpriteParticle() : Position(0.0f), Velocity(0.0f), alpha(0.0f), scale(0.0f), Life(0.0f) {}
    [[nodiscard]] glm::mat4 model() const {return glm::scale(glm::translate(glm::mat4(1.0f), Position), glm::vec3(scale));}
};

struct hitWall {
    float max_radius;
    float Life;
    glm::vec3 Position;
    int direction;
    hitWall(float r, float l, glm::vec3 p, int d) : max_radius(r), Life(l), Position(p), direction(d) {}
};

struct hitBall {
    float max_radius;
    float Life;
    glm::vec3 Position;
    glm::vec3 Velocity;
    hitBall(float r, float l, glm::vec3 p, glm::vec3 v) : max_radius(r), Life(l), Position(p), Velocity(v){}
};


class ParticleSystem {
public:
    float freeze;
    vector<CubicParticle> CubicParticles;
    vector<SpriteParticle> SpriteParticles;
    deque<hitWall> hitWalls;
    deque<hitBall> hitBalls;
    unsigned int Head;
    unsigned int Tail;
    glm::vec3 Position;
    glm::vec3 Velocity;
    const int new_particles = 4;
    int frameCnt;
    const float particle_life = 0.3f;
    const float hit_wall_life = 2.0f;
    glm::vec3 diffuse = glm::vec3(200.0f, 150.0f, 200.0f);
    const float k_scale = 0.08f;
    const float k_alpha = 0.8f;

    ParticleSystem() : Head(0), Tail(0), Position(0.0f), frameCnt(5), freeze(0.0f)
    {
        CubicParticles.resize(600);
        SpriteParticles.resize(100);
        for(auto & cubicParticle : CubicParticles)
        {
            cubicParticle.AngularVelocity = glm::vec3(rand() % 10 / 10.0f, rand() % 10 / 10.0f, rand() % 10 / 10.0f);
        }
    }

    void Update(float deltaTime, glm::vec3 position, glm::vec3 velocity)
    {
        if(freeze > 0.0f)
        {
            freeze -= deltaTime;
            return;
        }
        //deltaTime *= 0.2f;
        Position = position;
        Velocity = velocity;

        updateHitWalls(deltaTime);
        updateHitBalls(deltaTime);

        if(frameCnt > 0)
        {
            frameCnt--;
            return;
        }
        else
        {
            frameCnt = rand() % 5;
            for (int i = 0; i < new_particles; ++i) {
                unsigned int index = ActiveOneParticle();
            }
        }
        if(Tail > Head)
        {
            for(unsigned int i = Head; i < Tail; ++i)
            {
                updateParticle(CubicParticles[i], deltaTime);
            }
        }
        else
        {
            for(unsigned int i = Head; i < CubicParticles.size(); ++i)
            {
                updateParticle(CubicParticles[i], deltaTime);
            }
            for(unsigned int i = 0; i < Tail; ++i)
            {
                updateParticle(CubicParticles[i], deltaTime);
            }
        }

    }
    unsigned int ActiveOneParticle(int type = 1)
    {
        if((Tail + 1) % CubicParticles.size() == Head)
            return -1;
        unsigned int index = Tail;
        Tail = (Tail + 1) % CubicParticles.size();
        CubicParticles[index].type = type;
        CubicParticles[index].Life = particle_life;
        if(type == 1)
        {
            CubicParticles[index].Position = Position + glm::vec3((rand() % 100 - 50.0f) / diffuse.x,
                                                                  (rand() % 100 - 50.0f) / diffuse.y,
                                                                  (rand() % 100 - 50.0f) / diffuse.z);
            CubicParticles[index].Velocity = 0.4f * (Position - CubicParticles[index].Position) / particle_life;
            CubicParticles[index].AngularVelocity = glm::vec3(0.0f, 0.0f, 0.0f);
        }
        return index;
    }
    void updateParticle(CubicParticle& particle, float deltaTime)
    {
        particle.Position += particle.Velocity * deltaTime;
        particle.Life -= deltaTime;
        if(particle.Life <= 0.0f)
        {
            Head = (Head + 1) % CubicParticles.size();
            //particle.Position = Position;
            //particle.Velocity = glm::vec3(0.0f);
            particle.Life = 0.0f;
        }

        if(particle.type == 1)
        {
            float s = 1 - std::cos(particle.Life * M_PI / 0.8 / particle_life);
            particle.scale = k_scale * (s * 0.5f + 0.1f);
            particle.alpha = k_alpha * (std::pow(std::sin(particle.Life * M_PI / 1.5f / particle_life), 3));
        }
        else if(particle.type == 2)
        {
            float s = (1.0f - particle.Life/particle_life);
            particle.scale = particle.k_scale * 0.3f * (std::pow(s,2) - std::pow(s, 4));
            particle.alpha = 0.1f / (std::pow(s, 2) - std::pow(s, 4) + 0.01f);
        }
        else if(particle.type == 3)
        {
            auto k = particle.Life / particle_life;
            particle.scale = std::pow(1.0f-k, 0.2) * particle.k_scale;
            std::cout << "scale: " << particle.scale << std::endl;
            particle.alpha = particle.k_alpha * (std::pow(k,3) * 0.5f - 0.05f);
        }
    }

    void addHitWall(glm::vec3 position, int direction)
    {
        hitWalls.emplace_back(2.0f, hit_wall_life, position * 1.02f, direction);
    }
    void updateHitWalls(float deltaTime)
    {
        for(auto iter = hitWalls.begin(); iter != hitWalls.end(); ++ iter)
        {
            if(iter->Life <= 0.0f)
            {
                hitWalls.pop_front();
            }
            else if (iter->Life <= 0.65f)
            {
                iter->Life -= deltaTime;
            }
            else
            {
                iter->Life -= deltaTime;
                float k = (iter->Life-0.65f) / (hit_wall_life-0.65f);
                for(int i = 0; i < 7; ++i)
                {
                    auto idx = ActiveOneParticle(2);
                    if(idx == -1)
                        break;
                    auto offset = glm::normalize(glm::vec2((::rand()%10 - 5.0f)/10, (::rand()%10 - 5.0f)/10)) * (float)(((rand() % 10 -5.0f)/50 + 1.0f) * (1-k) * iter->max_radius);
                    //auto offset = glm::vec2((::rand()%10 - 5.0f)/10, (::rand()%10 - 5.0f)/10) * (1-k) * iter->max_radius;

                    CubicParticles[idx].Position = iter->Position;
                    //CubicParticles[idx].scale =
                    if(iter->direction == 0)
                        CubicParticles[idx].Position += glm::vec3(0.0f, offset.x,offset.y);
                    else if(iter->direction == 1)
                        CubicParticles[idx].Position += glm::vec3(offset.x, 0.0f, offset.y);
                    else if(iter->direction == 2)
                        CubicParticles[idx].Position += glm::vec3(offset.x, offset.y, 0.0f);

                    CubicParticles[idx].Velocity = 0.3f * (iter->Position - CubicParticles[idx].Position) / particle_life;
                    CubicParticles[idx].k_scale = (rand() % 10 - 5.0f)/ 10.0f + 1.4f;
                    //CubicParticles[idx].Life *= (rand()% 10 - 5.0f)/ 15.0f + 0.8f;
                }

            }
        }
    }

    void addHitBall(glm::vec3 position)
    {
        hitBalls.emplace_back(2.5f, hit_wall_life, position, glm::vec3(0.0f, 0.0f, 0.0f));
    }
    void updateHitBalls(float deltaTime)
    {
        for(auto iter = hitBalls.begin(); iter != hitBalls.end(); ++ iter)
        {
            if(iter->Life <= 0.0f)
            {
                auto idx = ActiveOneParticle(3);
                if(idx == -1)
                    break;
                CubicParticles[idx].Position = iter->Position;
                CubicParticles[idx].Velocity = iter->Velocity;
                CubicParticles[idx].k_scale = 0.25f;
                CubicParticles[idx].k_alpha = 7.5f;

                idx = ActiveOneParticle(3);
                if(idx == -1)
                    break;
                CubicParticles[idx].Position = iter->Position;
                CubicParticles[idx].Velocity = iter->Velocity;
                CubicParticles[idx].k_scale = 1.2f;
                CubicParticles[idx].k_alpha = 0.5f;

                hitBalls.pop_front();
            }
            else if (iter->Life <= 0.25f)
            {
                iter->Life -= deltaTime;
                iter->Position += iter->Velocity * deltaTime;
            }
            else
            {
                iter->Life -= deltaTime;
                iter->Position += iter->Velocity * deltaTime;
                float k = (iter->Life-0.65f) / (hit_wall_life-0.65f);
                for(int i = 0; i < (int)(10 * std::pow(k,0.5)); ++i)
                //if(rand() % 10 < 3)
                {
                    auto idx = ActiveOneParticle(2);
                    if(idx == -1)
                        break;
                    auto offset = glm::normalize(glm::vec3((rand()%10 - 5.0f)/10,
                                                           (rand()%10 - 5.0f)/10,
                                                            (rand()%10 - 5.0f)/10))
                                                                    * (float)((1.0f) * (k) * iter->max_radius);
                    //auto offset = glm::vec2((::rand()%10 - 5.0f)/10, (::rand()%10 - 5.0f)/10) * (1-k) * iter->max_radius;

                    CubicParticles[idx].Position = iter->Position + offset;
                    //CubicParticles[idx].Velocity = glm::vec3(0.0f);
                    CubicParticles[idx].Velocity = (iter->Position - CubicParticles[idx].Position) / particle_life;// + iter->Velocity;
                    CubicParticles[idx].k_scale = 0.7 * (rand() % 10 - 5.0f)/ 10.0f + 1.2f;
                }
            }
        }
    }


};



#endif //LEARNOPENGL_PARTICLE_SYSTEM_H
