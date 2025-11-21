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
#include <vector>
#include "deck.h"
//https://www.raylib.com
//https://www.raylib.com/cheatsheet/cheatsheet.html
//https://www.raylib.com/examples.html
//https://www.youtube.com/watch?v=Vk96jvoS9so
using namespace std;

// Forward declarations of helper functions
Color getCardColor(cardColor color);
string getCardValueString(cardValue type);
void DrawCard(const Card& card, int x, int y, int width, int height, bool faceUp = true);
void DrawButton(const char* text, int x, int y, int width, int height, Color color);
bool IsButtonClicked(int x, int y, int width, int height);

//th screens dimensions
const int SCREEN_WIDTH = 1280;
const int SCREEN_HEIGHT = 720;

//th cards dimensions
const int CARD_WIDTH = 60;
const int CARD_HEIGHT = 100;
const int CARD_SPACING = 5;


float aiTurnDelay = 0.0f;
const float AI_TURN_WAIT = 0.5f;

// Game states for main menu
enum MenuState {
    MENU_MAIN,
    MENU_GAME
};

int main() {
    //th initializeing the window
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "THE UNO Game");
    SetTargetFPS(60);

    //the game's variables for the game paly to make it workws
    Game game;
    MenuState menuState = MENU_MAIN;
    int numPlayers = 1;
    int numAI = 1;
    int selectedCardIndex = -1;
    cardColor selectedColor = REDS;
    bool showColorPicker = false;

    //the main game loop
    while (!WindowShouldClose()) {
        Vector2 mousePos = GetMousePosition(); //gets the mouse postion for clicking the buttons and selescting the cards

        if (menuState == MENU_MAIN) {
            //the main menu logic
            if (IsButtonClicked(SCREEN_WIDTH/2 - 100, 250, 200, 50)) {
                numPlayers = 2;
                numAI = 1;
                game.initialize(numPlayers, numAI);
                menuState = MENU_GAME;
            }
        }
        else if (menuState == MENU_GAME) {
            //the games logic
            GameState state = game.getState();

            if (state == WAITING_FOR_COLOR_CHOICE) {
                //it shows color picker for pinkign thsd color of the cards with the wilds
                showColorPicker = true;

                //it checks for color selection
                int colorBoxSize = 80;
                int startX = SCREEN_WIDTH/2 - 180;
                int startY = SCREEN_HEIGHT/2 - 40;

                for (int i = 0; i < 4; i++) {
                    int x = startX + i * (colorBoxSize + 20);
                    if (IsButtonClicked(x, startY, colorBoxSize, colorBoxSize)) {
                        selectedColor = static_cast<cardColor>(i);
                        game.chooseColorForWild(selectedColor);
                        showColorPicker = false;
                    }
                }
            }
            else if (state == GAME_PLAYING) {
                showColorPicker = false;
                const Player& currentPlayer = game.getCurrentPlayer();

                if (currentPlayer.getISAI()) {
                    aiTurnDelay += GetFrameTime();

                    if (aiTurnDelay >= AI_TURN_WAIT) {
                        const vector<Player>& players = game.getPlayers();
                        int nextPlayerIndex = game.isClockwise() ?
                            (game.getCurrentPlayerIndex() + 1) % players.size() :
                            (game.getCurrentPlayerIndex() - 1 + players.size()) % players.size();
                        int opponentHandSize = players[nextPlayerIndex].getHandSize();

                        // Check if there's a draw stack that needs to be handled
                        if (game.getDrawStack() > 0) {
                            // Try to find a Draw 2 or Wild Draw 4 to stack
                            int cardToPlay = currentPlayer.chooseOptimalCardMultiTurn(game.getTopCard(), opponentHandSize, 3);

                            const Card& topCard = game.getTopCard();
                            bool canStack = false;

                            if (cardToPlay != -1) {
                                const Card& selectedCard = currentPlayer.getHand()[cardToPlay];
                                if (selectedCard.type == DRAW_TWO || selectedCard.type == WILD_DRAW_FOUR) {
                                    canStack = true;
                                }
                            }

                            if (canStack) {
                                game.playTurn(cardToPlay);//is stacks another draw card
                            }
                            else {
                                game.playTurn(-1);//it draws the cards and skip turn
                            }
                        }
                        else {
                            //it is the normal turn
                            int cardToPlay = currentPlayer.chooseOptimalCardMultiTurn(game.getTopCard(), opponentHandSize, 3);

                            if (cardToPlay == -1) {
                                game.playTurn(-1); //it draw a card
                            }
                            else {
                                game.playTurn(cardToPlay);//it is the play the chosen card
                            }
                        }

                        aiTurnDelay = 0.0f;
                    }
                }

                if (!currentPlayer.getISAI()) {
                    const vector<Card>& hand = currentPlayer.getHand();

                    //it calculates the hand position
                    int totalHandWidth = hand.size() * (CARD_WIDTH + CARD_SPACING);
                    int startX = (SCREEN_WIDTH - totalHandWidth) / 2;
                    int handY = SCREEN_HEIGHT - CARD_HEIGHT - 20;

                    //it checks for card clicks
                    selectedCardIndex = -1;
                    for (int i = 0; i < hand.size(); i++) {
                        int cardX = startX + i * (CARD_WIDTH + CARD_SPACING);
                        if (mousePos.x >= cardX && mousePos.x <= cardX + CARD_WIDTH &&
                            mousePos.y >= handY && mousePos.y <= handY + CARD_HEIGHT) {
                            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                                selectedCardIndex = i;
                                game.playTurn(i);
                            }
                            break;
                        }
                    }

                    //it is the draw card button
                    if (IsButtonClicked(SCREEN_WIDTH/2 + 150, SCREEN_HEIGHT/2 - 60, 120, 50)) {
                        game.playTurn(-1);  //it draws the cards
                    }
                }
            }
            else if (state == GAME_OVER) {
                //it checks for the restart button
                if (IsButtonClicked(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 100, 200, 50)) {
                    game.initialize(numPlayers, numAI);
                }
                //it checks for main menu button
                if (IsButtonClicked(SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 170, 200, 50)) {
                    menuState = MENU_MAIN;
                }
            }
        }

        //it is the draw call/drawing
        BeginDrawing();
        ClearBackground(DARKGREEN);

        if (menuState == MENU_MAIN) {
            //it draws the main menu
            DrawText("THE UNO GAME", SCREEN_WIDTH/2 - MeasureText("THE UNO GAME", 120)/2, 100, 75, YELLOW);
            //draws the 2 player button that the player can press
            DrawButton("2 Players (1 AI)", SCREEN_WIDTH/2 - 100, 250, 200, 50, GREEN);

            DrawText("Click to start a game!", SCREEN_WIDTH/2 - MeasureText("Click to start a game!", 20)/2, 500, 20, WHITE);
        }
        else if (menuState == MENU_GAME) {
            GameState state = game.getState();

            if (state == GAME_PLAYING || state == WAITING_FOR_COLOR_CHOICE) {
                //it draws the top card
                const Card& topCard = game.getTopCard();
                DrawCard(topCard, SCREEN_WIDTH/2 - CARD_WIDTH/2 - 70, SCREEN_HEIGHT/2 - CARD_HEIGHT/2,
                         CARD_WIDTH, CARD_HEIGHT, true);
                DrawText("Top Card", SCREEN_WIDTH/2 - 130, SCREEN_HEIGHT/2 - CARD_HEIGHT/2 - 30, 20, WHITE);

                //it draws deck placeholder
                DrawRectangle(SCREEN_WIDTH/2 + 70, SCREEN_HEIGHT/2 - CARD_HEIGHT/2,
                             CARD_WIDTH, CARD_HEIGHT, DARKBLUE);
                DrawRectangleLines(SCREEN_WIDTH/2 + 70, SCREEN_HEIGHT/2 - CARD_HEIGHT/2,
                                  CARD_WIDTH, CARD_HEIGHT, WHITE);
                DrawText("DECK", SCREEN_WIDTH/2 + 85, SCREEN_HEIGHT/2, 15, WHITE);

                //it draws the current player info
                const Player& currentPlayer = game.getCurrentPlayer();
                string playerInfo = currentPlayer.getName() + "'s Turn";
                DrawText(playerInfo.c_str(), 20, 20, 37, YELLOW);

                //it draws the draw stack info for the player
                if (game.getDrawStack() > 0) {
                    string stackInfo = "Draw Stack: +" + to_string(game.getDrawStack());
                    DrawText(stackInfo.c_str(), 20, 60, 37, RED);
                }

                //it draws all of the players' hands
                const vector<Player>& players = game.getPlayers();
                for (int p = 0; p < players.size(); p++) {
                    const Player& player = players[p];
                    int handSize = player.getHandSize();

                    if (!player.getISAI()) {
                        // Always draw human player's hand face-up at the bottom
                        const vector<Card>& hand = player.getHand();
                        int totalHandWidth = hand.size() * (CARD_WIDTH + CARD_SPACING);
                        int startX = (SCREEN_WIDTH - totalHandWidth) / 2;
                        int handY = SCREEN_HEIGHT - CARD_HEIGHT - 20;

                        for (int i = 0; i < hand.size(); i++) {
                            int cardX = startX + i * (CARD_WIDTH + CARD_SPACING);
                            DrawCard(hand[i], cardX, handY, CARD_WIDTH, CARD_HEIGHT, true);
                        }
                    }
                    else {
                        //it draws the AI players' hands so that the player can not see their cards
                        int posX, posY;
                        if (p == 1) {
                            posX = SCREEN_WIDTH - 150;
                            posY = SCREEN_HEIGHT/2 - 60;
                        }
                        else if (p == 2) {
                            posX = SCREEN_WIDTH/2 - 40;
                            posY = 30;
                        }
                        else {
                            posX = 50;
                            posY = SCREEN_HEIGHT/2 - 60;
                        }

                        DrawText(player.getName().c_str(), posX, posY - 25, 18, WHITE);
                        for (int i = 0; i < handSize; i++) {
                            Card dummyCard = {REDS, ZERO};
                            DrawCard(dummyCard, posX + i * 15, posY, 60, 90, false);
                        }
                        string cardCount = to_string(handSize) + " cards";
                        DrawText(cardCount.c_str(), posX, posY + 95, 15, WHITE);
                    }
                }

                //it draws the action buttons for human player to be able to draw the cards from the deck
                if (!currentPlayer.getISAI()) {
                    DrawButton("Draw Card", SCREEN_WIDTH/2 + 150, SCREEN_HEIGHT/2 - 60, 120, 50, DARKBLUE);
                }

                //it draws the color picker if it is needed with the wilds
                if (showColorPicker) {
                    //it draws the overlay
                    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));

                    DrawText("Choose a Color:", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 - 100, 30, WHITE);

                    int colorBoxSize = 70;
                    int startX = SCREEN_WIDTH/2 - 180;
                    int startY = SCREEN_HEIGHT/2 - 40;

                    Color colors[] = {RED, BLUE, GREEN, YELLOW};
                    const char* colorNames[] = {"RED", "BLUE", "GREEN", "YELLOW"};

                    for (int i = 0; i < 4; i++) {
                        int x = startX + i * (colorBoxSize + 20);
                        DrawRectangle(x, startY, colorBoxSize, colorBoxSize, colors[i]);
                        DrawRectangleLines(x, startY, colorBoxSize, colorBoxSize, WHITE);
                        DrawText(colorNames[i], x + 5, startY + colorBoxSize + 5, 15, WHITE);
                    }
                }
            }
            else if (state == GAME_OVER) {
                //it draws the game over screen to tell the player that the game is over
                DrawText("GAME OVER!", SCREEN_WIDTH/2 - MeasureText("GAME OVER!", 60)/2, 150, 60, YELLOW);

                string winnerText = game.getPlayers()[game.getWinner()].getName() + " Wins!";
                DrawText(winnerText.c_str(), SCREEN_WIDTH/2 - MeasureText(winnerText.c_str(), 40)/2, 250, 40, WHITE);

                DrawButton("Play Again", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 100, 200, 50, GREEN);
                DrawButton("Main Menu", SCREEN_WIDTH/2 - 100, SCREEN_HEIGHT/2 + 170, 200, 50, BLUE);
            }
        }

        EndDrawing();
    }

    CloseWindow();
    return 0;
}

