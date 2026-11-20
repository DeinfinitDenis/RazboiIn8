#include <GLFW/glfw3.h>
#include <algorithm>
#include <cmath>
#include <vector>

using namespace std;

// Numerotăm pătratele de la a1; albul are semnul 1, iar negrul -1.
struct Move {
    int from;
    int to;
};

enum class Cell { EmptyBlack, EmptyWhite, WhitePiece, BlackPiece };

// Rândul 0 este la alb, iar rândul 7 este la negru.
struct Game {
    Cell board[8][8]{};
    int turn = 1;
    int ply = 0; // Numărul de mutări individuale făcute până acum.
};

Cell emptyCell(int x, int y) {
    return (x + y) % 2 == 0 ? Cell::EmptyBlack : Cell::EmptyWhite;
}

// Un pătrat liber nu aparține nimănui; pentru piese folosim aceleași semne ca la tură.
int owner(Cell cell) {
    if (cell == Cell::WhitePiece)
        return 1;
    if (cell == Cell::BlackPiece)
        return -1;
    return 0;
}

Game emptyGame() {
    Game game;
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            game.board[y][x] = emptyCell(x, y);
    return game;
}

Game newGame() {
    Game game = emptyGame();
    // Punem cele opt piese ale fiecărui jucător pe pătratele închise din primele două rânduri.
    for (int y = 0; y < 8; ++y)
        for (int x = 0; x < 8; ++x)
            if (game.board[y][x] == Cell::EmptyBlack && (y < 2 || y > 5))
                game.board[y][x] = y < 2 ? Cell::WhitePiece : Cell::BlackPiece;
    return game;
}

vector<int> destinations(const Game &game, int p) {
    vector<int> result;
    // Reținem fiecare poziție ca rând * 8 + coloană și încercăm cele patru diagonale.
    for (int dx : {-1, 1})
        for (int dy : {-1, 1}) {
            int x = p % 8 + dx;
            int y = p / 8 + dy;
            // Păstrăm doar pătratele aflate pe tablă și neocupate.
            if (x >= 0 && x < 8 && y >= 0 && y < 8 && owner(game.board[y][x]) == 0)
                result.push_back(y * 8 + x);
        }
    return result;
}

vector<Move> moves(const Game &game) {
    vector<Move> result;
    for (int p = 0; p < 64; ++p)
        if (owner(game.board[p / 8][p % 8]) == game.turn)
            for (int q : destinations(game, p))
                result.push_back({p, q});
    return result;
}

int pieceCount(const Game &game, int side) {
    int count = 0;
    for (const auto &row : game.board)
        for (Cell cell : row)
            if (owner(cell) == side)
                ++count;
    return count;
}

bool gameOver(const Game &game) {
    // Partida se oprește după 200 de mutări individuale, dacă o culoare dispare sau nu mai poate muta.
    return game.ply >= 200 || !pieceCount(game, 1) || !pieceCount(game, -1) || moves(game).empty();
}

Game applyMove(Game game, Move move) {
    // Modificăm o copie a tablei, astfel încât poziția primită să rămână neschimbată.
    game.board[move.to / 8][move.to % 8] = game.board[move.from / 8][move.from % 8];
    game.board[move.from / 8][move.from % 8] = emptyCell(move.from % 8, move.from / 8);
    ++game.ply;

    // După primele opt mutări individuale, căutăm piesele blocate ale ambelor culori.
    if (game.ply >= 8) {
        vector<int> captured;
        for (int p = 0; p < 64; ++p)
            if (owner(game.board[p / 8][p % 8]) && destinations(game, p).empty())
                captured.push_back(p);
        // Le scoatem abia după ce le-am găsit pe toate: eliminările sunt simultane.
        for (int p : captured)
            game.board[p / 8][p % 8] = emptyCell(p % 8, p / 8);
    }
    // Semnul turei trece de la alb la negru sau invers.
    game.turn = -game.turn;
    return game;
}

void color(float r, float g, float b) {
    glColor3f(r, g, b);
}

void rect(float x, float y, float w, float h) {
    glRectf(x, y, x + w, y + h);
}

void circle(float x, float y, float radius) {
    // Desenăm cercul din triunghiuri care pornesc din același centru.
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(x, y);
    for (int i = 0; i <= 64; ++i) {
        float angle = float(i) * 6.2831853f / 64;
        glVertex2f(x + radius * cos(angle), y + radius * sin(angle));
    }
    glEnd();
}

struct App {
    bool quit = false;
    Game game = newGame();
    int selected = -1; // -1 înseamnă că nu este selectată nicio piesă.
    float bx = 0;
    float by = 0;
    float cell = 75;
};

void handleKey(App &app, int key) {
    // Escape închide jocul, iar R începe o partidă nouă.
    if (key == GLFW_KEY_ESCAPE)
        app.quit = true;
    else if (key == GLFW_KEY_R) {
        app.game = newGame();
        app.selected = -1;
    }
}

