#include "opencv2/highgui.hpp"
#include <ArmorDescriptor.h>
#include <ArmorParam.h>
#include <LightDescriptor.h>
#include <iostream>
#include <opencv2/opencv.hpp>
#include <vector>
using namespace std;
using namespace cv;
#include "MvCameraControl.h"
#include <thread>

template <typename T>
float distance(const cv::Point_<T> &pt1, const cv::Point_<T> &pt2) {
  return std::sqrt(std::pow((pt1.x - pt2.x), 2) + std::pow((pt1.y - pt2.y), 2));
}

using namespace cv;
using namespace std;
#define MAX_BUF_SIZE (1920 * 1200 * 3)

bool PrintDeviceInfo(MV_CC_DEVICE_INFO *pstMVDevInfo) {
  if (NULL == pstMVDevInfo) {
    printf("The Pointer of pstMVDevInfo is NULL!\n");
    return false;
  }
  if (pstMVDevInfo->nTLayerType == MV_GIGE_DEVICE) {
    int nIp1 =
        ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0xff000000) >> 24);
    int nIp2 =
        ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x00ff0000) >> 16);
    int nIp3 =
        ((pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x0000ff00) >> 8);
    int nIp4 = (pstMVDevInfo->SpecialInfo.stGigEInfo.nCurrentIp & 0x000000ff);

    printf("Device Model Name: %s\n",
           pstMVDevInfo->SpecialInfo.stGigEInfo.chModelName);
    printf("CurrentIp: %d.%d.%d.%d\n", nIp1, nIp2, nIp3, nIp4);
    printf("UserDefinedName: %s\n\n",
           pstMVDevInfo->SpecialInfo.stGigEInfo.chUserDefinedName);
  } else if (pstMVDevInfo->nTLayerType == MV_USB_DEVICE) {
    printf("Device Model Name: %s\n",
           pstMVDevInfo->SpecialInfo.stUsb3VInfo.chModelName);
    printf("UserDefinedName: %s\n\n",
           pstMVDevInfo->SpecialInfo.stUsb3VInfo.chUserDefinedName);
  } else {
    printf("Not support.\n");
  }

  return true;
}

void __stdcall ImageCallBackEx(unsigned char *pData,
                               MV_FRAME_OUT_INFO_EX *pFrameInfo, void *pUser) {
  if (pFrameInfo) {
    printf("GetOneFrame, Width[%d], Height[%d], nFrameNum[%d]\n",
           pFrameInfo->nWidth, pFrameInfo->nHeight, pFrameInfo->nFrameNum);
  }
}

class ArmorDetector {
public:
  // 初始化各个参数和我方颜色
  void init(int selfColor) {
    if (selfColor == RED) {
      _enemy_color = BLUE;
      _self_color = RED;
    }
    cout << "hi" << endl;
  }

