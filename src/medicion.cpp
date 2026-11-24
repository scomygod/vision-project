#include "medicion.hpp"
#include <ctime>
#include <iomanip>

#if defined(__APPLE__) || defined(__linux__)
#include <sys/resource.h>
#endif

#ifdef __APPLE__
#include <mach/mach.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

using namespace std;

// Memoria residente de todo el proceso, no memoria exclusiva del filtro o GPU.
double memoriaResidenteMiB(){
#ifdef __APPLE__
    mach_task_basic_info_data_t info;
    mach_msg_type_number_t cantidad = MACH_TASK_BASIC_INFO_COUNT;
    kern_return_t resultado = task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                                        reinterpret_cast<task_info_t>(&info), &cantidad);
    if(resultado == KERN_SUCCESS){
        return double(info.resident_size) / (1024 * 1024);
    }
#elif defined(__linux__)
    ifstream archivo("/proc/self/statm");
    long total, residentes;
    long bytesPagina = sysconf(_SC_PAGESIZE);
    if(archivo >> total >> residentes && bytesPagina > 0){
        return double(residentes) * bytesPagina / (1024 * 1024);
    }
#endif
    return -1;
}

// Tiempo de usuario y sistema acumulado por todos los hilos del proceso.
double segundosCPUProceso(){
#if defined(__APPLE__) || defined(__linux__)
    rusage uso{};
    if(getrusage(RUSAGE_SELF, &uso) == 0){
        return uso.ru_utime.tv_sec + uso.ru_stime.tv_sec
             + (uso.ru_utime.tv_usec + uso.ru_stime.tv_usec) / 1e6;
    }
#endif
    return -1;
}

double porcentajeCPU(double inicio, double fin, double segundos){
    if(inicio < 0 || fin < inicio || segundos <= 0){
        return -1;
    }
    return 100 * (fin - inicio) / segundos;
}

void acumular(Tiempos& suma, const Tiempos& muestra){
    suma.captura += muestra.captura;
    suma.cpu += muestra.cpu;
    suma.tapi += muestra.tapi;
    suma.subida += muestra.subida;
    suma.bajada += muestra.bajada;
    suma.presentacion += muestra.presentacion;
}

Resumen resumir(const Tiempos& suma, int fotogramas, double segundos){
    Resumen resultado;
    resultado.fotogramas = fotogramas;
    resultado.segundos = segundos;
    resultado.fps = fotogramas / segundos;
    resultado.ramMiB = memoriaResidenteMiB();
    resultado.media.captura = suma.captura / fotogramas;
    resultado.media.cpu = suma.cpu / fotogramas;
    resultado.media.tapi = suma.tapi / fotogramas;
    resultado.media.subida = suma.subida / fotogramas;
    resultado.media.bajada = suma.bajada / fotogramas;
    resultado.media.presentacion = suma.presentacion / fotogramas;
    return resultado;
}

bool abrirCSV(ofstream& archivo){
    archivo.open(rutaCSV, ios::app);
    if(!archivo){
        return false;
    }
    if(archivo.tellp() == 0){
        archivo << "fecha_utc,modo,camara,ancho,alto,niveles,radio,kernel,sigma,etapa,"
                   "contexto_opencl,fotogramas,intervalo_s,captura_ms,cpu_ms,tapi_ms,"
                   "mat_umat_ms,umat_mat_ms,presentacion_ms,fps_aplicacion,rss_mib,cpu_proceso_pct\n";
    }
    return bool(archivo);
}

bool guardarCSV(ofstream& archivo, const Resumen& resumen,
                int modo, int indice, int ancho, int alto, int niveles,
                int grosor, int tamano, double sigma, int etapa, bool opencl){
    const string modos[] = {"CPU", "T-API", "Comparacion"};
    time_t ahora = time(nullptr);
    archivo << put_time(gmtime(&ahora), "%Y-%m-%dT%H:%M:%SZ") << ','
            << modos[modo] << ',' << indice << ',' << ancho << ',' << alto << ','
            << niveles << ',' << grosor << ',' << tamano << ',' << sigma << ','
            << etapa << ',' << (modo != 0 && opencl) << ',' << resumen.fotogramas << ','
            << fixed << setprecision(4) << resumen.segundos << ','
            << resumen.media.captura << ',';
    if(modo != 1){
        archivo << resumen.media.cpu;
    }
    archivo << ',';
    if(modo != 0){
        archivo << resumen.media.tapi;
    }
    archivo << ',';
    if(modo != 0){
        archivo << resumen.media.subida;
    }
    archivo << ',';
    if(modo != 0){
        archivo << resumen.media.bajada;
    }
    archivo << ',' << resumen.media.presentacion << ',' << resumen.fps << ',';
    if(resumen.ramMiB >= 0){
        archivo << resumen.ramMiB;
    }
    archivo << ',';
    if(resumen.cpuPorcentaje >= 0){
        archivo << resumen.cpuPorcentaje;
    }
    archivo << '\n';
    archivo.flush();
    return bool(archivo);
}
