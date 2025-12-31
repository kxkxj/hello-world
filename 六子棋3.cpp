#include <graphics.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <string.h>

#define BOARD_SIZE 15      // 15x15的棋盘
#define CELL_SIZE 40       // 每个格子的像素大小
#define OFFSET 50          // 棋盘边距
#define WIN_COUNT 6        // 六子棋胜利条件
#define MAX_MOVES 225      // 最大步数（15*15）

// 游戏状态枚举
typedef enum {
    GS_PLAYING,
    GS_BLACK_WIN,
    GS_WHITE_WIN,
    GS_DRAW
} GameStatus;

// 棋子类型枚举
typedef enum {
    CT_EMPTY,
    CT_BLACK,
    CT_WHITE
} ChessType;

// 游戏模式枚举
typedef enum {
    GM_PVP,
    GM_PVE_EASY,
    GM_PVE_MEDIUM,
    GM_PVE_HARD
} GameMode;

// 坐标结构体
typedef struct {
    int row;
    int col;
} Position;

// 全局变量
ChessType board[BOARD_SIZE][BOARD_SIZE];   // 棋盘
GameStatus gameStatus = GS_PLAYING;        // 游戏状态
ChessType currentPlayer = CT_BLACK;        // 当前玩家
GameMode gameMode = GM_PVP;                // 游戏模式
bool gameStarted = false;                  // 游戏是否开始
Position lastMove = {-1, -1};              // 最后一步的位置
int moveHistory[MAX_MOVES][2];             // 走棋历史记录
int moveCount = 0;                         // 当前步数

// 函数声明
void initBoard();
void showStartMenu();
void showGameStartPrompt();
void drawBoardBackground();
void drawChess(int row, int col, ChessType type);
void drawGameInfo();
bool checkWin(int row, int col, ChessType player);
bool isBoardFull();
Position easyAIMove();
Position mediumAIMove();
Position hardAIMove();
void makeMove(int row, int col, ChessType player);
void aiMakeMove();
void showEndMenu();

// 初始化棋盘
void initBoard() {
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            board[i][j] = CT_EMPTY;
        }
    }
    gameStatus = GS_PLAYING;
    currentPlayer = CT_BLACK;
    lastMove.row = -1;
    lastMove.col = -1;
    moveCount = 0;
}

// 显示游戏开始提示
void showGameStartPrompt() {
    BeginBatchDraw();
    
    // 半透明背景
    
    fillrectangle(0, 0, 800, 600);
    
    settextcolor(YELLOW);
    settextstyle(36, 0, _T("黑体"));
    
    char modeName[50];
    switch (gameMode) {
        case GM_PVP:
            strcpy(modeName, "双人对战模式");
            break;
        case GM_PVE_EASY:
            strcpy(modeName, "人机对战模式 (简单)");
            break;
        case GM_PVE_MEDIUM:
            strcpy(modeName, "人机对战模式 (中等)");
            break;
        case GM_PVE_HARD:
            strcpy(modeName, "人机对战模式 (困难)");
            break;
    }
    
    // 显示游戏模式
    outtextxy(250, 200, _T("游戏开始!"));
    outtextxy(200, 250, modeName);
    
    settextstyle(24, 0, _T("宋体"));
    outtextxy(200, 320, _T("黑方先行 ●"));
    outtextxy(200, 360, _T("点击任意位置开始游戏..."));
    
    EndBatchDraw();
    
    // 等待用户点击
    while (true) {
        if (MouseHit()) {
            MOUSEMSG msg = GetMouseMsg();
            if (msg.uMsg == WM_LBUTTONDOWN) {
                break;
            }
        }
        Sleep(10);
    }
}

