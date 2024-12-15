#ifndef ARMORDETECTOR_H
#define ARMORDETECTOR_H
#include<vector>
#include<ArmorParam.h>
#include<ArmorDescriptor.h>
#include<LightDescriptor.h>
#include<FindNum.h>
#include<opencv2/opencv.hpp>
#include "NoCopy.h"
using namespace std;
using namespace cv;
// #define RED 1
// #define BLUE 2 
class ArmorDetector : NoCopy {
public:
    void init(int selfColor);
    void loadImg(cv::Mat& img);

    cv::Mat seprateColors();
    void findContours(cv::Mat _grayImg,vector<std::vector<cv::Point>>& lightContours);
    void filteContours(std::vector<std::vector<cv::Point>>& lightContours,std::vector<LightDescriptor>& lightInfos);
    void drawLightInfo(std::vector<LightDescriptor>& lightInfos);
    std::vector<ArmorDescriptor> matchArmor(std::vector<LightDescriptor>& lightInfos);
    void adjustRec(cv::RotatedRect& rec);
    void detect();
    cv::Mat _armorsImg;

    cv::Mat  _armor;
    
    int _enemy_color;
    int _self_color;
    
    cv::Mat _debugImg;
private:
    cv::Mat _srcImg;
    cv::Mat _roiImg;
    // int _enemy_color;
    // int _self_color;

    cv::Rect roi;   
    
    
    cv::Mat _grayImg;
    cv::Mat binbrightImg;
    cv::Mat binNum;
    
    std::vector<ArmorDescriptor> _armors;
    ArmorParam _param;
};

#endif
