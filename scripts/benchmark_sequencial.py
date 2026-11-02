import subprocess
import os
import sys
import pandas as pd
import numpy as np
from openpyxl import load_workbook, Workbook
from leitor_instancia import ler_instancia

# ==============================================================================
# DICIONÁRIO BKS
# ==============================================================================
BKS_DICT = {
    "X-n101-k25": 27591.0,
    "X-n106-k14": 26362.0,
    "X-n110-k13": 14971.0,
    "X-n115-k10": 12747.0,
    "X-n120-k6": 13332.0,
    "X-n125-k30": 55539.0,
    "X-n129-k18": 28940.0,
    "X-n134-k13": 10916.0,
    "X-n139-k10": 13590.0,
    "X-n143-k7": 15700.0,
    "X-n148-k46": 43448.0,
    "X-n153-k22": 21220.0,
    "X-n157-k13": 16876.0,
    "X-n162-k11": 14138.0,
    "X-n167-k10": 20557.0,
    "X-n172-k51": 45607.0,
    "X-n176-k26": 47812.0,
    "X-n181-k23": 25569.0,
    "X-n186-k15": 24145.0,
    "X-n190-k8": 16980.0,
    "X-n195-k51": 44225.0,
    "X-n200-k36": 58578.0,
    "X-n204-k19": 19565.0,
    "X-n209-k16": 30656.0,
    "X-n214-k11": 10856.0,
    "X-n219-k73": 117595.0,
    "X-n223-k34": 40437.0,
    "X-n228-k23": 25742.0,
    "X-n233-k16": 19230.0,
    "X-n237-k14": 27042.0,
    "X-n242-k48": 82751.0,
    "X-n247-k50": 37274.0,
    "X-n251-k28": 38684.0,
    "X-n256-k16": 18839.0,
    "X-n261-k13": 26558.0,
    "X-n266-k58": 75478.0,
    "X-n270-k35": 35291.0,
    "X-n275-k28": 21245.0,
    "X-n280-k17": 33503.0,
    "X-n284-k15": 20215.0,
    "X-n289-k60": 95151.0,
    "X-n294-k50": 47161.0,
    "X-n298-k31": 34231.0,
    "X-n303-k21": 21736.0,
    "X-n308-k13": 25859.0,
    "X-n313-k71": 94043.0,
    "X-n317-k53": 78355.0,
    "X-n322-k28": 29834.0,
    "X-n327-k20": 27532.0,
    "X-n331-k15": 31102.0,
    "X-n336-k84": 139111.0,
    "X-n344-k43": 42050.0,
    "X-n351-k40": 25896.0,
    "X-n359-k29": 51505.0,
    "X-n367-k17": 22814.0,
    "X-n376-k94": 147713.0,
    "X-n384-k52": 65928.0,
    "X-n393-k38": 38260.0,
    "X-n401-k29": 66154.0,
    "X-n411-k19": 19712.0,
    "X-n420-k130": 107798.0,
    "X-n429-k61": 65449.0,
    "X-n439-k37": 36391.0,
    "X-n449-k29": 55233.0,
    "X-n459-k26": 24139.0,
    "X-n469-k138": 221824.0,
    "X-n480-k70": 89449.0,
    "X-n491-k59": 66483.0,
    "X-n502-k39": 69226.0,
    "X-n513-k21": 24201.0,
    "X-n524-k153": 154593.0,
    "X-n536-k96": 94832.0,
    "X-n548-k50": 86700.0,
    "X-n561-k42": 42717.0,
    "X-n573-k30": 50673.0,
    "X-n586-k159": 190316.0,
    "X-n599-k92": 108451.0,
    "X-n613-k62": 59535.0,
    "X-n627-k43": 62164.0,
    "X-n641-k35": 63682.0,
    "X-n655-k131": 106780.0,
    "X-n670-k130": 146332.0,
    "X-n685-k75": 68205.0,
    "X-n701-k44": 81899.0,
    "X-n716-k35": 43350.0,
    "X-n733-k159": 136187.0,
    "X-n749-k98": 77269.0,
    "X-n766-k71": 114417.0,
    "X-n783-k48": 72383.0,
    "X-n801-k40": 73305.0,
    "X-n819-k171": 158121.0,
    "X-n837-k142": 193721.0,
    "X-n856-k95": 88965.0,
    "X-n876-k59": 99299.0,
    "X-n895-k37": 53845.0,
    "X-n916-k207": 329179.0,
    "X-n936-k151": 132715.0,
    "X-n957-k87": 85465.0,
    "X-n979-k58": 118923.0,
    "X-n1001-k43": 72332.0,
}
# ==============================================================================

NUM_TESTES = 10
DIRETORIO_SAIDA = "/app/output/seq/v4"
ARQUIVO_DETALHE_CSV = os.path.join(DIRETORIO_SAIDA, "benchmark_final_detail.csv")
EXECUTAVEL = "/app/src/build/sycl-dev/app/cvrp-pso"

