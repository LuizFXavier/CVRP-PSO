import networkx as nx
from sys import argv
from random import randint
import matplotlib.pyplot as plt

"""
argv[1]: Arquivo de instancia
argv[2]: Arquivo com as soluções
argv[3]: Linha a ser desenhada
"""

if len(argv) < 4:
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

    G.add_node(int(id))
    coordenadas[int(id)] = (float(x), float(y))

instance_file.close()

result_file = open(argv[2])
linhas = result_file.readlines()

print(len(linhas))

alvo = int(argv[3])

linha_alvo = linhas[alvo]

rota = linha_alvo.split(';')[2].split('-')

cores = ['blue', 'red', 'green', 'cyan', 'navy', 'darkred', 'crimson', 'lime', 'springgreen', 'pink', 'peru', 'slateblue', 'dimgray', 'hotpink', 'aquamarine', 'olive', 'fuchsia', 'darkseagrean']

# cor_atual = f"#{randint(0, 255):02x}{randint(0, 255):02x}{randint(0, 255):02x}"

k = 0

for i in range(len(rota) - 1):
    G.add_edge(int(rota[i]) +1, int(rota[i+1]) +1, color = cores[k% len(cores)])

    if(int(rota[i+1]) == 0):
        # cor_atual = f"#{randint(0, 255):02x}{randint(0, 255):02x}{randint(0, 255):02x}"
        k += 1
    

edge_colors = [G.edges[edge]['color'] for edge in G.edges()] 

nx.draw(G, coordenadas, node_color='rebeccapurple', edge_color=edge_colors, node_size=10)

plt.title(nome_instancia)


print(k)
plt.show()