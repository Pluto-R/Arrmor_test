// FindNum.cpp
#include "FindNum.h"

FindNum::FindNum() {
    svm = ml::SVM::create();
    armorImgSize = Size(40, 40);
    p = Mat();
    
    warpPerspective_mat = Mat(3, 3, CV_32FC1);
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(armorImgSize.width, 0);
	dstPoints[2] = Point2f(armorImgSize.width, armorImgSize.height);
	dstPoints[3] = Point2f(0, armorImgSize.height);
}

FindNum::~FindNum() {}


cv::Mat FindNum::extractArmor(const cv::Mat& srcImg, const std::vector<cv::Point2f>& vertices) {
    cv::Mat croppedImg;
    
    if (vertices.size() != 4) {
        std::cerr << "Invalid number of vertices for cropping." << std::endl;
        return croppedImg; 
    }

    cv::Rect boundingRect;
    float minX = std::min({vertices[0].x, vertices[1].x, vertices[2].x, vertices[3].x});
    float maxX = std::max({vertices[0].x, vertices[1].x, vertices[2].x, vertices[3].x});
    float minY = std::min({vertices[0].y, vertices[1].y, vertices[2].y, vertices[3].y});
    float maxY = std::max({vertices[0].y, vertices[1].y, vertices[2].y, vertices[3].y});
    boundingRect = cv::Rect(static_cast<int>(minX), static_cast<int>(minY), static_cast<int>(maxX - minX), static_cast<int>(maxY - minY));

    srcImg(boundingRect).copyTo(croppedImg);
    return croppedImg;
}

// cv::Mat FindNum::bin_Armor(const cv::Mat& srcImg){
//     cv::Mat binNumImg;
//     cvtColor(srcImg,binNumImg,6);
//     threshold(binNumImg, binNumImg, 40, 255,cv::THRESH_BINARY);
//     return binNumImg;
// }

void FindNum::loadSvmModel(const char * model_path, Size armorImgSize){
    svm = StatModel::load<SVM>(model_path);
    if(svm.empty())
    {
        cout<<"Svm load error! Please check the path!"<<endl;
        exit(0);
    }
	this->armorImgSize = armorImgSize;

	//set dstPoints (the same to armorImgSize, as it can avoid resize armorImg)
	dstPoints[0] = Point2f(0, 0);
	dstPoints[1] = Point2f(armorImgSize.width, 0);
	dstPoints[2] = Point2f(armorImgSize.width, armorImgSize.height);
	dstPoints[3] = Point2f(0, armorImgSize.height);
}

void FindNum::loadImg(const Mat& srcImg){

	//copy srcImg as warpPerspective_src
	(srcImg).copyTo(warpPerspective_src);

	//preprocess srcImg for the goal of acceleration
	cvtColor(warpPerspective_src, warpPerspective_src, 6);  //CV_BGR2GRAY=6
	threshold(warpPerspective_src, warpPerspective_src, 40, 255, THRESH_BINARY);
}

void FindNum::getArmorImg(vector<Point2i> & armor){
	//set the armor vertex as srcPoints
	for (int i = 0; i < 4; i++)
		srcPoints[i] = armor[i];

	//get the armor image using warpPerspective
	warpPerspective_mat = getPerspectiveTransform(srcPoints, dstPoints);  // get perspective transform matrix  透射变换矩阵
	warpPerspective(warpPerspective_src, warpPerspective_dst, warpPerspective_mat, armorImgSize, INTER_NEAREST, BORDER_CONSTANT, Scalar(0)); //warpPerspective to get armorImage
	warpPerspective_dst.copyTo(_armorNumImg);
}

void FindNum::setArmorNum(vector<Point2i> & armor){

	// adapt armorImg to the SVM model sample-size requirement
	p = _armorNumImg.reshape(1, 1);
	p.convertTo(p, CV_32FC1);

	//set armor number according to the result of SVM  
	//cout << "The num is : " << (int)svm->predict(p) << endl;
}