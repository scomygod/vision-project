# vision-project

Proyecto de Adrian y John en C++17 y OpenCV. La cámara muestra el grabado CPU o T-API como textura dinámica de un plano 3D con VTK. El panel permite seleccionar un camino o compararlos, inspeccionar una etapa y registrar tiempos, FPS de la aplicación, RAM residente y uso de CPU del proceso. Conservamos el modo imagen desde una ruta.

## Compilar

Requiere CMake 3.16, C++17, VTK y OpenCV con core, imgcodecs, highgui, videoio, imgproc y viz. En macOS usamos el OpenCV aislado del entorno vision-project. Se añadió el módulo oficial opencv_contrib/viz 5.0.0, compilado contra ese OpenCV y el VTK 9.7.1 que ya estaba instalado. Su compilación independiente necesitó declaraciones explícitas de iostream. La configuración anterior de OpenCV se conserva como OpenCVConfig.cmake.before-viz en su carpeta de CMake.

```sh
conda activate vision-project
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
  -DOpenCV_DIR="$CONDA_PREFIX/opt/opencv-opencl-5.0.0/lib/cmake/opencv5"
cmake --build build
./build/vision_project --camera
```

CMake conserva `OpenCV_DIR` en `build`. Se comprobó OpenCV 5.0.0 con VTK 9.7.1 en Apple M4.

```sh
./build/vision_project --camera 1
./build/vision_project foto.jpg
```

La cámara predeterminada es 0. El índice debe contener dígitos y caber en un int no negativo. Si macOS solicita acceso, concédelo a la aplicación de terminal.

La captura solicita 640 × 480 al iniciar. El botón de resolución alterna esa solicitud con 1920 × 1080; Valores y el CSV muestran la resolución realmente recibida. El dispositivo puede no admitir ambas. En Comparación, CPU y T-API reciben el mismo fotograma y parámetros.

## Controles

| Control | Función |
| --- | --- |
| Niveles | Posición + 2, entre 2 y 16 bandas. Inicialmente 8. |
| Grosor | Radio entre 0 y 2. Vecindad de 1, 3 o 5 píxeles de lado. |
| Kernel | Lado Gaussiano = 2 × posición + 1, entre 1 y 31. Inicialmente 5. |
| Sigma | Sigma = 0.5 + 0.25 × posición. Inicialmente 1.5. |
| Luminosidad, Bandas, Sobel | Botones del lienzo para escoger la etapa. El seleccionado queda resaltado. |
| CPU, T-API, Comparacion | Inicialmente CPU. Cada modo individual ejecuta solo su camino y lo muestra en VTK. Comparación ejecuta ambos, muestra CPU en la miniatura y T-API en VTK y en la etapa. |
| CSV OFF / CSV ON | Activa o detiene el registro en `mediciones-v2.csv`, en el directorio desde el que se ejecuta. Añade filas sin borrar sesiones anteriores. |
| 640x480 / 1080p | Alterna la resolución solicitada. La etiqueta indica la solicitud; Valores muestra la real. |
| Salir | Botón del lienzo que cierra ambas ventanas y termina la cámara. |
| Ratón en Grabado 3D | Arrastrar con el botón izquierdo gira la vista; rueda o gesto de desplazamiento acerca y aleja. |
| Escape | Cierra la cámara desde cualquiera de las dos ventanas. En modo imagen se sale con cualquier tecla. |

Los números de los deslizadores son posiciones. La línea Valores muestra niveles, radio, tamaño y sigma reales. Niveles y grosor controlan el dibujo y también modifican trabajo. Kernel aumenta la vecindad procesada. Sigma cambia los pesos, sin garantizar más operaciones. Con tamaño 1 no mezcla vecinos. La mediana 5 × 5 permanece activa. El panel mide 640 × 516 píxeles más los controles y la escena 640 × 560. Las miniaturas conservan su proporción y solo se reducen para presentar, sin cambiar la resolución procesada. La cámara 3D se centra una vez sobre los objetos para que el giro manual use el centro de la escena.

El plano empieza casi frontal y conserva la orientación entre fotogramas. Los botones usan rectángulos y un callback de ratón HighGUI. En macOS, escape_macos.mm registra Escape localmente en Cocoa durante la cámara, porque VTK puede consumir el evento del Panel antes de waitKey. Se comprobó ese caso con eventos sintéticos: sin el registro se perdió Escape; con él se solicitó la salida desde ambas ventanas. También se comprobaron los manejadores con llamadas sintéticas. La interacción física queda pendiente de prueba.

## Efecto

Aplicamos mediana, Gaussiano, Sobel y operaciones morfológicas. La mediana elimina impulsos; Gaussiano reduce fluctuaciones y fija la escala; Sobel conserva bordes fuertes; la morfología encuentra límites entre bandas.

