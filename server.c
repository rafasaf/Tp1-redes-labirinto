// server.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#define PORT 51511
#define MAX_CLIENTS 1
#define LABIRINTO_SIZE 10

typedef struct {
    int type;
    int moves[100];
    int board[LABIRINTO_SIZE][LABIRINTO_SIZE];
} action_t;

int labyrinth[LABIRINTO_SIZE][LABIRINTO_SIZE];
int player_x = 0, player_y = 0; // Posição do jogador inicial
int initial_x = 0, initial_y = 0; // Posição inicial do jogador (entrada)
int labyrinth_rows = 0;
int labyrinth_cols = 0;

// Função para carregar o labirinto do arquivo
void load_labyrinth(const char *filename) {
    printf("Tentando abrir o arquivo: %s\n", filename);  // Log para debug

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Erro ao abrir o arquivo do labirinto");
        exit(EXIT_FAILURE);
    }

    char line[256];
    labyrinth_rows = 0;
    labyrinth_cols = 0;
    while (fgets(line, sizeof(line), file)) {
        if (labyrinth_rows == 0) {
            char *token = strtok(line, " ");
            while (token) {
                labyrinth_cols++;
                token = strtok(NULL, " ");
            }
        }
        labyrinth_rows++;
    }
    rewind(file);

    for (int i = 0; i < labyrinth_rows; i++) {
        for (int j = 0; j < labyrinth_cols; j++) {
            fscanf(file, "%d", &labyrinth[i][j]);
            if (labyrinth[i][j] == 2) {
                initial_x = player_x = i;
                initial_y = player_y = j;
            }
        }
    }

    fclose(file);
}

// Função para calcular os movimentos possíveis
void calculate_possible_moves(int *moves) {
    int index = 0;

    memset(moves, 0, 100 * sizeof(int)); // Limpa o vetor de movimentos

    // Verificar os movimentos possíveis (cima, direita, baixo, esquerda)
    if (player_x > 0 && labyrinth[player_x - 1][player_y] != 0) {
        moves[index++] = 1; // Cima
    }
    if (player_y < labyrinth_cols - 1 && labyrinth[player_x][player_y + 1] != 0) {
        moves[index++] = 2; // Direita
    }
    if (player_x < labyrinth_rows - 1 && labyrinth[player_x + 1][player_y] != 0) {
        moves[index++] = 3; // Baixo
    }
    if (player_y > 0 && labyrinth[player_x][player_y - 1] != 0) {
        moves[index++] = 4; // Esquerda
    }
}

// Função para enviar uma versão do labirinto que o jogador pode ver
void send_visible_labyrinth(action_t *action) {
    for (int i = 0; i < labyrinth_rows; i++) {
        for (int j = 0; j < labyrinth_cols; j++) {
            if (i == player_x && j == player_y) {
                action->board[i][j] = 5; // Jogador
            } else if (abs(player_x - i) <= 1 && abs(player_y - j) <= 1) {
                action->board[i][j] = labyrinth[i][j]; // Mostrar a célula ao redor do jogador
            } else {
                action->board[i][j] = 4; // Não descoberto
            }
        }
    }

    // Preencher o restante do labirinto que não está sendo usado
    for (int i = labyrinth_rows; i < LABIRINTO_SIZE; i++) {
        for (int j = labyrinth_cols; j < LABIRINTO_SIZE; j++) {
            action->board[i][j] = 4; // Não descoberto
        }
    }
}

// Função para encontrar um caminho até a saída
void find_path_to_exit(int *moves) {
    // Implementação simples para encontrar um caminho até a saída
    int index = 0;

    // Caminho simulado até a saída (esta lógica pode ser melhorada para evitar redundâncias)
    while (player_x < labyrinth_rows - 1 && labyrinth[player_x + 1][player_y] != 0 && labyrinth[player_x + 1][player_y] != 3) {
        moves[index++] = 3; // Baixo
        player_x++;
    }

    while (player_y < labyrinth_cols - 1 && labyrinth[player_x][player_y + 1] != 0 && labyrinth[player_x][player_y + 1] != 3) {
        moves[index++] = 2; // Direita
        player_y++;
    }

    // Preencher o resto com zeros
    for (int i = index; i < 100; i++) {
        moves[i] = 0;
    }
}

