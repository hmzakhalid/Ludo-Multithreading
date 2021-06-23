/*******************************************************************************************
*
*   Copyright (c) 2021 Hamza Khalid (@hmza.khalid on Instagram)
*
********************************************************************************************/
#include "raylib.h"
#include <iostream>
#include <array>
#include <vector>
#include <tuple>
#include <algorithm>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <random>
#include <time.h>
using namespace std;
// Screens
//--------------------------------------------------------------------------------------
int screen = 1;
// Variable Intialization
//--------------------------------------------------------------------------------------
int numTokens = -1;
tuple<int, int, int> **LudoGrid;
vector<int> diceVal(3, 0);
int diceCount = 0;
int dice = 1;
bool movePlayer = false; // Use Binary Semaphore
bool moveDice = true;    // Use Binary Semaphore
int turn = 1;
int lastTurn = turn; // Randomly generated
vector<int> nextTurn(4);
vector<int> winners;
// Mutex
//--------------------------------------------------------------------------------------
pthread_mutex_t mutex;
pthread_mutex_t mutexDice;
pthread_mutex_t mutexTurn;
void GenerateTurns()
{
    nextTurn.resize(4);
    int r = (rand() % 4) + 1;
    int count = 0;
    while (count != 4)
    {
        if (find(nextTurn.begin(), nextTurn.end(), r) != nextTurn.end())
        {
            r = (rand() % 4) + 1;
        }
        else
        {
            nextTurn[count] = r;
            count++;
        }
    }
}
int getTurn()
{
    int t;
    if (nextTurn.empty())
    {
        GenerateTurns();
        t = nextTurn[nextTurn.size() - 1];
        nextTurn.pop_back();
        while (count(winners.begin(), winners.end(), t))
        {
            t = nextTurn[nextTurn.size() - 1];
            nextTurn.pop_back();
        }
        return t;
    }
    else
    {
        t = nextTurn[nextTurn.size() - 1];
        nextTurn.pop_back();
        while (count(winners.begin(), winners.end(), t))
        {
            if (nextTurn.empty())
            {
                GenerateTurns();
                t = nextTurn[nextTurn.size() - 1];
                nextTurn.pop_back();
            }
            else
            {
                t = nextTurn[nextTurn.size() - 1];
                nextTurn.pop_back();
            }
        }
    }
    return t;
}
bool isTokenSafe(tuple<int, int, int> g)
{
    for (int i = 0; i < 4; i++)
    {
        if (g == make_tuple(i, 2, 1) || g == make_tuple(i, 0, 3))
            return true;
    }
    return false;
}

struct Token
{
    int id;
    int gridID;
    tuple<int, int, int> gridPos;
    int x, y;
    int initX, initY;
    bool isSafe;
    bool canGoHome;
    bool finished;
    bool isOut;
    sem_t semToken;
    Texture2D token;

    Token()
    {
        sem_init(&semToken, 0, 0);
        isSafe = true;
        finished = false;
        gridPos = make_tuple(-1, -1, -1);
        isOut = false;
        canGoHome = false;
        id = -1;
        x = y = initX = initY = 0;
    }

    void setTexture(Texture2D t)
    {
        sem_init(&semToken, 0, 0);
        isSafe = true;
        gridPos = make_tuple(-1, -1, -1);
        isOut = false;
        canGoHome = false;
        finished = false;
        id = -1;
        x = y = 0;
        token = t;
    }
    void setStart(int i)
    {
        id = i;
        switch (id)
        {
        case 0:
            x = 60;
            y = 360;
            break;
        case 1:
            x = 480;
            y = 60;
            break;
        case 2:
            x = 780;
            y = 480;
            break;
        case 3:
            x = 360;
            y = 780;
            break;
        default:
            break;
        }
    }
    void updateGrid()
    {
        LudoGrid[id][gridID] = gridPos;
        if (isTokenSafe(gridPos))
            isSafe = true;
        else
            isSafe = false;
    }
    void drawInit()
    {
        if (isOut == false && !finished)
            DrawTexture(token, initX, initY, WHITE);
        else
        {
            DrawTexture(token, x, y, WHITE);
        }
    }

