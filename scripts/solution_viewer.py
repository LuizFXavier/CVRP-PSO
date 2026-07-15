import networkx as nx
from sys import argv
from random import randint
import matplotlib.pyplot as plt
from leitor_instancia import ler_instancia

"""
argv[1]: Arquivo de instancia
argv[2]: Arquivo com a solução
"""

if len(argv) < 3:
    raise Exception("Número errado de argumentos!")

instancia = ler_instancia(argv[1], True)

G = nx.Graph()

#Leitura das coordenadas dos nós
coordenadas = {}

for i in range(instancia["DIMENSION"]):
    x, y = instancia["COORDS"][i]
    G.add_node(i)
    coordenadas[i] = (float(x), float(y))

solution_file = open(argv[2])
linhas = solution_file.readlines()


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

for line in linhas:
  if "Route" not in line:
    break
  
  clients = (line.split(':')[1])[1:]
  clients = clients.split(" ")

  clients = ["0"] + clients + ["0"]

  for i in range(len(clients) - 1):
    G.add_edge(int(clients[i]), int(clients[i+1]), color = cores[k% len(cores)])
    cores_nos[int(clients[i])] = cores[k% len(cores)]
  
  k = k + 1

cost = linhas[-1]

print(cost)

node_colors = [None] * len(cores_nos)

for chave, valor in cores_nos.items():
    node_colors[chave] = valor

node_colors[0] = 'black'

edge_colors = [G.edges[edge]['color'] for edge in G.edges()] 

nx.draw(G, coordenadas, node_color=node_colors, edge_color=edge_colors, node_size=10)

print("k:", k)
plt.show()