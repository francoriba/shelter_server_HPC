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

#ifndef _IMAGE_FILE_OPERATIONS_HPP
#define _IMAGE_FILE_OPERATIONS_HPP

#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>

/**
 * @brief The ImageFileOperations class provides methods to save and load images.
 */
class ImageFileOperations
{
public:
    /**
     * @brief Saves an image to a file.
     * @param filename The name of the file to save the image to.
     * @param image The image to save.
     */
    bool saveImage(const std::string& filename, const cv::Mat& image);

    /**
     * @brief Loads an image from a file.
     * @param filename The name of the file to load the image from.
     * @return The loaded image.
     */
    cv::Mat loadImage(const std::string& filename);
};

#endif /* _IMAGE_FILE_OPERATIONS_HPP */
