#include <SFML/Graphics.hpp>
#include <optional>
#include <iostream>
#include "Board.h"

int main() {
    // --- CONFIGURACIÓN DEL TABLERO Y LA VENTANA ---
    const unsigned int windowSize = 720;           // ventana 720x720
    const int          boardSize = Board::SIZE;   // 10x10

    // Tamaño original de la textura del tablero (gif/png 1000x1000)
    const float        textureSize = 1000.f;
    const float        scale = windowSize / textureSize; // 0.72

    // Medidas REALES del área jugable dentro de la imagen (en la textura grande):
    //  - horizontal: de x=67 a x=851  → 785 px
    //  - vertical:   de y≈230 a y=851 → 622 px
    // Luego las escalamos a la ventana (720x720)
    const float        boardOffsetX = 67.f * scale;   // margen izq dentro de la ventana
    const float        boardOffsetY = 230.f * scale;   // margen sup dentro de la ventana
    const float        innerWidth = 785.f * scale;   // ancho área jugable
    const float        innerHeight = 622.f * scale;   // alto área jugable

    const float        cellWidth = innerWidth / boardSize;
    const float        cellHeight = innerHeight / boardSize;

    sf::RenderWindow window(
        sf::VideoMode({ windowSize, windowSize }),
        "Damas Internacionales",
        sf::Style::Titlebar | sf::Style::Close  // ventana fija (no se puede estirar)
    );

    // --- TEXTURAS ---
    sf::Texture boardTexture;
    if (!boardTexture.loadFromFile("assets/tablero_madera_base.png")) {
        std::cout << "No se pudo cargar assets/tablero_madera_base.png\n";
        return 1;
    }
    sf::Sprite boardSprite(boardTexture);
    boardSprite.setScale({ scale, scale });
    boardSprite.setPosition({ 0.f, 0.f });

    sf::Texture whiteTexture;
    if (!whiteTexture.loadFromFile("assets/ficha_blanca.png")) {
        std::cout << "No se pudo cargar assets/ficha_blanca.png\n";
        return 1;
    }

    sf::Texture blackTexture;
    if (!blackTexture.loadFromFile("assets/ficha_negra.png")) {
        std::cout << "No se pudo cargar assets/ficha_negra.png\n";
        return 1;
    }

    // Escala de las fichas (quepan en el ancho de la casilla)
    sf::Vector2u texSize = whiteTexture.getSize();
    float pieceScale = (cellWidth * 0.8f) / static_cast<float>(texSize.x);

    // --- LÓGICA DE JUEGO ---
    Board board;
    PieceColor currentPlayer = PieceColor::White;

    bool hasSelected = false;
    int selectedX = -1, selectedY = -1;

    while (window.isOpen()) {
        // --- EVENTOS (SFML 3) ---
        while (const std::optional<sf::Event> event = window.pollEvent()) {
            // Cerrar ventana
            if (event->is<sf::Event::Closed>()) {
                window.close();
            }

            // Click de mouse
            if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (mousePressed->button == sf::Mouse::Button::Left) {
                    int mouseX = mousePressed->position.x;
                    int mouseY = mousePressed->position.y;

                    // ¿Está el clic dentro del área jugable?
                    if (mouseX >= boardOffsetX &&
                        mouseX < boardOffsetX + innerWidth &&
                        mouseY >= boardOffsetY &&
                        mouseY < boardOffsetY + innerHeight) {

                        // Pasar de píxeles a casilla (0-9) usando ANCHO y ALTO por separado
                        int cellX = static_cast<int>((mouseX - boardOffsetX) / cellWidth);
                        int cellY = static_cast<int>((mouseY - boardOffsetY) / cellHeight);

                        if (cellX >= 0 && cellX < boardSize && cellY >= 0 && cellY < boardSize) {
                            // 1) Sin selección previa → seleccionar ficha propia
                            if (!hasSelected) {
                                Piece p = board.get(cellX, cellY);
                                if (p.color == currentPlayer) {
                                    hasSelected = true;
                                    selectedX = cellX;
                                    selectedY = cellY;
                                    std::cout << "Seleccion (" << selectedX << ", " << selectedY << ")\n";
                                }
                            }
                            // 2) Ya hay ficha seleccionada → intentar mover
                            else {
                                bool didCapture = false;
                                bool becameKing = false;

                                if (board.move(selectedX, selectedY, cellX, cellY,
                                    currentPlayer, didCapture, becameKing)) {

                                    std::cout << "Movida a (" << cellX << ", " << cellY << ")\n";

                                    // ¿Puede seguir capturando con la misma ficha?
                                    if (didCapture && board.hasCaptureFrom(cellX, cellY)) {
                                        hasSelected = true;
                                        selectedX = cellX;
                                        selectedY = cellY;
                                        std::cout << "Debes seguir capturando con la misma ficha.\n";
                                    }
                                    else {
                                        // Termina el turno
                                        hasSelected = false;
                                        selectedX = selectedY = -1;

                                        currentPlayer = (currentPlayer == PieceColor::White)
                                            ? PieceColor::Black
                                            : PieceColor::White;
                                    }
                                }
                                else {
                                    std::cout << "Movimiento invalido\n";
                                    hasSelected = false;
                                    selectedX = selectedY = -1;
                                }
                            }
                        }
                    }
                }
            }
        }

        // --- DIBUJO ---
        window.clear(sf::Color::Black);
        window.draw(boardSprite);

        // Dibujar fichas
        for (int y = 0; y < boardSize; ++y) {
            for (int x = 0; x < boardSize; ++x) {
                Piece p = board.get(x, y);
                if (p.color == PieceColor::None) continue;

                const sf::Texture& tex = (p.color == PieceColor::White) ? whiteTexture : blackTexture;
                sf::Sprite pieceSprite(tex);

                pieceSprite.setScale({ pieceScale, pieceScale });

                // Centro de la casilla usando cellWidth / cellHeight distintos
                float posX = boardOffsetX + x * cellWidth + cellWidth / 2.f;
                float posY = boardOffsetY + y * cellHeight + cellHeight / 2.f;

                sf::FloatRect bounds = pieceSprite.getLocalBounds();
                pieceSprite.setOrigin({ bounds.size.x / 2.f, bounds.size.y / 2.f });
                pieceSprite.setPosition({ posX, posY });

                if (hasSelected && x == selectedX && y == selectedY) {
                    pieceSprite.setColor(sf::Color(255, 255, 0)); // amarilla
                }

                window.draw(pieceSprite);

                // Corona para damas
                if (p.isKing) {
                    float r = std::min(cellWidth, cellHeight) * 0.25f;
                    sf::CircleShape crown(r);
                    crown.setOrigin({ r, r });
                    crown.setPosition({ posX, posY });
                    crown.setFillColor(sf::Color::Transparent);
                    crown.setOutlineThickness(3.f);
                    crown.setOutlineColor(sf::Color(255, 215, 0));
                    window.draw(crown);
                }
            }
        }

        window.display();
    }

    return 0;
}
