#include "cliente.h"

extern WINDOW *win_output, *win_input;
extern pthread_mutex_t win_input_mutex, win_output_mutex;
extern int sock, flag_finalizacao_cliente, name_colors_count;
extern NameColor name_colors[100];

int main() {
    if(config_cliente() != 0)
        return -1;
    config_terminal_cliente();
    
    // Solicitar o nome do usuário
    char nome[50];
    pthread_mutex_lock(&win_input_mutex);
    wclear(win_input);
    mvwprintw(win_input, 0, 0, "Digite seu nome: ");
    wrefresh(win_input);
    wgetnstr(win_input, nome, sizeof(nome) - 1);
    pthread_mutex_unlock(&win_input_mutex);

    // Enviar o nome ao servidor
    strcat(nome, "\n"); // Adiciona '\n' ao final
    send(sock, nome, strlen(nome), 0);

    nome[strcspn(nome, "\n")] = '\0'; // Remove o '\n' para uso local

    // Adicionar o próprio nome à lista de cores
    strcpy(name_colors[name_colors_count].name, nome);
    name_colors[name_colors_count].color_pair = 1; // Cor do cliente local
    name_colors_count++;

    // Exibir mensagem de conexão estabelecida
    pthread_mutex_lock(&win_output_mutex);
    wattron(win_output, COLOR_PAIR(2)); // Ativa o par de cores 2 (Servidor)
    wprintw(win_output, "Conectado ao servidor.\n");
    wattroff(win_output, COLOR_PAIR(2)); // Desativa o par de cores 2
    wrefresh(win_output);
    pthread_mutex_unlock(&win_output_mutex);

    // Criando as threads
    pthread_t threads[2];
    if(pthread_create(&threads[0], NULL, receber_mensagens, NULL) != 0){
        printf("Erro ao criar thread que recebe mensagens\n");
        endwin(); // Encerra o modo ncurses
        close(sock);
        exit(1);
    }
    if(pthread_create(&threads[1], NULL, enviar_mensagens_cliente, NULL) != 0){
        printf("Erro ao criar thread que envia mensagens\n");
        endwin(); // Encerra o modo ncurses
        close(sock);
        exit(1);
    }

    // Aguardar que as threads terminem
    pthread_join(threads[0], NULL); // Aguarda a thread de receber mensagens
    pthread_join(threads[1], NULL); // Aguarda a thread de enviar mensagens

    // Limpar e finalizar ncurses
    pthread_mutex_destroy(&win_input_mutex);
    pthread_mutex_destroy(&win_output_mutex);
    delwin(win_output);
    delwin(win_input);
    endwin(); // Finaliza o modo ncurses

    // Fechar conexão
    close(sock);

    return 0;
}