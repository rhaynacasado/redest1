<h1 align="center">
    <p> Redes de Computadores - Trabalho 1  </p>
    <pp> Jogo Wavelength com Conexão TCP  </pp>
</h1>

## Sobre

Trabalho 1 da Disciplina SSC0641 - Redes de Computadores. Este trabalho visa aplicar conhecimentos e técnicas de redes de computadores, sockets, comunicação TCP/UDP, servidor e clientes e threads, desenvolvendo uma aplicação multicliente por rede.

Dessa forma optou-se por desenvolver um jogo multiplayer inspirado no jogo Wavelength. Conheça mais sobre o jogo original em: https://boardgamegeek.com/boardgame/262543/wavelength. 

## Como rodar

```bash
    # Install ncurses
    $ sudo apt-get install libncurses5-dev libncursesw5-dev
    
    # Clone the project
    $ git clone https://github.com/rhaynacasado/redest1
    $ cd redest1

    # Linux [Ubuntu 22.04.5 LTS] - Compilar e Rodar Servidor
    $ make compile_servidor
    $ make run_servidor_jogo

    # Linux [Ubuntu 22.04.5 LTS] - Compilar e Rodar Cliente
    $ make compile_cliente
    $ make run_cliente

```
## Vídeo de apresentação

Um vídeo explicando o desenvolvimento do projeto está disponível em: https://drive.google.com/file/d/1wkR-GlCKxFj_UHiI7KWuueztBqS1EKAS/view?usp=sharing

## Autores

- Agnes Bressan

- Carolina Elias

- Caroline Clapis

- Rauany Secci

- Rhayna Casado
