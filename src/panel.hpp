#pragma once

#include <opencv2/core.hpp>
#include <string>

struct EstadoPanel {
    int etapa = 0;
    bool salir = false;
    int modo = 0; // 0: CPU, 1: T-API, 2: Comparacion
    bool grabar = false;
    bool cargaAlta = false;
};

void ratonPanel(int evento, int x, int y, int flags, void* datos);
cv::Mat crearPanel(const cv::Mat& original, const cv::Mat& resultado, const cv::Mat& etapa,
               const EstadoPanel& estado, const std::string& textoCPU,
               const std::string& textoTAPI, const std::string& transferencias,
               const std::string& capturaPresentacion, const std::string& fpsRAM,
               const std::string& parametros, const std::string& estadoMedicion);
