#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char serverText[200] = "cleucilcs2600.duckdns.org";
char statusText[200] = "Not connected";
char boardText[50] = "1|2|3 4|5|6 7|8|9";
char cells[9][2];

int serverLength = 24;
int selectedMode = 0;   // 0 = none, 1 = one player, 2 = two player
int connected = 0;
int typingServer = 0;

void parseBoard() {
    int i;
    int j = 0;

    for (i = 0; i < 9; i++) {
        while (boardText[j] == '|' || boardText[j] == ' ') {
            j++;
        }
        cells[i][0] = boardText[j];
        cells[i][1] = '\0';
        j++;
    }
}

void publishSimple(char *topic, char *message) {
    char command[400];
    sprintf(command, "mosquitto_pub -h %s -t %s -m \"%s\"", serverText, topic, message);
    system(command);
}

void sendMove(char *move) {
    publishSimple("tictactoe/player1", move);
}

void readBoard() {
    FILE *fp;
    char command[400];
    char line[200];

    sprintf(command, "mosquitto_sub -h %s -t tictactoe/board -C 1", serverText);
    fp = popen(command, "r");

    if (fp != NULL) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            line[strcspn(line, "\n")] = 0;
            strcpy(boardText, line);
            parseBoard();
        }
        pclose(fp);
    }
}

void readStatus() {
    FILE *fp;
    char command[400];
    char line[200];

    sprintf(command, "mosquitto_sub -h %s -t tictactoe/status -C 1", serverText);
    fp = popen(command, "r");

    if (fp != NULL) {
        if (fgets(line, sizeof(line), fp) != NULL) {
            line[strcspn(line, "\n")] = 0;
            strcpy(statusText, line);
        }
        pclose(fp);
    }
}

void refreshAll() {
    if (connected == 1) {
        readBoard();
        readStatus();
    }
}

void startGame() {
    if (selectedMode == 1) {
        publishSimple("tictactoe/mode", "1");
        strcpy(statusText, "1-player mode selected");
    } else if (selectedMode == 2) {
        publishSimple("tictactoe/mode", "2");
        strcpy(statusText, "2-player mode selected");
    } else {
        strcpy(statusText, "Select a mode first");
        return;
    }

    connected = 1;
    WaitTime(0.5);
    refreshAll();
}

int main() {
    int i;
    int cellSize = 90;
    int startX = 430;
    int startY = 150;

    Rectangle serverBox = {40, 90, 320, 45};
    Rectangle onePlayerButton = {40, 160, 150, 50};
    Rectangle twoPlayerButton = {210, 160, 150, 50};
    Rectangle connectButton = {40, 230, 320, 50};

    parseBoard();

    InitWindow(850, 560, "Tic Tac Toe GUI");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        int key = GetCharPressed();

        while (key > 0) {
            if (typingServer == 1) {
                if (key >= 32 && key <= 126 && serverLength < 190) {
                    serverText[serverLength] = (char)key;
                    serverLength++;
                    serverText[serverLength] = '\0';
                }
            }
            key = GetCharPressed();
        }

        if (typingServer == 1 && IsKeyPressed(KEY_BACKSPACE)) {
            if (serverLength > 0) {
                serverLength--;
                serverText[serverLength] = '\0';
            }
        }

        if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
            if (CheckCollisionPointRec(mouse, serverBox)) {
                typingServer = 1;
            } else {
                typingServer = 0;
            }

            if (CheckCollisionPointRec(mouse, onePlayerButton)) {
                selectedMode = 1;
            }

            if (CheckCollisionPointRec(mouse, twoPlayerButton)) {
                selectedMode = 2;
            }

            if (CheckCollisionPointRec(mouse, connectButton)) {
                startGame();
            }

            if (connected == 1) {
                for (i = 0; i < 9; i++) {
                    int row = i / 3;
                    int col = i % 3;
                    Rectangle box = { startX + col * cellSize, startY + row * cellSize, cellSize, cellSize };

                    if (CheckCollisionPointRec(mouse, box)) {
                        if (cells[i][0] != 'X' && cells[i][0] != 'O') {
                            char move[2];
                            move[0] = cells[i][0];
                            move[1] = '\0';
                            sendMove(move);
                            WaitTime(0.5);
                            refreshAll();
                        }
                    }
                }
            }
        }

        if (IsKeyPressed(KEY_R)) {
            refreshAll();
        }

        BeginDrawing();
        ClearBackground(RAYWHITE);

        DrawText("Tic-Tac-Toe GUI", 260, 20, 34, BLACK);

        DrawText("Server / DuckDNS URL:", 40, 60, 20, BLACK);
        DrawRectangleLinesEx(serverBox, 2, BLACK);
        DrawText(serverText, 50, 102, 20, BLUE);

        if (typingServer == 1) {
            DrawText("typing...", 270, 140, 16, DARKGRAY);
        }

        DrawRectangleRec(onePlayerButton, selectedMode == 1 ? SKYBLUE : LIGHTGRAY);
        DrawRectangleLinesEx(onePlayerButton, 2, BLACK);
        DrawText("1 Player", 75, 175, 22, BLACK);

        DrawRectangleRec(twoPlayerButton, selectedMode == 2 ? SKYBLUE : LIGHTGRAY);
        DrawRectangleLinesEx(twoPlayerButton, 2, BLACK);
        DrawText("2 Player", 245, 175, 22, BLACK);

        DrawRectangleRec(connectButton, LIGHTGRAY);
        DrawRectangleLinesEx(connectButton, 2, BLACK);
        DrawText("Connect / Start", 115, 245, 24, BLACK);

        DrawText("Press R to refresh", 110, 310, 20, DARKGRAY);

        for (i = 0; i < 9; i++) {
            int row = i / 3;
            int col = i % 3;
            int x = startX + col * cellSize;
            int y = startY + row * cellSize;

            DrawRectangleLines(x, y, cellSize, cellSize, BLACK);
            DrawText(cells[i], x + 35, y + 28, 30, BLUE);
        }

        DrawText("Status:", 40, 390, 24, BLACK);
        DrawText(statusText, 130, 390, 24, DARKGRAY);

        DrawText("Player 1 GUI uses tictactoe/player1", 40, 450, 20, DARKGRAY);
        DrawText("Game board comes from tictactoe/board", 40, 480, 20, DARKGRAY);
        DrawText("Mode is sent on tictactoe/mode", 40, 510, 20, DARKGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}