// 绘制棋盘背景
void drawBoardBackground() {
    // 设置背景色
    setbkcolor(RGB(222, 184, 135));
    cleardevice();
    
    // 绘制棋盘线
    setlinecolor(BLACK);
    setlinestyle(PS_SOLID, 2);
    
    // 绘制横线
    for (int i = 0; i < BOARD_SIZE; i++) {
        line(OFFSET, OFFSET + i * CELL_SIZE, 
             OFFSET + (BOARD_SIZE - 1) * CELL_SIZE, OFFSET + i * CELL_SIZE);
    }
    
    // 绘制竖线
    for (int i = 0; i < BOARD_SIZE; i++) {
        line(OFFSET + i * CELL_SIZE, OFFSET,
             OFFSET + i * CELL_SIZE, OFFSET + (BOARD_SIZE - 1) * CELL_SIZE);
    }
    
    // 绘制天元和星位
    setfillcolor(BLACK);
    int center = BOARD_SIZE / 2;
    int smallPoints[4][2] = {
        {3, 3}, {3, 11}, {11, 3}, {11, 11}
    };
    
    fillcircle(OFFSET + center * CELL_SIZE, OFFSET + center * CELL_SIZE, 5);
    for (int i = 0; i < 4; i++) {
        fillcircle(OFFSET + smallPoints[i][1] * CELL_SIZE, 
                  OFFSET + smallPoints[i][0] * CELL_SIZE, 5);
    }
}

// 绘制棋子

void drawChess(int row, int col, ChessType type) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return;
    }
    
    if (type == CT_EMPTY) {
        // 空位不需要绘制任何东西
        return;
    }
    
    // 绘制黑子或白子
    if (type == CT_BLACK) {
        setfillcolor(BLACK);
        setlinecolor(BLACK);
    } else {
        setfillcolor(WHITE);
        setlinecolor(BLACK);
    }
    
    fillcircle(OFFSET + col * CELL_SIZE, 
              OFFSET + row * CELL_SIZE, 
              CELL_SIZE / 2 - 2);
}
// 绘制游戏信息
void drawGameInfo() {
    // 绘制信息区域背景
    setfillcolor(RGB(240, 240, 240));
    fillrectangle(10, 10, 790, 40);
    
    settextcolor(BLACK);
    settextstyle(20, 0, _T("宋体"));
    
    if (gameStatus == GS_PLAYING) {
        char msg[50];
        if (currentPlayer == CT_BLACK) {
            sprintf(msg, "当前: 黑方 ● (步数: %d)", moveCount);
        } else {
            sprintf(msg, "当前: 白方 ○ (步数: %d)", moveCount);
        }
        outtextxy(20, 15, msg);
    } else {
        char result[50];
        if (gameStatus == GS_BLACK_WIN) {
            sprintf(result, "游戏结束! 黑方胜利! (总步数: %d)", moveCount);
        } else if (gameStatus == GS_WHITE_WIN) {
            sprintf(result, "游戏结束! 白方胜利! (总步数: %d)", moveCount);
        } else {
            sprintf(result, "游戏结束! 平局! (总步数: %d)", moveCount);
        }
        outtextxy(20, 15, result);
    }
    
    // 绘制模式信息
    char modeMsg[50];
    if (gameMode == GM_PVP) {
        sprintf(modeMsg, "模式: 双人对战");
    } else if (gameMode == GM_PVE_EASY) {
        sprintf(modeMsg, "模式: 人机对战(简单)");
    } else if (gameMode == GM_PVE_MEDIUM) {
        sprintf(modeMsg, "模式: 人机对战(中等)");
    } else {
        sprintf(modeMsg, "模式: 人机对战(困难)");
    }
    outtextxy(400, 15, modeMsg);
    
    // 绘制最后一步提示
    if (lastMove.row != -1) {
        char lastMoveMsg[50];
        sprintf(lastMoveMsg, "最后一步: %c%d", 'A' + lastMove.col, lastMove.row + 1);
        outtextxy(600, 15, lastMoveMsg);
    }
}

