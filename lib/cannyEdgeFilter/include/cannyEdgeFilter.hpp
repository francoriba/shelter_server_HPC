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

#ifndef _CANNY_EDGE_FILTER_HPP
#define _CANNY_EDGE_FILTER_HPP

#include "imageFileOperations.hpp"
#include <opencv2/core/core.hpp>
#include <omp.h>
#include <chrono>

constexpr auto KERNEL_SIZE {3};

/**
 * @brief The EdgeDetection class applies Canny edge detection to an image.
 */
class EdgeDetection
{
public:
    /**
     * @brief Constructor for the EdgeDetection class.
     *
     * @param lowThreshold The lower threshold value for edge detection.
     * @param highThreshold The higher threshold value for edge detection.
     * @param sigma Standard deviation of the Gaussian kernel.
     */
    EdgeDetection(float lowThreshold, float highThreshold, float sigma);

    /**
     * @brief Applies Canny edge detection to an image.
     * @param inputImage The input image file.
     * @param outputImage The output image file.
     */
    void cannyEdgeDetection(const std::string& inputImage, const std::string& outputImage);

private:
    float m_lowThreshold;
    float m_highThreshold;
    float m_sigma;
    std::shared_ptr<ImageFileOperations> m_imageFileOperations;
    cv::Mat m_magnitude;
    cv::Mat m_direction;
    cv::Mat m_cannyEdges;
    cv::Mat m_originalImage;

    /**
     * @brief Applies Gaussian blur to an image.
     *
     */
    void applyGaussianBlur(const std::string& outputImage);

    /**
     * @brief Calculates gradient magnitudes and directions using Sobel operators.
     *
     * @details This function applies Sobel operators to the input image to compute
     * the gradient magnitudes and directions. It convolves the image with
     * Sobel kernels for both x and y directions and then computes the
     * magnitude and direction of the gradients.
     */
    void sobelOperator(const std::string& outputImage);

    /**
     *
     * @brief Performs non-maximum suppression for edge detection based on
     * gradient magnitude and direction.
     *
     */
    void nonMaximumSuppression(const std::string& outputImage);

    /**
     * @brief  Applies a double threshold and edge tracking by hysteresis to an edge map.
     * This function identifies strong edges and weak edges and attempts to
     * connect weak edges to strong edges to form continuous lines.
     *
     */
    void applyLinkingAndHysteresis(const std::string& outputImage);

    /**
     * @brief Checks the contours of the image.
     * @param strongEdges The strong edges of the image.
     * @param weakEdges The weak edges of the image.
     * @param row The row of the pixel.
     * @param col The column of the pixel.
     * @param prevRow The previous row of the pixel.
     * @param prevCol The previous column of the pixel.
     */
    void checkContours(cv::Mat& strongEdges, const cv::Mat& weakEdges, int row, int col, int prevRow, int prevCol);
};

#endif /* _CANNY_EDGE_FILTER_HPP */
