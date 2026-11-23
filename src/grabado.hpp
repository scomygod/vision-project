#pragma once

#include <opencv2/core.hpp>

// Mismas operaciones y parámetros. Las salidas de etapas permiten inspeccionarlas.
void grabarCPU(const cv::Mat& entrada, cv::Mat& salida, cv::Mat& luz,
               cv::Mat& bandas, cv::Mat& bordes,
               int niveles, int grosor, int tamano, double sigma);

void grabarTAPI(const cv::UMat& entrada, cv::UMat& salida, cv::UMat& luz,
                cv::UMat& bandas, cv::UMat& bordes,
                int niveles, int grosor, int tamano, double sigma);
