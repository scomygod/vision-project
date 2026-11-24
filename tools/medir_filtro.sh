#!/usr/bin/env bash
set -eu

if [ "$#" -ne 2 ]; then
    echo "Uso: bash tools/medir_filtro.sh <ejecutable benchmark_grabado> <directorio nuevo>" >&2
    exit 1
fi

ejecutable=$1
resultados=$2
mkdir "$resultados"

for resolucion in 480p 1080p; do
    for kernel in 5 31; do
        for repeticion in 1 2 3; do
            orden="cpu tapi"
            if [ "$repeticion" = 2 ]; then
                orden="tapi cpu"
            fi
            for modo in $orden; do
                base="$resultados/filtro-$resolucion-k$kernel-$modo-r$repeticion"
                "$ejecutable" "$modo" "$resolucion" "$kernel" > "$base.csv" 2> "$base.log"
            done
        done
    done
done