if len(sys.argv) < 3:
    print("Uso:")
    print("  python executar_benchmark_seq.py -f <caminho_arquivo.vrp> [caminho_excel.xlsx]")
    print("  python executar_benchmark_seq.py -d <caminho_diretorio> [caminho_excel.xlsx]")
    sys.exit(1)

modo = sys.argv[1].upper()
alvo = sys.argv[2]

arquivos_teste = []
caminho = ''

if modo == "-F":
    arquivos_teste.append(os.path.basename(alvo))
    caminho = os.path.dirname(alvo)
    if caminho and caminho[-1] != '/':
        caminho += '/'
elif modo == "-D":
    caminho = alvo
    if caminho[-1] != '/':
        caminho += '/'
    arquivos_teste = os.listdir(caminho)
else:
    raise Exception("Erro: Use '-f' para arquivo único ou '-d' para diretório!")

caminho_output = sys.argv[3] if len(sys.argv) >= 4 else os.path.join(DIRETORIO_SAIDA, "PSO_benchmark_final.xlsx")
arquivos_teste.sort()

# Garantir diretório de saída exclusivo para o sequencial
os.makedirs(DIRETORIO_SAIDA, exist_ok=True)

try:
    wb = load_workbook(caminho_output)
    ws = wb.active
    ws.append([])
except:
    wb = Workbook()
    ws = wb.active

# Parâmetros fixos
iterations = 300
swarm_size = 150
elite_size = 75
sector_size = swarm_size
w_max_fixed = 0.0
w_min_fixed = 0.0
c1_fixed = 1.0
c2_fixed = 1.0

count = 1

for caso_teste in arquivos_teste:
    if not caso_teste.lower().endswith(".vrp"):
        continue

    caminho_completo = os.path.join(caminho, caso_teste) if caminho else caso_teste
    if not os.path.exists(caminho_completo):
        print(f"❌ Erro: Arquivo '{caminho_completo}' não encontrado.")
        continue

    instancia = ler_instancia(caminho_completo)
    nome_instancia = instancia.get("NAME", caso_teste.replace(".vrp", ""))
    bks_valor = BKS_DICT.get(nome_instancia, None)

    if bks_valor is None:
        print(f"⚠️ Aviso: Instância '{nome_instancia}' não encontrada no BKS_DICT. O parâmetro --bks não será passado.")

    # --------------------------------------------------------------------------
    # LÓGICA DE DETECÇÃO DE RETOMADA (--start-run)
    # --------------------------------------------------------------------------
    runs_ja_executadas = 0
    if os.path.exists(ARQUIVO_DETALHE_CSV):
        try:
            df_existente = pd.read_csv(ARQUIVO_DETALHE_CSV)
            if 'inst' in df_existente.columns and 'run' in df_existente.columns:
                sub_df = df_existente[df_existente['inst'] == nome_instancia]
                runs_ja_executadas = len(sub_df)
        except Exception as e:
            print(f"⚠️ Aviso ao ler o CSV de detalhes para retomada: {e}")

    if runs_ja_executadas >= NUM_TESTES:
        print(f"\n✨ Instância '{nome_instancia}' já possui todas as {NUM_TESTES} execuções concluídas no CSV sequencial. Pulando...")
        continue

    start_run_id = runs_ja_executadas + 1
    runs_restantes = NUM_TESTES - runs_ja_executadas

    print(f"\n========================================================")
    print(f"🧪 INSTÂNCIA (SEQUANCIAL): {nome_instancia} | Retomando a partir da Run: {start_run_id} ({runs_restantes} restantes)")
    print(f"Configuração Fixa: Swarm={swarm_size}, Elite={elite_size}, Iter={iterations}")
    print(f"========================================================")

    comando = [
        EXECUTAVEL,
        "--in", caminho_completo,
        "--out", DIRETORIO_SAIDA,
        "--swarm", str(swarm_size),
        "--iter", str(iterations),
        "--elite", str(elite_size),
        "--runs", str(runs_restantes),
        "--start-run", str(start_run_id),
        "--sector", str(sector_size),
        "--w-max", str(w_max_fixed),
        "--w-min", str(w_min_fixed),
        "--cognitive-c", str(c1_fixed),
        "--social-c", str(c2_fixed),
        "--swap-cap", "20",
        "--save-detail", ARQUIVO_DETALHE_CSV
    ]

    if bks_valor is not None:
        comando.extend(["--bks", str(bks_valor)])

    try:
        output = subprocess.check_output(comando).decode().strip()
        linhas_output = output.split('\n')
        
        for linha in linhas_output:
            if not linha.strip():
                continue
            if ',' in linha:
                parts = linha.split(',')
                if len(parts) >= 2:
                    custo_str, tempo_str = parts[0], parts[1]
                    print(f"-> Custo retornado: {custo_str} | Tempo: {tempo_str}s")

    except Exception as e:
        print(f"❌ Erro ao executar o comando para '{nome_instancia}': {e}")

    print(f"{count}/{len(arquivos_teste)} - {caso_teste} processado.")
    count += 1

print("\n🏁 Execuções sequenciais finalizadas com sucesso! Detalhes consolidados em:", ARQUIVO_DETALHE_CSV)