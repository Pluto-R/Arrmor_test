#include "ArmorDetector.h"
#include "config.h"

template <typename T>
float distance(const cv::Point_<T> &pt1, const cv::Point_<T> &pt2) {
  return std::sqrt(std::pow((pt1.x - pt2.x), 2) + std::pow((pt1.y - pt2.y), 2));
}
// pre-processing
void ArmorDetector::init(int selfColor){
    if(selfColor == RED){
        _enemy_color = BLUE;
        _self_color = RED;
    }
}



void ArmorDetector::loadImg(cv::Mat& img){
    _srcImg = img;

    // Rect imgBound =
    //     Rect(cv::Point(50, 50), Point(_srcImg.cols - 50, _srcImg.rows - 50));

    // roi = imgBound;
    // _roiImg =
    //     _srcImg(roi)
    //         .clone(); // 注意一下，对_srcImg进行roi裁剪之后，原点坐标也会移动到裁剪后图片的左上角
  
}

cv::Mat ArmorDetector::seprateColors(){
    vector<Mat> channels;
    // 把一个3通道图像转换成3个单通道图像
    split(_srcImg, channels); // 分离色彩通道

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

void ArmorDetector::findContours(cv::Mat grayImg,vector<std::vector<cv::Point>>& lightContours){
    int brightness_threshold = 120;
    
    cv::threshold(grayImg,binbrightImg,brightness_threshold,255,cv::THRESH_BINARY);

    //cv::Mat element = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    cv::Mat element2 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));

    cv::dilate(binbrightImg,binbrightImg,element2);
    
    cv::findContours(binbrightImg.clone(),lightContours,1,2);
    _debugImg = _srcImg.clone();
    
    for(size_t i = 0;i < lightContours.size();i++){
        cv::drawContours(_debugImg,lightContours,i,cv::Scalar(0,255,255),1,0);
    }

}

void ArmorDetector::adjustRec(cv::RotatedRect& rec){
    while (rec.angle >= 90)
        rec.angle -= 180;
    while(rec.angle < -90)
        rec.angle += 180;
    
    if(rec.angle < -45){
        std::swap(rec.size.width,rec.size.height);
        rec.angle += 90;
    }else if(rec.angle >= 45){
        std::swap(rec.size.width,rec.size.height);
        rec.angle -= 90;
    }
}

// 筛选符合条件的轮廓
// 输入存储轮廓的矩阵，返回存储灯条信息的矩阵
void ArmorDetector::filteContours(vector<vector<cv::Point>> &lightContours,vector<LightDescriptor> &lightInfos) {
    
    for (const auto &contour : lightContours) {
      // 得到面积
      float lightContourArea = contourArea(contour);
      // 面积太小的不要
      if (lightContourArea < _param.light_min_area)
        continue;
      // 椭圆拟合区域得到外接矩形
      cv::RotatedRect lightRec = fitEllipse(contour);
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

void ArmorDetector::drawLightInfo(std::vector<LightDescriptor>& lightInfos){
    
    
    // cout << "here" << endl;
    std::vector<std::vector<cv::Point>> cons;
    int i = 0;
    for(auto& lightInfo : lightInfos){
        cv::RotatedRect rotate = lightInfo.rec();
        auto vertices = new cv::Point2f[4];
        rotate.points(vertices);
        std::vector<cv::Point> con;
        for (int i = 0; i < 4; i++) {
          con.push_back(vertices[i]);
          //cout << con[i] << endl;
        }
        cons.push_back(con);      
        cv::drawContours(_debugImg, cons, i, cv::Scalar(0, 255, 255), 1, 8);
        i++;
    }

    //cv::imshow("rotateRectangle", _debugImg);
    //cv::waitKey(0);
}

  // 匹配灯条，筛选出装甲板
vector<ArmorDescriptor> ArmorDetector::matchArmor(vector<LightDescriptor> &lightInfos) {
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

void ArmorDetector::detect(){
    _grayImg = seprateColors();

    vector<vector<cv::Point>> lightContours;
    findContours(_grayImg,lightContours);

    
    vector<LightDescriptor> lightInfos;

    filteContours(lightContours,lightInfos);
    drawLightInfo(lightInfos);

    // 匹配装甲板
    _armors = matchArmor(lightInfos);
    
    //_armorsImg = _debugImg.clone();
    // 绘制装甲板区域
    for (size_t i = 0; i < _armors.size(); i++) {
    
      vector<Point2i> points;
      for (int j = 0; j < 4; j++) {
        points.push_back(Point(static_cast<int>(_armors[i].vertex[j].x),
                               static_cast<int>(_armors[i].vertex[j].y)));
      }

      _armor = _debugImg.clone();
      
      FindNum findNum;
      findNum.loadSvmModel("/home/jqz/Desktop/course_/general/123svm.xml",findNum.armorImgSize);

       findNum.loadImg(_armor);
       

       findNum.getArmorImg(points);
       findNum.setArmorNum(points);

      clock_t startTime = clock();
      cout << " Find the armor! The armor num is "<< (int)findNum.svm->predict(findNum.p);
      cout << "\tuse time:" << (clock() - startTime) << "us" << endl;
      
      polylines(_debugImg, points, true, Scalar(0, 255, 0), 2, 8,
                 0); // 绘制两个不填充的多边形
      cv::line(_debugImg,points[0],points[2],cv::Scalar(0, 255, 0), 2, cv::LINE_8);
      cv::line(_debugImg,points[1],points[3],cv::Scalar(0, 255, 0), 2, cv::LINE_8);
      
    }
}