  void loadImg(Mat &img) {
    _srcImg = img;

    Rect imgBound =
        Rect(cv::Point(50, 50), Point(_srcImg.cols - 50, _srcImg.rows - 50));

    _roi = imgBound;
    _roiImg =
        _srcImg(_roi)
            .clone(); // 注意一下，对_srcImg进行roi裁剪之后，原点坐标也会移动到裁剪后图片的左上角
  }
  // 识别装甲板的主程序，
  int detect() {
    // 颜色分离
    _grayImg = separateColors();
    // imshow("_grarsy", _grayImg);

    int brightness_threshold = 120; // 设置阈值,取决于你的曝光度
    Mat binBrightImg;
    // 阈值化
    threshold(_grayImg, binBrightImg, brightness_threshold, 255,
              cv::THRESH_BINARY);
    // imshow("thresh", binBrightImg);

    // 膨胀
    Mat element = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    dilate(binBrightImg, binBrightImg, element);
    // imshow("dilate", binBrightImg);

    // 找轮廓
    vector<vector<Point>> lightContours;
    findContours(binBrightImg.clone(), lightContours, 1, 2);

    //////debug/////
    _debugImg = _roiImg.clone();
    for (size_t i = 0; i < lightContours.size(); i++) {
      drawContours(_debugImg, lightContours, i, Scalar(0, 0, 255), 1, 8);
    }

    std::cout << "lightcontours num : " << lightContours.size() << std::endl;
    for (size_t i = 0; i < lightContours.size(); ++i) {
      for (size_t j = 0; j < lightContours[i].size(); ++j) {
        std::cout << "(" << lightContours[i][j].x << ", "
                  << lightContours[i][j].y << ") ";
      }
      std::cout << std::endl;
    }

    // imshow("contours", _debugImg);
    ////////////////

    // 筛选灯条
    vector<LightDescriptor> lightInfos;
    filterContours(lightContours, lightInfos);

    // for(int i = 0;i < lightInfos.size();i++){
    //     cout << lightInfos[i] << endl;
    //     return 0;
    // }
    // 没找到灯条就返回没找到
    if (lightInfos.empty()) {
      cout << "I not find!" << endl;
      return -1;
    } else {
      cout << "I CAN FIND!" << endl;
    }

    // debug 绘制灯条轮廓
    drawLightInfo(lightInfos);

    // 匹配装甲板
    _armors = matchArmor(lightInfos);
    if (_armors.empty()) {
      return -1;
    }

    // 绘制装甲板区域
    for (size_t i = 0; i < _armors.size(); i++) {
      vector<Point2i> points;
      for (int j = 0; j < 4; j++) {
        points.push_back(Point(static_cast<int>(_armors[i].vertex[j].x),
                               static_cast<int>(_armors[i].vertex[j].y)));
      }

      polylines(_debugImg, points, true, Scalar(0, 255, 0), 1, 8,
                0); // 绘制两个不填充的多边形
    }
    imshow("aromors", _debugImg);
    //waitKey(1);

    return 0;
  }
  // 分离色彩，提取我们需要（也就是敌人）的颜色，返回灰度图
  Mat separateColors() {
    vector<Mat> channels;
    // 把一个3通道图像转换成3个单通道图像
    split(_roiImg, channels); // 分离色彩通道

    // imshow("split_B", channels[0]);
    // imshow("split_G", channels[1]);
    // imshow("split_R", channels[2]);
    Mat grayImg;

    // 剔除我们不想要的颜色
    // 对于图像中红色的物体来说，其rgb分量中r的值最大，g和b在理想情况下应该是0，同理蓝色物体的b分量应该最大,将不想要的颜色减去，剩下的就是我们想要的颜色
    if (_enemy_color == RED) {
      grayImg = channels.at(2) - channels.at(0); // R-B
    } else {
      grayImg = channels.at(0) - channels.at(2); // B-R
    }
    return grayImg;
  }

  // 筛选符合条件的轮廓
  // 输入存储轮廓的矩阵，返回存储灯条信息的矩阵
  void filterContours(vector<vector<Point>> &lightContours,
                      vector<LightDescriptor> &lightInfos) {
    for (const auto &contour : lightContours) {
      // 得到面积
      float lightContourArea = contourArea(contour);
      // 面积太小的不要
      if (lightContourArea < _param.light_min_area)
        continue;
      // 椭圆拟合区域得到外接矩形
      RotatedRect lightRec = fitEllipse(contour);
      // 矫正灯条的角度，将其约束为-45~45
      adjustRec(lightRec);
      // 宽高比、凸度筛选灯条  注：凸度=轮廓面积/外接矩形面积
      if (lightRec.size.width / lightRec.size.height > _param.light_max_ratio ||
          lightContourArea / lightRec.size.area() <
              _param.light_contour_min_solidity)
        continue;
      // 对灯条范围适当扩大
      lightRec.size.width *= _param.light_color_detect_extend_ratio;
      lightRec.size.height *= _param.light_color_detect_extend_ratio;

      // 因为颜色通道相减后己方灯条直接过滤，不需要判断颜色了,可以直接将灯条保存
      lightInfos.push_back(LightDescriptor(lightRec));
    }
  }

