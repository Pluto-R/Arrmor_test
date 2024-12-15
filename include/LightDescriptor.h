#ifndef LIGHTDESCRIPTOR_H
#define LIGHTDESCRIPTOR_H
#include <vector>
#include <opencv2/opencv.hpp>

class LightDescriptor{
public:
    LightDescriptor() {}
    LightDescriptor(const cv::RotatedRect& light){
        width = light.size.width;
        length = light.size.height;
        center = light.center;
        angle = light.angle;
        area = light.size.area();
    }

    cv::RotatedRect rec() const{
        return cv::RotatedRect(center,cv::Size2f(width,length),angle);
    
    }

    float width;
    float length;
    cv::Point2f center;
    float angle;
    float area;
};

#endif // LIGHTDESCRIPTOR_H
