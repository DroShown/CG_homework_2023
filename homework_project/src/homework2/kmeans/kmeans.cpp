// kmeans 实现

#include <algorithm>
#include <cassert>
#include <fstream>
#include <iterator>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>
#include <cfloat>

#include "kmeans.hpp"

kmeans::kmeans(int k, int max_iterations)
    :num_cluster_(k),
     max_iterations_(max_iterations){

}

kmeans::~kmeans()
{}

bool kmeans::init(const std::vector<Points>& points)
{
    //创建点对应的索引
    std::vector<int> points_idx;
    int points_num = 0;
    points_.clear();

    for(const auto &p : points)
    {
        points_.push_back(p);
        points_idx.push_back(points_num);
        points_num++;
    }

    //随机数生成在多线程中可能出现线程不安全
    //mt表示它基于Mersenne Twister算法，19937源于产生随的机数的周期长可达到2^19937-1
    std::random_device rd;
    std::mt19937 rng(rd());//梅森旋转法获得高质量随机数
    std::shuffle(points_idx.begin(),points_idx.end(),rng);
    //std::uniform_int_distribution<int> distribution(0, points.size());

    //待分类的点要大于簇的数量,通过上面的产生随机数并打乱，得到随机的k个中心点
    assert(points.size() >= num_cluster_);

    for(int idx = 0; idx < num_cluster_; idx++)
    {
        Points mean(points[points_idx[idx]]);
        means_.push_back(mean);
    }

    return true;
}

bool kmeans::initFromVec3(const std::vector<glm::vec3>& points, glm::vec3 *center)
{
    //创建点对应的索引
    Points center_point;
    if(center) center_point = Points(center->x, center->y, center->z);
    std::vector<int> points_idx;
    std::vector<double> probabilities;
    double max_dist = 0;
    int points_num = 0;
    points_.clear();

    for(const auto &p : points)
    {
        points_.push_back({p.x, p.y, p.z});
        points_idx.push_back(points_num);
        if(center) {
            probabilities.push_back(Points::distance(points_.back(), center_point));
            if(probabilities.back() > max_dist) max_dist = probabilities.back();
        }

        points_num++;
    }

    //随机数生成在多线程中可能出现线程不安全
    //mt表示它基于Mersenne Twister算法，19937源于产生随的机数的周期长可达到2^19937-1
    std::random_device rd;
    std::mt19937 rng(rd());//梅森旋转法获得高质量随机数
    std::shuffle(points_idx.begin(), points_idx.end(), rng);
    if(center == nullptr)
    {
        //待分类的点要大于簇的数量,通过上面的产生随机数并打乱，得到随机的k个中心点
        assert(points.size() >= num_cluster_);

        for (int idx = 0; idx < num_cluster_; idx++) {
            auto &point = points[points_idx[idx]];
            Points mean(point.x, point.y, point.z);
            means_.push_back(mean);
        }
    }
    else
    {
        std::normal_distribution<double> distribution_double(0, 0.3 * max_dist);
        unsigned int idx = 0;
        while(means_.size() < num_cluster_)
        {
            auto dist = distribution_double(rng);
            //离center近的点更容易被选上
            if(probabilities[points_idx[idx]] < dist)
            {
                auto &point = points[points_idx[idx]];
                Points mean(point.x, point.y, point.z);
                means_.push_back(mean);
            }
            idx++;
            if(idx >= points_idx.size()) std::cout << ">" << std::endl;
            idx = idx % points_idx.size();

        }

    }

    return true;
}




//k均值算法运行，到达最大迭代次数或者收敛后不再继续分配
bool kmeans::run()
{
    for(int i = 0; i<=max_iterations_; i++)
    {
        std::cout<<" == kmeans iteration "<< i <<" == "<<std::endl;
        bool changed = assign();
        update_means();

        //运行后如果点不再改变分配的簇认为收敛
        if(!changed)
        {
            std::cout<<"kmeans has converged after "<< i <<" iterations"<<std::endl;
            return false;
        }
    }

    return false;
}


//找到最近的簇
int kmeans::findNearestCluster(const Points& point)
{
    double min_dist = DBL_MAX;
    int min_cluster = -1;
    double dist = 0;
    
    for(int idx = 0; idx < num_cluster_; idx++)
    {
        dist = Points::distance(point,means_[idx]);
        if(dist < min_dist)
        {
            min_dist = dist;
            min_cluster = idx;//代表簇中心索引
        }
    }

    return min_cluster;
}


//分配每个点到每个簇
bool kmeans::assign()
{
    bool changed = false;

    for(auto& p : points_)
    {
        const int new_cludster = findNearestCluster(p);
        
        //判断和之分配的簇前比没有改变
        bool ret = p.Update(new_cludster);
        changed = changed || ret;

        //std::cout<<"assigned point "<< p <<" to cluster: "<<new_cludster<<std::endl;

    }
    return changed;
}

//更新簇均值
bool kmeans::update_means()
{
    //构建点的map，分别是分配的簇和对应点的地址
    //主要特点是允许有重复的key
    std::multimap<int,const Points*> point_cluster_map;
    for(const auto& p : points_)
    {
        auto pair = std::pair<int, const Points *>(p.cluster_,&p);
        point_cluster_map.insert(pair);
    }

    //重新计算均值
    for(int i = 0; i < num_cluster_; i++)
    {
        computeClusterMean(point_cluster_map,i,&means_[i]);
    }

    return true;
}


void kmeans::computeClusterMean(
    const std::multimap<int,const Points *> &multimap,
    int cluster,
    Points *mean)
{
    for(int i = 0; i<mean->dimensions_; i++)
    {
        mean->data_[i] = 0;
    }

    auto in_cluster = multimap.equal_range(cluster);
    int num_points = 0;

    for(auto itr = in_cluster.first; itr != in_cluster.second; itr++)
    {
        mean->add(*(itr->second));
        num_points++;
    }

    for(uint idx=0; idx<mean->dimensions_; idx++)
    {
        mean->data_[idx] /= float(num_points);
    }
}


bool kmeans::loadPoints(const std::string& filepath, std::vector<Points>* points)
{
    std::ifstream file_stream(filepath,std::ios_base::in);
    if(!file_stream)
    {
        std::cout<<"could not open file "<<filepath<<std::endl;
        return false;
    }

    std::string line;
    while(getline(file_stream,line,'\n'))
    {
        std::stringstream line_stream(line);

        std::istream_iterator<double> start(line_stream),end;
        std::vector<double> numbers(start,end);

        Points p(numbers);
        points->push_back(p);
    }

    return true;
}

void kmeans::PrintMeans()
{
    for(auto mean:means_)
    {
        std::cout<<"mean :"<<mean<<std::endl;
    }
}


void kmeans::writeMeans(const std::string& filepath)
{
    std::ofstream file_stream(filepath,std::ios_base::out);
    if(!file_stream)
    {
        std::cout<<" open file error "<<filepath<<std::endl;
        return; 
    }

    for(auto& mean: means_)
    {
        std::ostream_iterator<double> itr(file_stream," ");
        std::copy(mean.data_.begin(),mean.data_.end(),itr);
        file_stream<<std::endl;
    }

    return;
}