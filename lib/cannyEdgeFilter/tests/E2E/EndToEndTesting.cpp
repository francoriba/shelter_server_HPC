/*
 * LuckyAlgorithmForSatellites - cannyEdgeFilter
 * Copyright (C) 2024, Operating Systems II.
 * Apr 24, 2024.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 */

#include "SatelliteImageWrapper.hpp"
#include "ImageFileOperations.hpp"
#include "CannyEdgeFilter.hpp"
#include <opencv2/highgui/highgui.hpp>
#include <iostream>

void runTests(){
    SatelliteImageWrapper imageWrapper("../../tests/sobelTest.jpg");
    if (!imageWrapper.isValid()) {
        std::cerr << "Failed to load image" << std::endl;
        return;
    }

    cv::Mat image = imageWrapper.readBand(1);
    if (image.empty()) {
        std::cerr << "Failed to read band 1 from image" << std::endl;
        return;
    }

    EdgeDetection edgeDetection;

    // Applying Gaussian Blur
    cv::Mat blurred(image.size(), image.type());
    edgeDetection.applyGaussianBlur();

    // Calculating the gradient magnitude and direction
    cv::Mat magnitude(image.size(), CV_32F), direction(image.size(), CV_32F);
    edgeDetection.sobelOperator(image, magnitude, direction);

    // Non-maximum suppression
    cv::Mat nonMaxSupp(image.size(), CV_8UC1);
    edgeDetection.nonMaximumSuppression(magnitude, direction, nonMaxSupp);

    // Double threshold and edge tracking by hysteresis
    cv::Mat edges(image.size(), CV_8UC1);
    edgeDetection.applyLinkingAndHysteresis(nonMaxSupp, edges, 50, 150);

    // Display results
    cv::imshow("Original Image", image);
    cv::imshow("Magnitude", magnitude);
    cv::imshow("Direction", direction);
    cv::waitKey(0);

    std::cout << "Tests passed" << std::endl;
}

int main() {

    runTests();    

    return 0;
}
