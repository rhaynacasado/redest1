#include "wavelength.h"

// Usando variávies globais das bibliotecas relacionadas
extern int flag_finalizacao_servidor, num_clientes;
extern Cliente clientes[MAX_CLIENTES];
extern pthread_mutex_t clientes_mutex, win_mutex;

int cliente_dica = -1;    // Variável global para armazenar o número do cliente que dará a dica
int nota = -1;            // Variável global 

// Variável para armazenar o total de votos
int votos[MAX_CLIENTES];
int votos_recebidos = 0;
int dica_enviada = 0;
int jogo_iniciado = 0;

// Mutexes para sincronização
pthread_mutex_t votos_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex para coordenar a votação
pthread_cond_t cond_votos = PTHREAD_COND_INITIALIZER;     // Mutex para coordenar a votação
pthread_mutex_t jogo_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex para inicio do jogo
pthread_mutex_t dica_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex para envio de dica

// Função para definir a escala da rodada
void rand_escala(){
    const char* escalas[100][2] = {     // Matriz de escalas
        {"Quente", "Frio"},
        {"Velho", "Novo"},
        {"Rápido", "Devagar"},
        {"Claro", "Escuro"},
        {"Grande", "Pequeno"},
        {"Famoso", "Desconhecido"},
        {"Barato", "Caro"},
        {"Saboroso", "Insosso"},
        {"Alto", "Baixo"},
        {"Difícil", "Fácil"},
        {"Inteligente", "Tolo"},
        {"Perigoso", "Seguro"},
        {"Doce", "Salgado"},
        {"Limpo", "Sujo"},
        {"Pesado", "Leve"},
        {"Comum", "Raro"},
        {"Importante", "Irrelevante"},
        {"Sério", "Engraçado"},
        {"Natural", "Artificial"},
        {"Forte", "Fraco"},
        {"Antigo", "Moderno"},
        {"Luxuoso", "Simples"},
        {"Confuso", "Claro"},
        {"Barulhento", "Silencioso"},
        {"Agradável", "Desagradável"},
        {"Frágil", "Resistente"},
        {"Curto", "Longo"},
        {"Alto (som)", "Baixo (som)"},
        {"Cedo", "Tarde"},
        {"Simples", "Complexo"},
        {"Colorido", "Monocromático"},
        {"Cheio", "Vazio"},
        {"Rico", "Pobre"},
        {"Normal", "Estranho"},
        {"Animado", "Tedioso"},
        {"Pessimista", "Otimista"},
        {"Lento", "Rápido"},
        {"Solitário", "Social"},
        {"Frio (personalidade)", "Acolhedor"},
        {"Motivado", "Desmotivado"},
        {"Jovem", "Idoso"},
        {"Culto", "Ignorante"},
        {"Energético", "Exausto"},
        {"Sensível", "Insensível"},
        {"Carinhoso", "Distante"},
        {"Verdadeiro", "Falso"},
        {"Orgulhoso", "Humilde"},
        {"Aventureiro", "Conservador"},
        {"Moderno", "Clássico"},
        {"Extrovertido", "Introvertido"},
        {"Inteligível", "Incompreensível"},
        {"Relaxante", "Estressante"},
        {"Educado", "Rude"},
        {"Bem-sucedido", "Fracassado"},
        {"Adaptável", "Rígido"},
        {"Chamativo", "Discreto"},
        {"Popular", "Niche"},
        {"Sincero", "Fingido"},
        {"Estável", "Instável"},
        {"Convincente", "Dúbio"},
        {"Sensato", "Impulsivo"},
        {"Justo", "Injusto"},
        {"Familiar", "Estranho"},
        {"Fértil", "Árido"},
        {"Generoso", "Egoísta"},
        {"Criativo", "Literal"},
        {"Saudável", "Doente"},
        {"Verdade", "Mentira"},
        {"Simpático", "Irritante"},
        {"Divertido", "Enfadado"},
        {"Flexível", "Inflexível"},
        {"Inteligente", "Burrice"},
        {"Normal", "Bizarro"},
        {"Público", "Privado"},
        {"Aberto", "Fechado"},
        {"Curioso", "Indiferente"},
        {"Honesto", "Desonesto"},
        {"Confiante", "Inseguro"},
        {"Importante", "Insignificante"},
        {"Livre", "Restrito"},
        {"Inspirador", "Desmotivador"},
        {"Elegante", "Desleixado"},
        {"Saudável", "Não saudável"},
        {"Quente (personalidade)", "Frio"},
        {"Teimoso", "Flexível"},
        {"Convencional", "Inconvencional"},
        {"Organizado", "Desorganizado"},
        {"Tímido", "Desinibido"},
        {"Moderado", "Extremista"},
        {"Pesado (assunto)", "Leve"},
        {"Amoroso", "Indiferente"},
        {"Direto", "Sutil"},
        {"Criativo", "Racional"},
        {"Inovador", "Tradicional"},
        {"Compreensivo", "Julgador"},
        {"Realista", "Sonhador"},
        {"Rebelde", "Obediente"},
        {"Sério", "Brincalhão"},
        {"Ousado", "Cauteloso"},
        {"Pragmático", "Idealista"}
    };

    int i = rand() % 100;   // Define escala da rodada aleatoriamente

    // Manda mensagem para todos os clientes informando a escala da rodada
    char escala_msg[BUFFER_SIZE];
    snprintf(escala_msg, sizeof(escala_msg), "Servidor: A escala da rodada é %s (0) - %s (10)!\n", escalas[i][0], escalas[i][1]);
    broadcast_message(escala_msg, -1);
}

