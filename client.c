// client.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 51511
#define BUFFER_SIZE 1024

typedef struct {
    int type;
    int moves[100];
    int board[10][10];
} action_t;

// Função para exibir os movimentos possíveis
void display_possible_moves(int *moves) {
    printf("Possible moves: ");
    for (int i = 0; i < 100 && moves[i] != 0; i++) {
        switch (moves[i]) {
            case 1: printf("up"); break;
            case 2: printf("right"); break;
            case 3: printf("down"); break;
            case 4: printf("left"); break;
        }
        if (moves[i + 1] != 0) {
            printf(", ");
        }
    }
    printf(".\n");
}

// Exibir o mapa recebido do servidor
void display_map(action_t *action) {
    printf("Labirinto Atual:\n");
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++) {
            switch (action->board[i][j]) {
                case 0: printf("# "); break; // Muro
                case 1: printf("_ "); break; // Caminho livre
                case 2: printf("> "); break; // Entrada
                case 3: printf("X "); break; // Saída
                case 4: printf("? "); break; // Não descoberto
                case 5: printf("+ "); break; // Jogador
                default: printf("? "); break;
            }
        }
        printf("\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <IP_do_servidor>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int client_socket;
    struct sockaddr_in server_addr;

    // Criar socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket == -1) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, argv[1], &server_addr.sin_addr);

    // Conectar ao servidor
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao conectar ao servidor");
        close(client_socket);
        exit(EXIT_FAILURE);
    }

    printf("Conectado ao servidor!\n");

    action_t action;
    memset(&action, 0, sizeof(action));
    int game_started = 0;
    int game_won = 0; // Flag para verificar se o jogo foi vencido

    // Variável para armazenar a última lista de movimentos válidos antes de um erro
    int last_valid_moves[100] = {0};

    // Loop para interagir com o servidor
    while (1) {
        if (game_won) {
            printf("Você venceu o jogo! Somente os comandos 'reset' ou 'exit' são permitidos agora.\n");
        }

        printf("Enter a command (0=start, 1=up, 2=right, 3=down, 4=left, 5=map, 6=reset, 7=exit, 8=hint): ");
        int command;
        scanf("%d", &command);

        // Verificar se o comando é permitido após vencer o jogo
        if (game_won && (command != 6 && command != 7)) {
            printf("error: command not allowed after you escaped. Only 'reset' or 'exit' are allowed.\n");
            continue; // Retorna ao início sem processar o comando
        }

        if (command == 0) {
            // Enviar comando de "start"
            action.type = 0; // Tipo 0 para "start"
            send(client_socket, &action, sizeof(action), 0);
            recv(client_socket, &action, sizeof(action), 0);
            printf("Starting new game.\n");
            display_possible_moves(action.moves);
            game_started = 1;
        } else if (command == 7) {
            // Enviar comando de "exit"
            if (!game_started) {
                printf("error: start the game first\n");
                // Não usar continue, apenas reexibir movimentos
                display_possible_moves(action.moves);
                continue;
            }
            action.type = 7; // Tipo 7 para "exit"
            send(client_socket, &action, sizeof(action), 0);
            break;
        } else if (command == 5) {
            // Enviar comando de "map"
            if (!game_started) {
                printf("error: start the game first\n");
                // Não usar continue, apenas reexibir movimentos
                display_possible_moves(action.moves);
                continue;
            }
            action.type = 2; // Tipo 2 para "map"
            send(client_socket, &action, sizeof(action), 0);
            recv(client_socket, &action, sizeof(action), 0);
            printf("Labirinto Atual:\n");
            display_map(&action);
        } else if (command == 6) {
            // Enviar comando de "reset"
            if (!game_started) {
                printf("error: start the game first\n");
                // Não usar continue, apenas reexibir movimentos
                display_possible_moves(action.moves);
                continue;
            }
            action.type = 6; // Tipo 6 para "reset"
            send(client_socket, &action, sizeof(action), 0);
            recv(client_socket, &action, sizeof(action), 0);
            printf("Starting new game.\n");
            display_possible_moves(action.moves);
            game_won = 0; // Resetando a flag de vitória
        } else if (command == 8) {
            // Enviar comando de "hint"
            if (!game_started) {
                printf("error: start the game first\n");
                // Não usar continue, apenas reexibir movimentos
                display_possible_moves(action.moves);
                continue;
            }
            action.type = 3; // Tipo 3 para "hint"
            send(client_socket, &action, sizeof(action), 0);
            recv(client_socket, &action, sizeof(action), 0);
            printf("Hint: ");
            display_possible_moves(action.moves);
        } else if (command >= 1 && command <= 4) {
            // Enviar comando de movimento
            if (!game_started) {
                printf("error: start the game first\n");
                // Não usar continue, apenas reexibir movimentos
                display_possible_moves(action.moves);
                continue;
            }

            // Antes de tentar o movimento, armazenar a lista atual de movimentos válidos
            memcpy(last_valid_moves, action.moves, sizeof(action.moves));

            action.type = 1; // Tipo 1 para "move"
            memset(action.moves, 0, sizeof(action.moves));
            action.moves[0] = command;
            send(client_socket, &action, sizeof(action), 0);

            // Receber atualização do servidor
            recv(client_socket, &action, sizeof(action), 0);

            // Verificar se o movimento foi inválido
            if (action.moves[0] == 0) {
                printf("error: you cannot go this way\n");
                // Exibir a última lista de movimentos válidos após erro
                display_possible_moves(last_valid_moves);
                continue;
            }

            if (action.type == 5) {
                printf("You escaped!\n");
                display_map(&action);
                game_won = 1; // Ativar a flag de vitória
                // Exibir as opções de "reset" ou "exit"
                printf("Escolha uma opção: 'reset' para reiniciar ou 'exit' para sair.\n");
                continue; // Não termina o loop imediatamente
            }

            display_possible_moves(action.moves);
        } else {
            printf("error: command not found\n");
            // Exibir a última lista de movimentos válidos após erro
            display_possible_moves(last_valid_moves);
        }
    }

    printf("Desconectando do servidor...\n");
    close(client_socket);

    return 0;
}
