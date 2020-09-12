#include "opencv2/features2d/features2d.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/xfeatures2d/nonfree.hpp"
#include <opencv2/imgproc/imgproc.hpp>
#include<opencv2/opencv.hpp>
#include<opencv2/core.hpp>
#include <opencv2/highgui/highgui_c.h> 
#include <fstream>
#include <string>
#include <vector>
#include<iostream>
#include <iterator>
#include <cmath>
#include <time.h>


#define Pi 3.14


using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;


struct templatePointVector
{
	vector<int> templatePointX;
	vector<int> templatePointY;
};

struct Contours
{
	/*����ͼ*/
	Mat dstImage;

	/*��������*/
	int centerX;
	int centerY;
};

struct offsetResult
{
	/*�ҶȲ�����Ϣ*/
	int grayDiff;

	/*ģ��ƽ����*/
	int resultX;
	int resultY;
};


/*��ѷ���任����*/
struct bestAffineParm
{
	double angle;//��ת�Ƕ�
	int transX;//X ����ƽ����
	int transY;//Y ����ƽ����
	double scalarX;//X ����߶ȱ任
	double scalarY;//Y ����߶ȱ任
};


templatePointVector GetTemplatePoint(Contours templateContours,double angle)
{
	/*ģ��㼯*/
	templatePointVector templatePoint;

	for (int i = 0; i < templateContours.dstImage.cols; i++)
	{
		for (int j = 0; j < templateContours.dstImage.rows; j++)
		{
			if (templateContours.dstImage.at<uchar>(j,i)!=0)
			{
				/*��ת*/
				int cx = i - templateContours.centerX;
				int cy = j - templateContours.centerY;
				int rx = (int)cx * cos(angle) - cy * sin(angle) + templateContours.centerX;
				int ry = (int)cx * sin(angle) + cy * cos(angle) + templateContours.centerY;
				if (rx > 0 && rx < templateContours.dstImage.cols && ry>0 && ry < templateContours.dstImage.rows)
				{
					templatePoint.templatePointX.push_back(rx);
					templatePoint.templatePointY.push_back(ry);
				}

			}
		}
	}

	return templatePoint;
}


offsetResult TemplateMatch(Mat& image, templatePointVector& templatePoint, Mat& templateImage,int zoomScale)
{
	int pixelStep = 10/zoomScale;
	int patchStep = 64/zoomScale;
	int minX;
	int minY;
	int offsetX;//ģ�����Ͻǵ���������ڵ�ǰ�������X����ƽ����
	int offsetY;//ģ�����Ͻǵ���������ڵ�ǰ�������Y����ƽ����
	double temp;

	offsetResult result;
	result.grayDiff = 0;
	Point pt;

	double curPointValue;
	double curPointValue_UR1;
	double curPointValue_UR2;
	double curPointValue_UL1;
	double curPointValue_UL2;
	double curPointValue_DR1;
	double curPointValue_DR2;
	double curPointValue_DL1;
	double curPointValue_DL2;

	/*Ѱ��ģ��ͼ��������͹�����ε����Ͻǵ�*/
	vector<int>::iterator minTemplatePointX = min_element(begin(templatePoint.templatePointX), end(templatePoint.templatePointX));
	vector<int>::iterator minTemplatePointY = min_element(begin(templatePoint.templatePointY), end(templatePoint.templatePointY));
	Point minPosition = Point(*minTemplatePointX, *minTemplatePointY);

	for (int col = 0; col < image.cols; col += patchStep)
	{
		for (int row = 0; row < image.rows; row += patchStep)
		{
			//cout << "col\\cols:" << col << "\\" << image.cols <<","<< "row\\rows:" << row << "\\" << image.rows << endl;

			/*���㵱ǰ����ģ��Ľṹ��ƫ��*/
			minX = minPosition.x;
			minY = minPosition.y;
			offsetX = col - minX;
			offsetY = row - minY;
			//cout << offsetY << endl;
			/*��ȡ��ǰ��(row,col)��ģ��ṹ����Ϣ���֮���grayDiff*/
			temp = 0;
			for (int i = 0; i < templatePoint.templatePointX.size(); i++)
			{
				/*ģ��ͼ���е� - ƫ�� = ����ͼ���ж�Ӧ�ĵ�*/
				pt = Point(templatePoint.templatePointX[i] - offsetX, templatePoint.templatePointY[i] - offsetY);

				/*���㵱ǰ�����������ڻҶȲ���Ϣ*/
				if (pt.x >= 0 && pt.x < image.cols  && pt.y >= 2 * pixelStep && pt.y < image.rows - 2 * pixelStep)
				{
					curPointValue = image.at<uchar>(pt.y, pt.x);

					curPointValue_UR1 = image.at<uchar>(pt.y - 1 * pixelStep, pt.x + 1 * pixelStep);
					curPointValue_UR2 = image.at<uchar>(pt.y - 2 * pixelStep, pt.x + 2 * pixelStep);
					curPointValue_UL1 = image.at<uchar>(pt.y - 1 * pixelStep, pt.x - 1 * pixelStep);
					curPointValue_UL2 = image.at<uchar>(pt.y - 2 * pixelStep, pt.x - 2 * pixelStep);

					curPointValue_DR1 = image.at<uchar>(pt.y + 1 * pixelStep, pt.x + 1 * pixelStep);
					curPointValue_DR2 = image.at<uchar>(pt.y + 2 * pixelStep, pt.x + 2 * pixelStep);
					curPointValue_DL1 = image.at<uchar>(pt.y + 1 * pixelStep, pt.x - 1 * pixelStep);
					curPointValue_DL2 = image.at<uchar>(pt.y + 2 * pixelStep, pt.x - 2 * pixelStep);

					temp += abs(8 * curPointValue - curPointValue_DL1 - curPointValue_DL2 - curPointValue_DR1
						- curPointValue_DR2 - curPointValue_UL1 - curPointValue_UL2 - curPointValue_UR1 - curPointValue_UR2);
				}
			}

			if (temp > result.grayDiff)
			{
				result.grayDiff = temp;
				result.resultX = offsetX ;//
				result.resultY = offsetY ;
			}
		}
	}
	return result;
}


