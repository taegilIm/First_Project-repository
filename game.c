#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <time.h>
#include <conio.h>
#include <malloc.h>
#include <math.h>

#define ROW 0
#define COL 1
// 화면에 생성될 위치 조정
#define R_BASIS 6
#define C_BASIS 20

// 아이템 갯수
#define ITEM_COUNT 3
// 아이템 옵션 갯수
#define ITEM_OPTION 2

// 색깔 넘버
#define BLACK 0
#define GRAY 7
#define WHITE 15
#define BLUE 9
#define RED 12
#define YELLOW 14
#define GREEN 10

#define KEY_ESC 27
#define KEY_UP (256 + 72)
#define KEY_DOWN (256 + 80)
#define KEY_LEFT (256 + 75)
#define KEY_RIGHT (256 + 77)

#define WALL -1
#define MOB -2
#define PLAYER 1
#define GOLL 2
#define BOMB 3

#define NORTH 0
#define SOUTH 1
#define WEST 2
#define EAST 3

// 플레이어 정보
struct Objective
{
    int x;
    int y;
    // 혹시 모를 수정을 위한 옵션들
    double max_health;
    double health;
    double initial_dmg;
    double dmg;
};
struct Objective player = {0, 0, 100, 100, 10, 10};
struct Objective *mob;
int mob_count = 0;

// 스테이지 생성 관련 변수
int **stage = NULL;
int size[2];
int sizeV[50];
int sizeRVoid[50];
// 다음 스테이지, 종료 관련 변수
int Round = 1;
int end = 0;
int next = 0;
int hit_count = 0;

// 추후 생길 예정
int itemID[ITEM_COUNT][ITEM_OPTION] = {0};

// 기본 함수 셋팅
void setPos(int, int);
void SetCursorVisible();
void SetTextCol(int);
// 초기 설정
void SetStage();
void VoidShuffle();
void SetObj();
// 그리기
void DrawStage();
void TextDiff(int, int);
void DrawInv();
// 움직임
void Move();
void MobMove();
void MobEncounter(int);
// 파일 관련
void Rank();

int main(void)
{
    srand(time(NULL));
    system("mode CON COLS=140 LINES=40");
    SetConsoleTitle(TEXT("Simple Game"));
    SetCursorVisible();

    while (1)
    {
        SetStage();
        SetObj();
        DrawStage();

        while (end < 1)
        {
            if (player.health <= 0)
                end++;
            else if (next > 0)
                break;
            Move();
        }

        if (end > 0)
        {
            setPos(C_BASIS + size[COL] - 6, R_BASIS + size[ROW] / 2);
            printf("- Game Over -");
            Sleep(1000);
            break;
        }
        Round++;
        next = 0;
        player.max_health += player.max_health * 0.05;
        player.health += player.max_health * 0.1;
        if (player.health > player.max_health)
            player.health = player.max_health;
        hit_count = 0;
    }
    return 0;
}

void setPos(int x, int y)
{
    COORD pos = {x, y};
    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}

void SetCursorVisible()
{
    CONSOLE_CURSOR_INFO ci = {1, 0};
    SetConsoleCursorInfo(GetStdHandle(STD_OUTPUT_HANDLE), &ci);
}

void SetTextCol(int num)
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), num);
}

void SetStage()
{
    if (Round > 1)
    {
        for (int i = 0; i < size[ROW]; i++)
        {
            free(stage[i]);
        }
        free(stage);
    }
    // 동적할당
    int numR = rand() % 7 + 12 + (int)(Round / 2);
    if (numR > 25)
        numR = 25;
    stage = malloc(sizeof(int *) * numR);

    int numC = rand() % 5 + 18 + Round;
    if (numC > 50)
        numC = 50;
    for (int i = 0; i < numR; i++)
    {
        stage[i] = malloc(sizeof(int) * numC);
        if (stage[i] == NULL)
            stage[i] = malloc(sizeof(int) * numC);
    }

    size[ROW] = numR;
    size[COL] = numC;
    // 보이드 값 생성
    for (int i = 0; i < (size[ROW] - 2); i++)
    {
        int numV = rand() % size[COL] / 3 + 2;
        sizeV[i] = numV;
    }
    // 스테이지 값 채우기
    for (int i = 0; i < size[ROW]; i++)
    {
        stage[i][0] = WALL;
        stage[i][size[COL] - 1] = WALL;
        if (i == 0 || i == (size[ROW] - 1))
        {
            // 외곽 처리
            for (int j = 1; j < (size[COL] - 1); j++)
            {
                stage[i][j] = WALL;
            }
        }
        else if (i > 0 || i < (size[ROW] - 1))
        {
            // 빈공간 처리
            for (int j = 1; j < (size[COL] - 1); j++)
            {
                stage[i][j] = 0;
            }
            // 보이드 처리
            VoidShuffle();
            for (int j = 0; j < 2; j++)
            {
                if (j == 1)
                {
                    for (int k = 0; k < sizeRVoid[i - 1]; k++)
                    {
                        stage[i][numC - k - 2] = WALL;
                    }
                }
                else
                {
                    for (int k = 0; k < sizeV[i - 1]; k++)
                    {
                        stage[i][k + 1] = WALL;
                    }
                }
            }
        }
    }
}
// 오른쪽 보이드 행 랜덤화
void VoidShuffle()
{
    for (int i = 0; i < (size[ROW] - 2); i++)
    {
        sizeRVoid[i] = sizeV[i];
    }
    int temp, rNum;
    for (int i = 0; i < 20; i++)
    {
        rNum = rand() % (size[ROW] - 2);
        temp = sizeRVoid[0];
        sizeRVoid[0] = sizeRVoid[rNum];
        sizeRVoid[rNum] = temp;
    }
}

