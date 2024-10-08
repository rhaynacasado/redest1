#include "servidor.h"

extern WINDOW *win_output, *win_input;
extern pthread_mutex_t win_input_mutex, win_output_mutex;

int flag_finalizacao_servidor = 0;     // Flag para indicar que o servidor deve ser encerrado
Cliente clientes[MAX_CLIENTES];          // Array de clientes conectados
int num_clientes = 0;                    // Número atual de clientes conectados

// Mutexes para sincronização
pthread_mutex_t clientes_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para o array de clientes
pthread_mutex_t win_mutex = PTHREAD_MUTEX_INITIALIZER;      // Mutex para a janela do ncurses

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

// Função para enviar mensagens do servidor para todos os clientes
void *enviar_mensagens_servidor(void *arg) {
    char message[BUFFER_SIZE];
    int ch;
    int pos = 0;

    char input_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, sizeof(input_buffer));

    // Configurar a janela de entrada para modo não bloqueante
    nodelay(win_input, TRUE);
    keypad(win_input, TRUE);
    noecho();  // Desabilita o eco para controlar a entrada manualmente

    while (!flag_finalizacao_servidor) {
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
                        flag_finalizacao_servidor = 1; // Define a flag para encerrar o servidor
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

int config_servidor(){
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

    return server_fd;
}

void config_terminal_servidor(){
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
}