#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h> // Biblioteca de sockets para Windows

#pragma comment(lib,"ws2_32.lib") // Linkar a biblioteca Winsock

int main() {
    WSADATA wsa;
    SOCKET server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    char buffer[1024] = {0};
    const char message[1024] = "Hello from server";

    // Inicializar Winsock
    printf("Inicializando Winsock...\n");
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("Falha ao inicializar Winsock. Código: %d\n", WSAGetLastError());
        return 1;
    }

    // Criar socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID_SOCKET) {
        printf("Falha ao criar socket. Código: %d\n", WSAGetLastError());
        return 1;
    }
    printf("Socket criado.\n");

    // Definir endereço e porta
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    // Bindar o socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) == SOCKET_ERROR) {
        printf("Bind falhou. Código: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    printf("Bind concluído.\n");

    // Escutar por conexões
    if (listen(server_fd, 3) == SOCKET_ERROR) {
        printf("Falha ao escutar. Código: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    printf("Aguardando conexões...\n");

    // Aceitar conexão
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) == INVALID_SOCKET) {
        printf("Falha ao aceitar conexão. Código: %d\n", WSAGetLastError());
        closesocket(server_fd);
        WSACleanup();
        return 1;
    }
    printf("Conexão aceita.\n");

    int fim = 1;
    while(fim){
        memset(buffer, 0, sizeof(buffer));
        
        // Receber e enviar mensagens
        recv(new_socket, buffer, 1024, 0);
        printf("Cliente: %s\n", buffer);

        if(strcmp(buffer, "fim\n") == 0)
            fim = 0;

        printf("Servidor: ");
        fgets(message, sizeof(message), stdin);
        send(new_socket, message, strlen(message), 0);
    }


    // Fechar conexão
    closesocket(new_socket);
    closesocket(server_fd);
    WSACleanup();
    return 0;
}
