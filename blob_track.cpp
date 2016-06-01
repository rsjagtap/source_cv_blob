#include <stdio.h>
#include <iostream>

#include <opencv2/opencv.hpp>
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/video/background_segm.hpp>	// for backgound substraction
#include <cvblob.h>

#include "Blob.h"				// for blobs

#include<math.h>
#include <set>

using namespace cv;
using namespace std;

// method declaration ///////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName);
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName);
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs);
double distanceBetweenPoints(cv::Point point1, cv::Point point2);
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex);
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs);
bool checkIfBlobsCrossedTheLineHOut(std::vector<Blob> &blobs, int &intHorizontalLinePositionB, int &carCountHOut, int &carCountHIn);
bool checkIfBlobsCrossedTheLineHIn(std::vector<Blob> &blobs, int &intHorizontalLinePositionT, int &carCountHOut, int &carCountHIn);
bool checkIfBlobsCrossedTheLineVOut(std::vector<Blob> &blobs, int &intHorizontalLinePositionB, int &carCountVOut, int &carCountVIn);
bool checkIfBlobsCrossedTheLineVIn(std::vector<Blob> &blobs, int &intHorizontalLinePositionT, int &carCountVOut, int &carCountVIn);
void drawCarCountOnImage(int &carCountHOut, int &carCountHIn, int &carCountVOut, int &carCountVIn, cv::Mat &imgFrame2Copy);


// global variables ///////////////////////////////////////////////////////////////////////////////
const cv::Scalar SCALAR_BLACK = cv::Scalar(0.0, 0.0, 0.0);
const cv::Scalar SCALAR_WHITE = cv::Scalar(255.0, 255.0, 255.0);
const cv::Scalar SCALAR_YELLOW = cv::Scalar(0.0, 255.0, 255.0);
const cv::Scalar SCALAR_GREEN = cv::Scalar(0.0, 200.0, 0.0);
const cv::Scalar SCALAR_RED = cv::Scalar(0.0, 0.0, 255.0);


