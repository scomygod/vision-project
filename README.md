# vision-project

Proyecto de Adrian y John en C++17 y OpenCV. La cámara muestra el grabado T-API como textura dinámica de un plano 3D con VTK. Un panel reúne controles, tiempos grandes y miniaturas del original, CPU y una etapa intermedia. Conservamos el modo imagen desde una ruta.

## Compilar

Requiere CMake 3.16, C++17, VTK y OpenCV con core, imgcodecs, highgui, videoio, imgproc y viz. En macOS usamos el OpenCV aislado del entorno vision-project. Se añadió el módulo oficial opencv_contrib/viz 5.0.0, compilado contra ese OpenCV y el VTK 9.7.1 que ya estaba instalado. Su compilación independiente necesitó declaraciones explícitas de iostream. La configuración anterior de OpenCV se conserva como OpenCVConfig.cmake.before-viz en su carpeta de CMake.

```sh
conda activate vision-project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/vision_project --camera
```

CMake conserva OpenCV_DIR en build. En una carpeta de compilación nueva se puede indicar la instalación mediante `-DOpenCV_DIR="$CONDA_PREFIX/opt/opencv-opencl-5.0.0/lib/cmake/opencv5"`.

```sh
./build/vision_project --camera 1
./build/vision_project foto.jpg
```

La cámara predeterminada es 0. El índice debe contener dígitos y caber en un int no negativo. Si macOS solicita acceso, concédelo a la aplicación de terminal.

La captura solicita 640 × 480 para la interacción. La línea CPU muestra la resolución realmente recibida, ya que depende del dispositivo. CPU y T-API reciben el mismo fotograma a esa resolución.

## Controles

| Control | Función |
| --- | --- |
| Niveles | Posición + 2, entre 2 y 16 bandas. Inicialmente 8. |
| Grosor | Radio entre 0 y 2. Vecindad de 1, 3 o 5 píxeles de lado. |
| Kernel | Lado Gaussiano = 2 × posición + 1, entre 1 y 31. Inicialmente 5. |
| Sigma | Sigma = 0.5 + 0.25 × posición. Inicialmente 1.5. |
| Luminosidad, Bandas, Sobel | Botones del lienzo para escoger la etapa. El seleccionado queda resaltado. |
| Salir | Botón del lienzo que cierra ambas ventanas y termina la cámara. |
| Ratón en Grabado 3D | Arrastrar con el botón izquierdo gira la vista; rueda o gesto de desplazamiento acerca y aleja. |
| Escape | Cierra la cámara desde cualquiera de las dos ventanas. En modo imagen se sale con cualquier tecla. |

Los números de los deslizadores son posiciones. La línea Valores muestra niveles, radio, tamaño y sigma reales. Niveles y grosor controlan el dibujo y también modifican trabajo. Kernel aumenta la vecindad procesada. Sigma cambia los pesos, sin garantizar más operaciones. Con tamaño 1 no mezcla vecinos. La mediana 5 × 5 permanece activa. El panel mide 640 × 420 píxeles más los controles y la escena 640 × 560. Las miniaturas conservan su proporción y solo se reducen para presentar, sin cambiar la resolución procesada. La cámara 3D se centra una vez sobre los objetos para que el giro manual use el centro de la escena.

El plano empieza casi frontal y conserva la orientación entre fotogramas. Los botones usan rectángulos y un callback de ratón HighGUI. En macOS, escape_macos.mm registra Escape localmente en Cocoa durante la cámara, porque VTK puede consumir el evento del Panel antes de waitKey. Se comprobó ese caso con eventos sintéticos: sin el registro se perdió Escape; con él se solicitó la salida desde ambas ventanas. También se comprobaron los manejadores de los cuatro botones. La interacción física queda pendiente de prueba.

## Efecto

Aplicamos mediana, Gaussiano, Sobel y operaciones morfológicas. La mediana elimina impulsos; Gaussiano reduce fluctuaciones y fija la escala; Sobel conserva bordes fuertes; la morfología encuentra límites entre bandas.

