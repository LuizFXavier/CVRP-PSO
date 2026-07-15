import subprocess
import os


base_instance_path = os.path.expanduser("~/Projects/lscad/CVRP-PSO/instances/CMT/")
executable_path = os.path.expanduser("~/Projects/lscad/CVRP-PSO/src_new/build/profiling/app/cvrp-pso")
output_path = os.path.expanduser("~/Projects/lscad/CVRP-PSO/profiling/")

flamegraph_path = os.path.expanduser("~/Desktop/FlameGraph")
stackcollapse_script = os.path.join(flamegraph_path, "stackcollapse-perf.pl")
flamegraph_script = os.path.join(flamegraph_path, "flamegraph.pl")

instances = ["CMT1", "CMT2", "CMT3", "CMT4", "CMT5"]
iterations = ["5000"]
swarm_sizes = ["25", "50"]
elite_sizes = ["1", "5"]

for inst in instances[1:]:
  for iter_count in iterations:
    for swarm in swarm_sizes:
      for elite in elite_sizes:
        
        # Nome base para agrupar todos os arquivos dessa execução
        base_name = f"{inst}_iter{iter_count}_swarm{swarm}_elite{elite}"
        
        # Caminhos dos arquivos que serão gerados
        data_file = os.path.join(output_path, f"perf_{base_name}.data")
        perf_txt_file = os.path.join(output_path, f"out_{base_name}.perf")
        folded_file = os.path.join(output_path, f"out_{base_name}.folded")
        svg_file = os.path.join(output_path, f"flamegraph_{base_name}.svg")

        instance = os.path.join(base_instance_path, f"{inst}.vrp")

        # 1. Executar o Profiling (perf record)
        record_command = [
            "perf", "record", "-g", "-o", data_file,
            "--",
            executable_path,
            "--in", instance,
            "--out", ".", 
            "--iter", iter_count,
            "--swarm", swarm,
            "--elite", elite
        ]

        print(f"[{base_name}] 1. Rodando o PSO e gravando profiling...")
        subprocess.run(record_command, check=True)

        # 2. Extrai o texto do perf (perf script)
        print(f"[{base_name}] 2. Extraindo os dados do perf...")
        with open(perf_txt_file, "w") as f_out:
            subprocess.run(["perf", "script", "-i", data_file], stdout=f_out, check=True)

        # 3. Agrupa as pilhas de chamadas (stack collapse)
        print(f"[{base_name}] 3. Agrupando as chamadas (stackcollapse)...")
        with open(folded_file, "w") as f_out:
            subprocess.run([stackcollapse_script, perf_txt_file], stdout=f_out, check=True)

        # 4. Gera o arquivo SVG (Flame Graph)
        print(f"[{base_name}] 4. Gerando o Flame Graph SVG...")
        with open(svg_file, "w") as f_out:
            subprocess.run([flamegraph_script, folded_file], stdout=f_out, check=True)

        os.remove(perf_txt_file)
        os.remove(folded_file)

        print(f"[{base_name}] Concluído! SVG salvo em: {svg_file}\n")
