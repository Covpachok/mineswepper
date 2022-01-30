#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

enum CellInfo {
    CELL_EMPTY,
    CELL_NEAR_ONE,
    CELL_NEAR_TWO,
    CELL_NEAR_THREE,
    CELL_NEAR_FOUR,
    CELL_NEAR_FIVE,
    CELL_NEAR_SIX,
    CELL_NEAR_SEVEN,
    CELL_NEAR_EIGHT,
    CELL_BOMB,
};

enum { LOSE, WIN };
enum CellState { CELL_CLOSED, CELL_FLAGGED, CELL_OPENED };
enum OpeningResult { O_UNSUCCESS, O_SUCCESS, O_EXPLOSION };

const int kKeyEscape = 'q';
const int kKeyRestart = 'r';
const int kCellColorPairID[10] = {12, 1, 2, 3, 4, 5, 6, 7, 8, 9};
const char kCellChar[10] = {' ', '1', '2', '3', '4', '5', '6', '7', '8', '*'};
const int kCellStateColorPairID[3] = {10, 11, 12};
const int kCellStateChar[3] = {'#', '?', ' '};
const char kGameResultMsg[2][9] = {"GameOver", "You Win!"};

class SapperField {
    CellInfo **field_info;
    CellState **field_state;
    int field_width, field_height;
    int flags_amount, bflags_amount, bombs_amount;

public:
    SapperField(int width, int height, int bombs_number);
    ~SapperField();

    OpeningResult OpenCell(int x, int y);
    void FlagCell(int x, int y);

    void DrawCell(int x, int y) const;
    void DrawCellState(int x, int y) const;
    void DrawAllBombs() const;
    bool CheckWinCondition() const { return bflags_amount == bombs_amount; };

    void FieldToFile();

private:
    OpeningResult SubOpenCell(int x, int y);

    int CountNearbyFlags(int x, int y) const;
};

SapperField::SapperField(int width, int height, int bombs_number)
    : field_width(width), field_height(height), flags_amount(0),
      bflags_amount(0), bombs_amount(bombs_number)
{
    int i, j;

    field_info = new CellInfo *[height];
    field_state = new CellState *[height];
    for (i = 0; i < height; i++) {
        field_info[i] = new CellInfo[width];
        field_state[i] = new CellState[width];
        for (j = 0; j < width; j++) {
            field_info[i][j] = CELL_EMPTY;
            field_state[i][j] = CELL_CLOSED;
            DrawCellState(j, i);
        }
    }

    int x, y;
    for (i = 0; i < bombs_number; i++) {
        do {
            x = rand() % width;
            y = rand() % height;
        } while (field_info[y][x] == CELL_BOMB);
        field_info[y][x] = CELL_BOMB;
    }

    int bombs_count = 0;
    for (i = 0; i < height; i++) {
        for (j = 0; j < width; j++) {
            if (field_info[i][j] == CELL_EMPTY) {
                bombs_count = 0;

                if (i > 0) {
                    bombs_count += (field_info[i - 1][j] == CELL_BOMB);
                    if (j > 0)
                        bombs_count += (field_info[i - 1][j - 1] == CELL_BOMB);
                    if (j < width - 1)
                        bombs_count += (field_info[i - 1][j + 1] == CELL_BOMB);
                }

                if (i < height - 1) {
                    bombs_count += (field_info[i + 1][j] == CELL_BOMB);
                    if (j > 0)
                        bombs_count += (field_info[i + 1][j - 1] == CELL_BOMB);
                    if (j < width - 1)
                        bombs_count += (field_info[i + 1][j + 1] == CELL_BOMB);
                }

                if (j > 0)
                    bombs_count += (field_info[i][j - 1] == CELL_BOMB);
                if (j < width - 1)
                    bombs_count += (field_info[i][j + 1] == CELL_BOMB);

                field_info[i][j] = (CellInfo)bombs_count;
            }
        }
    }
}

SapperField::~SapperField()
{
    delete[] field_info;
    delete[] field_state;
}

void SapperField::DrawCell(int x, int y) const
{
    attrset(COLOR_PAIR(kCellColorPairID[field_info[y][x]]) | A_BOLD);
    mvaddch(y, x, kCellChar[field_info[y][x]]);
    attroff(COLOR_PAIR(kCellColorPairID[field_info[y][x]]) | A_BOLD);
}