// 判断是否胜利
bool checkWin(int row, int col, ChessType player) {
    // 检查方向: 水平、垂直、主对角线、副对角线
    int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
    
    for (int d = 0; d < 4; d++) {
        int count = 1;
        
        // 正向检查
        int r = row + directions[d][0];
        int c = col + directions[d][1];
        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
               board[r][c] == player) {
            count++;
            r += directions[d][0];
            c += directions[d][1];
        }
        
        // 反向检查
        r = row - directions[d][0];
        c = col - directions[d][1];
        while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE && 
               board[r][c] == player) {
            count++;
            r -= directions[d][0];
            c -= directions[d][1];
        }
        
        if (count >= WIN_COUNT) {
            // 高亮显示胜利的棋子
            
            return true;
        }
    }
    
    return false;
}

// 检查棋盘是否已满
bool isBoardFull() {
    return moveCount >= BOARD_SIZE * BOARD_SIZE;
}

// 计算棋型分数（改进的评估函数）
int evaluatePattern(int playerCount, int opponentCount, int emptyCount, int length) {
    if (playerCount == length && emptyCount > 0) {
        // 活棋型
        if (length == 6) return 10000;
        if (length == 5) return 1000;
        if (length == 4) return 100;
        if (length == 3) return 10;
        if (length == 2) return 1;
    } else if (playerCount == length && emptyCount == 0) {
        // 死棋型
        if (length == 6) return 5000;
        if (length == 5) return 500;
        if (length == 4) return 50;
        if (length == 3) return 5;
    }
    return 0;
}

// 简单AI - 随机落子
Position easyAIMove() {
    Position pos = {-1, -1};
    int emptyCount = 0;
    Position emptyPositions[BOARD_SIZE * BOARD_SIZE];
    
    // 收集所有空位
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                emptyPositions[emptyCount].row = i;
                emptyPositions[emptyCount].col = j;
                emptyCount++;
            }
        }
    }
    
    if (emptyCount > 0) {
        int index = rand() % emptyCount;
        pos = emptyPositions[index];
    }
    
    return pos;
}

// 中等AI - 基于棋型评估
Position mediumAIMove() {
    Position bestPos = {-1, -1};
    int bestScore = -1;
    
    // 1. 检查自己是否可以立即胜利
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                board[i][j] = CT_WHITE;
                if (checkWin(i, j, CT_WHITE)) {
                    board[i][j] = CT_EMPTY;
                    bestPos.row = i;
                    bestPos.col = j;
                    return bestPos;
                }
                board[i][j] = CT_EMPTY;
            }
        }
    }
    
    // 2. 检查是否需要阻止黑方胜利
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                board[i][j] = CT_BLACK;
                if (checkWin(i, j, CT_BLACK)) {
                    board[i][j] = CT_EMPTY;
                    bestPos.row = i;
                    bestPos.col = j;
                    return bestPos;
                }
                board[i][j] = CT_EMPTY;
            }
        }
    }
    
    // 3. 评估每个空位的分数
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                int score = 0;
                
                // 检查四个方向
                int directions[4][2] = {{0, 1}, {1, 0}, {1, 1}, {1, -1}};
                
                for (int d = 0; d < 4; d++) {
                    // 检查白子（AI）的棋型
                    int whiteCount = 1;
                    int emptyCount = 0;
                    
                    // 正向检查
                    int r = i + directions[d][0];
                    int c = j + directions[d][1];
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                        if (board[r][c] == CT_WHITE) whiteCount++;
                        else if (board[r][c] == CT_EMPTY) emptyCount++;
                        else break;
                        r += directions[d][0];
                        c += directions[d][1];
                    }
                    
                    // 反向检查
                    r = i - directions[d][0];
                    c = j - directions[d][1];
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                        if (board[r][c] == CT_WHITE) whiteCount++;
                        else if (board[r][c] == CT_EMPTY) emptyCount++;
                        else break;
                        r -= directions[d][0];
                        c -= directions[d][1];
                    }
                    
                    score += evaluatePattern(whiteCount, 0, emptyCount, whiteCount);
                    
                    // 检查黑子（玩家）的棋型并防守
                    int blackCount = 1;
                    emptyCount = 0;
                    
                    // 正向检查
                    r = i + directions[d][0];
                    c = j + directions[d][1];
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                        if (board[r][c] == CT_BLACK) blackCount++;
                        else if (board[r][c] == CT_EMPTY) emptyCount++;
                        else break;
                        r += directions[d][0];
                        c += directions[d][1];
                    }
                    
                    // 反向检查
                    r = i - directions[d][0];
                    c = j - directions[d][1];
                    while (r >= 0 && r < BOARD_SIZE && c >= 0 && c < BOARD_SIZE) {
                        if (board[r][c] == CT_BLACK) blackCount++;
                        else if (board[r][c] == CT_EMPTY) emptyCount++;
                        else break;
                        r -= directions[d][0];
                        c -= directions[d][1];
                    }
                    
                    // 防守分数比进攻分数更高
                    score += evaluatePattern(blackCount, 0, emptyCount, blackCount) * 2;
                }
                
                // 中心位置加分
                int center = BOARD_SIZE / 2;
                int distance = abs(i - center) + abs(j - center);
                score += (BOARD_SIZE - distance) * 2;
                
                // 更新最佳位置
                if (score > bestScore) {
                    bestScore = score;
                    bestPos.row = i;
                    bestPos.col = j;
                }
            }
        }
    }
    
    return bestPos;
}