void SetObj()
{
    // 플레이어
    while (1)
    {
        int rNumR = rand() % (size[ROW] - 2) + 1;
        if (rNumR < 1)
            rNumR = 1;
        int able = size[COL] - (sizeV[rNumR] + sizeRVoid[rNumR] + 2);
        int rNumC = rand() % able + sizeV[rNumR];
        player.y = rNumR;
        player.x = rNumC;
        if (stage[rNumR][rNumC] == 0)
        {
            stage[rNumR][rNumC] = PLAYER;
            break;
        }
    }
    // 다음 스테이지
    while (1)
    {
        int rNumR = rand() % (size[ROW] - 2) + 1;
        if (rNumR < 1)
            rNumR = 1;
        int able = size[COL] - (sizeV[rNumR] + sizeRVoid[rNumR] + 2);
        int rNumC = rand() % able + sizeV[rNumR];

        if (stage[rNumR][rNumC] == 0)
        {
            stage[rNumR][rNumC] = GOLL;
            break;
        }
    }
    // 지형지물
    int count = rand() % 5 + (Round * 3);
    if (count > 50)
        count = 50;
    while (count > 0)
    {
        int rNumR = rand() % (size[ROW] - 2) + 1;
        if (rNumR < 1)
            rNumR = 1;
        int able = size[COL] - (sizeV[rNumR] + sizeRVoid[rNumR] + 2);
        int rNumC = rand() % able + sizeV[rNumR];

        if (stage[rNumR][rNumC] == 0)
        {
            stage[rNumR][rNumC] = BOMB;
            count--;
        }
    }
    // 적
    if (mob != NULL)
        free(mob);
    mob_count = rand() % 2 + Round;
    if (mob_count > 40)
        mob_count = 40;
    mob = malloc(sizeof(struct Objective) * mob_count);

    int Mcount = mob_count;
    mob[0].dmg = 10 + (pow((double)(Round - 1), 1.25));
    if (Round > 20)
        mob[0].dmg = 100;

    while (Mcount > 0)
    {
        int rNumR = rand() % (size[ROW] - 2) + 1;
        if (rNumR < 1)
            rNumR = 1;
        int able = size[COL] - (sizeV[rNumR] + sizeRVoid[rNumR] + 2);
        int rNumC = rand() % able + sizeV[rNumR];

        if (stage[rNumR][rNumC] == 0)
        {
            mob[Mcount - 1].x = rNumC;
            mob[Mcount - 1].y = rNumR;
            stage[rNumR][rNumC] = MOB;
            Mcount--;
        }
    }
}

