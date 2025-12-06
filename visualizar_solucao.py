import networkx as nx
from sys import argv
from random import randint
import matplotlib.pyplot as plt
from leitor_instancia import ler_instancia

"""
argv[1]: Arquivo de instancia
argv[2]: Arquivo com as soluções
argv[3]: Linha a ser desenhada
"""

if len(argv) < 4:
    raise Exception("Número errado de argumentos!")

elif int(argv[3]) < 0:
    raise Exception("Escolha uma linha válida")

instancia = ler_instancia(argv[1], True)

G = nx.Graph()

#Leitura das coordenadas dos nós
coordenadas = {}

for i in range(instancia["DIMENSION"]):
    x, y = instancia["COORDS"][i]
    G.add_node(i+1)
    coordenadas[i+1] = (float(x), float(y))

result_file = open(argv[2])
linhas = result_file.readlines()

alvo = int(argv[3]) - 1

linha_alvo = linhas[alvo]

iteracao, custo, rota = linha_alvo.split(',')

print("custo:" ,custo)

rota = rota.split('-')

cores = ['#3366cc', 
         '#dc3912', 
         '#ff9900', 
         '#109618', 
         '#990099', 
         '#0099c6', 
         '#dd4477', 
         '#66aa00', 
         '#b82e2e', 
         '#316395', 
         '#994499', 
         '#22aa99', 
         '#aaaa11', 
         '#6633cc', 
         '#e67300', 
         'olive', 'fuchsia', 'darkseagreen']

# cor_atual = f"#{randint(0, 255):02x}{randint(0, 255):02x}{randint(0, 255):02x}"

k = 0
cores_nos = {}
for i in range(len(rota) - 1):
    G.add_edge(int(rota[i]) +1, int(rota[i+1]) +1, color = cores[k% len(cores)])
    cores_nos[int(rota[i])] = cores[k% len(cores)]
    if(int(rota[i+1]) == 0):
        # cor_atual = f"#{randint(0, 255):02x}{randint(0, 255):02x}{randint(0, 255):02x}"
        k += 1

node_colors = [None] * len(cores_nos)

for chave, valor in cores_nos.items():
    node_colors[chave] = valor

node_colors[0] = 'black'

edge_colors = [G.edges[edge]['color'] for edge in G.edges()] 

nx.draw(G, coordenadas, node_color=node_colors, edge_color=edge_colors, node_size=10)

print("k:", k)
plt.show()