Nuestra propuesta combina esas técnicas estándar. Si S es el gris suavizado y N los niveles, calculamos Q = suma de [S > 255 j / N], para j de 1 a N−1. Hay curva donde máximo3×3(Q) − mínimo3×3(Q) > 0. Unimos esas curvas con los bordes cuya magnitud Sobel L1, escalada por 1/8, supera 20. Engrosamos la máscara y la invertimos para obtener tinta negra sobre blanco. Las curvas representan luminosidad. Debemos confirmar con el docente la exclusividad y aceptación del diseño propio.

Mat y UMat usan las mismas operaciones y parámetros en float de 32 bits. La morfología OpenCL directa presentó fallback o diferencias visibles en este equipo. Por eso calculamos sus máximos y mínimos con nueve vistas vecinas, y expresamos la dilatación binaria como suma vecina mayor que cero. Ambas formulaciones se contrastaron con la morfología 2D CPU. BORDER_ISOLATED junto con BORDER_DEFAULT evita un mapeo interno en GaussianBlur y conserva el borde de nuestra imagen completa.

## Medición y comprobaciones

La pantalla muestra medias de ventanas de al menos un segundo. Se descartan 10 fotogramas al iniciar y al cambiar modo, parámetros o resolución. Cambiar etapa vacía la ventana estadística, sin recalentar el filtro. Durante el calentamiento siguen funcionando imagen y controles. Al cambiar configuración se reinicia la media de pantalla; las filas ya guardadas se conservan.

- **Filtrado CPU/T-API (ms):** solo la función de grabado, incluidas sus matrices auxiliares. T-API termina con `ocl::finish()` antes de detener el reloj. En Comparación se mide CPU y después T-API sobre la misma entrada; no son ejecuciones independientes.
- **Mat→UMat / UMat→Mat (ms):** copia de entrada y descarga del resultado final, cada una sincronizada. Son límites de llamadas de T-API, no una medición del bus físico. En memoria unificada pueden existir mapeos, y el controlador puede diferir trabajo hasta usar los datos. CPU no ejecuta estas copias y sus celdas CSV quedan vacías.
- **Captura / presentación (ms):** `read` por separado; presentación incluye inspección de etapa (y su descarga), panel, actualización de textura, render VTK y `waitKey`. Los bombeos adicionales de eventos y el registro no entran en esas medias, pero sí en el FPS global. La carga OpenGL de la textura pertenece a presentación.
- **FPS app:** vueltas completas con presentación divididas entre segundos reales de reloj estable, incluyendo captura, filtros, transferencias, eventos y registro. No se calcula como 1000/filtrado ni equivale a frecuencia del monitor. En Comparación se etiqueta FPS conjunto.
- **RSS (MiB):** memoria física residente de todo el proceso, con bibliotecas y mapeos compartidos; 1 MiB = 1 048 576 bytes. No es VRAM, memoria privada del filtro ni RAM total del equipo. Se consulta una vez por ventana, sin promediar muestras de RAM. macOS usa `task_info`; Linux incluye lectura de `/proc/self/statm`, pendiente de validar allí. Windows muestra N/D y deja la celda vacía.

- **CPU proc. (%):** diferencia del tiempo de usuario y sistema de todos los hilos del proceso, en el mismo intervalo de reloj que los FPS. 100 % equivale a un núcleo usado durante todo el intervalo. El proceso puede superar el 100 %. macOS y Linux usan `getrusage`. Si falta una lectura, el panel muestra N/D y la celda CSV queda vacía.

`CSV ON` guarda una fila por intervalo completo en `mediciones-v2.csv`, en el directorio desde donde se lanzó la aplicación. El archivo anterior `mediciones.csv` se conserva. La nueva columna `cpu_proceso_pct` sigue a `rss_mib`. El CSV incluye resolución real, parámetros, etapa (0 luminosidad, 1 bandas, 2 Sobel), cuadros, segundos y contexto OpenCL. Los caminos inactivos dejan sus celdas vacías. `contexto_opencl=1` señala que el contexto está activo.

### Repetir las mediciones

Prueba de filtros en procesos nuevos, con la misma secuencia sintética de ocho cuadros en cada par. Cada archivo contiene 64 muestras luego de 16 llamadas de calentamiento. La salida de diagnóstico identifica OpenCV, OpenCL y su dispositivo. Este ejecutable mide filtrado y copias fuera de la cámara y VTK.

```sh
cmake --build build --config Release --target benchmark_grabado
bash tools/medir_filtro.sh ./build/benchmark_grabado ./build/filtro-mac
```

