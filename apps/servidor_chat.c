#include "servidor.h"

// Usando variávies globais das bibliotecas relacionadas
extern WINDOW *win_output, *win_input;
extern pthread_mutex_t win_input_mutex, win_output_mutex;
extern int flag_finalizacao_servidor, num_clientes;
extern Cliente clientes[MAX_CLIENTES];
extern pthread_mutex_t clientes_mutex, win_mutex;

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

        // Verifica se o cliente enviou "fim" para encerrar a conexão
        if (strcmp(buffer, "fim\n") == 0 || strcmp(buffer, "fim") == 0) {
            goto disconnect;
        }

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

    while (!flag_finalizacao_servidor) {
        // Aceitar conexão do cliente
        new_sock = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (new_sock < 0) {
            if (flag_finalizacao_servidor) break;
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

int main() {
    int server_fd = config_servidor();  // Configura o socket do cliente
    config_terminal_servidor();         // Configura o terminal (nucurses)

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
    if (pthread_create(&send_thread, NULL, enviar_mensagens_servidor, NULL) != 0) {
        perror("Erro ao criar thread de envio");
        endwin();
        exit(1);
    }

    // Aguardar o término da thread de envio (quando o servidor digita "fim")
    pthread_join(send_thread, NULL);

    // Encerrar o servidor
    flag_finalizacao_servidor = 1;

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
    pthread_mutex_destroy(&win_mutex);
    pthread_mutex_destroy(&win_input_mutex);
    pthread_mutex_destroy(&win_output_mutex);
    delwin(win_output);
    delwin(win_input);
    endwin();

    return 0;
}