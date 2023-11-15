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
udp_s.MESSAGE = b"".join([b"\x50\x00", bytes("V", encoding='utf-8'),
bytes(str(vazao_p), encoding=
'utf-8'),
bytes("S", encoding='utf-8'),
bytes(str(setpoint[s]),
encoding='utf-8'),
bytes("I", encoding='utf-8'),