// Função para escolher um cliente aleatório para dar a dica
int escolher_cliente_dica() {
    srand(time(NULL));  // Seed
    rand_escala();      // Determina escala

    // Define jogador mestre aleatoriamente e avisa a todos os jogadores
    cliente_dica = rand() % num_clientes;
    char msg[BUFFER_SIZE];
    pthread_mutex_lock(&clientes_mutex);
    snprintf(msg, sizeof(msg), "Servidor: O jogador %s deve dar uma dica!\n", clientes[cliente_dica].nome);
    pthread_mutex_unlock(&clientes_mutex);
    broadcast_message(msg, -1);

    // Garante que não há residuos de votações anteriores
    pthread_mutex_lock(&votos_mutex);
    memset(votos, 0, sizeof(votos));    // Limpa vetor de votos
    pthread_mutex_unlock(&votos_mutex);

    // Determina a nota da rodada aleatoriamente e avisa ao jogador mestre
    nota = rand() % 11;
    char nota_msg[BUFFER_SIZE];
    snprintf(nota_msg, sizeof(nota_msg), "Servidor: %s a nota da rodada é %d. Dê uma dica correspondente!\n", clientes[cliente_dica].nome, nota);
    send(clientes[cliente_dica].sock, nota_msg, strlen(nota_msg), 0);

    return(nota);
}

// Função para validar as respostas no formato "resposta - X"
int validar_voto(int client_sock, const char *msg) {
    char prefixo[] = "resposta - ";     // Formatação aceita para considerar como voto
    int numero;

    // Verifica se a mensagem começa com "resposta - "
    if (strncmp(msg, prefixo, strlen(prefixo)) == 0) {
        // Extrai o número do voto
        if (sscanf(msg + strlen(prefixo), "%d", &numero) == 1) {
            if(numero < 0 || numero > 10){  // Verifica se o número está no padrão (0 a 10)
                char erro_msg[BUFFER_SIZE];
                snprintf(erro_msg, sizeof(erro_msg), "Servidor: Resposta inválida. Responda com um número entre 0 e 10.\n");
                send(client_sock, erro_msg, strlen(erro_msg), 0);  // Envia mensagem de erro ao cliente
                return -2;
            }
            return numero;  // Retorna o número se estiver no formato correto
        }
    }
    return -1;  // Retorna -1 se a mensagem não estiver no formato correto
}

// Função que lida com a coleta de votos
void coletar_votos(int client_sock, int voto, int nota) {
    // Armazena o voto do jogador
    pthread_mutex_lock(&votos_mutex);
    votos_recebidos++;
    votos[client_sock] = voto;  
    pthread_mutex_unlock(&votos_mutex);

    if (votos_recebidos == num_clientes - 1) { // Se todos votaram, processa os resultados
        char msg[BUFFER_SIZE];
        snprintf(msg, sizeof(msg), "Servidor: Todos votaram!\n");
        broadcast_message(msg, -1);

        // Calcula a média entre os votos        
        int resultado = 0;
        for(int i = 0; i < num_clientes; i++){
            pthread_mutex_lock(&clientes_mutex);
            resultado += votos[clientes[i].sock];
            pthread_mutex_unlock(&clientes_mutex);
        }
        resultado = resultado/(num_clientes-1);

        // Verifica se o resultado corresponde à nota atribuída na rodada
        char msg_final[BUFFER_SIZE];
        if(resultado == nota)
            snprintf(msg_final, sizeof(msg_final), "Servidor: Vocês acertaram! A resposta era %d.\n", resultado);
        else
            snprintf(msg_final, sizeof(msg_final), "Servidor: Não foi dessa vez! Vocês marcaram %d, mas era %d.\n", resultado, nota);
        broadcast_message(msg_final, -1);
        
        // Resetar parâmetros para nova rodada
        votos_recebidos = 0;
        cliente_dica = -1;
        dica_enviada = 0;
        pthread_mutex_lock(&votos_mutex);
        memset(votos, 0, sizeof(votos));
        pthread_mutex_unlock(&votos_mutex);

        escolher_cliente_dica(); // Inicia nova rodada
    }
}