  // 绘制旋转矩形
  void drawLightInfo(vector<LightDescriptor> &LD) {
    _debugImg = _roiImg.clone();

    vector<std::vector<cv::Point>> cons;
    int i = 0;
    for (auto &lightinfo : LD) {
      RotatedRect rotate = lightinfo.rec();
      auto vertices = new cv::Point2f[4];
      rotate.points(vertices);
      vector<Point> con;
      for (int i = 0; i < 4; i++) {
        con.push_back(vertices[i]);
      }
      cons.push_back(con);
      drawContours(_debugImg, cons, i, Scalar(0, 255, 255), 1, 8);
      // imshow("rotateRectangle", _debugImg);
      i++;
      // waitKey(0);
    }
  }

  // 匹配灯条，筛选出装甲板
  vector<ArmorDescriptor> matchArmor(vector<LightDescriptor> &lightInfos) {
    vector<ArmorDescriptor> armors;
    // 按灯条中心x从小到大排序
    sort(lightInfos.begin(), lightInfos.end(),
         [](const LightDescriptor &ld1, const LightDescriptor &ld2) {
           // Lambda函数,作为sort的cmp函数
           return ld1.center.x < ld2.center.x;
         });
    for (size_t i = 0; i < lightInfos.size(); i++) {
      // 遍历所有灯条进行匹配
      for (size_t j = i + 1; (j < lightInfos.size()); j++) {
        const LightDescriptor &leftLight = lightInfos[i];
        const LightDescriptor &rightLight = lightInfos[j];

        // 角差
        float angleDiff_ = abs(leftLight.angle - rightLight.angle);
        // 长度差比率
        float LenDiff_ratio = abs(leftLight.length - rightLight.length) /
                              max(leftLight.length, rightLight.length);
        // 筛选
        if (angleDiff_ > _param.light_max_angle_diff_ ||
            LenDiff_ratio > _param.light_max_height_diff_ratio_) {

          continue;
        }
        // 左右灯条相距距离
        float dis = distance(leftLight.center, rightLight.center);
        // 左右灯条长度的平均值
        float meanLen = (leftLight.length + rightLight.length) / 2;
        // 左右灯条中心点y的差值
        float yDiff = abs(leftLight.center.y - rightLight.center.y);
        // y差比率
        float yDiff_ratio = yDiff / meanLen;
        // 左右灯条中心点x的差值
        float xDiff = abs(leftLight.center.x - rightLight.center.x);
        // x差比率
        float xDiff_ratio = xDiff / meanLen;
        // 相距距离与灯条长度比值
        float ratio = dis / meanLen;
        // 筛选
        if (yDiff_ratio > _param.light_max_y_diff_ratio_ ||
            xDiff_ratio < _param.light_min_x_diff_ratio_ ||
            ratio > _param.armor_max_aspect_ratio_ ||
            ratio < _param.armor_min_aspect_ratio_) {
          continue;
        }

        // 按比值来确定大小装甲
        int armorType =
            ratio > _param.armor_big_armor_ratio ? BIG_ARMOR : SMALL_ARMOR;
        // 计算旋转得分
        float ratiOff =
            (armorType == BIG_ARMOR)
                ? max(_param.armor_big_armor_ratio - ratio, float(0))
                : max(_param.armor_small_armor_ratio - ratio, float(0));
        float yOff = yDiff / meanLen;
        float rotationScore = -(ratiOff * ratiOff + yOff * yOff);
        // 得到匹配的装甲板
        ArmorDescriptor armor(leftLight, rightLight, armorType, _grayImg,
                              rotationScore, _param);

        armors.emplace_back(armor);
        break;
      }
    }
    return armors;
  }