// 困难AI - 深度优先搜索（带深度限制）
Position hardAIMove() {
    Position bestPos = {-1, -1};
    
    // 1. 立即胜利检查
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                board[i][j] = CT_WHITE;
                if (checkWin(i, j, CT_WHITE)) {
                    board[i][j] = CT_EMPTY;
                    bestPos.row = i;
                    bestPos.col = j;
                    return bestPos;
                }
                board[i][j] = CT_EMPTY;
            }
        }
    }
    
    // 2. 防守玩家即将胜利
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                board[i][j] = CT_BLACK;
                if (checkWin(i, j, CT_BLACK)) {
                    board[i][j] = CT_EMPTY;
                    bestPos.row = i;
                    bestPos.col = j;
                    return bestPos;
                }
                board[i][j] = CT_EMPTY;
            }
        }
    }
    
    // 3. 考虑下一步的进攻（模拟一步）
    int maxScore = -10000;
    
    for (int i = 0; i < BOARD_SIZE; i++) {
        for (int j = 0; j < BOARD_SIZE; j++) {
            if (board[i][j] == CT_EMPTY) {
                // 尝试在此落子
                board[i][j] = CT_WHITE;
                
                // 计算这个位置的分数
                int score = 0;
                
                // 进攻分数
                for (int k = 0; k < BOARD_SIZE; k++) {
                    for (int l = 0; l < BOARD_SIZE; l++) {
                        if (board[k][l] == CT_EMPTY) {
                            board[k][l] = CT_WHITE;
                            if (checkWin(k, l, CT_WHITE)) {
                                score += 100;
                            }
                            board[k][l] = CT_EMPTY;
                            
                            board[k][l] = CT_BLACK;
                            if (checkWin(k, l, CT_BLACK)) {
                                score -= 150;  // 防守更重要
                            }
                            board[k][l] = CT_EMPTY;
                        }
                    }
                }
                
                // 中心控制分数
                int center = BOARD_SIZE / 2;
                int distance = abs(i - center) + abs(j - center);
                score += (BOARD_SIZE - distance) * 3;
                
                // 连接性分数（与已有棋子的连接）
                for (int di = -1; di <= 1; di++) {
                    for (int dj = -1; dj <= 1; dj++) {
                        if (di == 0 && dj == 0) continue;
                        int ni = i + di;
                        int nj = j + dj;
                        if (ni >= 0 && ni < BOARD_SIZE && nj >= 0 && nj < BOARD_SIZE) {
                            if (board[ni][nj] == CT_WHITE) score += 5;
                            else if (board[ni][nj] == CT_BLACK) score += 3;
                        }
                    }
                }
                
                // 恢复棋盘
                board[i][j] = CT_EMPTY;
                
                if (score > maxScore) {
                    maxScore = score;
                    bestPos.row = i;
                    bestPos.col = j;
                }
            }
        }
    }
    
    return bestPos;
}

