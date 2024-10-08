#ifndef SERVIDOR_H
#define SERVIDOR_H

#include "socket.h"
#include <string.h>

#define MAX_CLIENTES 10       // Número máximo de clientes suportados

// Estrutura para armazenar informações do cliente
typedef struct {
    int sock;                 // Socket do cliente
    char nome[50];            // Nome do cliente
} Cliente;

void broadcast_message(const char *message, int sender_sock);
void *enviar_mensagens_servidor(void *arg); // Função para enviar mensagens do servidor para todos os clientes
int config_servidor();
void config_terminal_servidor();

#endif