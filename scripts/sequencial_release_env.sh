#!/bin/bash

echo "⚙️ Initializing CMake..."
cmake --preset=sycl-release &&
cmake --build --preset=sycl-release &&

echo "🐍 Ativando ambiente virtual Python..."

source /opt/venv/bin/activate &&

echo "🚀 Executando testes de tempo..."

python3 -u /app/scripts/benchmark_sequencial.py -D /app/instances/X