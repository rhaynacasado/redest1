#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>  // Biblioteca de sockets para Windows
#include <Ws2tcpip.h>  // Biblioteca para inet_pton e inet_ntop no Windows

#pragma comment(lib, "ws2_32.lib")  // Linkar a biblioteca Winsock

int main() {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    const char *message = "Hello from client";

    // Inicializar Winsock
    printf("Inicializando Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Falha ao inicializar Winsock. Código: %d\n", WSAGetLastError());
        return 1;
    }

    // Criar socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Falha ao criar socket. Código: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket criado.\n");

    // Definir endereço do servidor
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    serv_addr.sin_addr.s_addr = inet_addr("172.26.0.1");  // Usando inet_addr para o IP

    // Conectar ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Conexão falhou. Código: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Conectado ao servidor.\n");

    // Enviar mensagem ao servidor
    send(sock, message, strlen(message), 0);
    printf("Mensagem enviada ao servidor.\n");

    // Receber resposta do servidor
    recv(sock, buffer, 1024, 0);
    printf("Mensagem recebida do servidor: %s\n", buffer);

    // Fechar conexão
    closesocket(sock);
    WSACleanup();
    return 0;
}
