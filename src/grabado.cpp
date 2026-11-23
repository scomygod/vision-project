#include "grabado.hpp"
#include <opencv2/imgproc.hpp>

using namespace cv;

// Adaptado de Unidad 2, ejemplos 5/9 (mediana), 6 (Gauss/Sobel) y 7 (morfología).
// La cuantización y composición de curvas con bordes define nuestra propuesta.
void grabarCPU(const Mat& entrada, Mat& salida, Mat& luz,
               Mat& bandas, Mat& bordes,
               int niveles, int grosor, int tamano, double sigma){
    Mat color, gris, mediana, gx, gy, magnitud, mascara;
    Mat maximo, minimo, curvas, tinta, intermedio;
    entrada.convertTo(color, CV_32F);
    cvtColor(color, gris, COLOR_BGR2GRAY);
    medianBlur(gris, mediana, 5);
    GaussianBlur(mediana, luz, Size(tamano, tamano), sigma, sigma,
                 BORDER_DEFAULT | BORDER_ISOLATED);

    Sobel(luz, gx, CV_32F, 1, 0, 3, 1.0 / 8);
    Sobel(luz, gy, CV_32F, 0, 1, 3, 1.0 / 8);
    absdiff(gx, Scalar(0), gx);
    absdiff(gy, Scalar(0), gy);
    add(gx, gy, magnitud);
    threshold(magnitud, bordes, 20, 255, THRESH_BINARY);
    bordes.convertTo(bordes, CV_8U);

    bandas.create(luz.size(), CV_32F);
    bandas.setTo(Scalar(0));
    for(int j = 1; j < niveles; j++){
        double umbral = 255.0 * j / niveles;
        threshold(luz, mascara, umbral, 1, THRESH_BINARY);
        add(bandas, mascara, bandas);
    }

    // Gradiente morfológico 3x3 mediante máximos y mínimos vecinos.
    // Evita diferencias del kernel morph de esta instalación en macOS.
    copyMakeBorder(bandas, intermedio, 1, 1, 1, 1, BORDER_REPLICATE);
    bandas.copyTo(maximo);
    bandas.copyTo(minimo);
    for(int fila = 0; fila < 3; fila++){
        for(int columna = 0; columna < 3; columna++){
            Mat vecino = intermedio(Rect(columna, fila, bandas.cols, bandas.rows));
            cv::max(maximo, vecino, maximo);
            cv::min(minimo, vecino, minimo);
        }
    }
    subtract(maximo, minimo, curvas);
    threshold(curvas, curvas, 0, 255, THRESH_BINARY);
    curvas.convertTo(curvas, CV_8U);
    bitwise_or(curvas, bordes, tinta);

    // En una máscara binaria, suma vecina > 0 equivale a dilatar.
    int ancho = 2 * grosor + 1;
    boxFilter(tinta, intermedio, CV_32F, Size(ancho, ancho),
              Point(-1, -1), false, BORDER_REPLICATE);
    threshold(intermedio, salida, 0, 255, THRESH_BINARY_INV);
    salida.convertTo(salida, CV_8U);
}

void grabarTAPI(const UMat& entrada, UMat& salida, UMat& luz,
                UMat& bandas, UMat& bordes,
                int niveles, int grosor, int tamano, double sigma){
    UMat color, gris, mediana, gx, gy, magnitud, mascara;
    UMat maximo, minimo, curvas, tinta, intermedio;
    entrada.convertTo(color, CV_32F);
    cvtColor(color, gris, COLOR_BGR2GRAY);
    medianBlur(gris, mediana, 5);
    GaussianBlur(mediana, luz, Size(tamano, tamano), sigma, sigma,
                 BORDER_DEFAULT | BORDER_ISOLATED);

    Sobel(luz, gx, CV_32F, 1, 0, 3, 1.0 / 8);
    Sobel(luz, gy, CV_32F, 0, 1, 3, 1.0 / 8);
    absdiff(gx, Scalar(0), gx);
    absdiff(gy, Scalar(0), gy);
    add(gx, gy, magnitud);
    threshold(magnitud, bordes, 20, 255, THRESH_BINARY);
    bordes.convertTo(bordes, CV_8U);

    bandas.create(luz.size(), CV_32F);
    bandas.setTo(Scalar(0));
    for(int j = 1; j < niveles; j++){
        double umbral = 255.0 * j / niveles;
        threshold(luz, mascara, umbral, 1, THRESH_BINARY);
        add(bandas, mascara, bandas);
    }

    // Gradiente morfológico 3x3 mediante máximos y mínimos vecinos.
    // Evita diferencias del kernel morph de esta instalación en macOS.
    copyMakeBorder(bandas, intermedio, 1, 1, 1, 1, BORDER_REPLICATE);
    bandas.copyTo(maximo);
    bandas.copyTo(minimo);
    for(int fila = 0; fila < 3; fila++){
        for(int columna = 0; columna < 3; columna++){
            UMat vecino = intermedio(Rect(columna, fila, bandas.cols, bandas.rows));
            cv::max(maximo, vecino, maximo);
            cv::min(minimo, vecino, minimo);
        }
    }
    subtract(maximo, minimo, curvas);
    threshold(curvas, curvas, 0, 255, THRESH_BINARY);
    curvas.convertTo(curvas, CV_8U);
    bitwise_or(curvas, bordes, tinta);

    // En una máscara binaria, suma vecina > 0 equivale a dilatar.
    int ancho = 2 * grosor + 1;
    boxFilter(tinta, intermedio, CV_32F, Size(ancho, ancho),
              Point(-1, -1), false, BORDER_REPLICATE);
    threshold(intermedio, salida, 0, 255, THRESH_BINARY_INV);
    salida.convertTo(salida, CV_8U);
}