Contours CurveComplete(Mat& srcImage ,int zoomScale)
{
	Contours result;

	int centerX = 0;
	int centerY = 0;
	int size = 0;

	Mat cannyOutput;
	vector<vector<Point>> contours;
	vector<Vec4i> hierachy;
	Canny(srcImage, cannyOutput, 150, 255, 3, false);

	findContours(cannyOutput, contours, hierachy, RETR_TREE, CHAIN_APPROX_NONE, Point(0, 0));

	Mat dstImg = Mat::zeros(srcImage.size(), CV_8UC1);
	//������
	for (int i = 0; i < contours.size(); i++)
	{
		double area = contourArea(contours[i]);
		double length = arcLength(contours[i], true);
		if (area > 50/ pow(zoomScale,2) && length > 50/ zoomScale)
		{
			drawContours(dstImg, contours, i, Scalar(255), 1, 8, hierachy, 1, Point(0, 0));
			for (int j = 0; j < contours[i].size(); j++)
			{
				centerX += contours[i][j].x;
				centerY += contours[i][j].y;
				size++;
			}
		}
	}
	result.centerX = centerX / size;
	result.centerY = centerY / size;
	result.dstImage = dstImg;

	return result;
}


void TemplateLocate(Mat& maskImage,Mat& maskImageeRGB,Mat& image,Mat& imageRGB)
{
	Contours templateContours = CurveComplete(maskImage,1);
	//namedWindow("templateImage", CV_WINDOW_NORMAL);
	//imshow("templateImage", templateContours.dstImage);

	templatePointVector templatePoint, bestTemplatePoint;
	double finalResultGrayDiff = 0;
	bestAffineParm bestAffine;

	for (double angle = -10 * Pi / 180; angle < 10 * Pi / 180; angle += Pi / 180)
	{
		cout << "angle = " << angle * 180 / Pi << "��" << endl;

		clock_t t1 = clock();
		templatePoint = GetTemplatePoint(templateContours, -angle);

		offsetResult curResult = TemplateMatch(image,templatePoint, templateContours.dstImage,1);

		if (curResult.grayDiff > finalResultGrayDiff)
		{
			finalResultGrayDiff = curResult.grayDiff;
			bestTemplatePoint = templatePoint;

			/*Get the best rotate angle*/
			bestAffine.angle = angle;

			/*Get the best translation in X direction*/
			bestAffine.transX = curResult.resultX;

			/*Get the best translation in Y direction*/
			bestAffine.transY = curResult.resultY;
		}

		clock_t t2 = clock();
		cout << "time is : " << (t2 - t1) * 1.0 / CLOCKS_PER_SEC << "s" << endl;
	}

	double x, y;
	for (int i = 0; i < bestTemplatePoint.templatePointX.size(); i++)
	{
		x = bestTemplatePoint.templatePointX[i] - bestAffine.transX;
		y = bestTemplatePoint.templatePointY[i] - bestAffine.transY;
		if (x >= 0 && x <= image.cols - 1 && y >= 0 && y <= image.rows - 1)
		{

			imageRGB.at<Vec3b>(y, x)[0] = 0;
			imageRGB.at<Vec3b>(y, x)[1] = 0;
			imageRGB.at<Vec3b>(y, x)[2] = 255;
		}
	}

	cout << "best angle is : " << bestAffine.angle * 180 / Pi << endl;
	cout << "best transX is : " << bestAffine.transX << endl;
	cout << "best transY is : " << bestAffine.transY / Pi << endl;
	
	namedWindow("��ƥ������", CV_WINDOW_NORMAL);
	imshow("��ƥ������", imageRGB);
}