    void outToken()
    {
        sem_post(&semToken);
        isOut = true;
        gridPos = make_tuple(id, 2, 1);
        DrawTexture(token, x, y, WHITE);
    }
    void inToken()
    {
        sem_wait(&semToken);
        isOut = false;
        setStart(id);
        gridPos = make_tuple(-1, -1, -1);
    }
    void move(int roll)
    {
        if (roll == 0)
            return;
        cout << "\nDice Val " << roll << endl;
        int g = get<0>(gridPos);
        int r = get<1>(gridPos);
        int c = get<2>(gridPos);
        cout << "Grid: " << g
             << " Row: " << r
             << " Col: " << c << endl;
        int next = 0, cur = 0;
        cout << "C+Roll " << c + roll << endl;
        if (c + roll >= 5)
        {
            next = (c + roll) - 5;
            cur = roll - next;
            cout << "Next: " << next
                 << " Cur: " << cur << endl;
        }
        else
        {
            cur = roll;
        }
        switch (g)
        {
        case 0:
            switch (r)
            {
            case 0:
                x = x - (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next >= 1)
                {
                    y = y - 60;
                    next--;
                    get<1>(gridPos) = 1;
                    get<2>(gridPos) = 0;
                    move(next);
                }
                break;
            case 1:
                if (canGoHome && id == get<0>(gridPos))
                {
                    if (roll + c <= 6)
                    {
                        x = x + (roll * 60);
                        get<2>(gridPos) = c + roll;
                        if (get<2>(gridPos) == 6)
                        {
                            // Implement semaphore
                            sem_destroy(&semToken);
                            isOut = false;
                            finished = true;
                            x = -100;
                            y = -100;
                        }
                    }
                }
                else
                {
                    next = 1;
                    y = y - (next * 60);
                    cur--;
                    get<1>(gridPos) = 2;
                    get<2>(gridPos) = 0;
                    move(cur);
                    return;
                }
                break;
            case 2:
                x = x + (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next != 0)
                {
                    x += 60;
                    y = y - (next * 60);
                    get<0>(gridPos) = 1;
                    get<1>(gridPos) = 0;
                    get<2>(gridPos) = next - 1;
                }
                break;
            default:
                break;
            }
            break;
        case 1:
            switch (r)
            {
            case 0:
                y = y - (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next >= 1)
                {
                    x = x + 60;
                    next--;
                    get<1>(gridPos) = 1;
                    get<2>(gridPos) = 0;
                    move(next);
                }
                break;
            case 1:
                if (canGoHome && id == get<0>(gridPos))
                {
                    if (roll + c <= 6)
                    {
                        y = y + (roll * 60);
                        get<2>(gridPos) = c + roll;
                        if (get<2>(gridPos) == 6)
                        {
                            // Implement semaphore
                            sem_destroy(&semToken);
                            //UnloadTexture(token);
                            finished = true;
                            isOut = false;
                            x = -100;
                            y = -100;
                        }
                    }
                    else
                    {
                        //diceVal.insert(diceVal.begin(), 1, roll);
                        cout << "no possible move" << endl;
                        return;
                    }
                }
                else
                {
                    // Sus
                    next = 1;
                    x = x + (next * 60);
                    cur--;
                    get<1>(gridPos) = 2;
                    get<2>(gridPos) = 0;
                    move(cur);
                    return;
                }
                break;
            case 2:
                y = y + (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next != 0)
                {
                    y += 60;
                    x = x + (next * 60);
                    get<0>(gridPos) = 2;
                    get<1>(gridPos) = 0;
                    get<2>(gridPos) = next - 1;
                }
                break;
            default:
                break;
            }
            break;
        case 2:
            switch (r)
            {
            case 0:
                x = x + (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next >= 1)
                {
                    y = y + 60;
                    next--;
                    get<1>(gridPos) = 1;
                    get<2>(gridPos) = 0;
                    move(next);
                }
                break;
            case 1:
                if (canGoHome && id == get<0>(gridPos))
                {
                    if (roll + c <= 6)
                    {
                        x = x - (roll * 60);
                        get<2>(gridPos) = c + roll;
                        if (get<2>(gridPos) == 6)
                        {
                            // Implement semaphore
                            sem_destroy(&semToken);
                            //UnloadTexture(token);
                            finished = true;
                            isOut = false;
                            x = -100;
                            y = -100;
                        }
                    }
                    else
                    {
                        // No Possible Move
                        return;
                    }
                }
                else
                {
                    next = 1;
                    y = y + (next * 60);
                    cur--;
                    get<1>(gridPos) = 2;
                    get<2>(gridPos) = 0;
                    move(cur);
                    return;
                }
                break;
            case 2:
                x = x - (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next != 0)
                {
                    x -= 60;
                    y = y + (next * 60);
                    get<0>(gridPos) = 3;
                    get<1>(gridPos) = 0;
                    get<2>(gridPos) = next - 1;
                }
                break;
            default:
                break;
            }
            break;
        case 3:
            switch (r)
            {
            case 0:
                y = y + (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next >= 1)
                {
                    x = x - 60;
                    next--;
                    get<1>(gridPos) = 1;
                    get<2>(gridPos) = 0;
                    move(next);
                }
                break;
            case 1:
                if (canGoHome && id == get<0>(gridPos))
                {
                    if (roll + c <= 6)
                    {
                        y = y - (roll * 60);
                        get<2>(gridPos) = c + roll;
                        if (get<2>(gridPos) == 6)
                        {
                            // Implement semaphore
                            sem_destroy(&semToken);
                            finished = true;
                            isOut = false;
                            x = -100;
                            y = -100;
                        }
                    }
                    else
                    {
                        // No Possible Move
                        return;
                    }
                }
                else
                {
                    next = 1;
                    x = x - (next * 60);
                    cur--;
                    get<1>(gridPos) = 2;
                    get<2>(gridPos) = 0;
                    move(cur);
                    return;
                }
                break;
            case 2:
                y = y - (cur * 60);
                get<2>(gridPos) = c + cur;
                if (next != 0)
                {
                    y -= 60;
                    x = x - (next * 60);
                    get<0>(gridPos) = 0;
                    get<1>(gridPos) = 0;
                    get<2>(gridPos) = next - 1;
                }
                break;
            default:
                break;
            }
            break;
        default:
            break;
        }
    }
};

