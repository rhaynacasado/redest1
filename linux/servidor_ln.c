#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <ncurses.h>
#include <unistd.h>
#include <arpa/inet.h>

// Variáveis globais
int sock = 0;  // Socket que será compartilhado entre as threads
int flag_finalizacao = 1;   // Flag para indicar que o programa deve ser encerrado

// Janelas do ncurses
WINDOW *win_output, *win_input;

// Mutexes para sincronizar o acesso às janelas entre as threads
pthread_mutex_t win_input_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t win_output_mutex = PTHREAD_MUTEX_INITIALIZER;

// Função para enviar mensagens ao cliente
void* enviar_mensagens(void* arg) {
    char message[1024]; // Buffer para armazenar a mensagem a ser enviada

    while (1) {
        if(flag_finalizacao == 0)
            break;

        // Limpar a janela de entrada e exibir o prompt com cor
        pthread_mutex_lock(&win_input_mutex);
        wclear(win_input); // Limpa a janela de entrada
        wattron(win_input, COLOR_PAIR(2)); // Ativa o par de cores 2 (Servidor)
        mvwprintw(win_input, 0, 0, "Servidor: "); // Exibe o prompt na janela de entrada
        wattroff(win_input, COLOR_PAIR(2)); // Desativa o par de cores 2
        wrefresh(win_input); // Atualiza a janela de entrada
        pthread_mutex_unlock(&win_input_mutex);

        // Ler a entrada do usuário (bloqueante)
        pthread_mutex_lock(&win_input_mutex);
        wgetnstr(win_input, message, sizeof(message) - 1); // Lê a mensagem digitada pelo usuário
        pthread_mutex_unlock(&win_input_mutex);

        if (flag_finalizacao == 0)
            break;

        // Enviar mensagem ao cliente
        send(sock, message, strlen(message), 0);

        // Verifica se a mensagem é "fim" para encerrar a conexão
        if (strcmp(message, "fim") == 0) {
            pthread_mutex_lock(&win_output_mutex);
            wprintw(win_output, "Conexão finalizada pelo servidor.\n"); // Informa que a conexão foi encerrada
            wrefresh(win_output);
            pthread_mutex_unlock(&win_output_mutex);
            flag_finalizacao = 0; // Define a flag para encerrar o programa
            break;
        }

        // Exibir mensagem na janela de saída com cor
        pthread_mutex_lock(&win_output_mutex);
        wattron(win_output, COLOR_PAIR(2)); // Ativa o par de cores 2 (Servidor)
        wprintw(win_output, "Servidor: "); // Exibe "Servidor:" em cor na janela de saída
        wattroff(win_output, COLOR_PAIR(2)); // Desativa o par de cores 2
        wprintw(win_output, "%s\n", message); // Exibe a mensagem enviada
        wrefresh(win_output); // Atualiza a janela de saída
        pthread_mutex_unlock(&win_output_mutex);
    }

    pthread_exit(0); // Encerra a thread
}

