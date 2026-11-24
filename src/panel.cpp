#include "panel.hpp"
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <algorithm>

using namespace std;
using namespace cv;

const Rect botonesEtapa[] = {
    Rect(8, 390, 170, 38), Rect(188, 390, 140, 38), Rect(338, 390, 130, 38)
};
const Rect botonSalir(478, 390, 154, 38);
const Rect botonesModo[] = {
    Rect(8, 440, 90, 38), Rect(108, 440, 100, 38), Rect(218, 440, 160, 38)
};
const Rect botonCSV(388, 440, 110, 38);
const Rect botonCarga(508, 440, 124, 38);
const string nombresModo[] = {"CPU", "T-API", "Comparacion"};

void ratonPanel(int evento, int x, int y, int, void* datos){
    if(evento != EVENT_LBUTTONDOWN){
        return;
    }
    EstadoPanel* estado = static_cast<EstadoPanel*>(datos);
    for(int i = 0; i < 3; i++){
        if(botonesEtapa[i].contains(Point(x, y))){
            estado->etapa = i;
            return;
        }
    }
    for(int i = 0; i < 3; i++){
        if(botonesModo[i].contains(Point(x, y))){
            estado->modo = i;
            return;
        }
    }
    if(botonCSV.contains(Point(x, y))){
        estado->grabar = !estado->grabar;
    }
    if(botonCarga.contains(Point(x, y))){
        estado->cargaAlta = !estado->cargaAlta;
    }
    if(botonSalir.contains(Point(x, y))){
        estado->salir = true;
    }
}

void escribir(Mat& imagen, const string& texto, Point posicion, double escala){
    putText(imagen, texto, posicion, FONT_HERSHEY_SIMPLEX,
            escala, Scalar(240, 240, 240), 1, LINE_AA);
}

void miniatura(const Mat& imagen, Mat& panel, Rect zona){
    Mat color, reducida;
    double escala = min(double(zona.width) / imagen.cols,
                        double(zona.height) / imagen.rows);
    resize(imagen, reducida, Size(), escala, escala, INTER_AREA);
    if(reducida.channels() == 1){
        cvtColor(reducida, color, COLOR_GRAY2BGR);
    }else{
        color = reducida;
    }
    int x = zona.x + (zona.width - reducida.cols) / 2;
    int y = zona.y + (zona.height - reducida.rows) / 2;
    color.copyTo(panel(Rect(x, y, color.cols, color.rows)));
}

// Lienzo de inspección adaptado de Unidad 2, ejemplos 5 y 6.
Mat crearPanel(const Mat& original, const Mat& resultado, const Mat& etapa,
               const EstadoPanel& estado, const string& textoCPU,
               const string& textoTAPI, const string& transferencias,
               const string& capturaPresentacion, const string& fpsRAM,
               const string& parametros, const string& estadoMedicion){
    Mat panel(516, 640, CV_8UC3, Scalar(28, 28, 28));
    escribir(panel, textoCPU, Point(12, 34), 0.85);
    escribir(panel, textoTAPI, Point(12, 70), 0.85);
    escribir(panel, transferencias, Point(12, 102), 0.6);
    escribir(panel, capturaPresentacion, Point(12, 131), 0.6);
    escribir(panel, fpsRAM, Point(12, 161), 0.65);
    escribir(panel, parametros, Point(12, 191), 0.6);
    escribir(panel, "Original", Point(12, 224), 0.65);
    escribir(panel, estado.modo == 2 ? "CPU" : "Resultado", Point(224, 224), 0.65);
    const string nombres[] = {"Luminosidad", "Bandas", "Sobel"};
    escribir(panel, nombres[estado.etapa], Point(436, 224), 0.6);
    miniatura(original, panel, Rect(8, 235, 200, 140));
    miniatura(resultado, panel, Rect(220, 235, 200, 140));
    miniatura(etapa, panel, Rect(432, 235, 200, 140));
    for(int i = 0; i < 3; i++){
        Scalar color(65, 65, 65);
        if(i == estado.etapa){
            color = Scalar(150, 95, 35);
        }
        rectangle(panel, botonesEtapa[i], color, FILLED);
        escribir(panel, nombres[i], Point(botonesEtapa[i].x + 12, 416), 0.7);
        color = Scalar(65, 65, 65);
        if(i == estado.modo){
            color = Scalar(150, 95, 35);
        }
        rectangle(panel, botonesModo[i], color, FILLED);
        escribir(panel, nombresModo[i], Point(botonesModo[i].x + 8, 465), 0.6);
    }
    rectangle(panel, botonSalir, Scalar(45, 45, 150), FILLED);
    escribir(panel, "Salir", Point(botonSalir.x + 12, 416), 0.7);
    rectangle(panel, botonCSV, estado.grabar ? Scalar(50, 120, 50) : Scalar(65, 65, 65), FILLED);
    escribir(panel, estado.grabar ? "CSV ON" : "CSV OFF", Point(396, 465), 0.6);
    rectangle(panel, botonCarga, Scalar(65, 65, 65), FILLED);
    escribir(panel, estado.cargaAlta ? "1080p" : "640x480", Point(516, 465), 0.6);
    escribir(panel, estadoMedicion, Point(12, 504), 0.6);
    return panel;
}
