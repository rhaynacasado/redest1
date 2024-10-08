#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ncurses.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <time.h>

#define MAX_CLIENTES 10       // Número máximo de clientes suportados
#define BUFFER_SIZE 1024      // Tamanho do buffer para mensagens
#define RESPOSTA 7

int flag_finalizacao = 0;     // Flag para indicar que o servidor deve ser encerrado

// Estrutura para armazenar informações do cliente
typedef struct {
    int sock;                 // Socket do cliente
    char nome[50];            // Nome do cliente
} Cliente;

Cliente clientes[MAX_CLIENTES];          // Array de clientes conectados
int num_clientes = 0;                    // Número atual de clientes conectados
int cliente_dica = -1;                   // Variável global para armazenar o número do cliente que dará a dica

// Variável para armazenar o total de votos
int votos[MAX_CLIENTES];
int votos_recebidos = 0;
int dica_enviada = 0;
int jogo_iniciado = 0;

// Mutexes para sincronização
pthread_mutex_t clientes_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para o array de clientes
pthread_mutex_t win_mutex = PTHREAD_MUTEX_INITIALIZER;      // Mutex para a janela do ncurses
pthread_mutex_t win_input_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para a janela de entrada
pthread_mutex_t win_output_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para a janela de saída
pthread_mutex_t votos_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex para coordenar a votação
pthread_cond_t cond_votos = PTHREAD_COND_INITIALIZER;   // Mutex para coordenar a votação
pthread_mutex_t jogo_mutex = PTHREAD_MUTEX_INITIALIZER;    // Mutex para inicio do jogo
pthread_mutex_t dica_mutex = PTHREAD_MUTEX_INITIALIZER;   // Mutex para envio de dica

// Janelas do ncurses
WINDOW *win_output, *win_input;

// Função para transmitir mensagens a todos os clientes
void broadcast_message(const char *message, int sender_sock) {
    pthread_mutex_lock(&clientes_mutex);
    for (int i = 0; i < num_clientes; ++i) {
        if (clientes[i].sock != sender_sock) { // Não envia para o remetente
            if (send(clientes[i].sock, message, strlen(message), 0) < 0) {
                perror("Erro ao enviar mensagem");
            }
        }
    }
    pthread_mutex_unlock(&clientes_mutex);
}

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
// void coletar_votos(int client_sock, const char *voto, int resposta) {
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