// Função para receber mensagens do cliente
void* receber_mensagens(void* arg) {
    char buffer[1024]; // Buffer para armazenar a mensagem recebida

    while (1) {
        if(flag_finalizacao == 0)
            break;

        memset(buffer, 0, sizeof(buffer)); // Limpa o buffer

        // Recebe a mensagem do cliente
        if(read(sock, buffer, sizeof(buffer)-1) <= 0){
            perror("Erro ao receber mensagem"); // Exibe erro se a leitura falhar
            flag_finalizacao = 0; // Define a flag para encerrar o programa
            break;
        }

        // Atualizar a janela de saída com cor
        pthread_mutex_lock(&win_output_mutex);
        wattron(win_output, COLOR_PAIR(1)); // Ativa o par de cores 1 (Cliente)
        wprintw(win_output, "Cliente: "); // Exibe "Cliente:" em cor na janela de saída
        wattroff(win_output, COLOR_PAIR(1)); // Desativa o par de cores 1
        wprintw(win_output, "%s\n", buffer); // Exibe a mensagem recebida
        wrefresh(win_output); // Atualiza a janela de saída
        pthread_mutex_unlock(&win_output_mutex);

        // Verifica se a mensagem recebida é "fim" para encerrar a conexão
        if(strcmp(buffer, "fim\n") == 0 || strcmp(buffer, "fim") == 0) {
            wprintw(win_output, "Cliente finalizou a conexão.\n"); // Informa que o cliente encerrou a conexão
            flag_finalizacao = 0; // Define a flag para encerrar o programa
            break;
        }
    }

    pthread_exit(0); // Encerra a thread
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // Criar o socket (IPv4, TCP)
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
    address.sin_family = AF_INET; // Família de endereços IPv4
    address.sin_addr.s_addr = INADDR_ANY; // Aceita conexões em qualquer endereço IP
    address.sin_port = htons(8080); // Porta 8080 (convertida para ordem de bytes de rede)

    // Vincular o socket ao IP/Porta
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Erro no bind"); // Exibe erro se o bind falhar
        close(server_fd); // Fecha o socket do servidor
        exit(EXIT_FAILURE);
    }

    // Escutar por conexões
    if (listen(server_fd, 3) < 0) {
        perror("Erro no listen"); // Exibe erro se o listen falhar
        close(server_fd); // Fecha o socket do servidor
        exit(EXIT_FAILURE);
    }

    printf("\nEsperando conexão do cliente...\n");

    // Aceitar conexão do cliente
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("Erro no accept"); // Exibe erro se o accept falhar
        close(server_fd); // Fecha o socket do servidor
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao cliente!\n");

    sock = new_socket; // Define o socket global para uso nas threads

    // Inicializar o ncurses
    initscr(); // Inicializa a tela padrão
    cbreak(); // Desabilita o buffer de linha, o input é recebido caracter por caracter
    keypad(stdscr, TRUE); // Habilita o uso de teclas especiais (setas, F1-F12, etc.)

    // Verificar se o terminal suporta cores
    if (has_colors() == FALSE) {
        endwin(); // Encerra o modo ncurses
        printf("Seu terminal não suporta cores.\n");
        close(sock);
        close(server_fd);
        exit(1);
    }
    start_color(); // Inicia o modo de cores

    // Definir pares de cores
    init_pair(1, COLOR_GREEN, COLOR_BLACK); // Par de cores 1: texto verde em fundo preto (Cliente)
    init_pair(2, COLOR_BLUE, COLOR_BLACK);  // Par de cores 2: texto azul em fundo preto (Servidor)

    // Criar janelas para saída e entrada
    int height_output = LINES - 3; // Altura da janela de saída (deixa 3 linhas para a janela de entrada)
    int width = COLS; // Largura da janela (tamanho total do terminal)
    win_output = newwin(height_output, width, 0, 0); // Janela de saída
    win_input = newwin(3, width, height_output, 0); // Janela de entrada
    scrollok(win_output, TRUE); // Permite scroll na janela de saída

    // Configurar a janela de entrada
    nodelay(win_input, FALSE); // Modo bloqueante (espera a entrada do usuário)
    keypad(win_input, TRUE); // Habilita teclas especiais na janela de entrada
    echo();  // Habilita o eco dos caracteres digitados na janela de entrada

    // Exibir mensagem de conexão estabelecida na janela de saída
    pthread_mutex_lock(&win_output_mutex);
    wattron(win_output, COLOR_PAIR(2)); // Ativa o par de cores 2 (Servidor)
    wprintw(win_output, "Conexão estabelecida com o cliente.\n"); // Mensagem inicial
    wattroff(win_output, COLOR_PAIR(2)); // Desativa o par de cores 2
    wrefresh(win_output); // Atualiza a janela de saída
    pthread_mutex_unlock(&win_output_mutex);

    // Criando as threads para enviar e receber mensagens
    pthread_t threads[2];
    if(pthread_create(&threads[0], NULL, receber_mensagens, NULL) != 0){
        printf("Erro ao criar thread que recebe mensagens\n");
        endwin(); // Encerra o modo ncurses
        close(sock);
        close(server_fd);
        exit(1);
    }
    if(pthread_create(&threads[1], NULL, enviar_mensagens, NULL) != 0){
        printf("Erro ao criar thread que envia mensagens\n");
        endwin(); // Encerra o modo ncurses
        close(sock);
        close(server_fd);
        exit(1);
    }

    // Aguardar que as threads terminem
    pthread_join(threads[0], NULL); // Aguarda a thread de receber mensagens
    pthread_join(threads[1], NULL); // Aguarda a thread de enviar mensagens

    // Limpar e finalizar ncurses
    pthread_mutex_destroy(&win_input_mutex); // Destroi o mutex da janela de entrada
    pthread_mutex_destroy(&win_output_mutex); // Destroi o mutex da janela de saída
    delwin(win_output); // Deleta a janela de saída
    delwin(win_input); // Deleta a janela de entrada
    endwin(); // Finaliza o modo ncurses

    // Fechar conexão
    close(sock); // Fecha o socket de comunicação com o cliente
    close(server_fd); // Fecha o socket do servidor

    return 0;
}
