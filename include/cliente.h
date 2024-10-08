#ifndef CLIENTE_H
#define CLIENTE_H

#include "socket.h"

#define MAX_COLORS 6     // Número máximo de cores para os clientes

// Mapeamento de nomes para cores
typedef struct {
    char name[50];
    int color_pair;
} NameColor;

int get_color_for_name(const char *name); // Função para obter ou atribuir uma cor a um nome
void* enviar_mensagens_cliente(void* arg); // Função para enviar mensagens ao servidor
void* receber_mensagens(void* arg); // Função para receber mensagens do servidor
int config_cliente();
void config_terminal_cliente();

#endif