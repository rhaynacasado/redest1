#ifndef WAVELENGTH_H
#define WAVELENGTH_H

#include "servidor.h"

void rand_escala();    // Função para definir a escala da rodada
int escolher_cliente_dica();    // Função para escolher um cliente aleatório para dar a dica
int validar_voto(int client_sock, const char *msg);    // Função para validar as respostas no formato "Resposta: X"
void coletar_votos(int client_sock, int voto, int resposta);    // Função que lida com a coleta de votos

#endif