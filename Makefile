cliente:
	gcc ./cliente.c -o ./bin/cliente -lws2_32
	./bin/cliente 127.0.0.1

servidor:
	gcc ./servidor.c -o ./bin/servidor -lws2_32
	./bin/servidor

all:
	gcc ./cliente.c -o ./bin/cliente -lws2_32
	gcc ./servidor.c -o ./bin/servidor -lws2_32

teste:
	gcc ./cliente_teste.c -o ./bin/cliente_teste -lws2_32
	./bin/cliente_teste

cliente-carol:
	gcc ./cliente.c -o ./bin/cliente -lws2_32
	./bin/cliente 172.26.167.178

cliente-rhay:
	gcc ./cliente.c -o ./bin/cliente -lws2_32
	./bin/cliente 172.26.192.137

servidor_ln:
	gcc -o bin/servidor_ln linux/servidor_ln.c -lncurses -lpthread
	./bin/servidor_ln

cliente_ln:
	gcc -o bin/cliente_ln linux/cliente_ln.c -lncurses -lpthread
	./bin/cliente_ln