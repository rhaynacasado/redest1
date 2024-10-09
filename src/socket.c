#include "socket.h"

// Janelas do ncurses
WINDOW *win_output, *win_input;

// Mutexes para sincronização
pthread_mutex_t win_input_mutex = PTHREAD_MUTEX_INITIALIZER;  // Mutex para a janela de entrada
pthread_mutex_t win_output_mutex = PTHREAD_MUTEX_INITIALIZER; // Mutex para a janela de saída