// Função que lida com cada cliente conectado
void *handle_client(void *arg) {
    int client_sock = *((int *)arg);
    free(arg);
    char buffer[BUFFER_SIZE];
    int nbytes;
    char client_name[50];

    // Receber o nome do cliente
    memset(client_name, 0, sizeof(client_name));
    int name_len = 0;
    while (1) {
        nbytes = recv(client_sock, client_name + name_len, 1, 0);
        if (nbytes <= 0) {
            goto disconnect;
        }
        if (client_name[name_len] == '\n') {
            client_name[name_len] = '\0'; // Remove o '\n'
            break;
        } else {
            name_len++;
            if (name_len >= sizeof(client_name) - 1) {
                // Nome muito longo
                client_name[name_len] = '\0';
                break;
            }
        }
    }

    // Armazena o nome do cliente
    pthread_mutex_lock(&clientes_mutex);
    for (int i = 0; i < num_clientes; ++i) {
        if (clientes[i].sock == client_sock) {
            strncpy(clientes[i].nome, client_name, sizeof(clientes[i].nome) - 1);
            clientes[i].nome[sizeof(clientes[i].nome) - 1] = '\0';
            break;
        }
    }
    pthread_mutex_unlock(&clientes_mutex);

    // Exibe mensagem de novo cliente
    pthread_mutex_lock(&win_output_mutex);
    wattron(win_output, COLOR_PAIR(2)); // Servidor
    wprintw(win_output, "Novo cliente conectado: %s\n", client_name);
    wattroff(win_output, COLOR_PAIR(2));
    wrefresh(win_output);
    pthread_mutex_unlock(&win_output_mutex);

    // Notifica outros clientes sobre o novo cliente
    char join_message[BUFFER_SIZE];
    snprintf(join_message, sizeof(join_message), "Servidor: %s entrou no chat.\n", client_name);
    broadcast_message(join_message, -1);

    while (1) {
        // Ler dados do cliente até encontrar um '\n' (delimitador de mensagem)
        int bytes_received = 0;
        while (1) {
            nbytes = recv(client_sock, buffer + bytes_received, 1, 0);
            if (nbytes <= 0) {
                // O cliente desconectou ou ocorreu um erro
                goto disconnect;
            }
            if (buffer[bytes_received] == '\n') {
                bytes_received++;
                break;
            } else {
                bytes_received++;
                if (bytes_received >= BUFFER_SIZE - 1) {
                    // Evitar estouro de buffer
                    break;
                }
            }
        }
        buffer[bytes_received] = '\0'; // Termina a string recebida

        // Verifica se o cliente enviou o comando "iniciar jogo"
        pthread_mutex_lock(&jogo_mutex);
        if (strcmp(buffer, "iniciar rodada\n") == 0 || strcmp(buffer, "iniciar rodada") == 0) {
            if (num_clientes >= 2 && jogo_iniciado == 0) {
                jogo_iniciado = 1; // Marca que o jogo foi iniciado
                escolher_cliente_dica();  // Escolhe o cliente para dar a dica
            } else if (jogo_iniciado == 0){
                // Caso não tenha clientes suficientes
                char erro_msg[BUFFER_SIZE];
                snprintf(erro_msg, sizeof(erro_msg), "Não é possível iniciar o jogo. Necessário pelo menos 2 jogadores.\n");
                send(client_sock, erro_msg, strlen(erro_msg), 0);  // Envia mensagem de erro ao cliente
            } else {
                // Caso o jogo já tenha começado
                char erro_msg[BUFFER_SIZE];
                snprintf(erro_msg, sizeof(erro_msg), "O jogo já está em andamento.\n");
                send(client_sock, erro_msg, strlen(erro_msg), 0);  // Envia mensagem de erro ao cliente
            }
        }
        pthread_mutex_unlock(&jogo_mutex);

        int resposta;
        pthread_mutex_lock(&dica_mutex);
        // pthread_mutex_lock(&clientes_mutex);
        // Se for o cliente da dica, compartilhe a dica
        if (client_sock == clientes[cliente_dica].sock && dica_enviada == 0) {
            char dica_msg[BUFFER_SIZE];
            snprintf(dica_msg, sizeof(dica_msg), "Servidor: %s deu uma dica: %s", client_name, buffer);
            // pthread_mutex_unlock(&clientes_mutex);
            broadcast_message(dica_msg, -1);
            dica_enviada = 1; 
        } else if (dica_enviada){
            resposta = validar_resposta(client_sock, buffer);
            if(resposta >= 0)
                coletar_votos(client_sock, resposta, RESPOSTA); // Outros clientes votam
        }
        // pthread_mutex_unlock(&clientes_mutex);
        pthread_mutex_unlock(&dica_mutex);

        // Verifica se o cliente enviou "fim" para encerrar a conexão
        if (strcmp(buffer, "fim\n") == 0 || strcmp(buffer, "fim") == 0) {
            goto disconnect;
        }
    
        pthread_mutex_lock(&dica_mutex);
        if(dica_enviada != 1 || resposta != -2){
            // Exibe a mensagem recebida na janela de saída com cor
            pthread_mutex_lock(&win_output_mutex);
            wattron(win_output, COLOR_PAIR(1)); // Ativa o par de cores 1 (Cliente)
            wprintw(win_output, "%s: %s", client_name, buffer);
            wattroff(win_output, COLOR_PAIR(1)); // Desativa o par de cores 1
            wrefresh(win_output);
            pthread_mutex_unlock(&win_output_mutex);

            // Prepara a mensagem para transmitir aos outros clientes
            char message_to_send[BUFFER_SIZE + 100]; // Ajuste do tamanho do buffer
            snprintf(message_to_send, sizeof(message_to_send), "%s: %s", client_name, buffer);

            // Transmite a mensagem para os outros clientes
            broadcast_message(message_to_send, client_sock);
        }
        pthread_mutex_unlock(&dica_mutex);
    }

disconnect:
    // Cliente desconectado, remove-o da lista
    close(client_sock);
    pthread_mutex_lock(&clientes_mutex);
    char client_left_message[BUFFER_SIZE];
    memset(client_left_message, 0, sizeof(client_left_message));
    for (int i = 0; i < num_clientes; ++i) {
        if (clientes[i].sock == client_sock) {
            // Notifica outros clientes sobre a saída
            snprintf(client_left_message, sizeof(client_left_message), "Servidor: %s saiu do chat.\n", clientes[i].nome);
            // Remove o cliente do array
            for (int j = i; j < num_clientes - 1; ++j) {
                clientes[j] = clientes[j + 1];
            }
            num_clientes--;
            break;
        }
    }
    pthread_mutex_unlock(&clientes_mutex);

    // Exibe mensagem de desconexão
    pthread_mutex_lock(&win_output_mutex);
    wprintw(win_output, "%s desconectado.\n", client_name);
    wrefresh(win_output);
    pthread_mutex_unlock(&win_output_mutex);

    // Envia a mensagem de saída para os outros clientes
    broadcast_message(client_left_message, -1);

    pthread_exit(NULL);
}

