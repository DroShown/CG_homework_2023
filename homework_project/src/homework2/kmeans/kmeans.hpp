#ifndef KMEANS_HPP_
#define KMEANS_HPP_

#include <map>
#include <string>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "points.hpp"

class kmeans
{
private:
    

    //计算每个分类的中心
    bool update_means();
    //找到最近的分类
    int findNearestCluster(const  Points& point);

    void computeClusterMean(
        const std::multimap<int, const Points*>& multimap,
        int cluster,
        Points* mean
    );
    

    int num_cluster_;
    int max_iterations_;
    std::vector<Points> means_;


public:
    std::vector<Points> points_;
    //构造函数，k个中心
    kmeans(int k, int max_iteration = 100);
    ~kmeans();

    //初始化添加所有点
    bool init(const std::vector<Points>& points);
    bool initFromVec3(const std::vector<glm::vec3>& points, glm::vec3 *center = nullptr);
    //分配点
    bool assign();
    //运行
    bool run();

    //载入点
    static bool loadPoints(const std::string& filepath,
                           std::vector<Points>* points);
    
    void PrintMeans();

    void writeMeans(const std::string& filepath);

    const std::vector<Points>& getPoints() const {
        return points_;
    }

    const std::vector<Points>& getMeans() const {
        return means_;
    }

};





#endif