  void adjustRec(cv::RotatedRect &rec) {
    using std::swap;

    float &width = rec.size.width;
    float &height = rec.size.height;
    float &angle = rec.angle;

    while (angle >= 90.0)
      angle -= 180.0;
    while (angle < -90.0)
      angle += 180.0;

    if (angle >= 45.0) {
      swap(width, height);
      angle -= 90.0;
    } else if (angle < -45.0) {
      swap(width, height);
      angle += 90.0;
    }
  }
  cv::Mat _debugImg;

private:
  int _enemy_color;
  int _self_color;

  cv::Rect _roi; // ROI区域

  cv::Mat _srcImg;  // 载入的图片保存于该成员变量中
  cv::Mat _roiImg;  // 从上一帧获得的ROI区域
  cv::Mat _grayImg; // ROI区域的灰度图
  vector<ArmorDescriptor> _armors;

  ArmorParam _param;
};

int main(int argc, char *argv[]) {
  int nRet = MV_OK;
  void *handle = NULL;
  unsigned int nIndex = 0;

  do {
    MV_CC_DEVICE_INFO_LIST stDeviceList;
    memset(&stDeviceList, 0, sizeof(MV_CC_DEVICE_INFO_LIST));
    // enum device
    nRet = MV_CC_EnumDevices(MV_GIGE_DEVICE | MV_USB_DEVICE, &stDeviceList);
    if (MV_OK != nRet) {
      printf("MV_CC_EnumDevices fail! nRet [%x]\n", nRet);
      break;
    }

    // select device and create handle
    nRet = MV_CC_CreateHandle(&handle, stDeviceList.pDeviceInfo[0]);
    if (MV_OK != nRet) {
      printf("MV_CC_CreateHandle fail! nRet [%x]\n", nRet);
      break;
    }

    // open device
    nRet = MV_CC_OpenDevice(handle);
    if (MV_OK != nRet) {
      printf("MV_CC_OpenDevice fail! nRet [%x]\n", nRet);
      break;
    }

    int WidthValue = 1024;
    int HeightValue = 768;
    int ExposureTimeValue = 40000;
    int GainValue = 60;

    // set trigger mode as off
    nRet = MV_CC_SetEnumValue(handle, "TriggerMode", 0);

    // start grab image
    //
    MV_CC_SetExposureTime(handle, ExposureTimeValue);
    MV_CC_SetGain(handle, GainValue);
    // MV_CC_RegisterImageCallBackEx(handle,ImageCallBackEx,NULL);
    nRet = MV_CC_StartGrabbing(handle);

    MVCC_INTVALUE stIntvalue = {0};
    nRet = MV_CC_GetIntValue(handle, "PayloadSize", &stIntvalue);
    int nBufSize = stIntvalue.nCurValue;
    nBufSize = MAX_BUF_SIZE;

    unsigned char *pFrameBuf = NULL;
    pFrameBuf = (unsigned char *)malloc(nBufSize);

    MV_FRAME_OUT_INFO_EX stInfo;
    memset(&stInfo, 0, sizeof(MV_FRAME_OUT_INFO_EX));
    ArmorDetector detector;
    detector.init(RED);

    while (true) {
      nRet = MV_CC_GetImageForBGR(handle, pFrameBuf, nBufSize, &stInfo, 1000);
      int width = stInfo.nWidth;
      int height = stInfo.nHeight;

      if (stInfo.enPixelType == PixelType_Gvsp_BGR8_Packed) {
        Mat pImg(height, width, CV_8UC3, pFrameBuf);
        cv::resize(pImg, pImg, cv::Size(), 0.75, 0.75);
        cv::rotate(pImg, pImg, cv::ROTATE_180);

        // imshow("Image1", pImg);

        // Mat img = imread("./armor.png");
        detector.loadImg(pImg);
        detector.detect();
        imshow("debug", detector._debugImg);
        waitKey(1);
      }
    }

  } while (0);
  return 0;
}
