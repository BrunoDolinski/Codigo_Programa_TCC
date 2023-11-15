mport socket
from time import sleep
from threading import Thread
import datetime
import time
import csv
import time
from decimal import Decimal
import random
import Adafruit_ADS1x15 as serial
import RPi.GPIO as gpio
# Inicia variaveis
global t0_rasp
global t3_rasp
global counter
global qtdDados
global k
t0_rasp = 0
t3_rasp = 0
counter = 0
k = 0
t1 = 0
vazao = 0
i=0
data = 0
#*** Entradas e saídas do Raspberry ***
gpio.setwarnings(False)
gpio.setmode(gpio.BOARD)
#Saída 1: pino 33
#Entrada 1: A0 do ADC
pino_saida1 = 33 # Pino de saída PWM para envio de sinal para a planta
chave1 = 37 # Pino de saída para envio de sinal ao enviar um pacote
chave2 = 35 # Pino de saída para envio de sinal ao receber um pacote
#define os pinos como saída e os inicializa em nível lógico baixo
gpio.setup(chave1,gpio.OUT)
gpio.setup(chave2,gpio.OUT)
statusChave1 = gpio.output(chave1, 0)
statusChave2 = gpio.output(chave2, 0)
#Inicializa a saída PWM
gpio.setup(pino_saida1,gpio.OUT)
saida1=gpio.PWM(pino_saida1,300)
saida1.start(0)
#define o objeto para a leitura do ADC
ADC = serial.ADS1115()
A3_min = 8350 #valor mínimo de vazão da planta
A3_max = 26500 #valor máximo de vazão da planta
#variáveis para armazenar a leitura parcial e convertida da planta
A3_value = 0
leitura = [0, 0, 0, 0, 0]
leituram = 0
# Classe para receber dados via UDP
class UDPReceiver:
def __init__(self, port=5005):
self.UDP_IP = ""
self.UDP_PORT = 5005
self.sock = socket.socket(socket.AF_INET, # Internet
socket.SOCK_DGRAM) # UDP
self.sock.bind((self.UDP_IP, self.UDP_PORT))
def receive(self):
data, addr = self.sock.recvfrom(1024) # tamanho do buffer 1024 bytes
return data, addr
# Classe para enviar dados via UDP
class UDPSend():
def __init__(self, port = 5006, MESSAGE=bytearray):

self.UDP_IP = "10.2.0.36"
self.UDP_PORT = 5006
def send(self):
sock = socket.socket(socket.AF_INET, # Internet
socket.SOCK_DGRAM) # UDP
sock.sendto(self.MESSAGE, (self.UDP_IP, self.UDP_PORT))
udp_r = UDPReceiver(5005) #define a porta do server
udp_s = UDPSend(5006) #define a porta do client
# Cria vetores para armazenar os resultados
Delay1 = []
Delay2 = []
U = []
E = []
Perda = []
Amostragem = []
setpoint = [50.0, 20.0, 30.0, 40.0, 25.0, 60.0, 70.0, 80.0, 90.0, 100.0] # setpoint em %
amostragem = [30, 50, 70, 90, 100, 100, 100, 100, 90, 100] # período de
amostragem em ms
#Cria a Thread de execução principal
class Principal:
def __init__(self):
self._running = True
def terminate(self):
self._running = False
def run(self):
#define as variáveis que devem ser utilizadas em todo o código como globais
global t0_rasp
global t3_rasp
global counter
global qtdDados
global tatual
global k
#seta os valores iniciais das variáveis
tatual = 0
t0_rasp = 0
t3_rasp = 0
counter = 0
qtdDados = 110
perda = 0
a = 0
k = 0
while True:
tatual = time.perf_counter()*1000 #atualiza o tempo atual
#compara a diferença tempo atual e o tempo do último envio com o tempo de
amostragem desejado
#garante um tempo de amostragem predefinido entre os pacotes
if(tatual - t0_rasp > amostragem[a]):
sp = 0 #define qual valor de setpoint será
utilizado
counter = counter + 1 #conta o ciclo executado
# Realiza a leitura do ADC
leituram = ADC.read_adc(1, gain=1)
# Converte o valor da leitura para porcentagem
A3_value = ((leituram-A3_min)/(A3_max-A3_min))*100
vazao_p = A3_value
#Monta os pacotes de acordo com o formato predefinido
udp_s.MESSAGE = b"".join([b"\x50\x00",  bytes("V", encoding='utf-8'),
                                        bytes(str(vazao_p), encoding=
'utf-8'),
                                        bytes("S", encoding='utf-8'),
                                        bytes(str(setpoint[s]),
encoding='utf-8'),
                                        bytes("I", encoding='utf-8'),
                                        bytes(str(counter), encoding=
'utf-8'),
                                        bytes("f", encoding='utf-8')])
