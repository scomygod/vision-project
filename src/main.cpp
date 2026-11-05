#include <iostream>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/highgui.hpp>

using namespace std;
using namespace cv;

int main(int argc, char* argv[]){
    cout << "working!" << "\n"; // Test Inicial

    // argc es la cantidad de argumentos, incluido el nombre del programa, en este caso esperamos 2. El programa y una ruta.
    if (argc != 2 ){ // Si es distinto de 2
        cerr << "Uso " << argv[0] << " <ruta_imagen>\n";
    }

    // argv contiene esa información, [0] programa, [1] ruta.
    // Matriz de OpenCV
    Mat imagen = imread(argv[1]);

    // Verificar que la imagen se pueda leer
    if (imagen.empty()){
        cerr << "Error. No se pudo cargar la imagen " << argv[1] << "\n";
        return 1;
    }

    // Mostrar la imagen y espera a que se toque cualquier tecla
    imshow("Imagen bonita", imagen);
    moveWindow("Imagen bonita", 550, 50);
    waitKey(0);

    // 
    cout << "Anchura: " << imagen.rows << "\n";
    cout << "Altura: " << imagen.cols << "\n";
    return 0;
}