int m = 0;

struct Player
{
    int id;
    Token *tokens;
    Color color;
    bool completed;
    int score;
    bool isPlaying;
    Player(){
    };

    Player(int i, Color c, Texture2D t)
    {
        score = 0;
        completed = false;
        id = i;
        color = c;
        isPlaying = false;
        tokens = new Token[numTokens];
        // tokens.fill(Token(t));

        for (int k = 0; k < numTokens; k++)
        {
            tokens[k].setTexture(t);
            tokens[k].setStart(id);
            for (int j = 0; j < numTokens; j++)
            {
                tokens[j].gridID = j;
            }
        }
        int arr[4][4][2] = {{{90, 90}, {200, 90}, {90, 200}, {200, 200}},
                            {{630, 90}, {740, 90}, {630, 200}, {740, 200}},
                            {{630, 630}, {740, 740}, {630, 740}, {740, 630}},
                            {{90, 630}, {200, 630}, {90, 740}, {200, 740}}};
        for (int k = 0; k < numTokens; k++)

        {
            tokens[k].initX = arr[id][k][0];
            tokens[k].initY = arr[id][k][1];
        }
    }

    void checkPlayState()
    {
        for (int i = 0; i < numTokens; i++)
        {
            tokens[i].isOut = true;
        }
    }
    void setPlayer(int i, Color c, Texture2D t)
    {
        completed = false;
        score = 0;
        id = i;
        color = c;
        isPlaying = false;
        tokens = new Token[numTokens];
        for (int k = 0; k < numTokens; k++)
        {
            tokens[k].setTexture(t);
            tokens[k].setStart(id);
            tokens[k].gridID = k;
        }
        int arr[4][4][2] = {{{90, 90}, {200, 90}, {90, 200}, {200, 200}},
                            {{630, 90}, {740, 90}, {630, 200}, {740, 200}},
                            {{630, 630}, {740, 740}, {630, 740}, {740, 630}},
                            {{90, 630}, {200, 630}, {90, 740}, {200, 740}}};

        for (int k = 0; k < numTokens; k++)
        {
            tokens[k].initX = arr[id][k][0];
            tokens[k].initY = arr[id][k][1];
        }
    }
    void Start()
    {
        bool found = false;
        bool finCheck = false;
        for (int i = 0; i < numTokens; i++)
        {
            if (LudoGrid[id][i] == make_tuple(-2, -2, -2))
            {
                cout << "Found this as well" << endl;
                tokens[i].inToken();
                LudoGrid[id][i] = make_tuple(-1, -1, -1);
            }
            tokens[i].drawInit();
            tokens[i].updateGrid();
            if (tokens[i].finished == false)
            {
                finCheck = true;
            }
            if (tokens[i].isOut == true)
            {
                found = true;
            }
        }
        if (!finCheck)
        {
            completed = true;
            cout << "THE PLAYER HAS COMPLETED" << endl;
            score++;
            winners.push_back(id + 1);
        }
        if (!found)
        {
            isPlaying = false;
        }
    }
    void allowHome()
    {
        for (int i = 0; i < numTokens; i++)
        {
            tokens[i].canGoHome = true;
        }
    }
    // implement collisions and Safe Spots
    void collision(int movedToken)
    {
        if (tokens[movedToken].isSafe)
        {
            cout << "Moved Token: " << movedToken << " is on a Safe Spot" << endl;
            return;
        }
        for (int pid = 0; pid < 4; pid++)
        {
            for (int tokenId = 0; tokenId < numTokens; tokenId++)
            {
                if (LudoGrid[pid][tokenId] == tokens[movedToken].gridPos && pid != id)
                {
                    allowHome();
                    // increase player score
                    score++;
                    LudoGrid[pid][tokenId] = make_tuple(-2, -2, -2);
                    cout << "found collison" << endl;
                    cout << "pid: " << pid << endl;
                    cout << "tokenID: " << tokenId << endl;
                    cout << "movedToken: " << movedToken << endl;
                    cout << "id: " << id << endl;
                }
            }
        }
    }