void play(App &app, Move move) {
    app.game = applyMove(app.game, move);
    app.selected = -1;
}

void click(App &app, double x, double y) {
    // După încheierea partidei nu mai acceptăm mutări.
    if (gameOver(app.game))
        return;
    // Aflăm pe ce pătrat s-a dat click; în fereastră, rândul 7 este sus.
    int cx = int(floor((x - app.bx) / app.cell));
    int cy = int(floor((y - app.by) / app.cell));
    // Un click în afara tablei anulează selecția.
    if (cx < 0 || cx >= 8 || cy < 0 || cy >= 8) {
        app.selected = -1;
        return;
    }
    int p = (7 - cy) * 8 + cx;
    // Un click pe o piesă proprie o selectează; următorul poate alege destinația.
    if (owner(app.game.board[p / 8][p % 8]) == app.game.turn) {
        app.selected = p;
        return;
    }
    if (app.selected >= 0) {
        auto options = destinations(app.game, app.selected);
        if (find(options.begin(), options.end(), p) != options.end())
            play(app, {app.selected, p});
        else
            app.selected = -1;
    }
}

void draw(App &app, int w, int h) {
    // Păstrăm tabla pătrată și centrată când se schimbă dimensiunea ferestrei.
    app.cell = min(w, h) / 8.f;
    app.bx = (w - 8 * app.cell) / 2;
    app.by = (h - 8 * app.cell) / 2;
    auto options = app.selected < 0 ? vector<int>{} : destinations(app.game, app.selected);

    for (int row = 0; row < 8; ++row) {
        for (int column = 0; column < 8; ++column) {
            int p = row * 8 + column;
            float x = app.bx + column * app.cell;
            float y = app.by + (7 - row) * app.cell;
            Cell cell = app.game.board[row][column];

            // Piesele stau doar pe pătratele închise, așa că le desenăm același fundal.
            switch (cell) {
            case Cell::EmptyWhite:
                color(.67f, .73f, .70f);
                break;
            case Cell::EmptyBlack:
            case Cell::WhitePiece:
            case Cell::BlackPiece:
                color(.18f, .28f, .32f);
                break;
            }
            rect(x, y, app.cell, app.cell);

            if (p == app.selected) {
                color(.87f, .65f, .23f);
                rect(x + 2, y + 2, app.cell - 4, app.cell - 4);
            }
            if (find(options.begin(), options.end(), p) != options.end()) {
                // Arătăm unde poate muta piesa selectată.
                color(.30f, .85f, .65f);
                circle(x + app.cell / 2, y + app.cell / 2, app.cell * .12f);
            }

            switch (cell) {
            case Cell::WhitePiece:
            case Cell::BlackPiece: {
                if (cell == Cell::WhitePiece)
                    color(.94f, .90f, .76f);
                else
                    color(.12f, .16f, .21f);
                circle(x + app.cell / 2, y + app.cell / 2, app.cell * .34f);
                break;
            }
            case Cell::EmptyBlack:
            case Cell::EmptyWhite:
                break;
            }
        }
    }
}

int main() {
    // Pregătim fereastra și contextul OpenGL în care vom desena tabla.
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow *window = glfwCreateWindow(720, 720, "War in Eight", nullptr, nullptr);
    glfwSetWindowSizeLimits(window, 320, 320, GLFW_DONT_CARE, GLFW_DONT_CARE);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    App app;
    glfwSetWindowUserPointer(window, &app);
    // Legăm starea jocului de fereastră, ca să o putem folosi la click și la apăsarea tastelor.
    glfwSetMouseButtonCallback(window, [](GLFWwindow *win, int button, int action, int) {
        if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
            double x, y;
            glfwGetCursorPos(win, &x, &y);
            click(*static_cast<App *>(glfwGetWindowUserPointer(win)), x, y);
        }
    });
    glfwSetKeyCallback(window, [](GLFWwindow *win, int key, int, int action, int) {
        if (action != GLFW_PRESS)
            return;
        auto &app = *static_cast<App *>(glfwGetWindowUserPointer(win));
        handleKey(app, key);
    });
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        if (app.quit)
            break;
        int w, h, fw, fh;
        glfwGetWindowSize(window, &w, &h);
        glfwGetFramebufferSize(window, &fw, &fh);
        // Folosim dimensiunea ferestrei pentru coordonate și dimensiunea framebufferului pentru afișare.
        glViewport(0, 0, fw, fh);
        glClearColor(.055f, .09f, .12f, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        // Originea este în colțul din stânga sus, ca la coordonatele mouse-ului.
        glOrtho(0, w, h, 0, -1, 1);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        draw(app, w, h);
        glfwSwapBuffers(window);
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}
