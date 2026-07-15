import networkx as nx
from sys import argv
from random import randint
import matplotlib.pyplot as plt

"""
argv[1]: Arquivo de instancia
argv[2]: Arquivo com as soluções
argv[3]: Primeira linha de solução
argv[4]: Segunda linha de solução
"""

if len(argv) < 5:
    raise Exception("Número errado de argumentos!")

elif int(argv[3]) < 2:
    raise Exception("Escolha uma linha válida")

instance_file = open(argv[1])

nome_instancia = instance_file.readline().split(": ")[1]
#Informação desnecessária
for i in range(2):
    instance_file.readline()

dimension = int(instance_file.readline().split(":")[1])

#Informação desnecessária
for i in range(3):
    instance_file.readline()

G = nx.Graph()

#Leitura das coordenadas dos nós
coordenadas = {}

for i in range(dimension):
    id, x, y = instance_file.readline().split(" ")

    if(i == 0):
        G.add_node(int(id), color = "blue")
    else:
        G.add_node(int(id), color = "green")
    coordenadas[int(id)] = (float(x), float(y))

instance_file.close()

result_file = open(argv[2])
linhas = result_file.readlines()

print(len(linhas))

alvo1 = int(argv[3])
alvo2 = int(argv[4])

linha_alvo1 = linhas[alvo1]
linha_alvo2 = linhas[alvo2]

rota1 = linha_alvo1.split(';')[2].split('-')
rota2 = linha_alvo2.split(';')[2].split('-')

# Degradê de vermelhos (do claro ao escuro)
tons_vermelho = [
    "#FF9999", "#FF6666", "#FF3333", 
    "#FF1111", "#EE6666", "#EE3333",
    "#DD0000", "#BB0000", "#990000"
]

# Degradê de azuis (do claro ao escuro)
tons_azul = [
    "#88AAFF", "#6699FF", "#4488FF",
    "#2277FF", "#0066FF", "#0055DD",
    "#0044BB", "#003399", "#002277"
]
k = 0
for i in range(len(rota1) - 1):
    G.add_edge(int(rota1[i]) +1, int(rota1[i+1]) +1, color = tons_vermelho[k% len(tons_vermelho)])

    if(int(rota1[i+1]) == 0):
        k += 1

k = 0

for i in range(len(rota2) - 1):
    G.add_edge(int(rota2[i]) +1, int(rota2[i+1]) +1, color = tons_azul[k% len(tons_azul)])

    if(int(rota2[i+1]) == 0):
        k += 1

edge_colors = [G.edges[edge]['color'] for edge in G.edges()]

node_colors = [G.nodes[node]['color'] for node in G.nodes()]

nx.draw(G, coordenadas, edge_color=edge_colors, node_color=node_colors,node_size=10)

plt.title(nome_instancia)

plt.show()