Para medir FPS con cámara, inicia una instancia nueva por modo y por repetición. Mantén VTK visible, la cámara quieta y los demás parámetros iniciales. Repite con resolución solicitada de 640×480 y 1080p y Kernel en posiciones 2 y 15 (5 y 31); confirma la resolución real en Valores. Selecciona CPU o T-API, activa CSV y espera al menos cinco intervalos completos. Alterna el orden entre repeticiones. Guarda los CSV junto con el modelo, las versiones y el orden. Usa Comparación aparte para revisar visualmente la salida y la equivalencia sobre el mismo cuadro. Anota los milisegundos del filtro además del FPS de la app. Para la RAM compara ejecuciones nuevas, porque al cambiar modos pueden quedar matrices y cachés asignadas.

### Actividad y memoria GPU

En el M4, `sudo powermetrics --samplers gpu_power -i 1000 -n 15` permite tomar muestras del dispositivo si el usuario tiene permiso. Se registran actividad y potencia cuando aparecen. La memoria es unificada. Deja la memoria GPU atribuible al proceso como N/D si no tienes un contador que la separe. El RSS de la app conserva su propia columna.

Con NVIDIA, abre otra terminal en Linux y ejecuta:

```sh
nvidia-smi --query-gpu=timestamp,name,utilization.gpu,memory.used,power.draw --format=csv -l 1
```