// Função para aceitar novos clientes
void *accept_clients(void *arg) {
    int server_fd = *((int *)arg);
    int new_sock;
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    while (!flag_finalizacao) {
        // Aceitar conexão do cliente
        new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (new_sock < 0) {
            if (flag_finalizacao) break;
            perror("Erro no accept");
            continue;
        }

        // Adiciona o novo cliente à lista
        pthread_mutex_lock(&clientes_mutex);
        if (num_clientes < MAX_CLIENTES) {
            clientes[num_clientes].sock = new_sock;
            strcpy(clientes[num_clientes].nome, ""); // Nome será recebido posteriormente
            num_clientes++;

            // Cria uma thread para lidar com o novo cliente
            pthread_t tid;
            int *pclient = malloc(sizeof(int));
            if (pclient == NULL) {
                perror("Erro ao alocar memória");
                close(new_sock);
                pthread_mutex_unlock(&clientes_mutex);
                continue;
            }
            *pclient = new_sock;
            if (pthread_create(&tid, NULL, handle_client, pclient) != 0) {
                perror("Erro ao criar thread para o cliente");
                free(pclient);
                close(new_sock);
                num_clientes--;
                pthread_mutex_unlock(&clientes_mutex);
                continue;
            }
            pthread_detach(tid); // Opcional: libera recursos da thread ao terminar
        } else {
            // Limite de clientes atingido
            close(new_sock);
            pthread_mutex_lock(&win_output_mutex);
            wprintw(win_output, "Cliente recusado: limite de conexões atingido.\n");
            wrefresh(win_output);
            pthread_mutex_unlock(&win_output_mutex);
        }
        pthread_mutex_unlock(&clientes_mutex);
    }
    pthread_exit(NULL);
}

// Função para enviar mensagens do servidor para todos os clientes
void *enviar_mensagens(void *arg) {
    char message[BUFFER_SIZE];

    // Configurar a janela de entrada para modo não bloqueante
    nodelay(win_input, TRUE);
    keypad(win_input, TRUE);
    noecho();  // Desabilita o eco para controlar a entrada manualmente

    char input_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, sizeof(input_buffer));
    int pos = 0;
    int ch;

    while (!flag_finalizacao) {
        // Ler entrada do usuário
        pthread_mutex_lock(&win_input_mutex);

        wmove(win_input, 0, 0);
        wclear(win_input);
        wattron(win_input, COLOR_PAIR(2));
        mvwprintw(win_input, 0, 0, "Servidor: %s", input_buffer);
        wattroff(win_input, COLOR_PAIR(2));
        wrefresh(win_input);

        pthread_mutex_unlock(&win_input_mutex);

        ch = wgetch(win_input);
        if (ch != ERR) {
            if (ch == '\n') {
                if (strlen(input_buffer) > 0) {
                    // Adicionar '\n' ao final da mensagem se não tiver
                    if (input_buffer[strlen(input_buffer) - 1] != '\n') {
                        strcat(input_buffer, "\n");
                    }

                    if (strcmp(input_buffer, "fim\n") == 0 || strcmp(input_buffer, "fim") == 0) {
                        flag_finalizacao = 1; // Define a flag para encerrar o servidor
                        break;
                    }

                    // Exibir mensagem na janela de saída com cor
                    pthread_mutex_lock(&win_output_mutex);
                    wattron(win_output, COLOR_PAIR(2)); // Ativa o par de cores 2 (Servidor)
                    wprintw(win_output, "Servidor: %s", input_buffer);
                    wattroff(win_output, COLOR_PAIR(2)); // Desativa o par de cores 2
                    wrefresh(win_output);
                    pthread_mutex_unlock(&win_output_mutex);

                    // Transmitir a mensagem para todos os clientes
                    char message_to_send[BUFFER_SIZE + 10];
                    snprintf(message_to_send, sizeof(message_to_send), "Servidor: %s", input_buffer);
                    broadcast_message(message_to_send, -1); // -1 indica que é mensagem do servidor

                    memset(input_buffer, 0, sizeof(input_buffer));
                    pos = 0;
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == 8) {
                if (pos > 0) {
                    input_buffer[--pos] = '\0';
                }
            } else if (pos < BUFFER_SIZE - 2 && isprint(ch)) {
                input_buffer[pos++] = ch;
                input_buffer[pos] = '\0';
            }
        } else {
            usleep(10000); // Espera um pouco para evitar alto uso de CPU
        }
    }
    pthread_exit(NULL);
}

