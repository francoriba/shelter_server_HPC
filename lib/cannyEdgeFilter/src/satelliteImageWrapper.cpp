/*
 * LuckyAlgorithmForSatellites - satelliteImageWrapper
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

#include "satelliteImageWrapper.hpp"

// SatelliteImageWrapper methods
SatelliteImageWrapper::SatelliteImageWrapper(const std::string& filename)
{
    GDALAllRegister();
    m_dataset = (GDALDataset*)GDALOpen(filename.c_str(), GA_ReadOnly);
    if (!m_dataset)
    {
        throw std::runtime_error("Failed to open file: " + filename);
    }
}

SatelliteImageWrapper::~SatelliteImageWrapper()
{
    GDALClose(m_dataset);
}

cv::Mat SatelliteImageWrapper::readBand(int bandNumber)
{

    auto band = m_dataset->GetRasterBand(bandNumber);
    int xSize = band->GetXSize();
    int ySize = band->GetYSize();
    cv::Mat image(ySize, xSize, CV_32FC1); // Float for GDAL compatibility
    if (band->RasterIO(GF_Read, 0, 0, xSize, ySize, image.data, xSize, ySize, GDT_Float32, 0, 0) != CE_None)
    {
        throw std::runtime_error("Failed to read raster data from band: " + std::to_string(bandNumber));
    }

    // Convert to 8-bit
    image.convertTo(image, CV_8UC1, 255.0 / (band->GetMaximum() - band->GetMinimum()), -band->GetMinimum());

    return image;
}

bool SatelliteImageWrapper::isValid() const
{
    return m_dataset != nullptr;
}
