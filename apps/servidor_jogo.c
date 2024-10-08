#include "wavelenght.h"
#include <time.h>

extern WINDOW *win_output, *win_input;
extern pthread_mutex_t win_input_mutex, win_output_mutex;
extern int flag_finalizacao_servidor, num_clientes;
extern Cliente clientes[MAX_CLIENTES];
extern pthread_mutex_t clientes_mutex, win_mutex;
extern int cliente_dica, votos[MAX_CLIENTES], votos_recebidos, dica_enviada, jogo_iniciado;
extern pthread_mutex_t votos_mutex, cond_votos, jogo_mutex, dica_mutex;

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
    char welcome_msg[BUFFER_SIZE];
    snprintf(welcome_msg, sizeof(welcome_msg), "Bem-vindo ao servidor do Wavelenght %s! Aqui estão algumas instruções básicas sobre o funcionamento do jogo.\n\nWavelength é um jogo em que um jogador dá uma dica para que sua equipe adivinhe a posição de um alvo em uma escala\nabstrata (como 'quente' a 'frio'). A equipe discute e tenta posicionar um ponteiro o mais próximo possível do alvo\ncom base na dica, tentando alinhar seus pensamentos.", client_name);
    send(client_sock, welcome_msg, strlen(welcome_msg), 0);  // Envia mensagem de boas-vindas ao cliente
    char rules_msg[BUFFER_SIZE];
    snprintf(rules_msg, sizeof(rules_msg), "Durante cada rodada do jogo, um jogador (mestre) será escolhido\npara fornecer a dica para os demais.\n\nAssim, todos receberão uma escala abstrata e o mestre receberá também uma nota de 0 a 10 dentro dessa escala.\nO mestre, então, fornecerá uma dica sobre esta nota e o objetivo dos jogadores é adivinhar qual a nota da rodada!\n\nVocês podem conversar aqui pelo chat para discutirem suas respostas, mas todos os jogadores precisam forncer um\nnúmero de 0 a 10 como resposta da rodada. A resposta final será computada como a média de todas as respostas.\nAqui vão alguns comandos importantes.\n\n");
    send(client_sock, rules_msg, strlen(rules_msg), 0);  // Envia mensagem de regras ao cliente
    char command_msg[BUFFER_SIZE];
    snprintf(command_msg, sizeof(command_msg), "'iniciar jogo' -> iniciar o jogo\n'resposta - X' -> fornece uma resposta\n'fechar jogo' -> encerra o jogo\n'fim' -> sai do servidor\n\nBom Jogo!\n\n");
    send(client_sock, command_msg, strlen(command_msg), 0);  // Envia mensagem de comando ao cliente

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

        if (strcmp(buffer, "fechar jogo\n") == 0 || strcmp(buffer, "fechar jogo") == 0) {
            jogo_iniciado = 0;
            votos_recebidos = 0;
            cliente_dica = -1;
            pthread_mutex_lock(&votos_mutex);
            memset(votos, 0, sizeof(votos));
            pthread_mutex_unlock(&votos_mutex);

            char fim_msg[BUFFER_SIZE];
            snprintf(fim_msg, sizeof(fim_msg), "Servidor: Jogo encerrado!\n");
            broadcast_message(fim_msg, -1);  // Envia mensagem de inicio do jogo aos clientes
        }

        // Verifica se o cliente enviou o comando "iniciar jogo"
        int nota, resposta;
        pthread_mutex_lock(&dica_mutex);
        pthread_mutex_lock(&jogo_mutex);
        if (strcmp(buffer, "iniciar jogo\n") == 0 || strcmp(buffer, "iniciar jogo") == 0) {
            if (num_clientes >= 2 && jogo_iniciado == 0) {
                jogo_iniciado = 1; // Marca que o jogo foi iniciado
                char inicio_msg[BUFFER_SIZE];
                snprintf(inicio_msg, sizeof(inicio_msg), "Servidor: Jogo iniciado!\n");
                broadcast_message(inicio_msg, -1);  // Envia mensagem de inicio do jogo aos clientes
                nota = escolher_cliente_dica();  // Escolhe o cliente para dar a dica
            } else if (jogo_iniciado == 0){
                // Caso não tenha clientes suficientes
                char erro_msg[BUFFER_SIZE];
                snprintf(erro_msg, sizeof(erro_msg), "Servidor: Não é possível iniciar o jogo. Necessário pelo menos 2 jogadores.\n");
                send(client_sock, erro_msg, strlen(erro_msg), 0);  // Envia mensagem de erro ao cliente
            } else {
                // Caso o jogo já tenha começado
                char erro_msg[BUFFER_SIZE];
                snprintf(erro_msg, sizeof(erro_msg), "Servidor: O jogo já está em andamento.\n");
                send(client_sock, erro_msg, strlen(erro_msg), 0);  // Envia mensagem de erro ao cliente
            }
        } else if (client_sock == clientes[cliente_dica].sock && dica_enviada == 0) { // Se for o cliente da dica, compartilhe a dica
            char dica_msg[BUFFER_SIZE];
            snprintf(dica_msg, sizeof(dica_msg), "Servidor: %s deu uma dica: %s", client_name, buffer);
            broadcast_message(dica_msg, -1);
            dica_enviada = 1; 
        } else if (client_sock != clientes[cliente_dica].sock && dica_enviada == 1){
            resposta = validar_resposta(client_sock, buffer);
            if(resposta >= 0)
                coletar_votos(client_sock, resposta, nota); // Outros clientes votam
        }
        pthread_mutex_unlock(&jogo_mutex);

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
            
            if(client_sock != clientes[cliente_dica].sock || dica_enviada == 0)
                broadcast_message(message_to_send, client_sock); // Transmite a mensagem para os outros clientes
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
    int server_fd = config_servidor();
    config_terminal_servidor();

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
