# Eixo x: False. Eixo y: True
if __name__ == '__main__':
    exit("Esse módulo foi executado indevidamente!")

class Cliente:
    def __init__(self, x, y, nome):
        self.x = x
        self.y = y
        self.nome = nome
    
    def greater(self, outra, eixo):
        if not eixo:
            return self.x > outra.x
        else:
            return self.y > outra.y

class Node:
    def __init__(self, cliente):
        self.cliente = cliente
        self.esq = None
        self.dir = None

class KD_tree:
    def __init__(self):
        self.raiz = None

    def print_in_ordem(self):
        self._percorre_in_ordem(self.raiz)
        print()
    
    def _percorre_in_ordem(self, no):
        if no == None:
            return
        print("/", end="")

        self._percorre_in_ordem(no.esq)

        print(no.cliente.nome, end="")

        self._percorre_in_ordem(no.dir)

        print("\\", end="")

    def _insere_no(self, no_atual, cliente, eixo):
        if cliente.greater(no_atual.cliente, eixo):
            if no_atual.dir == None:
                no_atual.dir = Node(cliente)
            else:
                self._insere_no(no_atual.dir, cliente, not eixo)
        else:
            if no_atual.esq == None:
                no_atual.esq = Node(cliente)
            else:
                self._insere_no(no_atual.esq, cliente, not eixo)

    def insert_node(self, cliente):
        if self.raiz == None:
            self.raiz = Node(cliente)
            return
        
        self._insere_no(self.raiz, cliente, False)    