t0_rasp= time.perf_counter()*1000 #marca o tempo de envio da
mensagem
message = udp_s.send() #envia a mensagem
data, addr = udp_r.receive() #recebe uma mensagem de
resposta
t3_rasp= time.perf_counter()*1000 #marca o tempo de recebimento
da resposta
data = str(data) #transforma a mensagem
recebida em string
#Extrai e separa os dados recebidos
data = str(data)
vazao = data.find("V")
t1 = data.find("T")
t2 = data.find("t")
ind = data.find("I")
f = data.find("f")
v_clp = float(data[vazao+1:t1])
t1_clp = int(data[t1+1:t2])
t2_clp = int(data[t2+1:ind])
indice = int(data[ind+1:f])
#Garante que o sinal de controle esteja dentro do range do raspberry
if(v_clp >= 100.0):
v_clp = 100.0
if(v_clp <= 0.0):
v_clp = 0.0
e = setpoint[sp] - A3_value #cálculo do erro
u = sai
#Escalona o sinal de acordo com o range de saída da raspberry
out1_UDP_f = float((10000-100*(1399/17.59)+(1399/17.59)*sai)/100)
#Envia um sinal para a planta de acordo com o controle feito pelo CLP
saida1.ChangeDutyCycle(out1_UDP_f)
#salva os dados em vetores para salvar na planilha
Delay1.append(int(t1_clp))
Delay2.append(int(t3_rasp - t0_rasp - t2_clp))
U.append(u)
E.append(e)
#contabiliza a perda de pacotes comparando o indice enviado ao
recebido
if counter != indice:
perda = perda + 1
Perda.append(perda)
else:
Perda.append(0)
Amostragem.append(amostragem[a])
#verifica se a quantidade de dados por período de amostragem foi
alcançada
if (counter >= qtdDados):
#se sim e ainda menor que a quantidade de períodos de
amostragem, passa para o próximo período
if(a < 4):
a = a + 1
counter = 0
#caso todos os dados tenham sido enviados e recebidos, sai do
loop
else:
k = 1 #seta flag em 1 indicando que
deve ser finalizada a Thread
sleep(0.5)
break
#Cria a classe da Thread

Secundario = Principal()
#Cria Thread
SecundarioThread = Thread(target=Secundario.run)
#Inicia a Thread
SecundarioThread.start()
#função da thread para acionar as saídas da raspberry ao enviar ou receber um sinal
Exit = False #Flag de saída
while Exit==False:
atual = time.perf_counter()*1000 #marca o tempo atual
#compara a diferença do tempo atual com o de envio para garantir o tempo do sinal alto
if((atual - t0_rasp < 10) and t0_rasp != 0):
statusChave1 = gpio.output(chave1, 1)
else:
statusChave1 = gpio.output(chave1, 0) #garante nível lógico baixo na saída
após o sinal ter sido enviado
#compara a diferença do tempo atual com o de recebimento para garantir o tempo do
sinal alto
if((atual - t3_rasp < 10) and t3_rasp != 0):
statusChave1 = gpio.output(chave2, 1)
else:
statusChave1 = gpio.output(chave2, 0) #garante nível lógico baixo na saída
após o sinal ter sido enviado
if (k == 1): Exit = True #sai da thread quando a
comunicação acaba e a flag k é setada em 1
Secundario.terminate()
#Cria a planilha para armazenamento dos dados dos testes
nome_do_arquivo = "Teste6_300_0.csv"
#Salva na planilha os dados do teste realizado
arquivo_csv = open(nome_do_arquivo, mode="w", newline="")
escritor_csv = csv.writer(arquivo_csv)
escritor_csv.writerow(['Delay1', 'Delay2', 'U', 'E', 'Perda de Pacote', 'Período de
Amostragem'])
for i, numero in enumerate(Delay1, start=1):
escritor_csv.writerow([Delay1[i-1], Delay2[i-1], U[i-1], E[i-1], Perda[i-1], Amostragem[i-
1]])
arquivo_csv.close()


