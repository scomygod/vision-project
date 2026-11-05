# Reglas de trabajo — vision-project

- Utilizar C++17 y CMake. Priorizar funciones sencillas; añadir clases o abstracciones solo cuando resuelvan una necesidad concreta.
- Implementar únicamente la etapa solicitada. No anticipar módulos, carpetas ni funcionalidades futuras.
- Mantener compatibilidad con macOS y Linux y evitar obstáculos innecesarios para Windows. No incorporar rutas personales ni opciones exclusivas de un compilador sin justificación.
- OpenCL es un requisito académico. No sustituir su API ni sus kernels por CUDA o Metal. Se permiten los controladores y componentes del fabricante necesarios para ejecutar OpenCL, incluidos los de NVIDIA. No asumir que OpenCL funciona en Apple Silicon; distinguir disponibilidad, compilación y ejecución comprobada.
- Añadir dependencias solo cuando la etapa las necesite y explicar su propósito. No instalar bibliotecas ni modificar configuraciones globales sin autorización.
- Antes de editar, revisar el código y las instrucciones existentes. Conservar los cambios de los estudiantes y evitar reescrituras ajenas al objetivo.
- Explicar cada modificación: qué hace, por qué se necesita, su sintaxis relevante, qué ocurre al eliminarla o modificarla y cómo comprobarla. No repetir conceptos ya comprendidos.
- Documentar las comprobaciones realizadas y sus límites. No afirmar compatibilidad o aceleración GPU sin evidencia.
