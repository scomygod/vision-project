#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/viz.hpp>
#include "grabado.hpp"

using namespace std;
using namespace cv;

struct EstadoPanel {
    int etapa = 0;
    bool salir = false;
};

const Rect botonesEtapa[] = {
    Rect(8, 370, 170, 38), Rect(188, 370, 140, 38), Rect(338, 370, 130, 38)
};
const Rect botonSalir(478, 370, 154, 38);

#ifdef __APPLE__
void observarEscape(bool* salir);
void dejarDeObservarEscape();
#endif

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
    if(imagen.channels() == 1){
        cvtColor(imagen, color, COLOR_GRAY2BGR);
    }else{
        color = imagen;
    }
    double escala = min(double(zona.width) / color.cols,
                        double(zona.height) / color.rows);
    resize(color, reducida, Size(), escala, escala, INTER_AREA);
    int x = zona.x + (zona.width - reducida.cols) / 2;
    int y = zona.y + (zona.height - reducida.rows) / 2;
    reducida.copyTo(panel(Rect(x, y, reducida.cols, reducida.rows)));
}

// Lienzo de inspección adaptado de Unidad 2, ejemplos 5 y 6.
Mat crearPanel(const Mat& original, const Mat& cpu, const Mat& etapa,
               int etapaActual, const string& textoCPU,
               const string& textoTAPI, const string& transferencias,
               const string& parametros){
    Mat panel(420, 640, CV_8UC3, Scalar(28, 28, 28));
    escribir(panel, textoCPU, Point(12, 40), 1.0);
    escribir(panel, textoTAPI, Point(12, 82), 1.0);
    escribir(panel, transferencias, Point(12, 119), 0.65);
    escribir(panel, parametros, Point(12, 152), 0.65);
    escribir(panel, "Original", Point(12, 195), 0.65);
    escribir(panel, "CPU", Point(224, 195), 0.65);
    const string nombres[] = {"Luminosidad", "Bandas", "Sobel"};
    escribir(panel, nombres[etapaActual], Point(436, 195), 0.6);
    miniatura(original, panel, Rect(8, 210, 200, 150));
    miniatura(cpu, panel, Rect(220, 210, 200, 150));
    miniatura(etapa, panel, Rect(432, 210, 200, 150));
    for(int i = 0; i < 3; i++){
        Scalar color(65, 65, 65);
        if(i == etapaActual){
            color = Scalar(150, 95, 35);
        }
        rectangle(panel, botonesEtapa[i], color, FILLED);
        escribir(panel, nombres[i], Point(botonesEtapa[i].x + 12, 396), 0.7);
    }
    rectangle(panel, botonSalir, Scalar(45, 45, 150), FILLED);
    escribir(panel, "Salir", Point(botonSalir.x + 12, 396), 0.7);
    return panel;
}

// VTK entrega las teclas a esta función cuando su ventana tiene el foco.
void tecladoVTK(const viz::KeyboardEvent& evento, void* datos){
    if(evento.action == viz::KeyboardEvent::KEY_DOWN && evento.code == 27){
        bool* salir = static_cast<bool*>(datos);
        *salir = true;
    }
}