// 处理落子
void makeMove(int row, int col, ChessType player) {
    if (row < 0 || row >= BOARD_SIZE || col < 0 || col >= BOARD_SIZE) {
        return;
    }
    
    if (board[row][col] == CT_EMPTY && gameStatus == GS_PLAYING) {
        board[row][col] = player;
        moveHistory[moveCount][0] = row;
        moveHistory[moveCount][1] = col;
        moveCount++;
        lastMove.row = row;
        lastMove.col = col;
        
        BeginBatchDraw();
        drawChess(row, col, player);
        drawGameInfo();
        EndBatchDraw();
        
        // 检查是否胜利
        if (checkWin(row, col, player)) {
            gameStatus = (player == CT_BLACK) ? GS_BLACK_WIN : GS_WHITE_WIN;
        } else if (isBoardFull()) {
            gameStatus = GS_DRAW;
        }
    }
}

// AI进行落子
void aiMakeMove() {
    if (gameStatus != GS_PLAYING || currentPlayer != CT_WHITE) {
        return;
    }
    
    Position aiMove;
    
    switch (gameMode) {
        case GM_PVE_EASY:
            aiMove = easyAIMove();
            break;
        case GM_PVE_MEDIUM:
            aiMove = mediumAIMove();
            break;
        case GM_PVE_HARD:
            aiMove = hardAIMove();
            break;
        default:
            return;
    }
    
    if (aiMove.row != -1 && aiMove.col != -1) {
        // 短暂延迟，模拟思考过程
        Sleep(500 + rand() % 500);
        
        makeMove(aiMove.row, aiMove.col, CT_WHITE);
        if (gameStatus == GS_PLAYING) {
            currentPlayer = CT_BLACK;
            drawGameInfo();
        }
    }
}

// 显示开始菜单
void showStartMenu() {
    BeginBatchDraw();
    
    setbkcolor(RGB(135, 206, 235));
    cleardevice();
    
    settextcolor(BLUE);
    settextstyle(48, 0, _T("黑体"));
    outtextxy(200, 100, _T("六子棋"));
    
    settextstyle(24, 0, _T("宋体"));
    settextcolor(BLACK);
    
    // 绘制菜单选项
    int optionsY = 250;
    int optionHeight = 50;
    
    RECT options[5];
    
    // 双人对战
    options[0].left = 250;
    options[0].top = optionsY;
    options[0].right = 550;
    options[0].bottom = optionsY + 40;
    
    setfillcolor(RGB(255, 228, 181));
    fillroundrect(options[0].left, options[0].top, 
                  options[0].right, options[0].bottom, 10, 10);
    outtextxy(320, optionsY + 5, _T("1. 双人对战 (PVP)"));
    
    // 人机对战简单
    options[1].left = 250;
    options[1].top = optionsY + optionHeight;
    options[1].right = 550;
    options[1].bottom = optionsY + optionHeight + 40;
    
    setfillcolor(RGB(255, 228, 181));
    fillroundrect(options[1].left, options[1].top, 
                  options[1].right, options[1].bottom, 10, 10);
    outtextxy(320, optionsY + optionHeight + 5, _T("2. 人机对战 (简单)"));
    
    // 人机对战中等
    options[2].left = 250;
    options[2].top = optionsY + 2 * optionHeight;
    options[2].right = 550;
    options[2].bottom = optionsY + 2 * optionHeight + 40;
    
    setfillcolor(RGB(255, 228, 181));
    fillroundrect(options[2].left, options[2].top, 
                  options[2].right, options[2].bottom, 10, 10);
    outtextxy(320, optionsY + 2 * optionHeight + 5, _T("3. 人机对战 (中等)"));
    
    // 人机对战困难
    options[3].left = 250;
    options[3].top = optionsY + 3 * optionHeight;
    options[3].right = 550;
    options[3].bottom = optionsY + 3 * optionHeight + 40;
    
    setfillcolor(RGB(255, 228, 181));
    fillroundrect(options[3].left, options[3].top, 
                  options[3].right, options[3].bottom, 10, 10);
    outtextxy(320, optionsY + 3 * optionHeight + 5, _T("4. 人机对战 (困难)"));
    
    // 退出游戏
    options[4].left = 250;
    options[4].top = optionsY + 4 * optionHeight;
    options[4].right = 550;
    options[4].bottom = optionsY + 4 * optionHeight + 40;
    
    setfillcolor(RGB(255, 228, 181));
    fillroundrect(options[4].left, options[4].top, 
                  options[4].right, options[4].bottom, 10, 10);
    outtextxy(320, optionsY + 4 * optionHeight + 5, _T("5. 退出游戏"));
    
    settextstyle(18, 0, _T("宋体"));
    outtextxy(280, 550, _T("使用鼠标点击选择游戏模式"));
    
    EndBatchDraw();
    
    // 等待用户选择
    while (true) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN) {
            for (int i = 0; i < 5; i++) {
                if (msg.x >= options[i].left && msg.x <= options[i].right &&
                    msg.y >= options[i].top && msg.y <= options[i].bottom) {
                    
                    switch (i) {
                        case 0: gameMode = GM_PVP; break;
                        case 1: gameMode = GM_PVE_EASY; break;
                        case 2: gameMode = GM_PVE_MEDIUM; break;
                        case 3: gameMode = GM_PVE_HARD; break;
                        case 4: exit(0);
                    }
                    
                    gameStarted = true;
                    initBoard();
                    
                    // 显示游戏开始提示
                    showGameStartPrompt();
                    
                    return;
                }
            }
        }
        Sleep(10);
    }
}

