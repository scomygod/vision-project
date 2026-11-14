#include <iostream>
#include <string>
#include <opencv2/imgcodecs.hpp> // Leer imagenes
#include <opencv2/highgui.hpp>   // Ventana para mostrar foto o video
#include <opencv2/videoio.hpp>   // Mostrar video
#include <opencv2/imgproc.hpp> // Filtros y procesamiento de imágenes
#include <sstream>               // Conversion de texto a numeros

using namespace std;
using namespace cv;

int main(int argc, char *argv[]){
    cout << "working!" << "\n"; // Test Inicial

    // Una ruta o --camera con un índice opcional
    if(argc < 2 || argc > 3){
        cerr << "Uso " << argv[0] << " <ruta_imagen>\n"
             << "    " << argv[0] << " --camera [índice]\n";
        return 1;
    }

    string modo = argv[1];
    int indice = 0;

    // leer el indice
    if(argc == 3){
        if(modo != "--camera"){
            cerr << "Error. La ruta de imagen no admite otro argumento\n";
            return 1;
        }
        string texto = argv[2];
        if(texto.empty() || texto.find_first_not_of("0123456789") != string::npos){
            cerr << "Error. El indice debe contener solamente dígitos.\n";
            return 1;
        }

        stringstream entrada(texto);

        if(!(entrada >> indice)){
            cerr << "Error. El indice supera el rango de int.\n";
            return 1;
        }

    }

    if(modo == "--camera"){
        VideoCapture video(indice); // intenta abrir la camara

        if(!video.isOpened()){
            cerr << "Error. No se pudo abrir la camara " << indice << endl;
            return 2;
        }

        namedWindow("Suavizado", WINDOW_AUTOSIZE); // crea la ventana
        createTrackbar("Posicion", "Suavizado", nullptr, 15); // añade el deslizador entre 0 y 15
        setTrackbarPos("Posicion", "Suavizado", 2); // posicion inicial en 2

        Mat frame;
        Mat suavizado;
        bool lectura;

        while(true){
            lectura = video.read(frame);

            if(!lectura || frame.empty()){
                cerr << "Error. No se pudo leer el fotograma.\n";
                return 3;
            }

            // desenfoque gaussiano
            int posicion = getTrackbarPos("Posicion", "Suavizado");
            int tamano = 2 * posicion + 1;
            GaussianBlur(frame, suavizado, Size(tamano,tamano), 0);

            imshow("Camara", frame);
            imshow("Suavizado", suavizado);
            if(waitKey(1) == 27){
                break;
            }
        }

        video.release();
        destroyAllWindows();
        return 0;
    }

    // Carga y visualización desde una ruta.
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
