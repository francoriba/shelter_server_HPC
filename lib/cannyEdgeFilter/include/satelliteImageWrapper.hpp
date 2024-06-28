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

#ifndef _SATELLITE_IMAGE_WRAPPER_HPP
#define _SATELLITE_IMAGE_WRAPPER_HPP

#include <gdal_priv.h>
#include <iostream>
#include <opencv2/core/core.hpp>
#include <string>

/**
 * @brief The SatelliteImageWrapper class provides methods to read satellite images.
 */
class SatelliteImageWrapper
{
public:
    /**
     * @brief Construct a new Satellite Image Wrapper object
     * @param filename Filename to open
     */
    SatelliteImageWrapper(const std::string& filename);

    /**
     * @brief Destroy the Satellite Image Wrapper object
     */
    ~SatelliteImageWrapper();

    /**
     * @brief Read a band from the image
     * @param bandNumber Band number to read
     */
    cv::Mat readBand(int bandNumber);

    /**
     * @brief Check if the image is valid
     * @return true if the image is valid, false otherwise
     */
    bool isValid() const;

private:
    GDALDataset* m_dataset;
};

#endif /* _SATELLITE_IMAGE_WRAPPER_HPP */
