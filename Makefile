compile_jogo:
	gcc -o bin/servidor_jogo src/servidor_jogo.c -lncurses -lpthread
	gcc -o bin/cliente_jogo src/cliente_jogo.c -lncurses -lpthread

servidor_jogo:
	gcc -o bin/servidor_jogo src/servidor_jogo.c -lncurses -lpthread
	./bin/servidor_jogo

cliente_jogo:
	gcc -o bin/cliente_jogo src/cliente_jogo.c -lncurses -lpthread
	./bin/cliente_jogo


compile_chat:
	gcc -o bin/servidor_chat src/servidor_chat.c -lncurses -lpthread
	gcc -o bin/cliente_chat src/cliente_chat.c -lncurses -lpthread

servidor_chat:
	gcc -o bin/servidor_chat src/servidor_chat.c -lncurses -lpthread
	./bin/servidor_chat

cliente_chat:
	gcc -o bin/cliente_chat src/cliente_chat.c -lncurses -lpthread
	./bin/cliente_chat


compile_win:
	gcc ./win/cliente_win.c -o ./bin/cliente_win -lws2_32
	gcc ./win/servidor_win.c -o ./bin/servidor_win -lws2_32

servidor_win:
	gcc ./win/servidor_win.c -o ./bin/servidor_win -lws2_32
	./bin/servidor_win

cliente_win:
	gcc ./win/cliente_win.c -o ./bin/cliente_win -lws2_32
	./bin/cliente_win 127.0.0.1