void SapperField::DrawCellState(int x, int y) const
{
    attrset(COLOR_PAIR(kCellStateColorPairID[field_state[y][x]]) | A_BOLD);
    mvaddch(y, x, kCellStateChar[field_state[y][x]]);
    attroff(COLOR_PAIR(kCellStateColorPairID[field_state[y][x]]) | A_BOLD);
}

void SapperField::DrawAllBombs() const
{
    for (int y = 0; y < field_height; y++) {
        for (int x = 0; x < field_width; x++) {
            if (field_info[y][x] == CELL_BOMB)
                DrawCell(x, y);
        }
    }
}

int SapperField::CountNearbyFlags(int x, int y) const
{
    int n = 0;
    if (y > 0) {
        n += (field_state[y - 1][x] == CELL_FLAGGED);
        if (x > 0)
            n += (field_state[y - 1][x - 1] == CELL_FLAGGED);
        if (x < field_width - 1)
            n += (field_state[y - 1][x + 1] == CELL_FLAGGED);
    }

    if (y < field_height - 1) {
        n += (field_state[y + 1][x] == CELL_FLAGGED);
        if (x > 0)
            n += (field_state[y + 1][x - 1] == CELL_FLAGGED);
        if (x < field_width - 1)
            n += (field_state[y + 1][x + 1] == CELL_FLAGGED);
    }

    if (x > 0)
        n += (field_state[y][x - 1] == CELL_FLAGGED);
    if (x < field_width - 1)
        n += (field_state[y][x + 1] == CELL_FLAGGED);

    return n;
}

OpeningResult SapperField::SubOpenCell(int x, int y)
{
    if (x < 0 || x >= field_width || y < 0 || y >= field_height)
        return O_UNSUCCESS;

    if (field_state[y][x] == CELL_CLOSED) {
        field_state[y][x] = CELL_OPENED;
        DrawCell(x, y);
        if (field_info[y][x] == CELL_EMPTY) {
            SubOpenCell(x - 1, y);
            SubOpenCell(x - 1, y - 1);
            SubOpenCell(x - 1, y + 1);
            SubOpenCell(x + 1, y);
            SubOpenCell(x + 1, y - 1);
            SubOpenCell(x + 1, y + 1);
            SubOpenCell(x, y - 1);
            SubOpenCell(x, y + 1);
        }
        else if (field_info[y][x] == CELL_BOMB)
            return O_EXPLOSION;
        return O_SUCCESS;
    }

    return O_UNSUCCESS;
}

OpeningResult SapperField::OpenCell(int x, int y)
{
    int explosion_flag = false;
    if (x < 0 || x >= field_width || y < 0 || y >= field_height)
        return O_UNSUCCESS;

    if (field_state[y][x] == CELL_OPENED) {
        if (field_info[y][x] == CELL_EMPTY) {
            // DrawCell(x, y);
        }
        else {
            if (CountNearbyFlags(x, y) == field_info[y][x]) {
                explosion_flag += SubOpenCell(x - 1, y) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x - 1, y - 1) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x - 1, y + 1) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x + 1, y) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x + 1, y - 1) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x + 1, y + 1) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x, y - 1) == O_EXPLOSION;
                explosion_flag += SubOpenCell(x, y + 1) == O_EXPLOSION;
                if (explosion_flag)
                    return O_EXPLOSION;
            }
        }
    }
    else if (field_state[y][x] == CELL_CLOSED) {
        field_state[y][x] = CELL_OPENED;

        DrawCell(x, y);
        if (field_info[y][x] != CELL_BOMB) {
            if (field_info[y][x] == CELL_EMPTY) {
                OpenCell(x - 1, y);
                OpenCell(x - 1, y - 1);
                OpenCell(x - 1, y + 1);
                OpenCell(x + 1, y);
                OpenCell(x + 1, y - 1);
                OpenCell(x + 1, y + 1);
                OpenCell(x, y - 1);
                OpenCell(x, y + 1);
            }
        }
        else {
            return O_EXPLOSION;
        }
    }
    else {
        return O_UNSUCCESS;
    }

    return O_SUCCESS;
}

void SapperField::FlagCell(int x, int y)
{
    if (field_state[y][x] == CELL_CLOSED) {
        field_state[y][x] = CELL_FLAGGED;
        if (field_info[y][x] == CELL_BOMB)
            bflags_amount++;
        flags_amount++;
        DrawCellState(x, y);
    }
    else if (field_state[y][x] == CELL_FLAGGED) {
        field_state[y][x] = CELL_CLOSED;
        if (field_info[y][x] == CELL_BOMB)
            bflags_amount--;
        flags_amount--;
        DrawCellState(x, y);
    }
}