// 显示结束界面
void showEndMenu() {
    // 等待一段时间再显示结束界面
    Sleep(1500);
    
    BeginBatchDraw();
    
    setbkcolor(RGB(255, 250, 205));
    cleardevice();
    
    settextcolor(RED);
    settextstyle(36, 0, _T("黑体"));
    
    char result[100];
    if (gameStatus == GS_BLACK_WIN) {
        sprintf(result, "黑方胜利!");
    } else if (gameStatus == GS_WHITE_WIN) {
        sprintf(result, "白方胜利!");
    } else {
        sprintf(result, "平局!");
    }
    
    outtextxy(300, 150, result);
    
    // 显示总步数
    settextstyle(24, 0, _T("宋体"));
    settextcolor(BLACK);
    char stepInfo[50];
    sprintf(stepInfo, "总步数: %d", moveCount);
    outtextxy(350, 220, stepInfo);
    
    // 显示游戏模式
    char modeInfo[50];
    switch (gameMode) {
        case GM_PVP: sprintf(modeInfo, "模式: 双人对战"); break;
        case GM_PVE_EASY: sprintf(modeInfo, "模式: 人机对战(简单)"); break;
        case GM_PVE_MEDIUM: sprintf(modeInfo, "模式: 人机对战(中等)"); break;
        case GM_PVE_HARD: sprintf(modeInfo, "模式: 人机对战(困难)"); break;
    }
    outtextxy(350, 260, modeInfo);
    
    settextstyle(24, 0, _T("宋体"));
    settextcolor(BLACK);
    
    // 绘制选项
    int optionsY = 350;
    int optionHeight = 50;
    
    RECT endOptions[3];
    
    // 重新开始
    endOptions[0].left = 250;
    endOptions[0].top = optionsY;
    endOptions[0].right = 550;
    endOptions[0].bottom = optionsY + 40;
    
    setfillcolor(RGB(144, 238, 144));
    fillroundrect(endOptions[0].left, endOptions[0].top, 
                  endOptions[0].right, endOptions[0].bottom, 10, 10);
    outtextxy(350, optionsY + 5, _T("重新开始"));
    
    // 返回主菜单
    endOptions[1].left = 250;
    endOptions[1].top = optionsY + optionHeight;
    endOptions[1].right = 550;
    endOptions[1].bottom = optionsY + optionHeight + 40;
    
    setfillcolor(RGB(135, 206, 235));
    fillroundrect(endOptions[1].left, endOptions[1].top, 
                  endOptions[1].right, endOptions[1].bottom, 10, 10);
    outtextxy(350, optionsY + optionHeight + 5, _T("主菜单"));
    
    // 退出游戏
    endOptions[2].left = 250;
    endOptions[2].top = optionsY + 2 * optionHeight;
    endOptions[2].right = 550;
    endOptions[2].bottom = optionsY + 2 * optionHeight + 40;
    
    setfillcolor(RGB(255, 182, 193));
    fillroundrect(endOptions[2].left, endOptions[2].top, 
                  endOptions[2].right, endOptions[2].bottom, 10, 10);
    outtextxy(350, optionsY + 2 * optionHeight + 5, _T("退出游戏"));
    
    EndBatchDraw();
    
    // 等待选择
    while (true) {
        MOUSEMSG msg = GetMouseMsg();
        if (msg.uMsg == WM_LBUTTONDOWN) {
            for (int i = 0; i < 3; i++) {
                if (msg.x >= endOptions[i].left && msg.x <= endOptions[i].right &&
                    msg.y >= endOptions[i].top && msg.y <= endOptions[i].bottom) {
                    
                    if (i == 0) {
                        // 重新开始
                        initBoard();
                        gameStarted = true;
                        showGameStartPrompt();
                        return;
                    } else if (i == 1) {
                        // 返回主菜单
                        gameStarted = false;
                        return;
                    } else if (i == 2) {
                        // 退出游戏
                        exit(0);
                    }
                }
            }
        }
        Sleep(10);
    }
}

