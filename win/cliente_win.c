#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>  // Biblioteca de sockets para Windows
#include <Ws2tcpip.h>  // Biblioteca para inet_pton e inet_ntop no Windows

#pragma comment(lib, "ws2_32.lib")  // Linkar a biblioteca Winsock

int main(int argc, char *argv[]) {
    WSADATA wsa;
    SOCKET sock;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    const char message[1024] = "Hello from client";

    // Verificar se o endereço IP foi fornecido
    if (argc != 2) {
        printf("Uso correto: %s <IP do servidor>\n", argv[0]);
        return 1;
    }

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
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);  // Recebe o IP do terminal

    // Conectar ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == SOCKET_ERROR) {
        printf("Conexão falhou. Código: %d\n", WSAGetLastError());
        closesocket(sock); // Fechar o socket se a conexão falhar
        WSACleanup();
        return 1;
    }
    printf("Conectado ao servidor no IP %s\n", argv[1]);

    while(1) {    
        printf("Cliente: ");
        fgets(message, sizeof(message), stdin);

        // Enviar mensagem ao servidor
        send(sock, message, strlen(message), 0);

        if(strcmp(message, "fim\n") == 0)
            break;
        
        // Receber resposta do servidor
        recv(sock, buffer, 1024, 0);
        printf("\nServidor: %s\n", buffer);
    }

    // Fechar conexão
    closesocket(sock);+
    WSACleanup();
    return 0;
}
