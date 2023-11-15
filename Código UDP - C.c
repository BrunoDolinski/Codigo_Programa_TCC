#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "ads1115_rpi.h"
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#define dadosTotal 550
int perda = 0,
BUFLEN = 1024,
atual = 0,
out = 0,
qtdDados = dadosTotal/5,
a = 0,
sockfd,
counter = 0,
port = 5006, //porta utilizada para o client
ports = 5005, //porta utilizada para o server
sockfds,
delay1[dadosTotal],
delay2[dadosTotal],
losspkg[dadosTotal];
char buffer[1024],
buffers[1024],
A3_value_s[6],
setpoint_s[6],
counters[6],
vazao[50],
t1[50],
t2[50],
indice[50],
chars_end[] = {'T', 't', 'I', 'f'},
chars_start[] = {'V', 'T', 't', 'I'},
extracted[50];
const int PWM_pin = 23, /* Pino de saída PWM para envio de sinal para a planta*/
chave1 = 25, /* Pino de saída para envio de sinal ao enviar um pacote */
chave2 = 24; /* Pino de saída para envio de sinal ao receber um pacote */
//variaveis para leitura e escrita na planta
float erro[dadosTotal],
saida[dadosTotal],
amostragem[dadosTotal],
amostragemi[5] = {30, 50, 70, 90, 100}, // período de amostragem
em ms
setpoint[] = {50, 20, 30, 40, 25, 60, 70, 80, 90, 100}, //setpoint em %
A3_min = 8350, //valor mínimo de vazão
da planta
A3_max = 26500, //valor máximo de vazão
da planta
//variáveis para armazenar a leitura parcial e convertida da planta
A3_value = 0,
leitura[5] = {0, 0, 0, 0, 0},
leituram;
//variaveis para marcar o tempo
double t0rasp = 0,
t3rasp = 0,
tatual = 0;
//Estruturas para o tempo inicial, tempo final e sockets para comunicação
struct sockaddr_in si_me, si_other;
struct sockaddr_in serverAddr;
struct timeval begin, end;
socklen_t addr_size;
//função para pegar o tempo em milissegundos em relação ao marco inicial
double elapsedtime(){
gettimeofday(&end, 0);
long seconds = end.tv_sec - begin.tv_sec;
long microseconds = end.tv_usec - begin.tv_usec;
double elapsedt = seconds*1000 + microseconds*1e-3;
return elapsedt;
}
//função da thread para acionar as saídas da raspberry ao enviar ou receber um sinal
void *funcprincipal(void*arg){
while(1){
atual = elapsedtime(); //marca o tempo atual
//compara a diferença do tempo atual com o de envio para garantir o tempo do sinal
alto
if((atual - t0rasp < 10) && t0rasp != 0)
digitalWrite(chave1, HIGH);
else
digitalWrite(chave1, LOW); //garante nível lógico baixo na saída
após o sinal ter sido enviado
//compara a diferença do tempo atual com o de recebimento para garantir o tempo do
sinal alto
if((atual - t3rasp < 10) && t3rasp != 0)
digitalWrite(chave2, HIGH);
else
digitalWrite(chave2, LOW); //garante nível lógico baixo na saída
após o sinal ter sido enviado
}
}
int main (void){
if (wiringPiSetup () == -1) //seta a biblioteca wiringPi para
utilizar as GPIOs
exit (1) ;
pinMode(PWM_pin, PWM_OUTPUT); //seta o pino PWM escolhido como saída
pinMode(chave1, OUTPUT); //seta o pino de envio de mensagem
escolhido como saída
pinMode(chave2, OUTPUT); //seta o pino de recebimento de mensagem
escolhido como saída
if(openI2CBus("/dev/i2c-1") == -1){ //abre o barramento I2C para a leitura do
sinal de entrada da planta convertido
return EXIT_FAILURE;
}
setI2CSlave(0x48); //Seta o dispositivo como escravo I2C
para recebimento de sinais
FILE *file = fopen("Teste7-75_12.csv", "w"); //Cria a planilha para armazenamento dos
dados dos testes
//cria o socket UDP server
sockfds = socket(AF_INET, SOCK_DGRAM, 0);
memset(&si_me, '\0', sizeof(si_me));
si_me.sin_family = AF_INET;
si_me.sin_port = htons(ports);
si_me.sin_addr.s_addr = inet_addr("10.182.4.71");
//vincula o socket ao seu endereço
bind(sockfds, (struct sockaddr*)&si_me, sizeof(si_me));
addr_size = sizeof(si_other);
//cria o socket UDP client
sockfd = socket(PF_INET, SOCK_DGRAM, 0);
memset(&serverAddr, '\0', sizeof(serverAddr));
serverAddr.sin_family = AF_INET;
serverAddr.sin_port = htons(port);
serverAddr.sin_addr.s_addr = inet_addr("10.2.0.36");
//Define o tempo de referência
gettimeofday(&begin, 0);
//Cria a thread para envio de sinais
pthread_t principal;
int a1;
pthread_create(&principal, NULL, funcprincipal, (void*)(&a1));
while(1){
tatual = elapsedtime(); //atualiza o tempo atual
//compara a diferença tempo atual e o tempo do último envio com o tempo de amostragem
desejado
//garante um tempo de amostragem predefinido entre os pacotes
if((tatual - t0rasp) > amostragemi[a]){
counter++; //conta o ciclo executado
//leitura e conversão para porcentagem
leituram = readVoltage(1);
A3_value = ((leituram-A3_min)/(A3_max-A3_min))*100;
//Monta os pacotes de acordo com o formato predefinido
strcpy(buffer," ");
sprintf(A3_value_s, "%.3f", A3_value);
strcat(buffer, "V");
strcat(buffer, A3_value_s);
strcat(buffer, "S");
sprintf(setpoint_s, "%.3f",setpoint[0]);
strcat(buffer, setpoint_s);
strcat(buffer, "I");
sprintf(counters, "%d",counter);
strcat(buffer, counters);
strcat(buffer, "f");
t0rasp = elapsedtime(); //marca o tempo de envio da
mensagem
sendto(sockfd, buffer, 1024, 0, (struct sockaddr*)&serverAddr, sizeof(serverAddr
)); //envia a mensagem
recvfrom(sockfds, buffers, BUFLEN, 0, (struct sockaddr*)& si_other, &addr_size);
//recebe uma mensagem de resposta
t3rasp = elapsedtime(); //marca o tempo de recebimento da
resposta
//arruma o pacote de acordo com seu tamanho
for(int i = 0; i < 40;i++){
buffers[i] = buffers[i+2];
}
//Extrai e separa os dados recebidos
for (int i = 0; i < 4; i++) {
char *startPtr = strchr(buffers, chars_start[i]);
char *endPtr = strchr(buffers, chars_end[i]);
if (startPtr != NULL && endPtr != NULL && endPtr > startPtr) {
int start_index = startPtr - buffers + 1;
int end_index = endPtr - buffers - 1;
strncpy(extracted, buffers + start_index, end_index - start_index + 1);
extracted[end_index - start_index + 1] = '\0';
switch(i){
case 0:
strcpy(vazao, extracted);
break;
case 1:
strcpy(t1, extracted);
break;
case 2:
strcpy(t2, extracted);
break;
case 3:
strcpy(indice, extracted);
break;
default:
break;
}
}
else
printf("Caracteres %c e %c não encontrados na string ou na ordem
correta.\n", chars_start[i], chars_end[i]); //Envia uma mensagem em
caso de erro na extração
}
out = (int)(atof(vazao)*(1023-209.37)/100)+209.37; //Escalona o sinal de
acordo com o range de saída da raspberry
pwmWrite (PWM_pin, out); //Envia um sinal para a
planta de acordo com o controle feito pelo CLP
//salva os dados em vetores para salvar na planilha
delay1[a*qtdDados+counter] = atoi(t1);
delay2[a*qtdDados+counter] = t3rasp - t0rasp - atoi(t2);
saida[a*qtdDados+counter] = atof(vazao);
erro[a*qtdDados+counter] = setpoint[0] - A3_value;
//contabiliza a perda de pacotes comparando o indice enviado ao recebido
if(counter!=atoi(indice))
losspkg[a*qtdDados+counter] = perda + 1;
else
losspkg[a*qtdDados+counter] = perda;
amostragem[a*qtdDados+counter] = amostragemi[a];
//verifica se a quantidade de dados por período de amostragem foi alcançada
if(counter >= qtdDados){
//se sim e ainda menor que a quantidade de períodos de amostragem, passa para
o próximo período
if(a < 4){
a++;
counter = 0;
}
//caso todos os dados tenham sido enviados e recebidos, sai do loop
else{
printf("fim");
pwmWrite (PWM_pin, 0);
delay(100);
break;
}
}
}
}
//salva na planilha os dados do teste realizado
fprintf(file, "Delay1, Delay2, U, E, Perda, Amostragem \n");
for(int r = 1;r <= qtdDados*5; r++){
fprintf(file, "%d, %d, %f, %f, %d, %f \n", delay1[r], delay2[r], saida[r], erro[r],
losspkg[r], amostragem[r]);
}
return 0;
exit(0);
}