Nuestra propuesta combina esas técnicas estándar. Si S es el gris suavizado y N los niveles, calculamos Q = suma de [S > 255 j / N], para j de 1 a N−1. Hay curva donde máximo3×3(Q) − mínimo3×3(Q) > 0. Unimos esas curvas con los bordes cuya magnitud Sobel L1, escalada por 1/8, supera 20. Engrosamos la máscara y la invertimos para obtener tinta negra sobre blanco. Las curvas representan luminosidad. Debemos confirmar con el docente la exclusividad y aceptación del diseño propio.

Mat y UMat usan las mismas operaciones y parámetros en float de 32 bits. La morfología OpenCL directa presentó fallback o diferencias visibles en este equipo. Por eso calculamos sus máximos y mínimos con nueve vistas vecinas, y expresamos la dilatación binaria como suma vecina mayor que cero. Ambas formulaciones se contrastaron con la morfología 2D CPU. BORDER_ISOLATED junto con BORDER_DEFAULT evita un mapeo interno en GaussianBlur y conserva el borde de nuestra imagen completa.

## Medición y comprobaciones

La pantalla separa filtrado CPU, T-API sincronizado con ocl::finish(), copia Mat→UMat y copia del resultado UMat→Mat. El filtrado incluye preparación de matrices y operaciones auxiliares. Cámara, etapa de inspección, miniaturas, textos y renderizado VTK quedan fuera. La textura se actualiza desde el Mat descargado; su carga en OpenGL pertenece a presentación. Descartamos 10 fotogramas iniciales y otros 10 tras cambiar parámetros o resolución. CPU y T-API procesan el mismo fotograma en ese orden. Estos tiempos no son FPS independientes. En Apple Silicon las copias usan memoria unificada. Renderizar en VTK no demuestra que los filtros se ejecuten en OpenCL.

Los eventos se atienden también después de capturar y después del par de procesamientos, fuera de los intervalos medidos. Durante el calentamiento se siguen mostrando resultados; solo se descartan muestras de tiempo. Cambiar Etapa no reinicia ese calentamiento.

Un diagnóstico temporal de cámara, con 35 vueltas posteriores a las 10 iniciales, midió una mediana de 133 ms por vuelta a 1920 × 1080. Captura, CPU, T-API y etapa/panel ocuparon aproximadamente 23, 25, 26 y 28 ms. Con captura 640 × 480 y atención intermedia de eventos, la vuelta final quedó en unos 75 ms, con CPU 4.3 ms y T-API 9.1 ms. Son cargas distintas y no acreditan aceleración del algoritmo. En la comprobación de coordenadas, 320 × 280 puntos Cocoa correspondieron a 640 × 560 píxeles VTK y tres posiciones se convirtieron correctamente a escala 2×. El arrastre físico sigue pendiente de evaluación por los estudiantes.

El 23 de noviembre de 2026 compilamos en Release con OpenCV 5.0.0 en macOS arm64. Una traza externa confirmó kernels OpenCL en Apple M4, con estado correcto y sin lecturas ni mapeos de imagen al host dentro del filtrado final. La captura y presentación siguen en CPU. Si no existe contexto OpenCL, la aplicación indica T-API/CPU.

Comparamos 48 casos externos con foto 500 × 352, cuatro imágenes uniformes, rampa, patrón radial y ruido 319 × 241; seis configuraciones cubrieron niveles 2–16, grosor 0–2, tamaños 1–31 y sigma 0.5–5.5. La tolerancia fue 0.001 en luminosidad sobre 255, hasta 0.5 % de píxeles binarios diferentes y un píxel de desplazamiento. Observamos error máximo 0.00012207 en luminosidad y cero diferencias binarias. La entrada se conservó y la morfología CPU de referencia coincidió exactamente.

Muestra inicial sobre la foto, N=8, grosor=1, sigma=1.5, calentamiento de 10 llamadas por configuración y media de 20, sin transferencias ni presentación:

| Tamaño | CPU, ms | T-API, ms |
| --- | --- | --- |
| 5 | 1.81 | 6.69 |
| 15 | 2.16 | 7.00 |
| 31 | 2.06 | 6.30 |

