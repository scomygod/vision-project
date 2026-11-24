#include "grabado.hpp"
#include <opencv2/core/ocl.hpp>
#include <opencv2/imgproc.hpp>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

using namespace std;
using namespace cv;
using Reloj = chrono::steady_clock;

double milisegundos(Reloj::time_point desde){
    return chrono::duration<double, milli>(Reloj::now() - desde).count();
}

int main(int argc, char* argv[]){
    if(argc != 4){
        cerr << "Uso: benchmark_grabado cpu|tapi 480p|1080p 5|31\n";
        return 1;
    }
    string modo = argv[1], resolucion = argv[2], kernel = argv[3];
    if((modo != "cpu" && modo != "tapi") ||
       (resolucion != "480p" && resolucion != "1080p") ||
       (kernel != "5" && kernel != "31")){
        cerr << "Modo, resolucion o kernel invalido\n";
        return 1;
    }

    bool tapi = modo == "tapi";
    ocl::setUseOpenCL(tapi);
    bool opencl = tapi && ocl::useOpenCL();
    cerr << "OpenCV " << CV_VERSION << ", contexto OpenCL: " << opencl;
    if(opencl){
        const ocl::Device& dispositivo = ocl::Device::getDefault();
        cerr << ", " << dispositivo.name() << ", tipo " << dispositivo.type()
             << ", controlador " << dispositivo.driverVersion();
    }
    cerr << '\n';

    Size tamanoImagen = resolucion == "480p" ? Size(640, 480) : Size(1920, 1080);
    int tamanoKernel = stoi(kernel);
    const int niveles = 8, radio = 0;
    const double sigma = 1.5;
    // Secuencia fija con ruido, zonas oscuras, bandas y bordes en movimiento.
    Mat cuadros[8];
    RNG azar(1987);
    for(int i = 0; i < 8; i++){
        cuadros[i].create(tamanoImagen, CV_8UC3);
        azar.fill(cuadros[i], RNG::UNIFORM, 30, 115);
        rectangle(cuadros[i], Rect(tamanoImagen.width / 8, tamanoImagen.height / 8,
                  tamanoImagen.width / 2, tamanoImagen.height / 2), Scalar(25, 80, 165), FILLED);
        circle(cuadros[i], Point(tamanoImagen.width / 3 + 7 * i, tamanoImagen.height / 2),
               tamanoImagen.height / 4, Scalar(180, 230, 80), FILLED);
    }

    Mat salida, luz, bandas, bordes;
    UMat entradaU, salidaU, luzU, bandasU, bordesU;
    struct Muestra {
        double cpu = 0, tapi = 0, subida = 0, bajada = 0;
    };
    Muestra medidas[64];
    for(int i = -16; i < 64; i++){
        const Mat& cuadro = cuadros[(i + 16) % 8];
        Muestra muestra;
        auto inicio = Reloj::now();
        if(tapi){
            cuadro.copyTo(entradaU);
            ocl::finish();
            muestra.subida = milisegundos(inicio);
            inicio = Reloj::now();
            grabarTAPI(entradaU, salidaU, luzU, bandasU, bordesU,
                       niveles, radio, tamanoKernel, sigma);
            ocl::finish();
            muestra.tapi = milisegundos(inicio);
            inicio = Reloj::now();
            salidaU.copyTo(salida);
            ocl::finish();
            muestra.bajada = milisegundos(inicio);
        }else{
            grabarCPU(cuadro, salida, luz, bandas, bordes,
                      niveles, radio, tamanoKernel, sigma);
            muestra.cpu = milisegundos(inicio);
        }
        if(i >= 0){
            medidas[i] = muestra;
        }
    }

    cout << "modo,contexto_opencl,ancho,alto,kernel,niveles,radio,sigma,muestra,cuadro,"
            "filtro_ms,mat_umat_ms,umat_mat_ms\n" << fixed << setprecision(6);
    for(int i = 0; i < 64; i++){
        cout << modo << ',' << opencl << ',' << tamanoImagen.width << ','
             << tamanoImagen.height << ',' << tamanoKernel << ',' << niveles << ','
             << radio << ',' << sigma << ',' << i << ',' << i % 8 << ',';
        if(tapi){
            cout << medidas[i].tapi << ',' << medidas[i].subida << ',' << medidas[i].bajada;
        }else{
            cout << medidas[i].cpu << ",,";
        }
        cout << '\n';
    }
}
