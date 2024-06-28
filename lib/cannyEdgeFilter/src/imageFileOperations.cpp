/*
 * LuckyAlgorithmForSatellites - imageFileOperations
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

#include "imageFileOperations.hpp"

bool ImageFileOperations::saveImage(const std::string& filename, const cv::Mat& image)
{
    return cv::imwrite(filename, image);
}

cv::Mat ImageFileOperations::loadImage(const std::string& filename)
{
    return cv::imread(filename, cv::IMREAD_GRAYSCALE);
}
