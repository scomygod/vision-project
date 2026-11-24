#pragma once

#include <fstream>
#include <string>

struct Tiempos {
    double captura = 0;
    double cpu = 0;
    double tapi = 0;
    double subida = 0;
    double bajada = 0;
    double presentacion = 0;
};

struct Resumen {
    Tiempos media;
    double segundos = 0;
    double fps = 0;
    double ramMiB = -1;
    double cpuPorcentaje = -1;
    int fotogramas = 0;
};

void acumular(Tiempos& suma, const Tiempos& muestra);
Resumen resumir(const Tiempos& suma, int fotogramas, double segundos);
double memoriaResidenteMiB();
double segundosCPUProceso();
double porcentajeCPU(double inicio, double fin, double segundos);
inline constexpr char rutaCSV[] = "mediciones-v2.csv";
bool abrirCSV(std::ofstream& archivo);
bool guardarCSV(std::ofstream& archivo, const Resumen& resumen,
                int modo, int indice, int ancho, int alto, int niveles,
                int grosor, int tamano, double sigma, int etapa, bool opencl);