int main() {
    int server_fd;
    struct sockaddr_in server_addr;

    // Criar o socket do servidor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Erro na criação do socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o socket para reutilizar o endereço e porta
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("Erro no setsockopt");
        exit(EXIT_FAILURE);
    }

    // Definir endereço e porta
    server_addr.sin_family = AF_INET; // IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY; // Qualquer endereço
    server_addr.sin_port = htons(8080); // Porta 8080

    // Vincular o socket ao endereço e porta
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Erro no bind");
        exit(EXIT_FAILURE);
    }

    // Escutar por conexões
    if (listen(server_fd, MAX_CLIENTES) < 0) {
        perror("Erro no listen");
        exit(EXIT_FAILURE);
    }

    // Inicializar o ncurses
    initscr();
    cbreak();
    keypad(stdscr, TRUE);

    // Verificar se o terminal suporta cores
    if (!has_colors()) {
        endwin();
        printf("Seu terminal não suporta cores.\n");
        exit(1);
    }
    start_color();

    // Definir pares de cores
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // Cliente
    init_pair(2, COLOR_BLUE, COLOR_BLACK);  // Servidor

    // Criar janelas
    int height_output = LINES - 3;
    int width = COLS;
    win_output = newwin(height_output, width, 0, 0);
    win_input = newwin(3, width, height_output, 0);
    scrollok(win_output, TRUE);

    // Exibir mensagem inicial
    pthread_mutex_lock(&win_output_mutex);
    wattron(win_output, COLOR_PAIR(2));
    wprintw(win_output, "Servidor iniciado. Aguardando conexões...\n");
    wattroff(win_output, COLOR_PAIR(2));
    wrefresh(win_output);
    pthread_mutex_unlock(&win_output_mutex);

    // Thread para aceitar novos clientes
    pthread_t accept_thread;
    if (pthread_create(&accept_thread, NULL, accept_clients, &server_fd) != 0) {
        perror("Erro ao criar thread de accept");
        endwin();
        exit(1);
    }

    // Thread para enviar mensagens do servidor
    pthread_t send_thread;
    if (pthread_create(&send_thread, NULL, enviar_mensagens, NULL) != 0) {
        perror("Erro ao criar thread de envio");
        endwin();
        exit(1);
    }

    // Aguardar o término da thread de envio (quando o servidor digita "fim")
    pthread_join(send_thread, NULL);

    // Encerrar o servidor
    flag_finalizacao = 1;

    // Fechar todos os sockets de clientes
    pthread_mutex_lock(&clientes_mutex);
    for (int i = 0; i < num_clientes; ++i) {
        close(clientes[i].sock);
    }
    num_clientes = 0;
    pthread_mutex_unlock(&clientes_mutex);

    // Fechar o socket do servidor
    close(server_fd);

    // Limpar e finalizar o ncurses
    pthread_mutex_destroy(&clientes_mutex);
    pthread_mutex_destroy(&votos_mutex);
    pthread_cond_destroy(&cond_votos);
    pthread_mutex_destroy(&win_mutex);
    pthread_mutex_destroy(&win_input_mutex);
    pthread_mutex_destroy(&win_output_mutex);
    delwin(win_output);
    delwin(win_input);
    endwin();

    return 0;
}
