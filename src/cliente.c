#include "cliente.h"

// Usando variávies globais das bibliotecas relacionadas
extern WINDOW *win_output, *win_input;
extern pthread_mutex_t win_input_mutex, win_output_mutex;

// Variáveis globais
int sock = 0;                       // Socket compartilhado entre as threads
int flag_finalizacao_cliente = 1;   // Flag para indicar que o programa deve ser encerrado
NameColor name_colors[100];
int name_colors_count = 0;

// Função para obter ou atribuir uma cor a um nome
int get_color_for_name(const char *name) {
    // Verifica se o nome já tem uma cor atribuída
    for (int i = 0; i < name_colors_count; i++) {
        if (strcmp(name_colors[i].name, name) == 0) {
            return name_colors[i].color_pair;
        }
    }
    // Se não, atribui uma nova cor
    int color_pair = (name_colors_count % MAX_COLORS) + 4; // Começa no par de cores 4
    strcpy(name_colors[name_colors_count].name, name);
    name_colors[name_colors_count].color_pair = color_pair;
    name_colors_count++;
    return color_pair;
}

// Função para enviar mensagens ao servidor
void* enviar_mensagens_cliente(void* arg) {
    char message[BUFFER_SIZE];  // Buffer para armazenar a mensagem a ser enviada
    int ch;
    int pos = 0;

    char input_buffer[BUFFER_SIZE];
    memset(input_buffer, 0, sizeof(input_buffer));

    // Configurar a janela de entrada para modo não bloqueante
    nodelay(win_input, TRUE);
    keypad(win_input, TRUE);
    noecho();  // Desabilita o eco para controlar a entrada manualmente

    while (flag_finalizacao_cliente) {
        // Ler entrada do usuário
        pthread_mutex_lock(&win_input_mutex);

        wmove(win_input, 0, 0);
        wclear(win_input);
        wattron(win_input, COLOR_PAIR(1));
        mvwprintw(win_input, 0, 0, "Você: %s", input_buffer);
        wattroff(win_input, COLOR_PAIR(1));
        wrefresh(win_input);

        pthread_mutex_unlock(&win_input_mutex);

        ch = wgetch(win_input);
        if (ch != ERR) {
            if (ch == '\n') {
                if (strlen(input_buffer) > 0) {
                    // Enviar mensagem ao servidor
                    strcat(input_buffer, "\n"); // Adiciona '\n' ao final
                    send(sock, input_buffer, strlen(input_buffer), 0);

                    // Verifica se a mensagem é "fim" para encerrar a conexão
                    if (strcmp(input_buffer, "fim\n") == 0 || strcmp(input_buffer, "fim") == 0) {
                        pthread_mutex_lock(&win_output_mutex);
                        wprintw(win_output, "Conexão finalizada pelo cliente.\n");
                        wrefresh(win_output);
                        pthread_mutex_unlock(&win_output_mutex);
                        flag_finalizacao_cliente = 0;
                        break;
                    }

                    // Exibir mensagem na janela de saída
                    pthread_mutex_lock(&win_output_mutex);
                    wattron(win_output, COLOR_PAIR(1)); // Cor do cliente local
                    wprintw(win_output, "Você: ");
                    wattroff(win_output, COLOR_PAIR(1));
                    wattron(win_output, COLOR_PAIR(7)); // Cor branca para mensagem
                    wprintw(win_output, "%s", input_buffer);
                    wattroff(win_output, COLOR_PAIR(7));
                    wrefresh(win_output);
                    pthread_mutex_unlock(&win_output_mutex);

                    memset(input_buffer, 0, sizeof(input_buffer));
                    pos = 0;
                }
            } else if (ch == KEY_BACKSPACE || ch == 127) {
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

    pthread_exit(0); // Encerra a thread
}

// Função para receber mensagens do servidor
void* receber_mensagens(void* arg) {
    char buffer[BUFFER_SIZE];  // Buffer para armazenar a mensagem recebida

    while (flag_finalizacao_cliente) {
        memset(buffer, 0, sizeof(buffer)); // Limpa o buffer

        // Ler dados do servidor até encontrar um '\n' (delimitador de mensagem)
        int bytes_received = 0;
        int nbytes;
        while (1) {
            nbytes = recv(sock, buffer + bytes_received, 1, 0);
            if(nbytes <= 0){
                perror("Erro ao receber mensagem");
                flag_finalizacao_cliente = 0;
                break;
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
        if (flag_finalizacao_cliente == 0)
            break;

        buffer[bytes_received] = '\0'; // Garante terminação da string

        // Atualizar a janela de saída
        pthread_mutex_lock(&win_output_mutex);

        // Salva o estado da janela de entrada
        pthread_mutex_lock(&win_input_mutex);
        char input_backup[BUFFER_SIZE];
        strcpy(input_backup, "");
        mvwinnstr(win_input, 0, 6, input_backup, BUFFER_SIZE - 1); // Captura o texto digitado
        pthread_mutex_unlock(&win_input_mutex);

        // Verifica se a mensagem é do servidor ou de um cliente
        char *colon_ptr = strchr(buffer, ':');
        if (colon_ptr != NULL) {
            // Separa o nome e a mensagem
            *colon_ptr = '\0';
            char *name = buffer;
            char *message = colon_ptr + 1;

            // Remove possíveis espaços em branco
            while (*message == ' ')
                message++;

            // Obtém a cor para o nome
            int color_pair = get_color_for_name(name);

            // Exibe o nome com a cor atribuída
            wattron(win_output, COLOR_PAIR(color_pair));
            wprintw(win_output, "%s: ", name);
            wattroff(win_output, COLOR_PAIR(color_pair));

            // Exibe a mensagem em branco
            wattron(win_output, COLOR_PAIR(7)); // Branco
            wprintw(win_output, "%s", message);
            wattroff(win_output, COLOR_PAIR(7));
        } else {
            // Mensagem geral
            wprintw(win_output, "%s", buffer);
        }

        wrefresh(win_output);

        // Restaura o estado da janela de entrada
        pthread_mutex_lock(&win_input_mutex);
        wmove(win_input, 0, 0);
        wclear(win_input);
        wattron(win_input, COLOR_PAIR(1));
        mvwprintw(win_input, 0, 0, "Você: %s", input_backup);
        wattroff(win_input, COLOR_PAIR(1));
        wrefresh(win_input);
        pthread_mutex_unlock(&win_input_mutex);

        pthread_mutex_unlock(&win_output_mutex);

        // Verifica se a mensagem é "fim" para encerrar a conexão
        if(strcmp(buffer, "fim\n") == 0 || strcmp(buffer, "fim") == 0) {
            flag_finalizacao_cliente = 0; // Define a flag para encerrar o programa
            break;
        }
    }

    pthread_exit(0); // Encerra a thread
}

// Função de configuração do socket do cliente
int config_cliente(){
    struct sockaddr_in serv_addr; // Estrutura de endereço do servidor

    // Criar o socket (IPv4, TCP)
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nErro na criação do socket\n");
        return -1;
    }

    // Definir endereço do servidor
    serv_addr.sin_family = AF_INET; // Família de endereços IPv4
    serv_addr.sin_port = htons(8080); // Porta 8080 (convertida para ordem de bytes de rede)

    // Converter o endereço IP para binário
    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) { // IP de localhost
        printf("\nEndereço inválido ou não suportado\n");
        return -1;
    }

    // Conectar ao servidor
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nFalha na conexão\n");
        return -1;
    }
    return 0;
}

// Função de coguração do terminal do cliente
void config_terminal_cliente(){
    // Inicializar o ncurses
    initscr(); // Inicializa a tela padrão
    cbreak(); // Desabilita o buffer de linha; obtém a entrada imediatamente
    keypad(stdscr, TRUE); // Habilita teclas especiais (setas, F1-F12)

    // Verificar se o terminal suporta cores
    if (has_colors() == FALSE) {
        endwin(); // Encerra o modo ncurses
        printf("Seu terminal não suporta cores.\n");
        close(sock);
        exit(1);
    }
    start_color(); // Inicia a funcionalidade de cores

    // Definir pares de cores
    init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Cliente local (Você)
    init_pair(2, COLOR_BLUE, COLOR_BLACK);   // Servidor
    init_pair(7, COLOR_WHITE, COLOR_BLACK);  // Mensagens em branco

    // Definir cores adicionais para outros clientes
    init_pair(4, COLOR_CYAN, COLOR_BLACK);
    init_pair(5, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(6, COLOR_YELLOW, COLOR_BLACK);
    // Pode adicionar mais pares de cores conforme necessário

    // Criar janelas para saída e entrada
    int height_output = LINES - 3; // Altura da janela de saída (deixa 3 linhas para a janela de entrada)
    int width = COLS; // Largura das janelas (largura total do terminal)
    win_output = newwin(height_output, width, 0, 0); // Janela de saída na posição (0,0)
    win_input = newwin(3, width, height_output, 0); // Janela de entrada na posição (height_output,0)
    scrollok(win_output, TRUE); // Permite scroll na janela de saída

    // Configurar a janela de entrada
    nodelay(win_input, FALSE); // Será ajustado na função de envio
    keypad(win_input, TRUE);
    echo();  // Habilita o eco na janela de entrada (desabilitado na função de envio)
}