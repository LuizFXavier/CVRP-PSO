import subprocess
import os
import pandas as pd
import numpy as np
import re
from openpyxl import load_workbook, Workbook

caminho_output = "./PSO_data.xlsx"

NUM_TESTES = 10

instance = ""

def reset_dados(dados):
    dados["Instância"] = []
    dados["Solução Ótima"] = []
    dados["Mínimo"] = []
    dados["Mediana"] = []
    dados["Máximo"] = []
    dados["Média"] = []

wb = None
ws = None
try:
    wb = load_workbook(caminho_output)
    ws = wb.active
    ws.append([])
except:
    wb = Workbook()
    ws = wb.active

valores = []
dados = {"Instância":[]}
reset_dados(dados)

dados["Nº Repetições"] = [10, 10, 10, 50, 500, 1000, 50, 500, 1000]
dados["Nº Partículas"] = [100, 1000, 10_000, 100, 100, 100, 1000, 1000, 1000]

num_configs = len(dados["Nº Partículas"])

count = 1
arquivos_teste = os.listdir("./test/A/")

for caso_teste in arquivos_teste:

    test_file = open("./test/A/"+caso_teste)

    line = test_file.readline()
    line = line.split(": ")[1]
    dados["Instância"].append(line)

    line = test_file.readline()
    line = re.findall(r"value: \d+", line)[0].split(":")[1]
    dados["Solução Ótima"].append(float(line))

    test_file.close()

    for c in range(num_configs):
        print("CONFIG", c)
        for i in range(1, NUM_TESTES +1):
            output = subprocess.check_output(["./vrp_pso",
                            "./test/A/" + caso_teste,
                            str(dados["Nº Partículas"][c]),
                            str(dados["Nº Repetições"][c])]
                            ).decode()
            print(i)
            valores.append(float(output))
        
        dados["Mínimo"].append(min(valores))
        dados["Máximo"].append(max(valores))
        dados["Média"].append(np.average(valores))
        dados["Mediana"].append(np.median(valores))
        valores = []

    reset_dados(dados)
    print(str(count) + "/" + str(len(arquivos_teste)), caso_teste)
    count += 1

tabela = pd.DataFrame(dados)

ws.append([])

ws.append(tabela.columns.tolist())

for row in tabela.itertuples(index=False):
    ws.append(row)

wb.save(caminho_output)
print("Resultado salvo em", caminho_output)