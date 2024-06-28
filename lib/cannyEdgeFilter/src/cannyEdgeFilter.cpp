#include "cannyEdgeFilter.hpp"
#include <iostream>
#include <omp.h>

EdgeDetection::EdgeDetection(float lowThreshold, float highThreshold, float sigma)
    : m_lowThreshold(lowThreshold)
    , m_highThreshold(highThreshold)
    , m_sigma(sigma)
{
}

void EdgeDetection::applyGaussianBlur(const std::string& outputImage)
{
    cv::Mat image_tmp(m_originalImage.rows + KERNEL_SIZE - 1,
                      m_originalImage.cols + KERNEL_SIZE - 1,
                      m_originalImage.type());        // Necessary to handle borders
    cv::Mat kernel(KERNEL_SIZE, KERNEL_SIZE, CV_64F); // CV_64F cuz it's storing double
    double kmean = KERNEL_SIZE / 2;                   // To calculate the gaussian function
    double kaccum = 0;                                // Necessary to posteriori normalization
    int border_val = 0;                               // Value filled in the border
    int half_kernel_size = KERNEL_SIZE / 2;           // An offset, respect to kernel center
    double blur_accum = 0;                            // Accumulate the value for the blurred pixel

    // 1. Create temporal image with extra borders
    #pragma omp parallel for collapse(2)
    for (int row = 0; row < half_kernel_size; row++)
    {
        for (int col = 0; col < image_tmp.cols; col++)
        {
            image_tmp.at<uchar>(row, col) = static_cast<uchar>(border_val);
        }
    }

    #pragma omp parallel for collapse(2)
    for (int row = image_tmp.rows - half_kernel_size; row < image_tmp.rows; row++)
    {
        for (int col = 0; col < image_tmp.cols; col++)
        {
            image_tmp.at<uchar>(row, col) = static_cast<uchar>(border_val);
        }
    }

    #pragma omp parallel for collapse(2)
    for (int col = 0; col < half_kernel_size; col++)
    {
        for (int row = 0; row < image_tmp.rows; row++)
        {
            image_tmp.at<uchar>(row, col) = static_cast<uchar>(border_val);
        }
    }

    #pragma omp parallel for collapse(2)
    for (int col = image_tmp.cols - half_kernel_size; col < image_tmp.cols; col++)
    {
        for (int row = 0; row < image_tmp.rows; row++)
        {
            image_tmp.at<uchar>(row, col) = static_cast<uchar>(border_val);
        }
    }

    #pragma omp parallel for collapse(2)
    for (int row = 0; row < m_originalImage.rows; row++)
    {
        for (int col = 0; col < m_originalImage.cols; col++)
        {
            image_tmp.at<uchar>(row + half_kernel_size, col + half_kernel_size) = m_originalImage.at<uchar>(row, col);
        }
    }

    // 2. Create Kernel
    #pragma omp parallel for collapse(2) reduction(+:kaccum)
    for (int x = 0; x < KERNEL_SIZE; ++x)
    {
        for (int y = 0; y < KERNEL_SIZE; ++y)
        {
            kernel.at<double>(x, y) = exp(-0.5 * (pow((x - kmean) / m_sigma, 2.0) + pow((y - kmean) / m_sigma, 2.0))) /
                                      (2 * M_PI * m_sigma * m_sigma);
            kaccum += kernel.at<double>(x, y);
        }
    }
    kernel /= kaccum; // Normalization

    // 3. Apply filter
    auto start_time = std::chrono::steady_clock::now();
    #pragma omp parallel for collapse(2) private(blur_accum)
    for (int imrow = half_kernel_size; imrow < image_tmp.rows - half_kernel_size; imrow++)
    {
        for (int imcol = half_kernel_size; imcol < image_tmp.cols - half_kernel_size; imcol++)
        {
            blur_accum = 0;
            for (int krow = -half_kernel_size; krow <= half_kernel_size; krow++)
            {
                for (int kcol = -half_kernel_size; kcol <= half_kernel_size; kcol++)
                {
                    blur_accum += static_cast<double>(image_tmp.at<uchar>(imrow + krow, imcol + kcol)) *
                                  kernel.at<double>(krow + half_kernel_size, kcol + half_kernel_size);
                }
            }
            m_cannyEdges.at<uint8_t>(imrow - half_kernel_size, imcol - half_kernel_size) =
                static_cast<uint8_t>(blur_accum);
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "[TIMER] apllyFilterLoop: " << duration.count() << " seconds\n";

    m_imageFileOperations->saveImage(outputImage + "gauss.png", m_cannyEdges);
}

void EdgeDetection::sobelOperator(const std::string& outputImage)
{
    int rows = m_originalImage.rows;
    int cols = m_originalImage.cols;

    m_magnitude.create(rows, cols, CV_32F);
    m_direction.create(rows, cols, CV_32F);

    // Sobel kernels for edge detection in x and y directions
    const int8_t sobelX[3][3] = {{-1, 0, 1}, {-2, 0, 2}, {-1, 0, 1}};
    const int8_t sobelY[3][3] = {{1, 2, 1}, {0, 0, 0}, {-1, -2, -1}};

    // Calculate gradients
    cv::Mat gradientX(rows, cols, CV_32F, cv::Scalar(0));
    cv::Mat gradientY(rows, cols, CV_32F, cv::Scalar(0));

    auto start_time = std::chrono::steady_clock::now();
    #pragma omp parallel for collapse(2)
    for (int rowIndex = 1; rowIndex < rows - 1; ++rowIndex)
    {
        for (int colIndex = 1; colIndex < cols - 1; ++colIndex)
        {
            float gx = 0, gy = 0;
            for (int kernelRowIndex = -1; kernelRowIndex <= 1; ++kernelRowIndex)
            {
                for (int kernelColIndex = -1; kernelColIndex <= 1; ++kernelColIndex)
                {
                    // Fetch the pixel value from the preprocessed image (cannyEdges)
                    uchar pixelValue = m_cannyEdges.at<uchar>(rowIndex + kernelRowIndex, colIndex + kernelColIndex);

                    // Apply Sobel X kernel
                    gx += pixelValue * sobelX[kernelRowIndex + 1][kernelColIndex + 1];

                    // Apply Sobel Y kernel
                    gy += pixelValue * sobelY[kernelRowIndex + 1][kernelColIndex + 1];
                }
            }

            // Store computed gradients
            gradientX.at<float>(rowIndex, colIndex) = gx;
            gradientY.at<float>(rowIndex, colIndex) = gy;

            // Calculate magnitude and direction
            m_magnitude.at<float>(rowIndex, colIndex) = std::sqrt(gx * gx + gy * gy);
            m_direction.at<float>(rowIndex, colIndex) = std::atan2(gy, gx);
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "[TIMER] SobelOperator: " << duration.count() << " seconds\n";

    // Set border pixels to zero
    m_magnitude.row(0).setTo(0);
    m_magnitude.row(rows - 1).setTo(0);
    m_magnitude.col(0).setTo(0);
    m_magnitude.col(cols - 1).setTo(0);

    m_direction.row(0).setTo(0);
    m_direction.row(rows - 1).setTo(0);
    m_direction.col(0).setTo(0);
    m_direction.col(cols - 1).setTo(0);

    // Normalize the direction for visualization purposes
    cv::normalize(m_direction, m_direction, 0, 255, cv::NORM_MINMAX);

    // Save the processed image for verification
    m_imageFileOperations->saveImage(outputImage + "sobelDirection.png",
                                     m_direction); // Saving the direction as an image for visualization
    m_imageFileOperations->saveImage(outputImage + "sobelMagnitude.png",
                                     m_magnitude); // Saving the direction as an image for visualization
}

void EdgeDetection::nonMaximumSuppression(const std::string& outputImage)
{
    int rows = m_magnitude.rows;
    int cols = m_magnitude.cols;

    #pragma omp parallel for collapse(2)
    for (int i = 1; i < rows - 1; ++i)
    {
        for (int j = 1; j < cols - 1; ++j)
        {
            float neighbor_q = 255;
            float neighbor_r = 255;

            // Get angle in degrees
            float angle = (m_direction.at<float>(i, j) - 128) * 180.0 / 128;
            if (angle < 0)
            {
                angle += 180;
            }

            // Determine the neighbors to compare based on the gradient direction
            if ((0 <= angle && angle < 22.5) || (157.5 <= angle && angle <= 180))
            {
                neighbor_q = m_magnitude.at<float>(i, j + 1);
                neighbor_r = m_magnitude.at<float>(i, j - 1);
            }
            else if (22.5 <= angle && angle < 67.5)
            {
                neighbor_q = m_magnitude.at<float>(i + 1, j - 1);
                neighbor_r = m_magnitude.at<float>(i - 1, j + 1);
            }
            else if (67.5 <= angle && angle < 112.5)
            {
                neighbor_q = m_magnitude.at<float>(i + 1, j);
                neighbor_r = m_magnitude.at<float>(i - 1, j);
            }
            else if (112.5 <= angle && angle < 157.5)
            {
                neighbor_q = m_magnitude.at<float>(i - 1, j - 1);
                neighbor_r = m_magnitude.at<float>(i + 1, j + 1);
            }

            float central = m_magnitude.at<float>(i, j);
            if (central >= neighbor_q && central >= neighbor_r)
            {
                m_cannyEdges.at<uint8_t>(i, j) = static_cast<uint8_t>(central); // Keep as maximum
            }
            else
            {
                m_cannyEdges.at<uint8_t>(i, j) = 0; // Suppress
            }
        }
    }

    // Handle borders separately
    m_cannyEdges.row(0).setTo(0);
    m_cannyEdges.row(rows - 1).setTo(0);
    m_cannyEdges.col(0).setTo(0);
    m_cannyEdges.col(cols - 1).setTo(0);

    m_imageFileOperations->saveImage(outputImage + "maxsupress.png", m_cannyEdges);
}

void EdgeDetection::checkContours(
    cv::Mat& strongEdges, const cv::Mat& weakEdges, int row, int col, int prevRow, int prevCol)
{
    // Return if the bridge is completed
    if (strongEdges.at<bool>(row, col))
    {
        return;
    }

    // If there is no weak contour clear the pixel
    if (!(strongEdges.at<bool>(row, col) = weakEdges.at<bool>(row, col)))
    {
        m_cannyEdges.at<uint8_t>(row, col) = 0;
        return;
    }

    // Now check all the connected pixels for any weak contours, avoid same pixel and previous pixel
    for (int side_row = row - 1; side_row <= row + 1; ++side_row)
    {
        for (int side_col = col - 1; side_col <= col + 1; ++side_col)
        {
            if ((side_col != prevCol && side_row != prevRow) && (side_col != col && side_row != row))
            {
                checkContours(strongEdges, weakEdges, side_row, side_col, row, col);
            }
        }
    }
}

void EdgeDetection::applyLinkingAndHysteresis(const std::string& outputImage)
{
    const auto& rows = m_cannyEdges.rows;
    const auto& cols = m_cannyEdges.cols;

    // Initialize matrices for strong and weak edges using OpenCV matrices for better performance
    cv::Mat strongEdges = cv::Mat::zeros(rows, cols, CV_32F);
    cv::Mat weakEdges = cv::Mat::zeros(rows, cols, CV_32F);

    // Identify strong and weak edges
    auto start_time = std::chrono::steady_clock::now();
    #pragma omp parallel for collapse(2)
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            float pixelValue = m_cannyEdges.at<uint8_t>(row, col);
            strongEdges.at<bool>(row, col) = (pixelValue >= m_highThreshold);
            weakEdges.at<bool>(row, col) = (pixelValue >= m_lowThreshold);
        }
    }
    auto end_time = std::chrono::steady_clock::now();
    std::chrono::duration<double> duration = end_time - start_time;
    std::cout << "[TIMER] apllyLinkingAndHysteresis: " << duration.count() << " seconds\n";

    // Update edges matrix based on connected strength
    #pragma omp parallel for collapse(2)
    for (int row = 0; row < rows; ++row)
    {
        for (int col = 0; col < cols; ++col)
        {
            if (strongEdges.at<bool>(row, col))
            {
                continue;
            }

            checkContours(strongEdges, weakEdges, row, col, row, col);
        }
    }
    m_imageFileOperations->saveImage(outputImage + "canny.png", m_cannyEdges);
}

void EdgeDetection::cannyEdgeDetection(const std::string& inputImage, const std::string& outputImage)
{
    m_imageFileOperations = std::make_shared<ImageFileOperations>();

    m_originalImage = m_imageFileOperations->loadImage(inputImage);
    if (m_originalImage.empty())
    {
        throw std::runtime_error("Failed to load image: " + inputImage);
    }
    m_cannyEdges = cv::Mat(m_originalImage.size(), m_originalImage.type());
    m_magnitude = cv::Mat(m_originalImage.size(), CV_32F);
    m_direction = cv::Mat(m_originalImage.size(), CV_32F);

    applyGaussianBlur(outputImage);

    sobelOperator(outputImage);

    nonMaximumSuppression(outputImage);

    applyLinkingAndHysteresis(outputImage);
}
