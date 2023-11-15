#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <wiringPi.h>
#include <stdlib.h>
#include <linux/i2c-dev.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include "ads1115_rpi.h"
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#define PORT 5006
#define dadosTotal 550
int perda = 0,
atual = 0,
out = 0,
qtdDados = dadosTotal/5,
a = 0,
counter = 0,
delay1[dadosTotal],
delay2[dadosTotal],
losspkg[dadosTotal],
status,
client_fd;
char buffer[1024],
A3_value_s[6],
setpoint_s[6],
counters[6],
vazao[50],
t1[50],
t2[50],
indice[50],
chars_end[] = {'T', 't', 'I', 'f'},
chars_start[] = {'V', 'T', 't', 'I'},
extracted[50],
mensagem [1024],
buffer[1024] = { 0 };
const int PWM_pin = 23, /* Pino de saída PWM para envio de sinal para a planta*/
chave1 = 25, /* Pino de saída para envio de sinal ao enviar um pacote */
chave2 = 24; /* Pino de saída para envio de sinal ao receber um pacote */
//variaveis para leitura e escrita na planta
float erro[dadosTotal],
saida[dadosTotal],
amostragem[dadosTotal],
amostragemi[5] = {30, 50, 70, 90, 100}, //período de amostragem
em ms
setpoint[] = {10, 20, 30, 40, 25, 60, 70, 80, 90, 100}, //setpoint em %
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
//Estruturas para o tempo inicial, tempo final e socket para comunicação
struct timeval begin, end;
struct sockaddr_in serv_addr;
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
int main(){
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
FILE *file = fopen("Teste9_0.csv", "w"); //Cria a planilha para armazenamento dos
dados dos testes
//Cria o socket para a comunicação TCP
if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
printf("\n Socket creation error \n"); //Caso falhe na criação do socket, printa
uma mensagem
return -1;
}
//define a porta e a família da comunicação TCP
serv_addr.sin_family = AF_INET;
serv_addr.sin_port = htons(PORT);
//define o IP para a comunicação
if (inet_pton(AF_INET, "10.2.0.36", &serv_addr.sin_addr) <= 0) {
printf("\nInvalid address/ Address not supported \n"); //caso não seja
possível comunicar com este IP, printa uma mensagem de erro
return -1;
}
//Conecta com o CLP comunicador
if ((status = connect(client_fd, (struct sockaddr*)&serv_addr, sizeof(serv_addr))) < 0) {
printf("\nConnection Failed \n"); //caso não seja
possível conectar printa uma mensagem de erro
return -1;
}
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
//zera a variável mensagem para que não sobre resquícios da última enviada devido ao
seu tamanho
for(int c = 0; c <40; c++){
mensagem[c]=0;
}
counter++; //conta o ciclo executado
//leitura e conversão para porcentagem
leituram = readVoltage(1);
A3_value = ((leituram-A3_min)/(A3_max-A3_min))*100;
//Monta os pacotes de acordo com o formato predefinido
strcpy(mensagem," ");
sprintf(A3_value_s, "%.3f", A3_value);
strcat(mensagem, "V");
strcat(mensagem, A3_value_s);
strcat(mensagem, "S");
sprintf(setpoint_s, "%.3f",setpoint[0]);
strcat(mensagem, setpoint_s);
strcat(mensagem, "I");
sprintf(counters, "%d",counter);
strcat(mensagem, counters);
strcat(mensagem, "f");
t0rasp = elapsedtime(); //marca o tempo de envio da mensagem
send(client_fd, mensagem, strlen(mensagem), 0); //envia a mensagem
read(client_fd, buffer, 2048); //recebe uma mensagem de resposta
t3rasp = elapsedtime(); //marca o tempo de recebimento da
resposta
//arruma o pacote de acordo com seu tamanho
for(int i = 0; i < 40;i++){
buffer[i] = buffer[i+2];
}
//Extrai e separa os dados recebidos
for (int i = 0; i < 4; i++) {
char *startPtr = strchr(buffer, chars_start[i]);
char *endPtr = strchr(buffer, chars_end[i]);
if (startPtr != NULL && endPtr != NULL && endPtr > startPtr) {
int start_index = startPtr - buffer + 1;
int end_index = endPtr - buffer - 1;
strncpy(extracted, buffer + start_index, end_index - start_index + 1);
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
} else
printf("Caracteres %c e %c não encontrados na string ou na ordem
correta.\n", chars_start[i], chars_end[i]); //Envia uma mensagem em
caso de erro na extração
}
out = (int)(atof(vazao)*(1023-209.37)/100)+209.37; //Escalona o sinal de
acordo com o range de saída da raspberry
pwmWrite (PWM_pin, out); //Envia um sinal para a planta de acordo com o
controle feito pelo CLP
//salva os dados em vetores para salvar na planilha
delay1[a*qtdDados+counter] = atoi(t1);
delay2[a*qtdDados+counter] = t3rasp - t0rasp - atoi(t2);
saida[a*qtdDados+counter] = atof(vazao);
erro[a*qtdDados+counter] = setpoint[0] - A3_value;
//contabiliza a perda de pacotes comparando o indice enviado ao recebido
//na comunicação TCP esta perda deve ser zero
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
#include <modbus/modbus.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#define dadosTotal 550
int perda = 0,
atual = 0,
out = 0,
qtdDados = dadosTotal/5,
a = 0,
sockfd,
counter = 0,
sockfds,
delay1[dadosTotal],
delay2[dadosTotal],
vazao,
losspkg[dadosTotal],
rc_read = 0,
rc_write = 0,
indice,
t1,
t2;
char A3_value_s[6],
setpoint_s[6],
counters[6],
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
amostragemi[5] = {30, 50, 70, 90, 100},
setpoint[] = {50, 20, 30, 40, 25, 60, 70, 80, 90, 100},
A3_min = 8350,
A3_max = 26500,
A3_value = 0,
leitura[5] = {0, 0, 0, 0, 0},
leituram;
//variaveis para marcar o tempo
double t0rasp = 0,
t3rasp = 0,
tatual = 0;
struct timeval begin, end;
/função para pegar o tempo em milissegundos em relação ao marco inicial
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
if (wiringPiSetup () == -1) //seta a biblioteca wiringPi para utilizar as
GPIOs
exit (1) ;
pinMode (PWM_pin, PWM_OUTPUT) ; //seta o pino PWM escolhido como saída
pinMode (chave1, OUTPUT) ; //seta o pino de envio de mensagem escolhido como
saída
pinMode (chave2, OUTPUT) ; //seta o pino de recebimento de mensagem
escolhido como saída
if(openI2CBus("/dev/i2c-1") == -1){ //abre o barramento I2C para a leitura do sinal
de entrada da planta convertido
return EXIT_FAILURE;
}
setI2CSlave(0x48); //Seta o dispositivo como escravo I2C para
recebimento de sinais
FILE *file = fopen("Teste11_300.csv", "w"); //Cria a planilha para armazenamento dos
dados dos testes
//Cria as variáreis para comunicação modbus: recebimento, envio, ip e porta
modbus_t *ctx;
uint16_t tab_dest[64];
uint16_t tab_src[10] = {1};
ctx = modbus_new_tcp("10.2.0.36", 502);
// Tenta conexão modbus
while(modbus_connect(ctx) == -1) {
printf("ERRO");
fprintf(stderr, "Connection failed: %s\n", modbus_strerror(errno));
}
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
//leitura e filtro de média móvel
leitura[0] = readVoltage(1);
leituram = 0;
for(int l = 1; l < 5; l++){
leitura[l]=leitura[l-1];
leituram = leitura[l] + leituram;
}
leituram = leituram/5;
A3_value = ((leituram-A3_min)/(A3_max-A3_min))*100;
//Monta os pacotes de acordo com os registradores predefinidos
tab_src[0] = A3_value;
tab_src[1] = setpoint[0];
tab_src[2] = a*qtdDados + counter;
tab_src[3] = 0;
tab_src[4] = 0;
tab_src[5] = 0;
t0rasp = elapsedtime(); //marca o tempo de
envio da mensagem
rc_write = modbus_write_registers(ctx, 32, 6, tab_src); //envia a mensagem
rc_read = modbus_read_input_registers(ctx, 0, 10, tab_dest);//recebe uma mensagem
de resposta
t3rasp = elapsedtime(); //marca o tempo de
recebimento da resposta
//Extrai os dados recebidos
vazao = tab_dest[0];
t1 = tab_dest[1];
t2 = tab_dest[3];
indice = tab_dest[5];
out = (int)(vazao*(1023-209.37)/100)+209.37; //Escalona o sinal de
acordo com o range de saída da raspberry
pwmWrite (PWM_pin, out); //Envia um sinal para a
planta de acordo com o controle feito pelo CLP
//salva os dados em vetores para salvar na planilha
delay1[a*qtdDados + counter] = t1;
delay2[a*qtdDados + counter] = (int) t3rasp - (int) t0rasp - t2;
saida[a*qtdDados + counter] = vazao;
erro[a*qtdDados + counter] = setpoint[0] - A3_value;
amostragem[a*qtdDados + counter] = 0;
//contabiliza a perda de pacotes comparando o indice enviado ao recebido
//comunicação Modbus esta perda deve ser zero
if(counter!= indice)
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
//limpa os recursos e fecha a conexão
modbus_close(ctx);
modbus_free(ctx);
digitalWrite(chave1, LOW);
digitalWrite(chave2, LOW);
//salva na planilha os dados do teste realizado
fprintf(file, "Delay1, Delay2, U, E, Perda, Amostragem \n");
for(int r = 1;r <= qtdDados*5; r++){
fprintf(file, "%d, %d, %f, %f, %d, %f \n", delay1[r], delay2[r], saida[r], erro[r],
losspkg[r], amostragem[r]);
}
return 0;
exit(0);
}