void DrawCard(const Card& card, int x, int y, int width, int height, bool faceUp) {
    if (!faceUp) {
        //it draws the cards back
        DrawRectangle(x, y, width, height, DARKBLUE);
        DrawRectangleLines(x, y, width, height, WHITE);
        DrawText("UNO", x + width/2 - 20, y + height/2 - 10, 20, YELLOW);
    }
    else {
        //it draws the cards face
        Color cardColor = getCardColor(card.color);
        DrawRectangle(x, y, width, height, cardColor);
        DrawRectangleLines(x, y, width, height, BLACK);

        //it draws the cards value
        string valueStr = getCardValueString(card.type);
        int fontSize = (valueStr.length() > 4) ? 15 : 25;
        int textWidth = MeasureText(valueStr.c_str(), fontSize);
        DrawText(valueStr.c_str(), x + width/2 - textWidth/2, y + height/2 - fontSize/2, fontSize, WHITE);

        //it draw a smaller value in the corner make it look like uno
        DrawText(valueStr.c_str(), x + 5, y + 5, 12, WHITE);
    }
}

void DrawButton(const char* text, int x, int y, int width, int height, Color color) {
    Vector2 mousePos = GetMousePosition();
    bool isHovered = (mousePos.x >= x && mousePos.x <= x + width &&
                     mousePos.y >= y && mousePos.y <= y + height);

    Color buttonColor = isHovered ? Fade(color, 0.7f) : color;
    DrawRectangle(x, y, width, height, buttonColor);
    DrawRectangleLines(x, y, width, height, WHITE);

    int textWidth = MeasureText(text, 20);
    DrawText(text, x + width/2 - textWidth/2, y + height/2 - 10, 20, WHITE);
}

bool IsButtonClicked(int x, int y, int width, int height) {
    Vector2 mousePos = GetMousePosition();
    return (mousePos.x >= x && mousePos.x <= x + width &&
            mousePos.y >= y && mousePos.y <= y + height &&
            IsMouseButtonPressed(MOUSE_LEFT_BUTTON));
}

Color getCardColor(cardColor color) {
    switch (color) {
    case REDS: return RED;
    case BLUES: return BLUE;
    case GREENS: return GREEN;
    case YELLOWS: return YELLOW;
    case WILDS: return BLACK;
    default: return WHITE;
    }
}

string getCardValueString(cardValue type) {
    switch (type) {
    case ZERO: return "0";
    case ONE: return "1";
    case TWO: return "2";
    case THREE: return "3";
    case FOUR: return "4";
    case FIVE: return "5";
    case SIX: return "6";
    case SEVEN: return "7";
    case EIGHT: return "8";
    case NINE: return "9";
    case SKIP: return "SKIP";
    case DRAW_TWO: return "+2";
    case REVERSE: return "REV";
    case WILD_DRAW_FOUR: return "+4";
    case WILD: return "WILD";
    default: return "?";
    }
}