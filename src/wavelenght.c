#include "wavelenght.h"

extern int flag_finalizacao_servidor, num_clientes;
extern Cliente clientes[MAX_CLIENTES];
extern pthread_mutex_t clientes_mutex, win_mutex;

int cliente_dica = -1;                   // Variável global para armazenar o número do cliente que dará a dica

// Variável para armazenar o total de votos
int votos[MAX_CLIENTES];
int votos_recebidos = 0;
int dica_enviada = 0;
int jogo_iniciado = 0;

// Mutexes para sincronização
pthread_mutex_t votos_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex para coordenar a votação
pthread_cond_t cond_votos = PTHREAD_COND_INITIALIZER;   // Mutex para coordenar a votação
pthread_mutex_t jogo_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex para inicio do jogo
pthread_mutex_t dica_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex para envio de dica

// Função para escolher um cliente aleatório para dar a dica
void escolher_cliente_dica() {
    srand(time(NULL));
    cliente_dica = rand() % num_clientes;
    char msg[BUFFER_SIZE];
    pthread_mutex_lock(&clientes_mutex);
    snprintf(msg, sizeof(msg), "Servidor: Jogo iniciado!\nServidor: O cliente %s deve dar uma dica!\n", clientes[cliente_dica].nome);
    pthread_mutex_unlock(&clientes_mutex);
    broadcast_message(msg, -1); // Notifica todos

    pthread_mutex_lock(&votos_mutex);
    memset(votos, 0, sizeof(votos)); // Limpa vetor de votos
    pthread_mutex_unlock(&votos_mutex);
}

// Função para validar as respostas no formato "Resposta: X"
int validar_resposta(int client_sock, const char *msg) {
    char prefixo[] = "Resposta: ";
    int numero;

    // Verifica se a mensagem começa com "Resposta: "
    if (strncmp(msg, prefixo, strlen(prefixo)) == 0) {
        // Extrai o número da resposta
        if (sscanf(msg + strlen(prefixo), "%d", &numero) == 1) {
            if(numero < 0 || numero > 10){
                char erro_msg[BUFFER_SIZE];
                snprintf(erro_msg, sizeof(erro_msg), "Resposta inválida. Responda com um número entre 0 e 10.\n");
                send(client_sock, erro_msg, strlen(erro_msg), 0);  // Envia mensagem de erro ao cliente
                return -2;
            }
            return numero;  // Retorna o número se estiver no formato correto
        }
    }
    return -1;  // Retorna -1 se a mensagem não estiver no formato correto
}

// Função que lida com a coleta de votos
void coletar_votos(int client_sock, int voto, int resposta) {
    pthread_mutex_lock(&votos_mutex);
    votos_recebidos++;
    votos[client_sock] = voto; // Armazena o voto do cliente
    pthread_mutex_unlock(&votos_mutex);

    if (votos_recebidos == num_clientes - 1) { // Se todos votaram, processa os resultados
        char resultado[BUFFER_SIZE];
        snprintf(resultado, sizeof(resultado), "Todos votaram!\n");
        broadcast_message(resultado, -1);

        int resultado_final = 0;
        for(int i = 0; i < num_clientes; i++){
            pthread_mutex_lock(&clientes_mutex);
            resultado_final += votos[clientes[i].sock];
            pthread_mutex_unlock(&clientes_mutex);
        }
        resultado_final = resultado_final/(num_clientes-1);

        char msg_final[BUFFER_SIZE];
        if(resultado_final == resposta)
            snprintf(msg_final, sizeof(msg_final), "R: %d. Vocês acertaram! A resposta era %d.\n", resultado_final, resposta);
        else
            snprintf(msg_final, sizeof(msg_final), "R: %d. Não foi dessa vez! A resposta era %d.\n", resultado_final, resposta);
        broadcast_message(msg_final, -1);
        
        // Resetar para nova rodada
        votos_recebidos = 0;
        cliente_dica = -1;
        jogo_iniciado = 0;
        pthread_mutex_lock(&votos_mutex);
        memset(votos, 0, sizeof(votos));
        pthread_mutex_unlock(&votos_mutex);
        // escolher_cliente_dica(); // Inicia nova rodada
    }
}

