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

using namespace std;
using namespace cv;
using namespace cv::xfeatures2d;


struct templatePointVector
{
	vector<int> templatePointX;
	vector<int> templatePointY;
};


//templatePointVector GetTemplate(Mat &image)
//{
//	Mat cannyout, dstImage, templateImage;
//
//	int thresholeLow = 100; int thresholdUp = 255;
//	Canny(image, cannyout, thresholeLow, thresholdUp);
//	namedWindow("Canny", CV_WINDOW_NORMAL);
//	imshow("Canny", cannyout);
//
//	vector<vector<Point>> contours;
//	//vector<int> templatePointX, templatePointY;
//
//	templatePointVector templatePoint;
//
//	vector<Vec4i> hierachy;
//	findContours(cannyout, contours, hierachy, RETR_TREE, CHAIN_APPROX_NONE, Point(0, 0));
//
//	dstImage = Mat::zeros(image.size(), CV_8UC1);
//	for (size_t i = 0; i < contours.size(); i++)
//	{
//		double Area = contourArea(contours[i]);
//		double Length = arcLength(contours[i], true);
//		if (Area > 60 & Length > 60)
//		{
//			drawContours(dstImage, contours, i, Scalar(255), 1, 8, hierachy, 1, Point(0, 0));
//			for (size_t j = 0; j < contours[i].size(); j++)
//			{
//				//double ans = contours[i][j].x;
//				//cout << 123 << endl;
//				templatePoint.templatePointX.push_back(contours[i][j].x);
//				templatePoint.templatePointY.push_back(contours[i][j].y);
//			}
//		}
//	}
//
//	namedWindow("��������", CV_WINDOW_NORMAL);
//	imshow("��������", dstImage);
//
//	return templatePoint;
//}


templatePointVector GetTemplate(Mat &image)
{
	templatePointVector templatePoint;

	for (size_t i = 0; i < image.cols; i++)
	{
		for (size_t j = 0; j < image.rows; j++)
		{
			if (image.at<uchar>(j,i)!=0)
			{
				templatePoint.templatePointX.push_back(i);
				templatePoint.templatePointY.push_back(j);
			}
		}
	}

	return templatePoint;
}


void TemplateMatch(Mat& image, Mat& imageRGB, templatePointVector& templatePoint, Mat& templateImage)
{
	int pixelStep = 10;
	int patchStep = 64;
	double grayDiff = 0;
	double resultX, resultY;
	Point result;

	vector<int> templatePointX = templatePoint.templatePointX;
	vector<int> templatePointY = templatePoint.templatePointY;

	/*Ѱ��ģ��ͼ��������͹�����ε����Ͻǵ�*/
	vector<int>::iterator minTemplatePointX = min_element(begin(templatePointX), end(templatePointX));
	vector<int>::iterator minTemplatePointY = min_element(begin(templatePointY), end(templatePointY));
	Point minPosition = Point(*minTemplatePointX, *minTemplatePointY);

	for (size_t col = 0; col < image.cols; col += patchStep)
	{
		for (size_t row = 0; row < image.rows; row += patchStep)
		{
			cout << "col\\cols:" << col << "\\" << image.cols << endl;

			/*���㵱ǰ����ģ��Ľṹ��ƫ��*/
			int minX = minPosition.x;
			int minY = minPosition.y;
			int offsetX = col - minX;
			int offsetY = row - minY;
			//cout << offsetY << endl;
			/*��ȡ��ǰ��(row,col)��ģ��ṹ����Ϣ���֮���grayDiff*/
			double temp = 0;
			for (size_t i = 0; i < templatePointX.size(); i++)
			{
				/*ģ��ͼ���е� - ƫ�� = ����ͼ���ж�Ӧ�ĵ�*/
				Point pt = Point(templatePointX[i] - offsetX, templatePointY[i] - offsetY);

				/*���㵱ǰ�����������ڻҶȲ���Ϣ*/
				if (pt.x >= 0 && pt.x < image.cols  && pt.y >= 2 * pixelStep && pt.y < image.rows - 2 * pixelStep)
				{
					double curPointValue = image.at<uchar>(pt.y, pt.x);
					double curPointValue_U1 = image.at<uchar>(pt.y, pt.x - 1 * pixelStep);
					double curPointValue_U2 = image.at<uchar>(pt.y, pt.x - 2 * pixelStep);
					double curPointValue_D1 = image.at<uchar>(pt.y, pt.x + 1 * pixelStep);
					double curPointValue_D2 = image.at<uchar>(pt.y, pt.x + 2 * pixelStep);

					temp += abs(4 * curPointValue - curPointValue_U1 - curPointValue_U2 - curPointValue_D1 - curPointValue_D2);
				}
			}

			if (temp > grayDiff)
			{
				grayDiff = temp;

				//resultX = offsetX + patchStep;
				//resultY = offsetY + patchStep;
				resultX = offsetX ;
				resultY = offsetY ;
			}
		}
	}

	for (size_t i = 0; i < templatePointX.size(); i++)
	{
		double x = templatePointX[i] - resultX;
		double y = templatePointY[i] - resultY;
		if (x >= 0 && x <= image.cols - 1 && y >= 0 && y <= image.rows - 1)
		{
			imageRGB.at<Vec3b>(y, x)[0] = 0;
			imageRGB.at<Vec3b>(y, x)[1] = 0;
			imageRGB.at<Vec3b>(y, x)[2] = 255;
		}
	}
	
	namedWindow("��ƥ������", CV_WINDOW_NORMAL);
	imshow("��ƥ������", imageRGB);
}


