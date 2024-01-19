#include <cassert>
#include <cmath>
#include "points.hpp"

Points::Points(int num_dim)
    :cluster_(-1),
    dimensions_(num_dim)
    {
        for(int i = 0; i < num_dim; i++)
        {
            data_.push_back(0.0);
        }
    }



Points::Points(double x, double y, double z)
    :cluster_(-1),dimensions_(3)
{
    data_.clear();
    data_.push_back(x);
    data_.push_back(y);
    data_.push_back(z);
}

Points::Points(const std::vector<double> &vector)
    :cluster_(-1)
{
    dimensions_ = (int)vector.size();
    data_.clear();
    data_.insert(data_.begin(),vector.begin(),vector.end());
}


bool Points::Update(int k)
{
    const bool ret = cluster_!=k;
    cluster_ = k;
    return ret;
}


//静态函数
double Points::distance(const Points &p1,const Points &p2)
{
    double dist = 0.0;
    if(p1.dimensions_ == p2.dimensions_)
    {
        for(int i=0; i<p1.dimensions_; i++)
        {
            dist += pow(p1.data_[i]-p2.data_[i],2);
        }
        dist = sqrt(dist);
    }
    else
    {
        dist = -1;
    }
    return dist;
}


void Points::add(const Points& points)
{
    if(dimensions_ = points.dimensions_)
    {
        for(int i=0; i<dimensions_; i++)
        {
            data_[i] += points.data_[i];
        }
    }
    else
    {
        std::cout<<"dimensions error"<<std::endl;
    }
}

std::ostream &operator<<(std::ostream &target, const Points& point)
{
    target << "[";
    for(const double &d : point.data_)
    {
        target << d << ",";
    }
    target << "]";
    return target;
}