int mostrarCamara(int indice){
    VideoCapture video(indice);
    if(!video.isOpened()){
        cerr << "Error. No se pudo abrir la camara " << indice << endl;
        return 2;
    }

    // Resolución interactiva; CPU y T-API procesan el mismo fotograma.
    video.set(CAP_PROP_FRAME_WIDTH, 640);
    video.set(CAP_PROP_FRAME_HEIGHT, 480);

    ocl::setUseOpenCL(true);
    bool opencl = ocl::useOpenCL();
    cout << "OpenCV " << CV_VERSION << ". OpenCL disponible: "
         << ocl::haveOpenCL() << ". Activo: " << opencl << endl;
    if(opencl){
        const ocl::Device& dispositivo = ocl::Device::getDefault();
        cout << "Dispositivo: " << dispositivo.name()
             << ". Tipo: " << dispositivo.type() << endl;
    }else{
        cout << "T-API usara CPU. No hay contexto OpenCL activo." << endl;
    }

    EstadoPanel estado;
    // Controles adaptados de Unidad 1, ejemplo 3, y Unidad 2, ejemplo 7.
    namedWindow("Panel", WINDOW_AUTOSIZE);
    moveWindow("Panel", 10, 40);
    setMouseCallback("Panel", ratonPanel, &estado);
    createTrackbar("Niveles", "Panel", nullptr, 14);
    createTrackbar("Grosor", "Panel", nullptr, 2);
    createTrackbar("Kernel", "Panel", nullptr, 15);
    createTrackbar("Sigma", "Panel", nullptr, 20);
    setTrackbarPos("Niveles", "Panel", 6);
    setTrackbarPos("Kernel", "Panel", 2);
    setTrackbarPos("Sigma", "Panel", 4);

    // Viz3d y widgets adaptados del tutorial del profesor, páginas 18 y 19.
    viz::Viz3d escena("Grabado 3D");
    escena.setWindowSize(Size(640, 560));
    escena.setWindowPosition(Point(670, 40));
    escena.setBackgroundColor(viz::Color(35, 35, 35));
    escena.showWidget("Ejes", viz::WCoordinateSystem(0.3),
                      Affine3d(Vec3d(0, 0, 0), Vec3d(-1.0, -0.75, 0)));
    Mat texturaInicial(1, 1, CV_8UC1, Scalar(255));
    viz::WImage3D plano(texturaInicial, Size2d(2, 1.5));
    Size resolucionPlano;
    escena.registerKeyboardCallback(tecladoVTK, &estado.salir);
#ifdef __APPLE__
    observarEscape(&estado.salir);
#endif

    Mat frame, resultadoCPU, luzCPU, bandasCPU, bordesCPU, resultadoTAPI, etapa;
    UMat entradaU, resultadoU, luzU, bandasU, bordesU, etapaU;
    int nivelesAnterior = -1, grosorAnterior = -1;
    int tamanoAnterior = -1, sigmaAnterior = -1;
    Size resolucionAnterior;
    int calentamiento = 10;

    while(true){
        if(!video.read(frame) || frame.empty()){
            cerr << "Error. No se pudo leer el fotograma." << endl;
#ifdef __APPLE__
            dejarDeObservarEscape();
#endif
            return 3;
        }

        escena.spinOnce(1, false);
        if(estado.salir || escena.wasStopped()){
            break;
        }

        int niveles = getTrackbarPos("Niveles", "Panel") + 2;
        int grosor = getTrackbarPos("Grosor", "Panel");
        int tamano = 2 * getTrackbarPos("Kernel", "Panel") + 1;
        int posicionSigma = getTrackbarPos("Sigma", "Panel");
        double sigma = 0.5 + 0.25 * posicionSigma;
        int etapaActual = estado.etapa;

        if(niveles != nivelesAnterior || grosor != grosorAnterior ||
           tamano != tamanoAnterior || posicionSigma != sigmaAnterior ||
           frame.size() != resolucionAnterior){
            calentamiento = 10;
            nivelesAnterior = niveles;
            grosorAnterior = grosor;
            tamanoAnterior = tamano;
            sigmaAnterior = posicionSigma;
            resolucionAnterior = frame.size();
        }

        ocl::finish();
        auto inicio = chrono::steady_clock::now();
        frame.copyTo(entradaU);
        ocl::finish();
        auto subidaFin = chrono::steady_clock::now();

        grabarCPU(frame, resultadoCPU, luzCPU, bandasCPU, bordesCPU,
                  niveles, grosor, tamano, sigma);
        auto cpuFin = chrono::steady_clock::now();

        grabarTAPI(entradaU, resultadoU, luzU, bandasU, bordesU,
                   niveles, grosor, tamano, sigma);
        ocl::finish();
        auto tapiFin = chrono::steady_clock::now();

        resultadoU.copyTo(resultadoTAPI);
        ocl::finish();
        auto bajadaFin = chrono::steady_clock::now();

        double subida = chrono::duration<double, milli>(subidaFin - inicio).count();
        double cpu = chrono::duration<double, milli>(cpuFin - subidaFin).count();
        double tapi = chrono::duration<double, milli>(tapiFin - cpuFin).count();
        double bajada = chrono::duration<double, milli>(bajadaFin - tapiFin).count();

        // Atender interacción fuera de los intervalos de CPU y T-API.
        escena.spinOnce(1, false);
        if(estado.salir || escena.wasStopped()){
            break;
        }

        // La inspección de etapas y los textos quedan fuera de los intervalos.
        if(etapaActual == 0){
            luzU.convertTo(etapaU, CV_8U);
        }else if(etapaActual == 1){
            bandasU.convertTo(etapaU, CV_8U, 255.0 / (niveles - 1));
        }else{
            bordesU.copyTo(etapaU);
        }
        etapaU.copyTo(etapa);

        stringstream textoCPU, textoTAPI, transferencias, parametros;
        textoCPU << fixed << setprecision(2);
        textoCPU << "CPU " << frame.cols << "x" << frame.rows << ": ";
        textoTAPI << fixed << setprecision(2);
        transferencias << fixed << setprecision(2);
        if(calentamiento > 0){
            textoCPU << "calentando";
            textoTAPI << "T-API: calentando";
            transferencias << "Transferencias: calentando";
            calentamiento--;
        }else{
            textoCPU << cpu << " ms";
            textoTAPI << "T-API";
            if(opencl){
                textoTAPI << "/OpenCL";
            }else{
                textoTAPI << "/CPU";
            }
            textoTAPI << ": " << tapi << " ms";
            transferencias << "H->U: " << subida << " ms  U->H: " << bajada << " ms";
        }
        parametros << "Valores: N=" << niveles << " radio=" << grosor
                   << " k=" << tamano << "x" << tamano << " sigma=" << sigma;
        Mat panel = crearPanel(frame, resultadoCPU, etapa, etapaActual,
                               textoCPU.str(), textoTAPI.str(),
                               transferencias.str(), parametros.str());
        imshow("Panel", panel);

        if(frame.size() != resolucionPlano){
            double alto = 2.0 * frame.rows / frame.cols;
            plano = viz::WImage3D(resultadoTAPI, Size2d(2, alto));
            escena.showWidget("Grabado", plano,
                              Affine3d(Vec3d(0, -0.04, 0), Vec3d(0, 0, 0)));
            escena.setViewerPose(viz::makeCameraPose(
                Vec3d(0, 0, -4), Vec3d(0, 0, 0), Vec3d(0, 1, 0)));
            // Centrar el punto de giro en los objetos, solo al crear la escena.
            escena.resetCamera();
            resolucionPlano = frame.size();
        }else{
            plano.setImage(resultadoTAPI);
        }
        escena.spinOnce(1, true);

        int tecla = waitKey(1);
        if(tecla == 27 || estado.salir || escena.wasStopped()){
            break;
        }
    }

#ifdef __APPLE__
    dejarDeObservarEscape();
#endif
    video.release();
    escena.close();
    destroyAllWindows();
    return 0;
}