    void rollDice()
    {
        if (moveDice == true)
        {
            //sleep(0.1);
            pthread_mutex_lock(&mutexDice);
            int temp[3] = {6, 5, 4};
            if (id == turn - 1 && movePlayer == false && !completed)
            {
                if (completed)
                {
                    turn = getTurn();
                    pthread_mutex_unlock(&mutexDice);
                    return;
                }
                Rectangle diceRec = (Rectangle){990, 300, 108.0, 108.0};
                while(!IsMouseButtonDown(MOUSE_LEFT_BUTTON)) // Kinda works but idk change this shit
                {
                    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                    {
                        if (CheckCollisionPointRec(GetMousePosition(), diceRec))
                        {
                            dice = (rand() % 6) + 1;
                            diceCount++;
                            if (diceCount == 3 && dice == 6)
                            {
                                pthread_mutex_lock(&mutexTurn);
                                diceVal.resize(3);
                                fill(diceVal.begin(), diceVal.end(), 0);
                                turn = getTurn();
                                diceCount = 0;
                                lastTurn = turn;
                                pthread_mutex_unlock(&mutexTurn);
                                pthread_mutex_unlock(&mutexDice);
                                return;
                            }
                            if (dice == 6)
                            {
                                diceVal[diceCount - 1] = dice;
                                m++;
                                lastTurn = turn;
                                pthread_mutex_unlock(&mutexDice);
                                return;
                            }
                            else
                            {
                                diceVal[diceCount - 1] = dice;
                                if (isPlaying == true || diceVal[0] == 6)
                                {
                                    movePlayer = true;
                                    moveDice = false;
                                    lastTurn = turn;
                                    pthread_mutex_unlock(&mutexDice);
                                    return;
                                }
                                else
                                {
                                    pthread_mutex_lock(&mutexTurn);
                                    diceVal.resize(3);
                                    fill(diceVal.begin(), diceVal.end(), 0);
                                    turn = getTurn();
                                    lastTurn = turn;
                                    pthread_mutex_unlock(&mutexTurn);
                                }
                                diceCount = 0;
                                m = 0;
                            }
                        }
                    }
                }
            }

            pthread_mutex_unlock(&mutexDice);
        }
    }

