# vision-project

Proyecto de visión por computador en C++17 con OpenCV. Actualmente permite abrir una imagen desde la terminal y mostrarla en una ventana.

## Compilar

Se requiere CMake 3.16 o posterior, un compilador compatible con C++17 y OpenCV con los módulos core, imgcodecs y highgui.

Si tienes preparado el entorno Conda del proyecto, actívalo con `conda activate vision-project`. Desde la raíz del repositorio:

```sh
cmake -S . -B build
cmake --build build
```

## Mostrar una imagen

```sh
./build/vision_project "/ruta/a/foto.png"
```

Pulsa una tecla con la ventana enfocada para salir.