int main(int argc, char* argv[])
{
if(argc != 2)
{
    cout
    << "--------------------------------------------------------------------------" << endl
    << "Usage:"                                                                     << endl
    << "./blob_track video_filename"			                            << endl
    << "--------------------------------------------------------------------------" << endl
    << endl;
	
exit(1);
}


	std::vector<Blob> blobs;
	//global variables
	Mat frame1; //current frame
	Mat frame2;


	Mat resize_blur_Img1;
	Mat resize_blur_Img2;

	Mat fgMaskMOG2; //fg mask fg mask generated by MOG2 method
	Mat binaryImg;
	//Mat TestImg;

	Mat ContourImg; //fg mask fg mask generated by MOG2 method
	Ptr< BackgroundSubtractor> pMOG2; //MOG2 Background subtractor
	pMOG2 = createBackgroundSubtractorMOG2(300,32,true);//300,0.0);


	//For Crossing line
	cv::Point crossingLineB[2];
	cv::Point crossingLineT[2];

	cv::Point crossingLineVB[2];
	cv::Point crossingLineVT[2];

	//string vid = argv[1];
	//	char fileName[100] = "../dataset/CarsDrivingUnderBridge.mp4";
	char* fileName = argv[1];// "../dataset/1.mp4";
	//	char fileName[100] = "//home//rohit//Desktop//CV_BLOB_Examples//OpenCV_3_Car_Counting//CarsDrivingUnderBridge.mp4";
	//	char fileName[100] = "rtsp://admin:12345@10.73.73.26/h264/ch1/main/av_stream?tcp";


	VideoCapture stream1(fileName);   //0 is the id of video device.0 if you have only one camera   


	stream1.read(frame1);
	stream1.read(frame2);

	int intHorizontalLinePositionB = (int)round((double)frame1.rows * 0.80);
	int intHorizontalLinePositionT = (int)round((double)frame1.rows * 0.10);
	int intVerticalLinePositionB = (int)round((double)frame1.cols * 0.10);
	int intVerticalLinePositionT = (int)round((double)frame1.cols * 0.80);

	crossingLineB[0].x = 0;
	crossingLineB[0].y = intHorizontalLinePositionB;
	crossingLineB[1].x = frame1.cols - 1;
	crossingLineB[1].y = intHorizontalLinePositionB;

	crossingLineT[0].x = 0;
	crossingLineT[0].y = intHorizontalLinePositionT;
	crossingLineT[1].x = frame1.cols - 1;
	crossingLineT[1].y = intHorizontalLinePositionT;

	crossingLineVB[0].x = intVerticalLinePositionB;
	crossingLineVB[0].y = 0;
	crossingLineVB[1].x = intVerticalLinePositionB;
	crossingLineVB[1].y = frame1.rows - 1;

	crossingLineVT[0].x = intVerticalLinePositionT;
	crossingLineVT[0].y = 0;
	crossingLineVT[1].x = intVerticalLinePositionT;
	crossingLineVT[1].y = frame1.rows - 1;



	int carCountHOut = 0;
	int carCountHIn = 0;
	int carCountVOut = 0;
	int carCountVIn = 0;

	int frameCount = 2;
	bool blnFirstFrame = true;
	//morphology element
	Mat element = getStructuringElement(MORPH_RECT, Size(7, 7), Point(3,3) );   


	//unconditional loop   
	while (stream1.isOpened()) {   
		Mat cameraFrame;   
		//		if(!(stream1.read(frame))) //get one frame form video   
		//			break;

		//Resize
		resize(frame1, resize_blur_Img1, Size(frame1.size().width/1, frame1.size().height/1) );
		resize(frame2, resize_blur_Img2, Size(frame2.size().width/1, frame2.size().height/1) );

		imshow("Blur_Resize", resize_blur_Img1);

		//Blur
		//		blur(resize_blur_Img, resize_blur_Img, Size(4,4) );

		std::vector<Blob> currentFrameBlobs;

		cv::Mat imgFrame1Copy = resize_blur_Img1.clone();
		cv::Mat imgFrame2Copy = resize_blur_Img2.clone();

		cv::Mat imgDifference;
		cv::Mat imgThresh;

		cv::cvtColor(imgFrame1Copy, imgFrame1Copy, CV_BGR2GRAY);
		cv::cvtColor(imgFrame2Copy, imgFrame2Copy, CV_BGR2GRAY);

		cv::GaussianBlur(imgFrame1Copy, imgFrame1Copy, cv::Size(5, 5), 0);
		cv::GaussianBlur(imgFrame2Copy, imgFrame2Copy, cv::Size(5, 5), 0);


				cv::absdiff(imgFrame1Copy, imgFrame2Copy, imgDifference);

				cv::threshold(imgDifference, imgThresh, 30, 255.0, CV_THRESH_BINARY);

				cv::imshow("imgThresh", imgThresh);

				cv::Mat structuringElement5x5 = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));


				for (unsigned int i = 0; i < 2; i++) {
					cv::dilate(imgThresh, imgThresh, structuringElement5x5);
					cv::dilate(imgThresh, imgThresh, structuringElement5x5);
					cv::erode(imgThresh, imgThresh, structuringElement5x5);
				}

				cv::Mat imgThreshCopy = imgThresh.clone();

				std::vector<std::vector<cv::Point> > contours;

				cv::findContours(imgThreshCopy, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

				drawAndShowContours(imgThresh.size(), contours, "imgContours");



				std::vector<std::vector<cv::Point> > convexHulls(contours.size());

				for (unsigned int i = 0; i < contours.size(); i++) {
					cv::convexHull(contours[i], convexHulls[i]);
				}

				drawAndShowContours(imgThresh.size(), convexHulls, "imgConvexHulls");

				for (auto &convexHull : convexHulls) {
					Blob possibleBlob(convexHull);

					if (possibleBlob.currentBoundingRect.area() > 400 &&
							possibleBlob.dblCurrentAspectRatio > 0.2 &&
							possibleBlob.dblCurrentAspectRatio < 4.0 &&
							possibleBlob.currentBoundingRect.width > 30 &&
							possibleBlob.currentBoundingRect.height > 30 &&
							possibleBlob.dblCurrentDiagonalSize > 60.0 &&
							(cv::contourArea(possibleBlob.currentContour) / (double)possibleBlob.currentBoundingRect.area()) > 0.50) {
						currentFrameBlobs.push_back(possibleBlob);
					}
				}

				drawAndShowContours(imgThresh.size(), currentFrameBlobs, "imgCurrentFrameBlobs");

				if (blnFirstFrame == true) {
					for (auto &currentFrameBlob : currentFrameBlobs) {
						blobs.push_back(currentFrameBlob);
					}
				} else {
					matchCurrentFrameBlobsToExistingBlobs(blobs, currentFrameBlobs);
				}

				drawAndShowContours(imgThresh.size(), blobs, "imgBlobs");



				imgFrame2Copy = frame2.clone();          // get another copy of frame 2 since we changed the previous frame 2 copy in the processing above

				//drawBlobInfoOnImage(blobs, imgFrame2Copy);

				bool blnAtLeastOneBlobCrossedTheLineB = checkIfBlobsCrossedTheLineHOut(blobs, intHorizontalLinePositionB, carCountHOut, carCountHIn);
				bool blnAtLeastOneBlobCrossedTheLineT = checkIfBlobsCrossedTheLineHIn(blobs, intHorizontalLinePositionT, carCountHOut, carCountHIn);
				bool blnAtLeastOneBlobCrossedTheLineVB = checkIfBlobsCrossedTheLineVOut(blobs, intVerticalLinePositionB, carCountVOut, carCountVIn);
				bool blnAtLeastOneBlobCrossedTheLineVT = checkIfBlobsCrossedTheLineVIn(blobs, intVerticalLinePositionT, carCountVOut, carCountVIn);


				if (blnAtLeastOneBlobCrossedTheLineB == true) {
					cv::line(imgFrame2Copy, crossingLineB[0], crossingLineB[1], SCALAR_GREEN, 2);
				}
				else {
					cv::line(imgFrame2Copy, crossingLineB[0], crossingLineB[1], SCALAR_RED, 2);
				}


				if (blnAtLeastOneBlobCrossedTheLineT == true) {
					cv::line(imgFrame2Copy, crossingLineT[0], crossingLineT[1], SCALAR_GREEN, 2);
				}
				else {
					cv::line(imgFrame2Copy, crossingLineT[0], crossingLineT[1], SCALAR_RED, 2);
				}

				if (blnAtLeastOneBlobCrossedTheLineVB == true) {
					cv::line(imgFrame2Copy, crossingLineVB[0], crossingLineVB[1], SCALAR_GREEN, 2);
				}
				else {
					cv::line(imgFrame2Copy, crossingLineVB[0], crossingLineVB[1], SCALAR_RED, 2);
				}

				if (blnAtLeastOneBlobCrossedTheLineVT == true) {
					cv::line(imgFrame2Copy, crossingLineVT[0], crossingLineVT[1], SCALAR_GREEN, 2);
				}
				else {
					cv::line(imgFrame2Copy, crossingLineVT[0], crossingLineVT[1], SCALAR_RED, 2);
				}



				drawCarCountOnImage(carCountHOut, carCountHIn,carCountVOut, carCountVIn, imgFrame2Copy);

		cv::imshow("imgFrame2Copy", imgFrame2Copy);

		//cv::waitKey(0);                 // uncomment this line to go frame by frame for debugging

		// now we prepare for the next iteration

		currentFrameBlobs.clear();

		frame1 = frame2.clone();           // move frame 1 up to where frame 2 is

		if ((stream1.get(CV_CAP_PROP_POS_FRAMES) + 1) < stream1.get(CV_CAP_PROP_FRAME_COUNT)) {
			stream1.read(frame2);
		}
		else {
			std::cout << "end of video\n";
			break;
		}

		blnFirstFrame = false;
		frameCount++;

		//        chCheckForEscKey = cv::waitKey(1);
		cv::waitKey(1);
}

}

