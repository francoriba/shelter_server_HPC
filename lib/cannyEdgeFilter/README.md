# The Luck is on Our Side 

[![Minimal QA Workflow](https://github.com/ICOMP-UNC/cannyEdgeFilter/actions/workflows/StyleAndDoc.yml/badge.svg?branch=master)](https://github.com/ICOMP-UNC/cannyEdgeFilter/actions/workflows/StyleAndDoc.yml)

## 🎯 Objectives

- **Develop a C++ library** that encapsulates the complexity of the Canny edge detection algorithm, providing a high-performance tool for image processing tasks.

## 📝 Evaluation Aspects

1. **Code Quality and Organization**: Code should be well-organized, commented, and adhere to modern C++ standards (C++17 and above).
2. **Functionality**: All functions must perform as specified, robustly processing images to detect edges.
3. **Robustness**: The application must efficiently handle large images and ensure accurate edge detection under varying conditions without crashing.
4. **Documentation**: Comprehensive documentation must be provided, detailing how to use the library and the underlying design choices.

## 📽 🎞 And one day...

_In the shadowed silence of a long-abandoned technological building, two explorers, Maya and Eliot, stumble upon a relic of the past—a dusty, seemingly forsaken laptop. Upon awakening its slumbering screen, they discover a treasure trove of code—development work on a sophisticated image processing library designed by two legendary developers, **Gabriel** and **Agustin**, known for their pioneering contributions to computational photography. This library, aimed at mastering the complexities of the Canny edge detection algorithm, could provide invaluable insights into both the past and the future of image analysis._

<p align="center">
  <img src="img/canny.png"/>
</p>

### Application Description:

The Canny Edge Detection algorithm implemented in this library involves several key steps, each optimized for performance and accuracy. Here is the prototype in C++ for each step:

```cpp
// Pending finish this.

// Divide in classes for each task:
// - satelliteImageWrapper to handle GDAL operations
// - imageFileOperations to store and read files images
// - applyEdgeDetection to encapsulate the Canny Algorithm logic

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

#include "gdal_priv.h"

/**
 * Applies Gaussian blur to an image.
 * @param sigma Standard deviation of the Gaussian kernel. ?
 */
void applyGaussianBlur(...args) {
    // [Implement Gaussian blurring]
}

/**
 * Calculates gradient magnitudes and directions using Sobel operators.
 * @param args ???????????????
 */
void sobelOperator(...args) {
    // [Calculate gradients, magnitude, and direction]
}

/**
 * Performs non-maximum suppression to thin out the edges.
 * @param args ????????????
 */
void nonMaximumSuppression(...args) {
    // [Implement non-maximum suppression]
}

/**
 * Applies a double threshold and edge tracking by hysteresis to an edge map.
 * This function identifies strong edges and weak edges and attempts to connect weak edges to strong edges to form continuous lines.
 *
 * @param args ????????????
 */
void applyLinkingAndHysteresis(...args) {
    // [Apply hysteresis]
}


// Our standard is old, but next week GCC 12 is released !!!
int main() {
    GDALAllRegister();  // Register all GDAL drivers

    // Open the GeoTIFF file
    GDALDataset *dataset = (GDALDataset *) GDALOpen("path_to_your_geotiff.tif", GA_ReadOnly);
    if (dataset == nullptr) {
        std::cout << "Failed to open file" << std::endl;
        return -1;
    }

    // Read the raster data from the first band
    GDALRasterBand *band = dataset->GetRasterBand(1);
    int xSize = band->GetXSize();
    int ySize = band->GetYSize();
    cv::Mat image(ySize, xSize, CV_32FC1); // Float for GDAL compatibility
    if (band->RasterIO(GF_Read, 0, 0, xSize, ySize, image.data, xSize, ySize, GDT_Float32, 0, 0) != CE_None) {
        std::cout << "Failed to read raster data" << std::endl;
        return -1;
    }

    // Convert image to 8-bit (needed for processing)
    image.convertTo(image, CV_8UC1, 255.0 / (band->GetMaximum() - band->GetMinimum()), -band->GetMinimum());

    // Applying Gaussian Blur
    cv::Mat blurred(image.size(), image.type());
    GaussianBlur(image, blurred, 5, 1.4);

    // Calculating the gradient magnitude and direction
    cv::Mat magnitude(image.size(), CV_32F), direction(image.size(), CV_32F);
    GradientCalculation(blurred, magnitude, direction);

    // Non-maximum suppression
    cv::Mat nonMaxSupp(image.size(), CV_8UC1);
    NonMaximumSuppression(magnitude, direction, nonMaxSupp);

    // Double threshold and edge tracking by hysteresis
    cv::Mat edges(image.size(), CV_8UC1);
    Hysteresis(nonMaxSupp, edges, 50, 150); // Threshold values can be adjusted

    // Display results
    cv::imshow("Original Image", image);
    cv::imshow("Detected Edges", edges);
    cv::waitKey(0);

    // Clean up
    GDALClose(dataset);

    return 0;
}
```

Each method in the `CannyEdgeDetector` class corresponds to a step in the Canny edge detection process:

- **applyGaussianBlur**: Smooths the image to reduce noise and detail.
- **findIntensityGradient**: Calculates the gradient of the image's intensity to identify regions with high spatial derivatives.
- **applyNonMaximumSuppression**: Thins edges to ensure that the final image has thin lines.
- **applyHysteresis**: Uses two thresholds to detect strong and weak edges, and includes weak edges only if they are connected to strong edges.

## Tasks to Implement

- **Complete the Code**: Ensure all methods are correctly implemented and optimize for performance.
- **Document the Code**: Provide detailed comments and usage instructions within the codebase and create an accompanying markdown documentation.
- **Test (E2E)**: Implement end-to-end testing to validate the functionality of the edge detection under various scenarios and image conditions.

By fulfilling these tasks, "The Luck is on Our Side" aims to not only revive a piece of forgotten technology but to propel it into the future, helping users rediscover and utilize the power of visual data through the lens of advanced edge detection.
