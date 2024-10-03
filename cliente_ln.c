#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    const char *message = "Hello from client";

    // Criar o socket (IPv4, TCP)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    // Definir endereço do servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);

    // Converter o endereço IP para binário
    if (inet_pton(AF_INET, "172.18.192.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    // Conectar ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    // Enviar mensagem ao servidor
    send(sock, message, strlen(message), 0);
    printf("Mensagem enviada\n");

    // Receber mensagem do servidor
    read(sock, buffer, 1024);
    printf("Mensagem recebida: %s\n", buffer);

    // Fechar conexão
    close(sock);
    return 0;
}
