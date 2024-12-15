#pragma once

#ifndef FINDNUM_H
#define FINDNUM_H
#include<opencv2/opencv.hpp>
#include<opencv2/ml.hpp>
// #include<ArmorDescriptor.h>

using namespace std;
using namespace cv;
using namespace ml;
// #define RED 1
// #define BLUE 2
#define SVM_MODEL_PATH "general/123svm.xml"

class FindNum{
public:
    FindNum();
    ~FindNum();
    
    // 提取装甲板图像
    cv::Mat extractArmor(const cv::Mat& srcImg, const std::vector<cv::Point2f>& vertices);
    // 进行灰度图的二值化
    cv::Mat bin_Armor(const cv::Mat& srcImg);

    void loadSvmModel(const char * model_path, Size armorImgSize);
    void getArmorImg(vector<Point2i> & armor);

    void loadImg(const Mat & srcImg);

    void setArmorNum(vector<Point2i> & armor);

    int _enemy_color;
    int _self_color;
    cv::Mat _armorNumImg;
    cv::Size armorImgSize;
    Ptr<ml::SVM> svm;   //svm model
    Mat p;              //载入到SVM中识别的矩阵
private:
    cv::Mat warpPerspective_src;	//warpPerspective srcImage  透射变换的原图
	cv::Mat warpPerspective_dst;	//warpPerspective dstImage   透射变换生成的目标图
	cv::Mat warpPerspective_mat;	//warpPerspective transform matrix 透射变换的变换矩阵
	cv::Point2f srcPoints[4];		//warpPerspective srcPoints		透射变换的原图上的目标点 tl->tr->br->bl  左上 右上 右下 左下
	cv::Point2f dstPoints[4];		//warpPerspective dstPoints     透射变换的目标图中的点   tl->tr->br->bl  左上 右上 右下 左下
};


#endif