#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char board[10] = {'0','1','2','3','4','5','6','7','8','9'};

void showBoard() {
    printf("\n");
    printf(" %c | %c | %c \n", board[1], board[2], board[3]);
    printf("---+---+---\n");
    printf(" %c | %c | %c \n", board[4], board[5], board[6]);
    printf("---+---+---\n");
    printf(" %c | %c | %c \n", board[7], board[8], board[9]);
    printf("\n");
}

int checkWin() {
    if (board[1] == board[2] && board[2] == board[3])
        return 1;
    else if (board[4] == board[5] && board[5] == board[6])
        return 1;
    else if (board[7] == board[8] && board[8] == board[9])
        return 1;
    else if (board[1] == board[4] && board[4] == board[7])
        return 1;
    else if (board[2] == board[5] && board[5] == board[8])
        return 1;
    else if (board[3] == board[6] && board[6] == board[9])
        return 1;
    else if (board[1] == board[5] && board[5] == board[9])
        return 1;
    else if (board[3] == board[5] && board[5] == board[7])
        return 1;
    else
        return 0;
}

int checkDraw() {
    int i;
    for (i = 1; i <= 9; i++) {
        if (board[i] != 'X' && board[i] != 'O') {
            return 0;
        }
    }
    return 1;
}

int isValidMove(int choice) {
    if (choice < 1 || choice > 9) {
        return 0;
    }

    if (board[choice] == 'X' || board[choice] == 'O') {
        return 0;
    }

    return 1;
}

void publishMessage(char *topic, char *message) {
    char command[300];
    sprintf(command, "mosquitto_pub -h localhost -t %s -r -m \"%s\"", topic, message);
    system(command);
}

void makeBoardString(char *buffer) {
    sprintf(buffer,
        "%c|%c|%c %c|%c|%c %c|%c|%c",
        board[1], board[2], board[3],
        board[4], board[5], board[6],
        board[7], board[8], board[9]
    );
}

int getMoveFromMQTT(char *topicName) {
    char command[300];
    char moveText[20];
    int choice;
    FILE *file;

    sprintf(command, "mosquitto_sub -h localhost -t %s -C 1 > /tmp/tictactoe_move.txt", topicName);
    system(command);

    file = fopen("/tmp/tictactoe_move.txt", "r");
    if (file == NULL) {
        return -1;
    }

    if (fgets(moveText, sizeof(moveText), file) == NULL) {
        fclose(file);
        return -1;
    }

    fclose(file);

    choice = atoi(moveText);
    return choice;
}

int getModeFromMQTT() {
    char command[300];
    char modeText[20];
    int mode;
    FILE *file;

    sprintf(command, "mosquitto_sub -h localhost -t tictactoe/mode -C 1 > /tmp/tictactoe_mode.txt");
    system(command);

    file = fopen("/tmp/tictactoe_mode.txt", "r");
    if (file == NULL) {
        return 2;
    }

    if (fgets(modeText, sizeof(modeText), file) == NULL) {
        fclose(file);
        return 2;
    }

    fclose(file);

    mode = atoi(modeText);

    if (mode != 1 && mode != 2) {
        mode = 2;
    }

    return mode;
}
void startRandomPlayer() {
    system("pkill -f random_player.sh > /dev/null 2>&1");
    system("nohup /home/cleucilf/tictactoe_project/scripts/random_player.sh > /tmp/random_player.log 2>&1 &");
}

void stopRandomPlayer() {
    system("pkill -f random_player.sh > /dev/null 2>&1");
}

int main() {
    int player = 1;
    int choice;
    int mode;
    char mark;
    int gameOver = 0;
    char boardMessage[100];

    printf("Waiting for game mode from GUI...\n");
    mode = getModeFromMQTT();

    if (mode == 1) {
        printf("1-player mode selected.\n");
        publishMessage("tictactoe/status", "1-player mode");
        startRandomPlayer();
        printf("Random player started automatically.\n");
    } else {
        printf("2-player mode selected.\n");
        publishMessage("tictactoe/status", "2-player mode");
        stopRandomPlayer();
        printf("Random player stopped.\n");
    }

    while (gameOver == 0) {
        showBoard();
        makeBoardString(boardMessage);
        publishMessage("tictactoe/board", boardMessage);

        if (player == 1) {
            mark = 'X';
            publishMessage("tictactoe/status", "Player 1 turn");
            printf("Waiting for Player 1 move...\n");
            choice = getMoveFromMQTT("tictactoe/player1");
        } else {
            mark = 'O';
            publishMessage("tictactoe/status", "Player 2 turn");
            printf("Waiting for Player 2 move...\n");
            choice = getMoveFromMQTT("tictactoe/player2");
        }

        if (choice == 0) {
            printf("Game ended by user.\n");
            publishMessage("tictactoe/status", "Game ended by user");
            break;
        }

        if (isValidMove(choice) == 0) {
            printf("Invalid move. Try again.\n");
            publishMessage("tictactoe/status", "Invalid move");
            continue;
        }

        board[choice] = mark;

        if (checkWin() == 1) {
            showBoard();
            makeBoardString(boardMessage);
            publishMessage("tictactoe/board", boardMessage);

            if (player == 1) {
                publishMessage("tictactoe/status", "Player 1 wins");
            } else {
                publishMessage("tictactoe/status", "Player 2 wins");
            }

            printf("Player %d wins!\n", player);
            gameOver = 1;
        } else if (checkDraw() == 1) {
            showBoard();
            makeBoardString(boardMessage);
            publishMessage("tictactoe/board", boardMessage);
            publishMessage("tictactoe/status", "Game is a draw");
            printf("Game is a draw.\n");
            gameOver = 1;
        } else {
            if (player == 1) {
                player = 2;
            } else {
                player = 1;
            }
        }
    }

    return 0;
}