void DrawStage()
{
    system("cls");
    setPos(0, 0);
    // 라운드, 데미지 별 글자 색 변경
    if (Round >= 5)
    {
        if (Round >= 10)
        {
            if (Round >= 20)
                TextDiff(1, RED);
            else
                TextDiff(1, YELLOW);
        }
        else
            TextDiff(1, BLUE);
    }
    else
        TextDiff(1, GREEN);

    setPos(C_BASIS + size[1] * 2 + 4, R_BASIS);
    if (mob[0].dmg >= 15)
    {
        if (mob[0].dmg >= 30)
        {
            if (mob[0].dmg >= 50)
                TextDiff(2, RED);
            else
                TextDiff(2, YELLOW);
        }
        else
            TextDiff(2, BLUE);
    }
    else
        TextDiff(2, GREEN);

    setPos(20, 0);
    printf("Health: %.1f / %.1f", player.health, player.max_health);
    setPos(C_BASIS + size[COL] * 2 - 13, R_BASIS + size[ROW] + 1);
    printf("ESC: game end");
    // 스테이지 생성
    for (int i = 0; i < size[ROW]; i++)
    {
        for (int j = 0; j < size[COL]; j++)
        {
            // 기본 row columm 시작값에 따라 생성위치 변화
            if (stage[i][j] == 0)
            {
                setPos((j * 2) + C_BASIS, i + R_BASIS);
                printf("  ");
            }
            else if (stage[i][j] == WALL)
            {
                setPos((j * 2) + C_BASIS, i + R_BASIS);
                SetTextCol(GRAY);
                printf("■ ");
                SetTextCol(WHITE);
            }
            else if (stage[i][j] == PLAYER)
            {
                setPos((j * 2) + C_BASIS, i + R_BASIS);
                SetTextCol(BLUE);
                printf("○ ");
                SetTextCol(WHITE);
            }
            else if (stage[i][j] == GOLL)
            {
                setPos((j * 2) + C_BASIS, i + R_BASIS);
                SetTextCol(BLUE);
                printf("◈ ");
                SetTextCol(WHITE);
            }
            else if (stage[i][j] == BOMB)
            {
                setPos((j * 2) + C_BASIS, i + R_BASIS);
                SetTextCol(RED);
                printf("■");
                SetTextCol(WHITE);
            }
            else if (stage[i][j] == MOB)
            {
                setPos((j * 2) + C_BASIS, i + R_BASIS);
                SetTextCol(RED);
                printf("○ ");
                SetTextCol(WHITE);
            }
        }
        printf("\n");
    }
}

void TextDiff(int num, int color)
{
    SetTextCol(color);
    if (num == 1)
        printf("Stage: %d", Round);
    else if (num == 2)
        printf("Monster DMG: %.1f", mob[0].dmg);
    SetTextCol(WHITE);
}

void DrawInv()
{
    // 아직 해당 내용 없음
}

void Move()
{
    if (_kbhit())
    {
        int key = _getch();
        if (key == 0 || key == 224)
            key = 256 + _getch();
        setPos(10, size[ROW] + R_BASIS + 2);
        printf("                                   \n                                    ");
        switch (key)
        {
        case KEY_ESC:
            end++;
            break;
        case KEY_DOWN:
            if (stage[player.y + 1][player.x] >= 0)
            {
                // 장애물
                if (stage[player.y + 1][player.x] == BOMB)
                {
                    stage[player.y + 1][player.x] = 0;
                    player.health -= player.max_health / 4;
                    setPos(20, 0);
                    printf("                                ");
                    setPos(20, 0);
                    printf("Health: %.1f / %.1f", player.health, player.max_health);
                }
                // 다음 스테이지
                else if (stage[player.y + 1][player.x] == GOLL)
                {
                    next = 1;
                    break;
                }
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                printf("  ");
                player.y += 1;
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                SetTextCol(BLUE);
                printf("○ ");
                SetTextCol(WHITE);
                // 내가 움직이면 몹도 같이 움직임
                MobMove();
            }
            // 몹
            else if (stage[player.y + 1][player.x] == MOB)
            {
                MobEncounter(0);
                MobMove();
                break;
            }
            break;
        case KEY_UP:
            if (stage[player.y - 1][player.x] >= 0)
            {
                if (stage[player.y - 1][player.x] == BOMB)
                {
                    stage[player.y - 1][player.x] = 0;
                    player.health -= player.max_health / 4;
                    setPos(20, 0);
                    printf("                                    ");
                    setPos(20, 0);
                    printf("Health: %.1f / %.1f", player.health, player.max_health);
                }
                else if (stage[player.y - 1][player.x] == GOLL)
                {
                    next = 1;
                    break;
                }
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                printf("  ");
                player.y -= 1;
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                SetTextCol(BLUE);
                printf("○ ");
                SetTextCol(WHITE);
                MobMove();
            }
            else if (stage[player.y - 1][player.x] == MOB)
            {
                MobEncounter(0);
                MobMove();
                break;
            }
            break;
        case KEY_LEFT:
            if (stage[player.y][player.x - 1] >= 0)
            {
                if (stage[player.y][player.x - 1] == BOMB)
                {
                    stage[player.y][player.x - 1] = 0;
                    player.health -= player.max_health / 4;
                    setPos(20, 0);
                    printf("                                    ");
                    setPos(20, 0);
                    printf("Health: %.1f / %.1f", player.health, player.max_health);
                }
                else if (stage[player.y][player.x - 1] == GOLL)
                {
                    next = 1;
                    break;
                }
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                printf("  ");
                player.x -= 1;
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                SetTextCol(BLUE);
                printf("○ ");
                SetTextCol(WHITE);
                MobMove();
            }
            else if (stage[player.y][player.x - 1] == MOB)
            {
                MobEncounter(0);
                MobMove();
                break;
            }
            break;
        case KEY_RIGHT:
            if (stage[player.y][player.x + 1] >= 0)
            {
                if (stage[player.y][player.x + 1] == BOMB)
                {
                    stage[player.y][player.x + 1] = 0;
                    player.health -= player.max_health / 4;
                    setPos(20, 0);
                    printf("                                    ");
                    setPos(20, 0);
                    printf("Health: %.1f / %.1f", player.health, player.max_health);
                }
                else if (stage[player.y][player.x + 1] == GOLL)
                {
                    next = 1;
                    break;
                }
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                printf("  ");
                player.x += 1;
                setPos((player.x * 2) + C_BASIS, player.y + R_BASIS);
                SetTextCol(BLUE);
                printf("○ ");
                SetTextCol(WHITE);
                MobMove();
            }
            else if (stage[player.y][player.x + 1] == MOB)
            {
                MobEncounter(0);
                MobMove();
                break;
            }
            break;
        }
    }
}