void SapperField::FieldToFile()
{
    FILE *f;
    f = fopen("field.txt", "w");
    if (!f) {
        perror("field.txt");
        return;
    }

    for (int i = 0; i < field_height; i++) {
        for (int j = 0; j < field_width; j++)
            fprintf(f, "%c ", kCellChar[field_info[i][j]]);
        fprintf(f, "\n");
    }

    fclose(f);
}

void GameOver(bool status)
{
    attrset(COLOR_PAIR(CELL_NEAR_FIVE));
    mvprintw(getmaxy(stdscr) / 2, (getmaxx(stdscr) - 8) / 2, "%s",
             kGameResultMsg[status]);
    attroff(COLOR_PAIR(CELL_NEAR_FIVE));
    refresh();
    napms(1000);
}

void colorinit()
{
    init_pair(CELL_NEAR_ONE, COLOR_BLUE, COLOR_BLACK);
    init_pair(CELL_NEAR_TWO, COLOR_GREEN, COLOR_BLACK);
    init_pair(CELL_NEAR_THREE, COLOR_RED, COLOR_BLACK);
    init_pair(CELL_NEAR_FOUR, COLOR_CYAN, COLOR_BLACK);
    init_pair(CELL_NEAR_FIVE, COLOR_YELLOW, COLOR_BLACK);
    init_pair(CELL_NEAR_SIX, COLOR_BLACK, COLOR_CYAN);
    init_pair(CELL_NEAR_SEVEN, COLOR_BLACK, COLOR_BLUE);
    init_pair(CELL_NEAR_EIGHT, COLOR_BLACK, COLOR_GREEN);
    init_pair(CELL_BOMB, COLOR_BLACK, COLOR_RED);
    init_pair(10 + CELL_CLOSED, COLOR_YELLOW, COLOR_WHITE);
    init_pair(10 + CELL_FLAGGED, COLOR_RED, COLOR_WHITE);
    init_pair(12 + CELL_EMPTY, COLOR_BLACK, COLOR_BLACK);
}

void ncinit()
{
    initscr();
    noecho();
    cbreak();
    curs_set(false);
    keypad(stdscr, true);
    mouseinterval(0);

    mousemask(ALL_MOUSE_EVENTS, nullptr);

    if (has_colors()) {
        start_color();
        colorinit();
    }
    refresh();
}

int main(int argc, char **argv)
{
    srand(time(nullptr));

    if (argc < 2) {
        fprintf(stderr, "Usage: ./game {number of bombs}\n");
        return 1;
    }

    int bombs_amount = atoi(argv[1]);

    if (bombs_amount == 0) {
        fprintf(
            stderr,
            "Error: Invalid argument, positive non-zero number expected.\n");
        return 1;
    }

    ncinit();

    if (bombs_amount >= getmaxx(stdscr) * getmaxy(stdscr)) {
        endwin();
        fprintf(stderr, "Error: Bombs everywhere O_O\n");
        return 1;
    }

    bool game_over = false;
    int key;
    OpeningResult op_res;
    MEVENT mouse_event;

    SapperField *game;
    game = new SapperField(getmaxx(stdscr), getmaxy(stdscr), bombs_amount);

    while ((key = getch()) != kKeyEscape) {
        switch (key) {

        case KEY_MOUSE:
            if (getmouse(&mouse_event) == OK) {
                if (mouse_event.bstate & BUTTON1_PRESSED) {
                    op_res = game->OpenCell(mouse_event.x, mouse_event.y);
                    if (op_res == O_EXPLOSION) {
                        game->DrawAllBombs();
                        game_over = true;
                        GameOver(LOSE);
                    }
                }
                else if (mouse_event.bstate & BUTTON3_PRESSED) {
                    game->FlagCell(mouse_event.x, mouse_event.y);
                    if (game->CheckWinCondition()) {
                        game_over = true;
                        GameOver(WIN);
                    }
                }
            }
            refresh();
            break;

        case kKeyRestart:
            game_over = true;
            break;

        case 'w':
            game->FieldToFile();
            break;
        }

        if (game_over) {
            game_over = false;
            delete game;
            game =
                new SapperField(getmaxx(stdscr), getmaxy(stdscr), bombs_amount);
        }
    }

    endwin();
    return 0;
}