Guarda la salida por modo. Son contadores del dispositivo, en %, MiB y W según el campo. Si el hardware admite `nvidia-smi pmon -s um`, guarda también la fila del PID de la app para contrastar uso y memoria por proceso. VTK puede generar actividad gráfica en CPU y T-API. Para atribuir trabajo al filtro OpenCL, usa la prueba de filtrado sin VTK y una traza de kernels separada de los tiempos. Si falta un contador, anota N/D. [Referencia de NVIDIA](https://docs.nvidia.com/deploy/nvidia-smi/index.html)

Pregunta para el docente: ¿qué recursos además de RAM y CPU deben mostrarse en el Panel, y se admite un monitor externo para actividad, potencia o memoria GPU?

### Resultados de esta revisión

En Apple M4 se hicieron tres ejecuciones del filtro por modo y carga, y dos de cámara con VTK. Con entrada fija de 640×480 y K=5, las medianas fueron 2.73 ms CPU y 7.38 ms T-API. A 1920×1080 y K=31 fueron 22.28 ms y 20.77 ms, con 0.57 ms de subida y 0.34 ms de bajada como medianas por separado. En esta última carga la app sostuvo 9.94 FPS en CPU y 9.95 en T-API. La diferencia de filtro en esa carga no se reflejó en el FPS global. Los CSV crudos y el resumen de esta máquina se guardaron localmente en `build/resultados-escritorio/m4/`. Copia ese directorio antes de limpiar `build`.

Una traza aparte, con el ejecutable de filtro a 1080p y K=31, registró 4400 llamadas a `clEnqueueNDRangeKernel` con éxito en T-API y cero en CPU. Al desactivar el runtime, T-API siguió ejecutándose con contexto 0. La traza muestra ejecución en Apple M4 para esa carga. Actividad, potencia y memoria GPU por proceso quedaron N/D en la captura. `powermetrics` solicitó permisos de administrador para muestrear actividad y potencia.

### Evidencia previa del grabado

Una traza externa en Apple M4 observó kernels OpenCL en el grabado T-API. La variante inicial de morfología directa presentó fallback o diferencias, por lo que se sustituyó por máximos y mínimos de vecinos. En 48 casos sintéticos y de imagen, el grabado final no presentó diferencias binarias; el máximo error observado en luminosidad fue 0.00012207 con tolerancia 0.001. La ejecución de la misma ruta en NVIDIA sigue pendiente.

Los códigos de salida son 0 al terminar normalmente, 1 por argumentos inválidos o imagen ilegible, 2 por fallo de apertura de cámara y 3 por fallo de lectura.

## Fuentes adaptadas

Material de Vladimir Robles Bykbaev conservado junto al repositorio:

- Unidad 1, ejemplos 2 y 3. Captura, gris, ventanas y trackbars.
- Unidad 2, ejemplos 5/9, 6 y 7. Mediana, Gaussiano/Sobel y morfología. Adaptamos Sobel a float con escala 1/8 y la morfología a las formulaciones equivalentes descritas.
- Fundamentos_Operador_Canny.pdf, páginas 4–7. Suavizado y gradiente.
- Tutorial-OpenCL-Parallel-Computing-OpenCV-v1.0.pdf, páginas 15–19. Mat/UMat, comparación y Viz3d/widgets. Adaptamos la escena para actualizar WImage3D con spinOnce en vez de bloquear la captura con spin.
- Unidad 2, ejemplo 4: `ofstream` para CSV. Ejemplo 9: `steady_clock` para una semilla aleatoria, no para medir rendimiento. La medición por intervalos y los FPS observados se añadieron en este proyecto.
- Unidad 2, ejemplos 5 y 6. Lienzo de varias imágenes con Rect y copyTo, adaptado al panel de miniaturas y mediciones.
- [Morfología de OpenCV 5.0.0](https://github.com/opencv/opencv/blob/5.0.0/modules/imgproc/src/morph.dispatch.cpp) y [Gaussiano](https://github.com/opencv/opencv/blob/5.0.0/modules/imgproc/src/smooth.dispatch.cpp), para las adaptaciones.

El desarrollo contó con asistencia de Codex. Ambos integrantes deben comprender y sustentar el código y sus fórmulas.

## Traspaso a John en CachyOS

Usa el commit de traspaso que publique Adrian. Compila con CMake 3.16 o posterior, un compilador C++17, OpenCV local y módulos `core`, `imgcodecs`, `highgui`, `videoio`, `imgproc` y `viz`. Si el OpenCV instalado no incluye `viz`, usa una instalación construida con VTK y el módulo `viz` de `opencv_contrib` de la misma versión. Configura HighGUI para una sesión gráfica. Linux usa su controlador NVIDIA y su runtime OpenCL, y puede comprobar el dispositivo con `nvidia-smi` y `clinfo` si están disponibles.

Desde el repositorio de John, tras recibir el commit:

```sh
git fetch origin
git switch -c feat/android origin/feat/camera-gaussian
cmake -S . -B build-linux -DCMAKE_BUILD_TYPE=Release
cmake --build build-linux --config Release --target vision_project benchmark_grabado
./build-linux/vision_project --camera
```

Si CMake no encuentra OpenCV o elige otra instalación, configura en una carpeta de compilación nueva con `-DOpenCV_DIR=/ruta/local/al/directorio/de/OpenCVConfig.cmake`. Si falta `viz`, comprueba el módulo en esa instalación antes de cambiar el proyecto. Si la app indica T-API/CPU, comprueba si `clinfo` enumera un dispositivo GPU, y revisa la versión y el cliente OpenCL de la instalación que está enlazando la app. Reporta por separado errores de CMake, apertura de cámara, ventana VTK y OpenCL.

Aceptación: envía versiones, salida de compilación Release, dispositivo OpenCL, foto de prueba y cámara funcionando, giro/zoom y Escape, y un CSV con filas CPU, T-API y Comparación, RSS y CPU del proceso. Si `foto.jpg` no está en su copia, use una foto local en su lugar. En otra terminal, puedes contrastar RSS con `ps -C vision_project -o pid,rss,args` mientras corre; Linux informa KiB. Para el filtro fijo usa `bash tools/medir_filtro.sh ./build-linux/benchmark_grabado ./build-linux/filtro-john`. CachyOS y NVIDIA quedan por comprobar en su equipo.

- **Adrian** cierra escritorio, verifica la interacción física, corrige la compatibilidad, reúne evidencia de recursos GPU y prepara los datos de escritorio del informe.
- **John** cierra Android nativo. Reutiliza `src/grabado.cpp` y `src/grabado.hpp` con OpenCV compilado para Android en un proyecto JNI propio. Primero confirma la ABI del Tecno KI7 con Android 13 y ejecuta dentro del APK un kernel OpenCL mínimo con salida verificable. Después implementa cámara, original/resultado/etapas, controles, equivalencia, FPS, memoria y evidencia GPU móvil. El proyecto Android compila el grabado compartido. La captura, VTK, Cocoa y las métricas de escritorio pertenecen al ejecutable de escritorio.
- **Ambos** acuerdan cambios en la interfaz compartida, confirman con el docente la aceptación del efecto y los cuatro filtros y cierran informe con diagrama, justificación, gráficas y citas, vídeo en inglés y ensayo en vivo. Queda aclarar la escala de evaluación del informe (5 puntos en el texto y 4 en la tabla).


## Orden para estudiar el código

1. `grabado.hpp` y `grabado.cpp` contienen el procesamiento compartido para escritorio y Android. Las dos funciones son explícitas para comparar Mat con UMat sin plantillas.
2. `panel.hpp` y `panel.cpp` reúnen los botones, las miniaturas y el dibujo del panel.
3. `medicion.hpp` y `medicion.cpp` calculan los intervalos, FPS, RSS, CPU del proceso y el CSV.
4. `main.cpp` conecta argumentos, cámara, filtros, mediciones, eventos y escena VTK. `escape_macos.mm` resuelve Escape solo en macOS.
5. `tools/benchmark_grabado.cpp` y `tools/medir_filtro.sh` sirven para repetir mediciones fuera de la app. El benchmark solo se compila cuando se pide su target.

Para Android, John debe reutilizar el grabado, no trasladar el panel HighGUI, VTK ni las funciones de medición de escritorio al APK. Los resultados crudos del M4 siguen en `build/resultados-escritorio/m4/`, que Git ignora. Deben conservarse antes de limpiar esa carpeta.
