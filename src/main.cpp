#include <iostream>
#include <string>
#include <sstream>
#include <chrono>
#include <iomanip>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>
#include <opencv2/videoio.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/core/ocl.hpp>
#include <opencv2/viz.hpp>
#include "grabado.hpp"
#include "medicion.hpp"
#include "panel.hpp"

using namespace std;
using namespace cv;

#ifdef __APPLE__
void observarEscape(bool* salir);
void dejarDeObservarEscape();
#endif

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
    int modoAnterior = -1, etapaAnterior = -1;
    bool cargaAnterior = false, grabando = false;
    ofstream archivoCSV;
    Tiempos suma;
    Resumen resumen;
    int fotogramas = 0;
    auto inicioVentana = chrono::steady_clock::now();
    double cpuInicioVentana = -1;
    string mensajeCSV;
    int codigoSalida = 0;

    while(true){
        if(estado.cargaAlta != cargaAnterior){
            int ancho = estado.cargaAlta ? 1920 : 640;
            int alto = estado.cargaAlta ? 1080 : 480;
            video.set(CAP_PROP_FRAME_WIDTH, ancho);
            video.set(CAP_PROP_FRAME_HEIGHT, alto);
            cargaAnterior = estado.cargaAlta;
            calentamiento = 10;
        }
        bool reiniciarVentana = false;
        Tiempos muestra;
        auto capturaInicio = chrono::steady_clock::now();
        if(!video.read(frame) || frame.empty()){
            cerr << "Error. No se pudo leer el fotograma." << endl;
            codigoSalida = 3;
            break;
        }

        auto capturaFin = chrono::steady_clock::now();
        muestra.captura = chrono::duration<double, milli>(capturaFin - capturaInicio).count();
        escena.spinOnce(1, false);
        if(estado.salir || escena.wasStopped()){
            break;
        }

        int niveles = getTrackbarPos("Niveles", "Panel") + 2;
        int grosor = getTrackbarPos("Grosor", "Panel");
        int tamano = 2 * getTrackbarPos("Kernel", "Panel") + 1;
        int posicionSigma = getTrackbarPos("Sigma", "Panel");
        double sigma = 0.5 + 0.25 * posicionSigma;
        // Mantener modo, parámetros y etiquetas coherentes durante esta vuelta.
        EstadoPanel seleccion = estado;
        int etapaActual = seleccion.etapa;
        int modoActual = seleccion.modo;
        if(estado.grabar != grabando){
            if(estado.grabar){
                mensajeCSV.clear();
                if(!abrirCSV(archivoCSV)){
                    cerr << "Error. No se pudo abrir " << rutaCSV << endl;
                    mensajeCSV = "Error al abrir CSV";
                    archivoCSV.close();
                    archivoCSV.clear();
                    estado.grabar = false;
                }
            }else{
                archivoCSV.close();
            }
            reiniciarVentana = true;
            grabando = estado.grabar;
            seleccion.grabar = grabando;
        }

        if(niveles != nivelesAnterior || grosor != grosorAnterior ||
           tamano != tamanoAnterior || posicionSigma != sigmaAnterior ||
           frame.size() != resolucionAnterior || modoActual != modoAnterior){
            calentamiento = 10;
            nivelesAnterior = niveles;
            grosorAnterior = grosor;
            tamanoAnterior = tamano;
            sigmaAnterior = posicionSigma;
            resolucionAnterior = frame.size();
            modoAnterior = modoActual;
        }

        if(reiniciarVentana || calentamiento > 0 || etapaActual != etapaAnterior){
            reiniciarVentana = true;
            suma = Tiempos();
            resumen = Resumen();
            fotogramas = 0;
            etapaAnterior = etapaActual;
        }

        if(modoActual != 0){
            ocl::finish();
            auto inicio = chrono::steady_clock::now();
            frame.copyTo(entradaU);
            ocl::finish();
            muestra.subida = chrono::duration<double, milli>(chrono::steady_clock::now() - inicio).count();
        }
        if(modoActual != 1){
            auto inicio = chrono::steady_clock::now();
            grabarCPU(frame, resultadoCPU, luzCPU, bandasCPU, bordesCPU,
                      niveles, grosor, tamano, sigma);
            muestra.cpu = chrono::duration<double, milli>(chrono::steady_clock::now() - inicio).count();
        }
        if(modoActual != 0){
            auto inicio = chrono::steady_clock::now();
            grabarTAPI(entradaU, resultadoU, luzU, bandasU, bordesU,
                       niveles, grosor, tamano, sigma);
            ocl::finish();
            muestra.tapi = chrono::duration<double, milli>(chrono::steady_clock::now() - inicio).count();

            inicio = chrono::steady_clock::now();
            resultadoU.copyTo(resultadoTAPI);
            ocl::finish();
            muestra.bajada = chrono::duration<double, milli>(chrono::steady_clock::now() - inicio).count();
        }

        // Atender interacción fuera de los intervalos de CPU y T-API.
        escena.spinOnce(1, false);
        if(estado.salir || escena.wasStopped()){
            break;
        }

        // Inspección, dibujo y renderizado fuera de los tiempos de filtrado.
        auto presentacionInicio = chrono::steady_clock::now();
        if(modoActual == 0){
            if(etapaActual == 0){
                luzCPU.convertTo(etapa, CV_8U);
            }else if(etapaActual == 1){
                bandasCPU.convertTo(etapa, CV_8U, 255.0 / (niveles - 1));
            }else{
                etapa = bordesCPU;
            }
        }else{
            if(etapaActual == 0){
                luzU.convertTo(etapaU, CV_8U);
            }else if(etapaActual == 1){
                bandasU.convertTo(etapaU, CV_8U, 255.0 / (niveles - 1));
            }else{
                bordesU.copyTo(etapaU);
            }
            etapaU.copyTo(etapa);
            ocl::finish();
        }

        stringstream textoCPU, textoTAPI, transferencias, capturaPresentacion, fpsRAM, parametros;
        textoCPU << fixed << setprecision(2) << "CPU: ";
        textoTAPI << fixed << setprecision(2) << (opencl ? "T-API/OpenCL: " : "T-API/CPU: ");
        transferencias << fixed << setprecision(2);
        capturaPresentacion << fixed << setprecision(2);
        fpsRAM << fixed << setprecision(1);
        textoCPU << (modoActual == 1 ? "inactivo" : "");
        textoTAPI << (modoActual == 0 ? "inactivo" : "");
        string estadoMedicion = calentamiento > 0 ? "Calentando (10 cuadros excluidos)" : "Midiendo (media >= 1 s)";
        if(resumen.fotogramas > 0){
            if(modoActual != 1){
                textoCPU << resumen.media.cpu << " ms";
            }
            if(modoActual != 0){
                textoTAPI << resumen.media.tapi << " ms";
                transferencias << "Mat->U: " << resumen.media.subida
                               << " ms  U->Mat: " << resumen.media.bajada << " ms";
            }else{
                transferencias << "Transferencias: no se ejecutan";
            }
            capturaPresentacion << "Captura: " << resumen.media.captura
                                << " ms  Present.: " << resumen.media.presentacion << " ms";
            fpsRAM << "FPS app" << (modoActual == 2 ? " conjunto" : "") << ": " << resumen.fps << "  RSS: ";
            if(resumen.ramMiB >= 0){
                fpsRAM << resumen.ramMiB << " MiB";
            }else{
                fpsRAM << "N/D";
            }
            fpsRAM << "  CPU proc.: ";
            if(resumen.cpuPorcentaje >= 0){
                fpsRAM << resumen.cpuPorcentaje << "%";
            }else{
                fpsRAM << "N/D";
            }
        }else{
            if(modoActual != 1){
                textoCPU << "esperando media";
            }
            if(modoActual != 0){
                textoTAPI << "esperando media";
            }
        }
        parametros << "Valores: " << frame.cols << "x" << frame.rows << " N=" << niveles
                   << " r=" << grosor << " k=" << tamano << " sigma=" << sigma;
        if(grabando){
            estadoMedicion += string(" | CSV: ") + rutaCSV;
        }else if(!mensajeCSV.empty()){
            estadoMedicion = mensajeCSV;
        }
        const Mat& resultado = modoActual == 0 ? resultadoCPU : resultadoTAPI;
        const Mat& miniaturaResultado = modoActual == 2 ? resultadoCPU : resultado;
        Mat panel = crearPanel(frame, miniaturaResultado, etapa, seleccion,
                               textoCPU.str(), textoTAPI.str(), transferencias.str(),
                               capturaPresentacion.str(), fpsRAM.str(), parametros.str(), estadoMedicion);
        imshow("Panel", panel);

        if(frame.size() != resolucionPlano){
            double alto = 2.0 * frame.rows / frame.cols;
            plano = viz::WImage3D(resultado, Size2d(2, alto));
            escena.showWidget("Grabado", plano,
                              Affine3d(Vec3d(0, -0.04, 0), Vec3d(0, 0, 0)));
            escena.setViewerPose(viz::makeCameraPose(
                Vec3d(0, 0, -4), Vec3d(0, 0, 0), Vec3d(0, 1, 0)));
            // Centrar el punto de giro en los objetos, solo al crear la escena.
            escena.resetCamera();
            resolucionPlano = frame.size();
        }else{
            plano.setImage(resultado);
        }
        escena.spinOnce(1, true);

        int tecla = waitKey(1);
        auto finVuelta = chrono::steady_clock::now();
        muestra.presentacion = chrono::duration<double, milli>(finVuelta - presentacionInicio).count();
        if(calentamiento > 0 || reiniciarVentana){
            if(calentamiento > 0){
                calentamiento--;
            }
            inicioVentana = finVuelta;
            if(calentamiento == 0){
                cpuInicioVentana = segundosCPUProceso();
            }
        }else{
            acumular(suma, muestra);
            fotogramas++;
            double segundos = chrono::duration<double>(finVuelta - inicioVentana).count();
            if(segundos >= 1.0){
                double cpuFinVentana = segundosCPUProceso();
                resumen = resumir(suma, fotogramas, segundos);
                resumen.cpuPorcentaje = porcentajeCPU(cpuInicioVentana, cpuFinVentana, segundos);
                if(grabando && !guardarCSV(archivoCSV, resumen, modoActual, indice,
                    frame.cols, frame.rows, niveles, grosor, tamano, sigma, etapaActual, opencl)){
                    cerr << "Error. No se pudo escribir " << rutaCSV << endl;
                    mensajeCSV = "Error al escribir CSV";
                    estado.grabar = false;
                }
                suma = Tiempos();
                fotogramas = 0;
                inicioVentana = finVuelta;
                cpuInicioVentana = cpuFinVentana;
            }
        }
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
    return codigoSalida;
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