En esta muestra T-API tardó más. Faltan cargas representativas, distribuciones y alternancia de orden para el benchmark final. Adrian confirmó cámara en directo, controles, equivalencia visual y etapas de la versión anterior. La nueva presentación compiló en Release y se probó con 30 actualizaciones de una entrada con movimiento sintético y cambio de pose. Se inspeccionaron capturas del panel y VTK. La interacción real con ratón y Escape en ambas ventanas y la cámara en esta presentación quedan pendientes de comprobar. También faltan consumo de GPU y ejecución en Linux.

Los códigos de salida son 0 al terminar normalmente, 1 por argumentos inválidos o imagen ilegible, 2 por fallo de apertura de cámara y 3 por fallo de lectura.

## Fuentes adaptadas

Material de Vladimir Robles Bykbaev conservado junto al repositorio:

- Unidad 1, ejemplos 2 y 3. Captura, gris, ventanas y trackbars.
- Unidad 2, ejemplos 5/9, 6 y 7. Mediana, Gaussiano/Sobel y morfología. Adaptamos Sobel a float con escala 1/8 y la morfología a las formulaciones equivalentes descritas.
- Fundamentos_Operador_Canny.pdf, páginas 4–7. Suavizado y gradiente.
- Tutorial-OpenCL-Parallel-Computing-OpenCV-v1.0.pdf, páginas 15–19. Mat/UMat, comparación y Viz3d/widgets. Adaptamos la escena para actualizar WImage3D con spinOnce en vez de bloquear la captura con spin.
- Unidad 2, ejemplos 5 y 6. Lienzo de varias imágenes con Rect y copyTo, adaptado al panel de miniaturas y mediciones.
- [Morfología de OpenCV 5.0.0](https://github.com/opencv/opencv/blob/5.0.0/modules/imgproc/src/morph.dispatch.cpp) y [Gaussiano](https://github.com/opencv/opencv/blob/5.0.0/modules/imgproc/src/smooth.dispatch.cpp), para las adaptaciones.

El desarrollo contó con asistencia de Codex. Ambos integrantes deben comprender y sustentar el código y sus fórmulas.

## Pendientes de Parte I

Primero debemos cerrar la prueba manual VTK, probar OpenCL en el teléfono real y confirmar el efecto y los cuatro filtros con el docente. Siguen Android nativo con el mismo procesamiento y vistas intermedias; RAM, recursos, FPS por modo y benchmark final; informe con gráficas, bibliografía y diagrama; vídeo en inglés y ensayo de demostración. Falta acreditar actividad GPU en ambas plataformas. La guía indica 5 puntos para el informe, pero su tabla lleva un encabezado sobre 4, pendiente de aclarar.


## Para trabajar en el equipo de John

En CachyOS o Windows se necesitan CMake, un compilador C++17 y OpenCV con los módulos indicados arriba, incluido viz con VTK. La instalación del Mac no se copia al otro equipo. Para ejecutar los filtros en la GPU NVIDIA, hace falta su controlador con soporte OpenCL; CUDA no sustituye el requisito OpenCL del proyecto. La compilación y ejecución en esos sistemas siguen pendientes de comprobar.

Desde la raíz del repositorio, con el entorno local preparado:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Si CMake no encuentra OpenCV, indicar su instalación local con `-DOpenCV_DIR=<carpeta que contiene OpenCVConfig.cmake>`. Con un generador de una configuración, el ejecutable suele estar en `build/vision_project` en Linux. Con Visual Studio suele estar en `build/Release/vision_project.exe`. Ejecutarlo con `--camera` y comprobar que la terminal identifica la GPU NVIDIA y OpenCL activo. Eso comprueba el contexto; todavía hay que verificar ejecución efectiva y comparar tiempos. `escape_macos.mm` solo se compila en macOS.

Para estudiar el código, empezar por `grabado.hpp`, seguir una llamada de `grabarCPU` en `grabado.cpp` y compararla con `grabarTAPI`. Después revisar en `main.cpp` la captura, los parámetros, las mediciones y la presentación VTK. Leer `escape_macos.mm` al final para entender la corrección específica del Mac. Cada integrante debe explicar la fórmula de bandas, el borde morfológico, Sobel y la diferencia entre tiempo de filtrado y tiempo total.
