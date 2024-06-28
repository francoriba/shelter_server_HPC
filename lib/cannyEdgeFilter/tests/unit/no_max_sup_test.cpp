#include "CannyEdgeFilter.hpp"
#include <gtest/gtest.h>

#define DEGRESS_TO_RAD(angle) ((angle * CV_PI) / 180)

TEST(NonMaxSuppTests, NonMaxSupp) {
  cv::Mat testImage(5, 5, CV_32F);

  // High values
  testImage.at<uint8_t>(0, 0) = 30;
  testImage.at<uint8_t>(0, 2) = 30;
  testImage.at<uint8_t>(0, 4) = 30;
  testImage.at<uint8_t>(2, 0) = 30;

  // Middle values
  testImage.at<uint8_t>(1, 1) = 10;
  testImage.at<uint8_t>(1, 2) = 10;
  testImage.at<uint8_t>(1, 3) = 10;
  testImage.at<uint8_t>(2, 1) = 10;
  testImage.at<uint8_t>(2, 2) = 10;
  testImage.at<uint8_t>(2, 3) = 10;
  testImage.at<uint8_t>(3, 1) = 10;
  testImage.at<uint8_t>(3, 2) = 10;
  testImage.at<uint8_t>(3, 3) = 10;

  // Low values
  testImage.at<uint8_t>(2, 4) = 5;
  testImage.at<uint8_t>(4, 0) = 5;
  testImage.at<uint8_t>(4, 2) = 5;
  testImage.at<uint8_t>(4, 4) = 5;

  cv::Mat testDirs(5, 5, CV_32F);
  testDirs.at<float>(1, 1) = DEGRESS_TO_RAD(157);  // 135
  testDirs.at<float>(1, 2) = DEGRESS_TO_RAD(68);   // 90
  testDirs.at<float>(1, 3) = DEGRESS_TO_RAD(23);   // 45
  testDirs.at<float>(2, 1) = DEGRESS_TO_RAD(22);   // 0
  testDirs.at<float>(2, 2) = DEGRESS_TO_RAD(0);    // 0
  testDirs.at<float>(2, 3) = DEGRESS_TO_RAD(-158); // 0
  testDirs.at<float>(3, 1) = DEGRESS_TO_RAD(-157); // 45
  testDirs.at<float>(3, 2) = DEGRESS_TO_RAD(-22);  // 90
  testDirs.at<float>(3, 3) = DEGRESS_TO_RAD(-23);  // 135

  cv::Mat testResults(5, 5, CV_32F);

  EdgeDetection edgeDetection;

  edgeDetection.nonMaximumSuppression(testImage, testDirs, testResults);

  ASSERT_EQ(testResults.at<uint8_t>(1, 1), 0);
  ASSERT_EQ(testResults.at<uint8_t>(1, 2), 0);
  ASSERT_EQ(testResults.at<uint8_t>(1, 3), 0);
  ASSERT_EQ(testResults.at<uint8_t>(2, 1), 0);
  ASSERT_EQ(testResults.at<uint8_t>(2, 3), 10);
  ASSERT_EQ(testResults.at<uint8_t>(3, 1), 10);
  ASSERT_EQ(testResults.at<uint8_t>(3, 2), 10);
  ASSERT_EQ(testResults.at<uint8_t>(3, 3), 10);
}