void handle_client(int client_socket) {
    action_t action;
    memset(&action, 0, sizeof(action));
    int game_started = 0;  // Flag para verificar se o jogo foi iniciado

    while (1) {
        recv(client_socket, &action, sizeof(action), 0);

        switch (action.type) {
            case 0: // Start
                printf("Cliente iniciou o jogo.\n");
                action.type = 4; // Update para o cliente com o estado inicial
                player_x = initial_x;
                player_y = initial_y;
                calculate_possible_moves(action.moves);
                send_visible_labyrinth(&action);
                send(client_socket, &action, sizeof(action), 0);
                game_started = 1;  // O jogo foi iniciado
                break;

            case 1: // Move
                if (!game_started) {
                    // Se o jogo não foi iniciado, retornar um erro
                    printf("Erro: start the game first\n");
                    action.type = 4; // Tipo update
                    memset(action.moves, 0, sizeof(action.moves)); // Limpar movimentos
                    send(client_socket, &action, sizeof(action), 0);  // Enviar a resposta de erro
                    break;  // Termina a execução deste comando e retorna ao loop principal
                }

                // Verificar se o movimento é válido
                if (action.moves[0] == 1 && player_x > 0 && labyrinth[player_x - 1][player_y] != 0) {
                    player_x--;  // Cima
                } else if (action.moves[0] == 2 && player_y < labyrinth_cols - 1 && labyrinth[player_x][player_y + 1] != 0) {
                    player_y++;  // Direita
                } else if (action.moves[0] == 3 && player_x < labyrinth_rows - 1 && labyrinth[player_x + 1][player_y] != 0) {
                    player_x++;  // Baixo
                } else if (action.moves[0] == 4 && player_y > 0 && labyrinth[player_x][player_y - 1] != 0) {
                    player_y--;  // Esquerda
                } else {
                    // Movimento inválido
                    printf("Movimento inválido.\n");

                    // Enviar uma resposta de erro para o cliente
                    action.type = 4; // Tipo update (erro)
                    memset(action.moves, 0, sizeof(action.moves)); // Limpar movimentos
                    send(client_socket, &action, sizeof(action), 0);  // Enviar a resposta de erro

                    // Reexibir a lista de movimentos possíveis para o cliente tentar novamente
                    calculate_possible_moves(action.moves);
                    send_visible_labyrinth(&action);
                    break;  // Termina a execução do comando e volta ao loop de espera por novos comandos
                }

                // Checar se o jogador chegou à saída
                if (labyrinth[player_x][player_y] == 3) {
                    action.type = 5; // Win
                    memcpy(action.board, labyrinth, sizeof(labyrinth));
                    send(client_socket, &action, sizeof(action), 0);
                    printf("Cliente venceu o jogo!\n");
                    // Não finaliza a conexão aqui, mas permite que o cliente escolha entre reset ou exit
                    break; // Permite que o cliente escolha entre reset ou exit
                }

                // Atualizar os movimentos possíveis e enviar de volta
                calculate_possible_moves(action.moves);
                send_visible_labyrinth(&action);
                action.type = 4; // Update
                send(client_socket, &action, sizeof(action), 0);
                break;

            case 2: // Map request
                printf("Cliente requisitou o mapa.\n");
                if (!game_started) {
                    printf("Erro: start the game first\n");
                    action.type = 4; // Tipo update
                    memset(action.moves, 0, sizeof(action.moves)); // Limpar movimentos
                    send(client_socket, &action, sizeof(action), 0);  // Enviar a resposta de erro
                    break;
                }
                action.type = 4; // Update com o estado do labirinto conhecido
                send_visible_labyrinth(&action);
                send(client_socket, &action, sizeof(action), 0);
                break;

            case 6: // Reset
                printf("Resetando o jogo.\n");
                // Resetando as variáveis do jogo
                action.type = 4; // Update com estado inicial
                player_x = initial_x;  // Reiniciar na posição inicial
                player_y = initial_y;  // Reiniciar na posição inicial
                calculate_possible_moves(action.moves);
                send_visible_labyrinth(&action);
                send(client_socket, &action, sizeof(action), 0);
                break;

            case 7: // Exit
                printf("Cliente desconectado.\n");
                close(client_socket);
                return;

            default:
                // Comando não suportado
                printf("Ação não suportada: %d\n", action.type);
                action.type = 4; // Tipo update
                memset(action.moves, 0, sizeof(action.moves)); // Limpar movimentos
                send(client_socket, &action, sizeof(action), 0);  // Enviar a resposta de erro
        }
    }
}


int main(int argc, char *argv[]) {
    if (argc < 5) {
        printf("Uso: %s <v4> <porta> -i <arquivo_labirinto>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    int port = atoi(argv[2]);
    const char *filename = NULL;

    // Processar argumentos de linha de comando
    for (int i = 3; i < argc; i++) {
        if (strcmp(argv[i], "-i") == 0 && (i + 1 < argc)) {
            filename = argv[i + 1];
        }
    }

    if (filename == NULL) {
        printf("Erro: arquivo de labirinto não especificado. Use a opção -i <arquivo_labirinto>\n");
        exit(EXIT_FAILURE);
    }

    load_labyrinth(filename);

    int server_socket, client_socket;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    // Criar socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        perror("Falha ao criar o socket");
        exit(EXIT_FAILURE);
    }

    // Configurar o endereço do servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    // Vincular o socket ao endereço e porta
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("Falha ao fazer o bind");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    // Escutar por conexões
    if (listen(server_socket, MAX_CLIENTS) < 0) {
        perror("Falha ao escutar");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Servidor aguardando conexão na porta %d...\n", port);

    // Aceitar a conexão do cliente
    client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
    if (client_socket < 0) {
        perror("Falha ao aceitar a conexão");
        close(server_socket);
        exit(EXIT_FAILURE);
    }

    printf("Cliente conectado!\n");
    handle_client(client_socket);

    // Fechar socket do servidor
    close(server_socket);

    return 0;
}