/*��߶Ƚ�����ģ��ƥ��*/
void PyramidTemplateLocate(Mat maskImage, Mat& maskImageeRGB, Mat image, Mat& imageRGB)
{
	Mat maskImage_DS, image_DS;

	templatePointVector afterZoomPoint, templatePoint, bestTemplatePoint;
	bestAffineParm bestAffine;
	offsetResult tempResult, curResult;

	int zoomScale = 1;
	int col = maskImage.cols;
	int row = maskImage.rows;
	double finalResultGrayDiff = 0;
	double afterZoomGrayDiff;

	for (double angle = -10 * Pi / 180; angle < 10 * Pi / 180; angle += Pi / 180)
	{
		cout << "angle = " << angle * 180 / Pi << "��" << endl;

		afterZoomGrayDiff = 0;

		/*�����²���*/
		clock_t t1 = clock();
		maskImage_DS = maskImage.clone();
		image_DS = image.clone();
		for (size_t i = 0; i < 3; i++)
		{
			zoomScale *= 2;//2,4,8

			pyrDown(maskImage_DS, maskImage_DS, Size(maskImage_DS.cols / 2, maskImage_DS.rows / 2));
			pyrDown(image_DS, image_DS, Size(image_DS.cols / 2, image_DS.rows / 2));

			/*�²���֮���ͼ�����ģ��ƥ��*/
			// ���㵱ǰ��ģ��ͼ��
			Contours templateContours = CurveComplete(maskImage_DS, zoomScale);

			afterZoomPoint = GetTemplatePoint(templateContours, -angle);

			/*���㵱ǰģ���µĻҶȲ���Ϣ��ģ��ƽ����*/
			tempResult = TemplateMatch(image_DS, afterZoomPoint, templateContours.dstImage, zoomScale);

			curResult.resultX += tempResult.resultX * zoomScale;
			curResult.resultY += tempResult.resultY * zoomScale;

			afterZoomGrayDiff += tempResult.grayDiff;
		}

		zoomScale = 1;

		/*���������ת�ǶȺ�ƽ����*/
		//�������Ѳ������㶼����ԭͼ1/2��С��ͼ���ϼ����
		if (afterZoomGrayDiff > finalResultGrayDiff)
		{
			finalResultGrayDiff = afterZoomGrayDiff;

			/*���ģ��㼯*/
			bestTemplatePoint = templatePoint;

			/*Get the best rotate angle*/
			bestAffine.angle = angle;

			/*Get the best translation in X direction*/
			bestAffine.transX = curResult.resultX / 3;

			/*Get the best translation in Y direction*/
			bestAffine.transY = curResult.resultY / 3;
		}

		clock_t t2 = clock();
		cout << "time is : " << (t2 - t1) * 1.0 / CLOCKS_PER_SEC << "s" << endl;
	}

	double x, y;
	for (int i = 0; i < bestTemplatePoint.templatePointX.size(); i++)
	{
		x = bestTemplatePoint.templatePointX[i] - bestAffine.transX;
		y = bestTemplatePoint.templatePointY[i] - bestAffine.transY;
		if (x >= 0 && x <= image.cols - 1 && y >= 0 && y <= image.rows - 1)
		{

			imageRGB.at<Vec3b>(y, x)[0] = 0;
			imageRGB.at<Vec3b>(y, x)[1] = 0;
			imageRGB.at<Vec3b>(y, x)[2] = 255;
		}
	}

	cout << "best angle is : " << bestAffine.angle * 180 / Pi << endl;
	cout << "best transX is : " << bestAffine.transX << endl;
	cout << "best transY is : " << bestAffine.transY << endl;

	namedWindow("��ƥ������", CV_WINDOW_NORMAL);
	imshow("��ƥ������", imageRGB);
}

int main(int argc, char *argv[])
{
	int index = 1;
	char maskImagePath[100], maskImageeRgbPath[100], imagePath[100], imageRgbPath[100];

	sprintf_s(maskImagePath, "E:\\���ݼ�\\�ʺ���ͼ��-0901\\02\\%d.bmp", index);
	sprintf_s(maskImageeRgbPath, "E:\\���ݼ�\\�ʺ���ͼ��-0901\\02\\%d.bmp", index);
	sprintf_s(imagePath, "E:\\���ݼ�\\�ʺ���ͼ��-0901\\01\\%d.bmp", index);
	sprintf_s(imageRgbPath, "E:\\���ݼ�\\�ʺ���ͼ��-0901\\01\\%d.bmp", index);

	Mat maskImage = imread(maskImagePath, 0);
	Mat maskImageeRGB = imread(maskImageeRgbPath, 1);
	Mat image = imread(imagePath, 0);
	Mat imageRGB = imread(imageRgbPath, 1);

	//Mat test;
	//pyrDown(maskImage, maskImage, Size(maskImage.cols / 2, maskImage.rows / 2));

	if (image.empty() || maskImage.empty())
	{
		cout << "could not load image...\n" << endl;
	}

	//TemplateLocate(maskImage, maskImageeRGB, image, imageRGB);
	PyramidTemplateLocate(maskImage, maskImageeRGB, image, imageRGB);

	waitKey(0);
	return 0;
}