    void move()
    {
        bool found = false;
        if (movePlayer == true && lastTurn - 1 == id)
        {
            for (int i = 0; i < numTokens; i++)
            {
                Rectangle diceRec;
                if (tokens[i].isOut)
                {
                    diceRec = (Rectangle){(float)tokens[i].x, (float)tokens[i].y, 60.0, 60.0};
                }
                else
                {
                    diceRec = (Rectangle){(float)tokens[i].initX, (float)tokens[i].initY, 60.0, 60.0};
                }
                if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))
                {
                    if (CheckCollisionPointRec(GetMousePosition(), diceRec))
                    {
                        found = true;
                        if (tokens[i].isOut == false && diceVal[0] == 6)
                        {
                            tokens[i].outToken();
                            isPlaying = true;
                            tokens[i].updateGrid();
                            diceVal.erase(diceVal.begin());
                        }
                        else if (tokens[i].isOut == true)
                        {
                            tokens[i].move(diceVal[0]);
                            tokens[i].updateGrid();
                            collision(i); // checking collision
                            diceVal.erase(diceVal.begin());
                            if (diceVal.empty() || diceVal[0] == 0)
                            {
                                pthread_mutex_lock(&mutexTurn);
                                turn = getTurn();
                                lastTurn = turn;
                                diceVal.resize(3);
                                fill(diceVal.begin(), diceVal.end(), 0);
                                movePlayer = false;
                                moveDice = true;
                                diceCount = 0;
                                m = 0;
                                pthread_mutex_unlock(&mutexTurn);
                            }
                        }
                    }
                }
                if (found)
                    break;
            }
        }
    }
};

void *playerThread(void *args)
{
    Player *p = (Player *)args;
    while (!WindowShouldClose())
    {
        pthread_mutex_lock(&mutex);
        p->rollDice();
        p->move();
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void DrawDice(Texture2D arr[])
{
    DrawTexture(arr[dice - 1], 990, 300, WHITE);
}
void DrawScore(int p1, int p2, int p3, int p4)
{
    DrawText("SCORE", 960, 0, 50, BLACK);
    DrawText(FormatText("RED:     %01i", p1), 910, 50, 30, DARKGRAY);
    DrawText(FormatText("Green:   %01i", p2), 910, 100, 30, DARKGRAY);
    DrawText(FormatText("Yellow:  %01i", p3), 910, 150, 30, DARKGRAY);
    DrawText(FormatText("Blue:    %01i", p4), 910, 200, 30, DARKGRAY);
    DrawText("Click to Roll Dice ", 910, 250, 30, BLACK);
    switch (turn)
    {
    case 1:
        DrawText(FormatText("Player : %01i RED", turn), 910, 450, 30, DARKGRAY);
        break;
    case 2:
        DrawText(FormatText("Player : %01i GREEN", turn), 910, 450, 30, DARKGRAY);
        break;
    case 3:
        DrawText(FormatText("Player : %01i YELLOW", turn), 910, 450, 30, DARKGRAY);
        break;
    case 4:
        DrawText(FormatText("Player : %01i BLUE", turn), 910, 450, 30, DARKGRAY);
        break;
    default:
        break;
    }
    DrawText(FormatText("Player : %01i", turn), 910, 450, 30, DARKGRAY);
    DrawText("Dice Values", 910, 500, 30, DARKGRAY);
    for (unsigned int i = 0; i < diceVal.size(); i++)
    {
        DrawText(FormatText("%01i", diceVal[i]), 910 + (i * 50), 550, 30, DARKGRAY);
    }
}

void DrawStartScreen()

{

    DrawRectangle(0, 0, 900, 900, RAYWHITE);

    DrawText("L", 400, 50, 150, RED);

    DrawText("U", 500, 50, 150, GREEN);

    DrawText("D", 600, 50, 150, YELLOW);

    DrawText("O", 700, 50, 150, BLUE);

    DrawText("Enter Number of Tokens", 300, 250, 50, DARKGRAY);
    DrawText("By Hamza Khalid", 700, 750, 50, DARKGRAY);

    if (numTokens != -1)

        DrawText(FormatText("%01i", numTokens), 550, 350, 60, DARKGRAY);

    if (IsKeyPressed('1'))

    {

        numTokens = 1;
    }

    if (IsKeyPressed('2'))

    {

        numTokens = 2;
    }

    if (IsKeyPressed('3'))

    {

        numTokens = 3;
    }

    if (IsKeyPressed('4'))

    {

        numTokens = 4;
    }

    Rectangle st = {450, 450, 220, 100};

    DrawRectangleRec(st, DARKGRAY);

    DrawText("START", 470, 470, 50, WHITE);

    if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON))

    {

        if (CheckCollisionPointRec(GetMousePosition(), st))

        {

            if (numTokens <= 4 && numTokens >= 1)

            {

                screen = 2;

                LudoGrid = new tuple<int, int, int> *[4];

                for (int i = 0; i < 4; i++)

                {

                    LudoGrid[i] = new tuple<int, int, int>[numTokens];
                }
            }
        }
    }
}

