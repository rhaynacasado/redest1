compile_socketlib:
	gcc -c ./src/socket.c -I ./include -o ./obj/socket.o

compile_wavelenghtlib:
	gcc -c ./src/wavelenght.c -I ./include -o ./obj/wavelenght.o

compile_servidorlib:
	gcc -c ./src/servidor.c -I ./include -o ./obj/servidor.o

compile_clientelib:
	gcc -c ./src/cliente.c -I ./include -o ./obj/cliente.o

app_servidor_jogo:
	gcc apps/servidor_jogo.c ./obj/*.o -I ./include -o bin/servidor_jogo -lncurses -lpthread

app_servidor_chat:
	gcc apps/servidor_chat.c ./obj/*.o -I ./include -o bin/servidor_chat -lncurses -lpthread

app_cliente:
	gcc apps/cliente.c ./obj/*.o -I ./include -o bin/cliente -lncurses -lpthread

run_servidor_jogo:
	./bin/servidor_jogo

run_servidor_chat:
	./bin/servidor_chat

run_cliente:
	./bin/cliente



compile_servidor:
	gcc -c ./src/socket.c -I ./include -o ./obj/socket.o
	gcc -c ./src/servidor.c -I ./include -o ./obj/servidor.o
	gcc -c ./src/wavelenght.c -I ./include -o ./obj/wavelenght.o

compile_cliente:
	gcc -c ./src/socket.c -I ./include -o ./obj/socket.o
	gcc -c ./src/cliente.c -I ./include -o ./obj/cliente.o
	gcc apps/cliente.c ./obj/*.o -I ./include -o bin/cliente -lncurses -lpthread



compile_jogo:
	make compile_servidor
	make app_servidor_jogo
	make compile_cliente

compile_chat:
	make compile_servidor
	make app_servidor_chat
	make compile_cliente



servidor_jogo:
	make compile_servidor
	gcc apps/servidor_jogo.c ./obj/*.o -I ./include -o bin/servidor_jogo -lncurses -lpthread
	./bin/servidor_jogo

servidor_chat:
	make compile_servidor
	gcc apps/servidor_chat.c ./obj/*.o -I ./include -o bin/servidor_chat -lncurses -lpthread
	./bin/servidor_chat



cliente:
	make compile_cliente
	./bin/cliente	

cliente_jogo:
	make cliente

cliente_chat:
	make cliente



compile_win:
	gcc ./win/cliente_win.c -o ./bin/cliente_win -lws2_32
	gcc ./win/servidor_win.c -o ./bin/servidor_win -lws2_32

servidor_win:
	gcc ./win/servidor_win.c -o ./bin/servidor_win -lws2_32
	./bin/servidor_win

cliente_win:
	gcc ./win/cliente_win.c -o ./bin/cliente_win -lws2_32
	./bin/cliente_win 127.0.0.1