import subprocess
import os
import pandas as pd
import numpy as np
import re
from openpyxl import load_workbook, Workbook

caminho_output = "./PSO_data.xlsx"

NUM_TESTES = 10

instance = ""

dados = {"Instância":[]}

for i in range(1, NUM_TESTES +1):
    dados[i] = []

dados["Solução Ótima"] = []
dados["Mínimo"] = []
dados["Mediana"] = []
dados["Máximo"] = []
dados["Média"] = []
dados["Melhor Caminho"] = []
valores = []

wb = None
ws = None
try:
    wb = load_workbook(caminho_output)
    ws = wb.active
    ws.append([])
except:
    wb = Workbook()
    ws = wb.active

count = 1
arquivos_teste = os.listdir("./test")

for caso_teste in arquivos_teste:
    
    best_solution = ""
    min_value = 0xffffff

    test_file = open("./test/"+caso_teste)
    test_file.readline()

    line = test_file.readline()

    line = re.findall(r"value: \d+", line)[0].split(":")[1]

    dados["Solução Ótima"].append(float(line))
    test_file.close()

    for i in range(1, NUM_TESTES +1):
        processo = subprocess.run(["/mnt/c/Users/irine/Documents/Projetos/LSCAD/tsp_pso/vrp_pso",
                        "./test/" + caso_teste,
                        "./pso.config.txt",
                        "./output.txt"]
                        )
        file = open("output.txt")
        valor = float(file.readline())

        solucao = file.readline()
        
        if min_value > valor:
            min_value = valor
            best_solution = solucao

        if(i == 1):
            instance = file.readline()
            dados["Instância"].append(instance)
        
        valores.append(valor)
        dados[i].append(valor)
        file.close()
    
    dados["Mínimo"].append(min_value)
    dados["Máximo"].append(max(valores))
    dados["Média"].append(np.average(valores))
    dados["Mediana"].append(np.median(valores))
    dados["Melhor Caminho"].append(best_solution)
    valores = []

    print(str(count) + "/" + str(len(arquivos_teste)), caso_teste)
    count += 1

tabela = pd.DataFrame(dados)

config_file = open("./pso.config.txt")

ws.append(["Configurações PSO:"])
for i in range(6):
    ws.append(list(config_file.readline().split(": ")))

ws.append([])

ws.append(tabela.columns.tolist())

for row in tabela.itertuples(index=False):
    ws.append(row)

wb.save(caminho_output)
print("Resultado salvo em", caminho_output)