int main() {
    // 初始化图形窗口
    initgraph(800, 600);
    
    // 设置随机种子
    srand((unsigned)time(NULL));
    
    // 主循环
    while (true) {
        // 显示开始菜单
        if (!gameStarted) {
            showStartMenu();
            
            // 初始化游戏界面
            BeginBatchDraw();
            drawBoardBackground();
            drawGameInfo();
            EndBatchDraw();
        }
        
        // 游戏进行中
        if (gameStarted && gameStatus == GS_PLAYING) {
            // 处理鼠标消息
            if (MouseHit()) {
                MOUSEMSG msg = GetMouseMsg();
                
                if (msg.uMsg == WM_LBUTTONDOWN) {
                    // 将鼠标坐标转换为棋盘坐标
                    int col = (msg.x - OFFSET + CELL_SIZE / 2) / CELL_SIZE;
                    int row = (msg.y - OFFSET + CELL_SIZE / 2) / CELL_SIZE;
                    
                    // 处理玩家落子
                    if (row >= 0 && row < BOARD_SIZE && 
                        col >= 0 && col < BOARD_SIZE && 
                        board[row][col] == CT_EMPTY) {
                        
                        makeMove(row, col, currentPlayer);
                        
                        // 如果不是人机对战模式，切换玩家
                        if (gameStatus == GS_PLAYING) {
                            if (gameMode == GM_PVP) {
                                currentPlayer = (currentPlayer == CT_BLACK) ? CT_WHITE : CT_BLACK;
                                drawGameInfo();
                            } else {
                                // 人机对战模式，AI自动下棋
                                currentPlayer = CT_WHITE;
                                // 延迟一下，让玩家能看到自己的落子
                                Sleep(300);
                                aiMakeMove();
                            }
                        }
                    }
                }
            }
        }
        
        // 检查游戏是否结束
        if (gameStarted && gameStatus != GS_PLAYING) {
            showEndMenu();
            
            // 如果继续游戏，重新绘制界面
            if (gameStarted) {
                BeginBatchDraw();
                drawBoardBackground();
                drawGameInfo();
                EndBatchDraw();
            }
        }
        
        // 短暂延迟，降低CPU占用
        Sleep(10);
    }
    
    // 关闭图形窗口
    closegraph();
    return 0;
}
