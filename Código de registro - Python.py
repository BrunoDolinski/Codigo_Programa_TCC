import socket
from time import sleep
from threading import Thread
import csv
import time
from decimal import Decimal
# Classe para receber dados via UDP
class UDPReceiver:
def __init__(self, port=5005):
self.UDP_IP = ""
self.UDP_PORT = 5005
self.sock = socket.socket(socket.AF_INET, # Internet
socket.SOCK_DGRAM) # UDP
self.sock.bind((self.UDP_IP, self.UDP_PORT))
def receive(self):
data, addr = self.sock.recvfrom(1024) #tamanho do buffer para receber dados
return data, addr
udp_r = UDPReceiver(5005) #define a porta em que os dados serão recebidos
#Define os vetores para salvar os dados
T0 = []
T1 = []
T2 = []
T3 = []
I = []
Dados = []
counter = 0 #Contador da quantidade de dados recebidas
qtdDados = 1000 #Quantidade de dados que será recebida para salvar
while(True):
if __name__ == "__main__":
counter += 1 #incrementa um dado no contador a cada ciclo, contando a
quantidade de dados recebida
k = 0
data, addr = udp_r.receive() #recebe dados do CLP registrador
#Extrai os dados desejados da mensagem recebida, a partir do formato definido
data = str(data)
Dados.append(data)
Tempo0 = data.find("A")
Tempo1 = data.find("B")
Tempo2 = data.find("C")
Tempo3 = data.find("D")
f = data.find("f")
#Salva nas matrizes as informações desejadas a partir dos dados recebidos
T0.append(data[Tempo0+1:Tempo1])
T1.append(data[Tempo1+1:Tempo2])
T2.append(data[Tempo2+1:Tempo3])
T3.append(data[Tempo3+1:f])
#Verifica se a quantidade desejada de dados foi atingida, se sim, para de receber
dados
if (counter >= qtdDados):
k = 1
break
#Cria uma planilha e salva todos os dados das matrizes nela
nome_do_arquivo = "Teste9-225.csv"
arquivo_csv = open(nome_do_arquivo, mode="w", newline="")
escritor_csv = csv.writer(arquivo_csv)
escritor_csv.writerow(['Tempo 0', 'Tempo 1', 'Tempo 2', 'Tempo 3'])
tamanho = [T0, T1, T2, T3]
tmax = max(tamanho)
for i, numero in enumerate(tmax, start=1):
escritor_csv.writerow([T0[i-1], T1[i-1], T2[i-1], T3[i-1]])
arquivo_csv.close()
