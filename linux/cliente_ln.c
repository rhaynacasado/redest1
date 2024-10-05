#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <arpa/inet.h>

int sock = 0;  // socket será compartilhado entre as threads
int flag_finalizacao = 1;   // flag para indicar que uma das threads finalizou (programa cliente deve ser encerrado)

// Função para enviar mensagens ao servidor
void* enviar_mensagens(void* arg) {
    char message[1024];

    while (1) {
        // Verifica se o programa deve ser finalizado
        if(flag_finalizacao == 0)
            break;

        // Digitar a mensagem que será enviada ao servidor
        printf("Cliente: ");
        fgets(message, sizeof(message), stdin);

        // Enviar mensagem ao servidor
        send(sock, message, strlen(message), 0);

        // Se a mensagem enviada for "fim" o programa é encerrado
        if (strcmp(message, "fim\n") == 0) {
            printf("Conexão finalizada pelo cliente.\n");
            flag_finalizacao = 0;
            break;
        }
    }

    pthread_exit(0);
}

// Função para receber mensagens do servidor
void* receber_mensagens(void* arg) {
    char buffer[1024] = {0};    // inicializa o buffer

    while (1) {
        // Verifica se o programa deve ser finalizado
        if(flag_finalizacao == 0)
            break;

        memset(buffer, 0, sizeof(buffer));  // limpa o buffer

        // Receber resposta do servidor
        if(read(sock, buffer, 1024) < 0){
            perror("Erro ao receber mensagem");
            break;
        }
        printf("\nServidor: %s\n", buffer);

        if(strcmp(buffer, "fim\n") == 0) {
            flag_finalizacao = 0;
            break;
        }
    }

    pthread_exit(0);
}



int main() {
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char message[1024] = "Hello from client";

    // Criar o socket (IPv4, TCP)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    // Definir endereço do servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Converter o endereço IP para binário
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Conectar ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }
    printf("\nConectado ao servidor!\n");

    // // Receber mensagem do servidor
    // read(sock, buffer, 1024);
    // printf("Mensagem recebida: %s\n", buffer);

    // while(1) {    
    //     memset(buffer, 0, sizeof(buffer));

    //     printf("Cliente: ");
    //     fgets(message, sizeof(message), stdin);

    //     // Enviar mensagem ao servidor
    //     send(sock, message, strlen(message), 0);

    //     if(strcmp(message, "fim\n") == 0)
    //         break;
        
    //     // Receber resposta do servidor
    //     read(sock, buffer, 1024);
    //     printf("\nServidor: %s\n", buffer);
    // }    
    
    // Criando as threads
    pthread_t threads[2];
    if(pthread_create(&threads[0], 0, (void *) receber_mensagens, 0) != 0){
        printf("Erro ao criar thread que recebe mensagens\n");
        exit(1);
    }
    if(pthread_create(&threads[1], 0, (void *) enviar_mensagens, 0) != 0){
        printf("Erro ao criar thread que envia mensagens\n");
        exit(1);
    }

    // Aguardar que qualquer uma das threads termine
    pthread_join(threads[0], NULL);
    pthread_join(threads[1], NULL);

    // Fechar conexão
    close(sock);
    return 0;
}
