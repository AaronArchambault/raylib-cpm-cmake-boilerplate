#include <raylib.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <algorithm>
//#include <assert.h>
#include <stack>
#include <string>
#include <cmath>
#include <map>
#include "deck.h"

using namespace std;



//};
/*
//working on linear programming with card score system for the linear programming optimization
struct CardScore {
    double attackingValue; //a score for more offensive cards good for winning
    double defendingValue; //a score for more defensive cards good for protection
    double strategicValue; //a score for normal/overall useful cards
};

class LPOptimizer {
public:*/
    








/*};

class Game {
    Deck deck;
    std::vector<Player> players;
    Card discardPile;
public:
    void playTurn();
    bool checkWinner();
};*/




int main()
{
    const int screenWidth = 800;
    const int screenHeight = 450;
    const int imgWidth = 400;
    const int imgHeight = 400;

    InitWindow(screenWidth, screenHeight, "Pixel Manipulation");

    SetTargetFPS(60);

    Image img = GenImageColor(imgHeight, imgHeight, WHITE);
    ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto pixels = (Color*)img.data;
    Texture tex = LoadTextureFromImage(img);

    // Main game loop
    while (!WindowShouldClose())    // Detect window close button or ESC key
    {
        // your update logic goes here
        for(int row = 0; row < imgHeight; row++)
        {
            for(int col = 0; col < imgWidth; col++)
            {
                pixels[row * imgWidth + col] = Color {
                    (uint8_t)(GetRandomValue(0, 255)),
                    (uint8_t)(GetRandomValue(0, 255)),
                    (uint8_t)(GetRandomValue(0, 255)),
                    255
                };
            }
        }
        UpdateTexture(tex, pixels);

        // drawing logic goes here
        BeginDrawing();
        ClearBackground(BLACK);
        DrawTexture(tex, 200, 25, WHITE);
        EndDrawing();
    }
    UnloadTexture(tex);

    CloseWindow();
    return 0;
}