void MobMove()
{
    for (int i = 0; i < mob_count; i++)
    {
        int direction = rand() % 4;
        switch (direction)
        {
        case NORTH:
            if (stage[mob[i].y - 1][mob[i].x] == 0)
            {
                if ((player.x == mob[i].x) && (player.y == mob[i].y - 1))
                {
                    MobEncounter(1);
                    break;
                }
                else
                {
                    stage[mob[i].y][mob[i].x] = 0;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    printf("  ");
                    mob[i].y -= 1;
                    stage[mob[i].y][mob[i].x] = MOB;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    SetTextCol(RED);
                    printf("○ ");
                    SetTextCol(WHITE);
                }
            }
            break;
        case SOUTH:
            if (stage[mob[i].y + 1][mob[i].x] == 0)
            {
                if (player.x == mob[i].x && player.y == mob[i].y + 1)
                {
                    MobEncounter(1);
                    break;
                }
                else
                {
                    stage[mob[i].y][mob[i].x] = 0;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    printf("  ");
                    mob[i].y += 1;
                    stage[mob[i].y][mob[i].x] = MOB;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    SetTextCol(RED);
                    printf("○ ");
                    SetTextCol(WHITE);
                }
            }
            break;
        case WEST:
            if (stage[mob[i].y][mob[i].x - 1] == 0)
            {
                if (player.x == mob[i].x - 1 && player.y == mob[i].y)
                {
                    MobEncounter(1);
                    break;
                }
                else
                {
                    stage[mob[i].y][mob[i].x] = 0;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    printf("  ");
                    mob[i].x -= 1;
                    stage[mob[i].y][mob[i].x] = MOB;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    SetTextCol(RED);
                    printf("○ ");
                    SetTextCol(WHITE);
                }
            }
            break;
        case EAST:
            if (stage[mob[i].y][mob[i].x + 1] == 0)
            {
                if (player.x == mob[i].x + 1 && player.y == mob[i].y)
                {
                    MobEncounter(1);
                    break;
                }
                else
                {
                    stage[mob[i].y][mob[i].x] = 0;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    printf("  ");
                    mob[i].x += 1;
                    stage[mob[i].y][mob[i].x] = MOB;
                    setPos((mob[i].x * 2) + C_BASIS, mob[i].y + R_BASIS);
                    SetTextCol(RED);
                    printf("○ ");
                    SetTextCol(WHITE);
                }
            }
            break;
        }
    }
}

void MobEncounter(int num)
{
    player.health -= mob[0].dmg;
    setPos(20, 0);
    printf("                                    ");
    setPos(20, 0);
    printf("Health: %.1f / %.1f", player.health, player.max_health);
    if (num == 0)
    {
        setPos(10, size[ROW] + R_BASIS + 2);
        printf("You are Damaged!! %.1f dmg", mob[0].dmg);
        hit_count++;
    }
    else
    {
        setPos(10, size[ROW] + R_BASIS + 3);
        printf("Mob Damaged you!! %.1f dmg", mob[0].dmg);
        hit_count++;
    }
    setPos(0, size[ROW] + R_BASIS + 2);
    printf("%d hits", hit_count);
}

void Rank()
{
    // 랭킹 파일 입출력 추후 추가
}