void DrawWinScreen()
{

    DrawRectangle(0, 0, 900, 900, RAYWHITE);

    DrawText("WINNERS", 320, 50, 150, DARKGRAY);

    for (int i = 0; i < 4; i++)
    {
        DrawText(FormatText("%01i.", i + 1), 450, 350 + (i * 80), 60, DARKGRAY);
        switch (winners[i])
        {
        case 1:
            DrawText("RED", 550, 350 + (i * 80), 60, DARKGRAY);
            break;
        case 2:
            DrawText("GREEN", 550, 350 + (i * 80), 60, DARKGRAY);
            break;
        case 3:
            DrawText("YELLOW", 550, 350 + (i * 80), 60, DARKGRAY);
            break;
        case 4:
            DrawText("BLUE", 550, 350 + (i * 80), 60, DARKGRAY);
            break;
        default:
            break;
        }
    }
}

void *Master(void *args)
{
    // Initialization
    //--------------------------------------------------------------------------------------
    const int screenWidth = 1200;
    const int screenHeight = 900;
    // Start Screen
    //--------------------------------------------------------------------------------------
    InitWindow(screenWidth, screenHeight, "Ludo Game");
    SetTargetFPS(60);
    // Loading Textures
    //--------------------------------------------------------------------------------------
    Texture2D LudoBoard = LoadTexture("assets/board1.png");
    Texture2D red = LoadTexture("assets/red.png");
    Texture2D green = LoadTexture("assets/green.png");
    Texture2D blue = LoadTexture("assets/blue.png");
    Texture2D yellow = LoadTexture("assets/yellow.png");
    Texture2D Dice[6];
    for (int i = 0; i < 6; i++)
    {
        string path = "assets/" + to_string(i + 1) + ".png";
        Dice[i] = LoadTexture(path.c_str());
    }
    // Creating Players
    //--------------------------------------------------------------------------------------
    Player P1, P2, P3, P4;
    // Creating Threads
    //--------------------------------------------------------------------------------------
    pthread_mutex_init(&mutex, NULL);
    pthread_mutex_init(&mutexDice, NULL);
    pthread_mutex_init(&mutexTurn, NULL);
    pthread_t th[4];
    bool Initial = true;
    vector<bool> FinishedThreads(4, false);
    // Initializing Variables
    //--------------------------------------------------------------------------------------
    bool WinnerScreen = false;
    GenerateTurns();
    for (auto i : nextTurn)
    {
        cout << i << " ";
    }
    cout << endl;
    turn = nextTurn[nextTurn.size() - 1];
    nextTurn.pop_back();

    // Main game loop
    //--------------------------------------------------------------------------------------
    while (!WindowShouldClose()) // Detect window close button or ESC key
    {
        // Update and Draw
        //----------------------------------------------------------------------------------
        ClearBackground(RAYWHITE);
        BeginDrawing();
        if (screen == 1)
        {
            DrawStartScreen();
        }
        else if (screen == 2)
        {
            if (Initial)
            {
                P1.setPlayer(0, RED, red);
                P2.setPlayer(1, GREEN, green);
                P3.setPlayer(2, YELLOW, yellow);
                P4.setPlayer(3, BLUE, blue);
                pthread_create(&th[0], NULL, &playerThread, &P1);
                pthread_create(&th[1], NULL, &playerThread, &P2);
                pthread_create(&th[2], NULL, &playerThread, &P3);
                pthread_create(&th[3], NULL, &playerThread, &P4);
                Initial = false;
            }
            // Draw Ludo Board
            //----------------------------------------------------------------------------------
            DrawTexture(LudoBoard, 0, 0, WHITE);
            DrawScore(P1.score, P2.score, P3.score, P4.score);
            DrawDice(Dice);
            int count = 0;
            int index = 0;
            if (!P1.completed)
                P1.Start();
            else
            {
                if (!FinishedThreads[0])
                {
                    cout << "THREAD CANCELLED" << endl;
                    pthread_cancel(th[0]);
                    diceVal.resize(3);
                    fill(diceVal.begin(), diceVal.end(), 0);
                    turn = getTurn();
                    movePlayer = false;
                    FinishedThreads[0] = true;
                }
            }
            if (!P2.completed)
                P2.Start();
            else
            {
                if (!FinishedThreads[1])
                {
                    cout << "THREAD CANCELLED" << endl;
                    pthread_cancel(th[1]);
                    turn = getTurn();
                    movePlayer = false;
                    diceVal.resize(3);
                    fill(diceVal.begin(), diceVal.end(), 0);
                    FinishedThreads[1] = true;
                }
            }
            if (!P3.completed)
                P3.Start();
            else
            {
                if (!FinishedThreads[2])
                {
                    cout << "THREAD CANCELLED" << endl;
                    pthread_cancel(th[2]);
                    turn = getTurn();
                    movePlayer = false;
                    diceVal.resize(3);
                    fill(diceVal.begin(), diceVal.end(), 0);
                    FinishedThreads[2] = true;
                }
            }
            if (!P4.completed)
                P4.Start();
            else
            {
                if (!FinishedThreads[3])
                {
                    cout << "THREAD CANCELLED" << endl;
                    pthread_cancel(th[3]);
                    diceVal.resize(3);
                    fill(diceVal.begin(), diceVal.end(), 0);
                    turn = getTurn();
                    movePlayer = false;
                    FinishedThreads[3] = true;
                }
            }

            for (int g = 0; g < 4; g++)
            {
                if (FinishedThreads[g] == true)
                    count++;
                else
                {
                    index = g;
                }
            }
            if (count >= 3)
            {
                winners.push_back(index + 1);
                screen = 3;
                cout << "Game Finished" << endl;
            }
        }
        else
        {

            DrawWinScreen();
        }
        EndDrawing();
        //----------------------------------------------------------------------------------
    }
    // Joining Threads
    //----------------------------------------------------------------------------------
    pthread_mutex_destroy(&mutex);
    pthread_mutex_destroy(&mutexDice);
    pthread_mutex_destroy(&mutexTurn);
    pthread_join(th[0], NULL);
    pthread_join(th[1], NULL);
    pthread_join(th[2], NULL);
    pthread_join(th[3], NULL);
    // De-Initialization
    //--------------------------------------------------------------------------------------
    UnloadTexture(LudoBoard);
    UnloadTexture(red);
    UnloadTexture(green);
    UnloadTexture(blue);
    UnloadTexture(yellow);
    for (int i = 0; i < 6; i++)
    {

        UnloadTexture(Dice[i]);
    }
    CloseWindow(); // Close window and OpenGL context
    //--------------------------------------------------------------------------------------
    return NULL;
}

int main(void)

{

    srand(time(NULL));
    pthread_t masterThread;
    pthread_create(&masterThread, NULL, &Master, NULL);
    pthread_join(masterThread, NULL);
    return 0;
}