#!/bin/bash

echo "⚙️ Initializing CMake..."
cmake --preset=sycl-dev -DACPP_TARGETS=generic &&
cmake --build  --preset=sycl-dev &&
      /app/src/build/sycl-dev/app/cvrp-pso \
      --in ${TEST_INPUT_DATASET} \
      --out /app/output/ \
      --runs ${TEST_NUM_RUNS} \
      --iterations ${TEST_ITERATIONS} \
      --swarm ${TEST_SWARM_SIZE} \
      --elite ${TEST_ELITE_SIZE};


echo "🔭 Watching for changes on /app/src..."

# Loop de monitoramento (inotifywait)
while inotifywait -q -r -e modify,create,delete /app/src --exclude 'nohup.out|output|run.py|build|out'; do
  echo "🔄 Change detected! Recompiling..."
  cmake --build --preset=sycl-dev &&
      /app/src/build/sycl-dev/app/cvrp-pso \
      --in ${TEST_INPUT_DATASET} \
      --out /app/output/ \
      --runs ${TEST_NUM_RUNS} \
      --iterations ${TEST_ITERATIONS} \
      --swarm ${TEST_SWARM_SIZE} \
      --elite ${TEST_ELITE_SIZE};
  echo "✅ Execution finished. Waiting for changes..."
done
