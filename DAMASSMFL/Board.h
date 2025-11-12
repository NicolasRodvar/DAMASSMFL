#pragma once
#include <cmath>

enum class PieceColor { None, White, Black };

struct Piece {
    PieceColor color;
    bool isKing;

    Piece(PieceColor c = PieceColor::None, bool k = false)
        : color(c), isKing(k) {
    }
};

class Board {
public:
    static const int SIZE = 10;
    Piece cells[SIZE][SIZE];

    Board() {
        init();
    }

    // Reinicia el tablero con 20 fichas por lado
    void init() {
        // Vaciar tablero
        for (int y = 0; y < SIZE; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                cells[x][y] = Piece();
            }
        }

        // Negras arriba (filas 0-3) en casillas oscuras
        for (int y = 0; y < 4; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                if ((x + y) % 2 == 1) {
                    cells[x][y] = Piece(PieceColor::Black, false);
                }
            }
        }

        // Blancas abajo (filas 6-9) en casillas oscuras
        for (int y = SIZE - 4; y < SIZE; ++y) {
            for (int x = 0; x < SIZE; ++x) {
                if ((x + y) % 2 == 1) {
                    cells[x][y] = Piece(PieceColor::White, false);
                }
            }
        }
    }

    bool isInside(int x, int y) const {
        return x >= 0 && x < SIZE && y >= 0 && y < SIZE;
    }

    Piece get(int x, int y) const {
        if (!isInside(x, y)) return Piece();
        return cells[x][y];
    }

    void set(int x, int y, const Piece& p) {
        if (isInside(x, y)) {
            cells[x][y] = p;
        }
    }

    PieceColor opponentOf(PieceColor c) const {
        if (c == PieceColor::White) return PieceColor::Black;
        if (c == PieceColor::Black) return PieceColor::White;
        return PieceColor::None;
    }

    // Movimiento simple (sin captura)
    bool canSimpleMove(int fromX, int fromY, int toX, int toY, PieceColor player) const {
        if (!isInside(fromX, fromY) || !isInside(toX, toY)) return false;

        Piece p = get(fromX, fromY);
        if (p.color != player) return false;
        if (get(toX, toY).color != PieceColor::None) return false;

        int dx = toX - fromX;
        int dy = toY - fromY;

        // Solo diagonal 1 casilla
        if (std::abs(dx) != 1 || std::abs(dy) != 1) return false;

        if (!p.isKing) {
            int dir = (p.color == PieceColor::White) ? -1 : 1; // blancas suben, negras bajan
            if (dy != dir) return false;
        }
        // Si es rey, puede mover 1 casilla en cualquier diagonal

        return true;
    }

    // Movimiento con captura (salto de 2 casillas)
    bool canCaptureMove(int fromX, int fromY, int toX, int toY,
        PieceColor player, int& capX, int& capY) const {
        if (!isInside(fromX, fromY) || !isInside(toX, toY)) return false;

        Piece p = get(fromX, fromY);
        if (p.color != player) return false;
        if (get(toX, toY).color != PieceColor::None) return false;

        int dx = toX - fromX;
        int dy = toY - fromY;

        if (std::abs(dx) != 2 || std::abs(dy) != 2) return false;

        capX = (fromX + toX) / 2;
        capY = (fromY + toY) / 2;

        Piece middle = get(capX, capY);
        if (middle.color != opponentOf(player)) return false;

        if (!p.isKing) {
            int dir = (p.color == PieceColor::White) ? -1 : 1;
            if (dy != 2 * dir) return false;
        }
        // Rey puede capturar en cualquier diagonal de 2 casillas

        return true;
    }

    // Movimiento general (simple o captura).
    // didCapture: true si comió
    // becameKing: true si coronó
    bool move(int fromX, int fromY, int toX, int toY,
        PieceColor player, bool& didCapture, bool& becameKing) {
        didCapture = false;
        becameKing = false;

        if (!isInside(fromX, fromY) || !isInside(toX, toY)) return false;

        Piece p = get(fromX, fromY);
        if (p.color != player) return false;
        if (get(toX, toY).color != PieceColor::None) return false;

        int dx = toX - fromX;
        int dy = toY - fromY;

        // ¿Captura?
        if (std::abs(dx) == 2 && std::abs(dy) == 2) {
            int capX, capY;
            if (!canCaptureMove(fromX, fromY, toX, toY, player, capX, capY))
                return false;

            // Ejecutar captura
            set(capX, capY, Piece()); // quitar ficha comida
            set(toX, toY, p);
            set(fromX, fromY, Piece());
            didCapture = true;
        }
        // ¿Movimiento simple?
        else if (std::abs(dx) == 1 && std::abs(dy) == 1) {
            if (!canSimpleMove(fromX, fromY, toX, toY, player))
                return false;

            set(toX, toY, p);
            set(fromX, fromY, Piece());
        }
        else {
            return false;
        }

        // Coronación
        Piece moved = get(toX, toY);
        if (!moved.isKing) {
            if (moved.color == PieceColor::White && toY == 0) {
                moved.isKing = true;
                becameKing = true;
            }
            else if (moved.color == PieceColor::Black && toY == SIZE - 1) {
                moved.isKing = true;
                becameKing = true;
            }
            set(toX, toY, moved);
        }

        return true;
    }

    // ¿Quedan capturas posibles desde (x, y)?
    bool hasCaptureFrom(int x, int y) const {
        if (!isInside(x, y)) return false;

        Piece p = get(x, y);
        if (p.color == PieceColor::None) return false;

        PieceColor player = p.color;

        const int deltas[4][2] = { {2, 2}, {2, -2}, {-2, 2}, {-2, -2} };
        for (auto& d : deltas) {
            int toX = x + d[0];
            int toY = y + d[1];
            int capX, capY;
            if (canCaptureMove(x, y, toX, toY, player, capX, capY)) {
                return true;
            }
        }
        return false;
    }
};
