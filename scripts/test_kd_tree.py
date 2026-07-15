from kd_tree import KD_tree, Cliente
from leitor_instancia import ler_instancia
import matplotlib.pyplot as plt
import networkx as nx
from sys import argv

"""
argv[1]: Instância
"""
if len(argv) < 2:
    exit("Insira o arquivo de instância desejado!")

tree = KD_tree()
G = nx.Graph()
instancia = ler_instancia(argv[1], True)

coords = {}
for i in range(instancia["DIMENSION"]):
    x = instancia["COORDS"][i][0]
    y = instancia["COORDS"][i][1]
    nome = chr(65 + i)
    G.add_node(nome)
    coords[nome] = (x, y)
    tree.insert_node(Cliente(x, y, nome))

margem = 10
Xs = [p[0] for p in instancia["COORDS"]]
Ys = [p[1] for p in instancia["COORDS"]]

x_max = max(Xs) + margem
x_min = min(Xs) - margem
y_max = max(Ys) + margem
y_min = min(Ys) - margem

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

def draw_2dtree(node, xmin, xmax, ymin, ymax, depth=0, ax=None, is_root = False):
    global cores
    """Função recursiva para desenhar as divisões da 2D-tree"""
    if ax is None:
        fig, ax = plt.subplots(figsize=(8, 8))
        ax.set_xlim(xmin, xmax)
        ax.set_ylim(ymin, ymax)
        ax.set_aspect('equal')
        ax.grid(True)
    
    if node is None:
        return
    
    # Alterna entre divisão vertical e horizontal
    if depth % 2 == 0:
        # Divisão vertical (x)
        if node.esq or node.dir:
            ax.plot([node.cliente.x, node.cliente.x], [ymin, ymax], cores[depth % len(cores)])
        # Chamadas recursivas para subárvores
        draw_2dtree(node.esq, xmin, node.cliente.x, ymin, ymax, depth+1, ax)
        draw_2dtree(node.dir, node.cliente.x, xmax, ymin, ymax, depth+1, ax)
    else:
        # Divisão horizontal (y)
        if node.esq or node.dir:
            ax.plot([xmin, xmax], [node.cliente.y, node.cliente.y], cores[depth % len(cores)])
        # Chamadas recursivas para subárvores
        draw_2dtree(node.esq, xmin, xmax, ymin, node.cliente.y, depth+1, ax)
        draw_2dtree(node.dir, xmin, xmax, node.cliente.y, ymax, depth+1, ax)
    
    # Desenha o ponto atual
    if not is_root:
        ax.plot(node.cliente.x, node.cliente.y, 'ko')
    else:
        ax.plot(node.cliente.x, node.cliente.y, 'ro')
    
    return ax

ax = draw_2dtree(tree.raiz, x_min, x_max, y_min, y_max, is_root=True)
ax.set_title(instancia["NAME"])
plt.show()