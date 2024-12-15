#include<iostream>
#include<opencv2/opencv.hpp>
#include<vector>
#include<ArmorDetector.h>
#include<ArmorParam.h>
#include<ArmorDescriptor.h>
#include<LightDescriptor.h>
#include<chrono>
#include<FindNum.h>
#include "opencv2/highgui.hpp"
#include "MvCameraControl.h"
#include "config.h"

#define MAX_BUF_SIZE (1920 * 1200 * 3)
#include "Debug.h"

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

using namespace std;
//using namespace cv;
//0:00 c++ 2:00 statistic 3:00 linux
template<typename T>
float distance(const cv::Point_<T>& p1,const cv::Point_<T>& p2)
{
    return std::sqrt(std::pow(p1.x - p2.x,2),std::pow(p1.y-p2.y,2));
}

ConsoleDebug deb;

// int main()
// {
//     deb << "hello world";
//     cv::Mat img = cv::imread("/home/jqz/Desktop/course_/GetImage.png");
//     img.convertTo(img, img.type(), 1.2);
//     //cv::imshow("img",img);
//     // cv::Mat rotImg;
//     // cv::rotate(img,rotImg,cv::ROTATE_90_CLOCKWISE);
//     // cv::imshow("rot",rotImg);
//     ArmorDetector detector;
//     detector.init(RED);
//     detector.loadImg(img);
//     detector.detect();
//     imshow("armors",detector._debugImg);
//     cv::waitKey(0);
// }

int main(){
  deb.autoEndl = true;
  deb.outTime = true;
  deb << "Debug is pre start.";
  deb << "123" << "456" << "asdsa";
  std::chrono::steady_clock::time_point lastTime = std::chrono::steady_clock::now();
  float fps = 0.0;

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

    // int WidthValue = 1024;
    // int HeightValue = 768;
    int WidthValue = 480;
    int HeightValue = 360;
    int ExposureTimeValue = 40000;  //40000
    int GainValue = 10;     //10

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

    MV_FRAME_OUT_INFO_EX stInfo{};
    ArmorDetector detector;
    
    detector.init(RED);

    nRet = MV_CC_GetImageForBGR(handle, pFrameBuf, nBufSize, &stInfo, 1000);
    int width = stInfo.nWidth;
    int height = stInfo.nHeight;
    // cout << width << ' ' << height << endl;
    // return 0;
    while (true) {
      nRet = MV_CC_GetImageForBGR(handle, pFrameBuf, nBufSize, &stInfo, 1000);
      Mat pImg(height, width, CV_8UC3, pFrameBuf);

      if (stInfo.enPixelType == PixelType_Gvsp_BGR8_Packed) {
        cv::resize(pImg, pImg, cv::Size(WidthValue,HeightValue));
        cv::rotate(pImg, pImg, cv::ROTATE_180);

        // 计算FPS
        auto currentTime = std::chrono::steady_clock::now();
        std::chrono::duration<float> elapsedTime = currentTime - lastTime;
        lastTime = currentTime;
        fps = 1.0 / elapsedTime.count();
      
        // 打印FPS
        std::cout << "FPS: " << fps << std::endl;

        
        detector.loadImg(pImg);
        detector.detect();
        imshow("aromors", detector._debugImg);
        cv::waitKey(1);
      }
      pImg.release();
    }

  } while (1);
  return 0;
}    