/*

//Background subtraction
//		pMOG2->apply(resize_blur_Img, fgMaskMOG2, -7);//-0.5);

///////////////////////////////////////////////////////////////////
//pre procesing
//1 point delete
//morphologyEx(fgMaskMOG2, fgMaskMOG2, CV_MOP_ERODE, element);
//		morphologyEx(fgMaskMOG2, binaryImg, CV_MOP_CLOSE, element);

 */
void drawAndShowContours(cv::Size imageSize, std::vector<std::vector<cv::Point> > contours, std::string strImageName) {
	cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

	cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

	cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void drawAndShowContours(cv::Size imageSize, std::vector<Blob> blobs, std::string strImageName) {

	cv::Mat image(imageSize, CV_8UC3, SCALAR_BLACK);

	std::vector<std::vector<cv::Point> > contours;

	for (auto &blob : blobs) {
		if (blob.blnStillBeingTracked == true) {
			contours.push_back(blob.currentContour);
		}
	}

	cv::drawContours(image, contours, -1, SCALAR_WHITE, -1);

	cv::imshow(strImageName, image);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void matchCurrentFrameBlobsToExistingBlobs(std::vector<Blob> &existingBlobs, std::vector<Blob> &currentFrameBlobs) {

	for (auto &existingBlob : existingBlobs) {

		existingBlob.blnCurrentMatchFoundOrNewBlob = false;

		existingBlob.predictNextPosition();
	}

	for (auto &currentFrameBlob : currentFrameBlobs) {

		int intIndexOfLeastDistance = 0;
		double dblLeastDistance = 100000.0;

		for (unsigned int i = 0; i < existingBlobs.size(); i++) {

			if (existingBlobs[i].blnStillBeingTracked == true) {

				double dblDistance = distanceBetweenPoints(currentFrameBlob.centerPositions.back(), existingBlobs[i].predictedNextPosition);

				if (dblDistance < dblLeastDistance) {
					dblLeastDistance = dblDistance;
					intIndexOfLeastDistance = i;
				}
			}
		}

		if (dblLeastDistance < currentFrameBlob.dblCurrentDiagonalSize * 0.5) {
			addBlobToExistingBlobs(currentFrameBlob, existingBlobs, intIndexOfLeastDistance);
		}
		else {
			addNewBlob(currentFrameBlob, existingBlobs);
		}

	}

	for (auto &existingBlob : existingBlobs) {

		if (existingBlob.blnCurrentMatchFoundOrNewBlob == false) {
			existingBlob.intNumOfConsecutiveFramesWithoutAMatch++;
		}

		if (existingBlob.intNumOfConsecutiveFramesWithoutAMatch >= 5) {
			existingBlob.blnStillBeingTracked = false;
		}

	}

}


/////////////////////////////////////////////////////////////////////////////////////////////////
double distanceBetweenPoints(cv::Point point1, cv::Point point2) {

	int intX = abs(point1.x - point2.x);
	int intY = abs(point1.y - point2.y);

	return(sqrt(pow(intX, 2) + pow(intY, 2)));
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addBlobToExistingBlobs(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs, int &intIndex) {

	existingBlobs[intIndex].currentContour = currentFrameBlob.currentContour;
	existingBlobs[intIndex].currentBoundingRect = currentFrameBlob.currentBoundingRect;

	existingBlobs[intIndex].centerPositions.push_back(currentFrameBlob.centerPositions.back());

	existingBlobs[intIndex].dblCurrentDiagonalSize = currentFrameBlob.dblCurrentDiagonalSize;
	existingBlobs[intIndex].dblCurrentAspectRatio = currentFrameBlob.dblCurrentAspectRatio;

	existingBlobs[intIndex].blnStillBeingTracked = true;
	existingBlobs[intIndex].blnCurrentMatchFoundOrNewBlob = true;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void addNewBlob(Blob &currentFrameBlob, std::vector<Blob> &existingBlobs) {

	currentFrameBlob.blnCurrentMatchFoundOrNewBlob = true;

	existingBlobs.push_back(currentFrameBlob);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineHOut(std::vector<Blob> &blobs, int &intHorizontalLinePositionB, int &carCountHOut, int &carCountHIn) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].y > intHorizontalLinePositionB && blob.centerPositions[currFrameIndex].y <= intHorizontalLinePositionB) {
				carCountHOut++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}

		}

	}

	return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineHIn(std::vector<Blob> &blobs, int &intHorizontalLinePositionT, int &carCountHOut, int &carCountHIn) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].y < intHorizontalLinePositionT && blob.centerPositions[currFrameIndex].y >= intHorizontalLinePositionT) {
				carCountHIn++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}

		}

	}

	return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineVOut(std::vector<Blob> &blobs, int &intVerticalLinePositionB, int &carCountVOut, int &carCountVIn) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].x < intVerticalLinePositionB && blob.centerPositions[currFrameIndex].x >= intVerticalLinePositionB) {
				carCountVOut++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}

		}

	}

	return blnAtLeastOneBlobCrossedTheLine;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