int main(int argc, char *argv[]){
    if(argc < 2 || argc > 3){
        cerr << "Uso " << argv[0] << " <ruta_imagen>\n"
             << "    " << argv[0] << " --camera [indice]\n";
        return 1;
    }

    string modo = argv[1];
    int indice = 0;
    if(argc == 3){
        if(modo != "--camera"){
            cerr << "Error. La ruta de imagen no admite otro argumento\n";
            return 1;
        }
        string texto = argv[2];
        if(texto.empty() || texto.find_first_not_of("0123456789") != string::npos){
            cerr << "Error. El indice debe contener solamente digitos.\n";
            return 1;
        }
        stringstream entrada(texto);
        if(!(entrada >> indice)){
            cerr << "Error. El indice supera el rango de int.\n";
            return 1;
        }
    }

    if(modo == "--camera"){
        return mostrarCamara(indice);
    }

    Mat imagen = imread(argv[1]);
    if(imagen.empty()){
        cerr << "Error. No se pudo cargar la imagen " << argv[1] << endl;
        return 1;
    }
    imshow("Imagen bonita", imagen);
    moveWindow("Imagen bonita", 550, 50);
    waitKey(0);
    cout << "Altura: " << imagen.rows << endl;
    cout << "Anchura: " << imagen.cols << endl;
    destroyAllWindows();
    return 0;
}
