/***
 * @data: 220730
 * @点的结构
 * ***/

#ifndef POINTS_HPP_
#define POINTS_HPP_

#include <vector>
#include <iostream>

class Points
{
private:
    /* data */
public:
    Points() {}

    //初始化点维度，填充为0
    Points(int num_dim);
    //初始化点的数据
    Points(double x, double y, double z);
    //使用向量初始化
    Points(const std::vector<double>& vector);

    ~Points() {};


    //计算两点距离
    static double distance(const Points& p1, const Points& p2);

    //添加点
    void add(const Points &p);
    
    //更新分类结果
    bool Update(int k);

    friend std::ostream &operator<<(std::ostream &target, const Points& point);
    int cluster_;
    int dimensions_;
    std::vector<double> data_;
    

};

#endif