bool checkIfBlobsCrossedTheLineVIn(std::vector<Blob> &blobs, int &intVerticalLinePositionT, int &carCountVOut, int &carCountVIn) {
	bool blnAtLeastOneBlobCrossedTheLine = false;

	for (auto blob : blobs) {

		if (blob.blnStillBeingTracked == true && blob.centerPositions.size() >= 2) {
			int prevFrameIndex = (int)blob.centerPositions.size() - 2;
			int currFrameIndex = (int)blob.centerPositions.size() - 1;

			if (blob.centerPositions[prevFrameIndex].x > intVerticalLinePositionT && blob.centerPositions[currFrameIndex].x <= intVerticalLinePositionT) {
				carCountVIn++;
				blnAtLeastOneBlobCrossedTheLine = true;
			}

		}

	}

	return blnAtLeastOneBlobCrossedTheLine;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
void drawCarCountOnImage(int &carCountHOut, int &carCountHIn,int &carCountVOut, int &carCountVIn, cv::Mat &imgFrame2Copy) {

	int intFontFace = CV_FONT_HERSHEY_SIMPLEX;
	double dblFontScale = (imgFrame2Copy.rows * imgFrame2Copy.cols) / 900000.0;
	int intFontThickness = (int)round(dblFontScale * 1.5);

	cv::Size textSize = cv::getTextSize(std::to_string(carCountHOut), intFontFace, dblFontScale, intFontThickness, 0);

	cv::Point ptTextBottomRightPosition;
	cv::Point ptTextBottomLeftPosition;
	cv::Point ptTextTopLeftPosition;
	cv::Point ptTextTopRightPosition;

	ptTextBottomRightPosition.x = imgFrame2Copy.cols - 100 - (int)((double)textSize.width * 1.25);
	//    ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);
	ptTextBottomRightPosition.y = imgFrame2Copy.rows - 1 -  (int)((double)textSize.height * 1.25);

	ptTextBottomLeftPosition.x = 10;// imgFrame2Copy.cols - 200 - (int)((double)textSize.width * 1.25);
	//    ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);
	ptTextBottomLeftPosition.y = imgFrame2Copy.rows - 1 -  (int)((double)textSize.height * 1.25);

	ptTextTopLeftPosition.x = 15;// imgFrame2Copy.cols - 200 - (int)((double)textSize.width * 1.25);
	//    ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);
	ptTextTopLeftPosition.y = 20;//imgFrame2Copy.rows - 1 -  (int)((double)textSize.height * 1.25);

	ptTextTopRightPosition.x = imgFrame2Copy.cols - 100 - (int)((double)textSize.width * 1.25);
	//    ptTextBottomLeftPosition.y = (int)((double)textSize.height * 1.25);
	ptTextTopRightPosition.y = (int)((double)textSize.height * 1.25);

	cv::putText(imgFrame2Copy, std::to_string(carCountHOut) + " vOut", ptTextBottomRightPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
	cv::putText(imgFrame2Copy, std::to_string(carCountHIn) + " vIn", ptTextBottomLeftPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
	cv::putText(imgFrame2Copy, std::to_string(carCountVOut) + " hOut", ptTextTopLeftPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);
	cv::putText(imgFrame2Copy, std::to_string(carCountVIn) + " hIn", ptTextTopRightPosition, intFontFace, dblFontScale, SCALAR_GREEN, intFontThickness);

}