void PixelGrow(Mat srcImage, Mat& Curve, Mat& srcClone, Point pt, int Thres, int threshExpect, int LowerBind, int UpperBind)
{
	Point pToGrowing;								    //��������λ��
	int pGrowValue = 0;                                 //��������Ҷ�ֵ
	double pSrcValue = 0;                               //�������Ҷ�ֵ
	double pCurValue = 0;                               //��ǰ������Ҷ�ֵ

	//��������˳������
	int DIR[8][2] = { {-1,-1}, {0,-1}, {1,-1}, {1,0}, {1,1}, {0,1}, {-1,1}, {-1,0} };
	vector<Point> growPtVector;							//������ջ
	growPtVector.push_back(pt);                         //��������ѹ��ջ��

	pSrcValue = srcImage.at<uchar>(pt.y, pt.x);         //��¼������ĻҶ�ֵ

	while (!growPtVector.empty())                       //����ջ��Ϊ��������
	{
		pt = growPtVector.back();                       //ȡ��һ��������
		growPtVector.pop_back();

		//�ֱ�԰˸������ϵĵ��������
		for (int i = 0; i < 8; ++i)
		{
			pToGrowing.x = pt.x + DIR[i][0];
			pToGrowing.y = pt.y + DIR[i][1];
			//����Ƿ��Ǳ�Ե��
			if (pToGrowing.x < 0 || pToGrowing.y < 0 || pToGrowing.x >(srcImage.cols - 1) || (pToGrowing.y > srcImage.rows - 1))
				continue;
			if (Curve.at<uchar>(pToGrowing.y, pToGrowing.x) != 0)continue;

			pSrcValue = srcImage.at<uchar>(pt.y, pt.x);
			if (pGrowValue == 0)//�����ǵ㻹û�б�����
			{
				pCurValue = srcImage.at<uchar>(pToGrowing.y, pToGrowing.x);
				if (pCurValue <= UpperBind && pCurValue >= LowerBind)
				{
					if (((abs(pCurValue - pSrcValue) < Thres) || (pCurValue > threshExpect)) && (pCurValue > threshExpect) && (pSrcValue > threshExpect)) //����ֵ��Χ��������
					{
						srcClone.at<Vec3b>(pToGrowing.y, pToGrowing.x) = Vec3b(0, 0, 255);
						Curve.at<uchar>(pToGrowing.y, pToGrowing.x) = 255;//���Ϊ��ɫ
						growPtVector.push_back(pToGrowing);//����һ��������ѹ��ջ��
					}
				}
			}
		}
	}
}


Mat CurveComplete(Mat& srcImage, Mat& srcClone)
{
	int thresholdValue = 50;
	int thresholdMax = 255;
	int Thres = 20;
	int threshExpect = 140;
	int LowerBind = 0; 
	int UpperBind = 255;
	double center = 0.0;

	Mat cannyOutput;
	vector<vector<Point>> contours;
	vector<Vec4i> hierachy;
	Canny(srcImage, cannyOutput, 150, 255, 3, false);
	namedWindow("Canny output", CV_WINDOW_NORMAL);
	imshow("Canny output", cannyOutput);

	findContours(cannyOutput, contours, hierachy, RETR_TREE, CHAIN_APPROX_NONE, Point(0, 0));

	Mat dstImg = Mat::zeros(srcImage.size(), CV_8UC1);
	//������
	for (size_t i = 0; i < contours.size(); i++)
	{
		double area = contourArea(contours[i]);
		double length = arcLength(contours[i], true);
		if (area > 50 & length > 50)
		{
			drawContours(dstImg, contours, i, Scalar(255), 1, 8, hierachy, 1, Point(0, 0));
			for (size_t j = 0; j < contours[i].size(); j++)
			{
				srcClone.at<Vec3b>(contours[i][j])[0] = 0;
				srcClone.at<Vec3b>(contours[i][j])[1] = 0;
				srcClone.at<Vec3b>(contours[i][j])[2] = 255;
			}
		}
	}
	namedWindow("������ͼ��", CV_WINDOW_NORMAL);
	imshow("������ͼ��", dstImg);

	namedWindow("��srcClone��", CV_WINDOW_NORMAL);
	imshow("��srcClone��", srcClone);

	return dstImg;
}


int main(int argc, char *argv[])
{
	Mat maskImage =     imread("E:\\���ݼ�\\�ʺ���ͼ��-0901\\02\\6.bmp", 0);
	Mat maskImageeRGB = imread("E:\\���ݼ�\\�ʺ���ͼ��-0901\\02\\6.bmp", 1);
	Mat image =         imread("E:\\���ݼ�\\�ʺ���ͼ��-0901\\01\\6.bmp", 0);
	Mat imageRGB =      imread("E:\\���ݼ�\\�ʺ���ͼ��-0901\\01\\6.bmp", 1);

	Mat templateImage = CurveComplete(maskImage, maskImageeRGB);
	namedWindow("templateImage", CV_WINDOW_NORMAL);
	imshow("templateImage", templateImage);

	if (image.empty()|| maskImage.empty())
	{
		cout << "could not load image...\n" << endl;
	}

	templatePointVector templatePoint = GetTemplate(templateImage);

	TemplateMatch(image, imageRGB, templatePoint, templateImage);

	waitKey(0);
	return 0;

}


