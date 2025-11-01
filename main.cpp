/*#include <raylib.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <algorithm>
//#include <assert.h>
#include <stack>
#include <string>
#include <vector>
#include <random>
using namespace std;

class Card {
    enum Color {REDS, BLUES, GREENS, YELLOWS, WILDS };
    enum Type {ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGTHT, NINE, REVERSE, DRAW_TWO, WILD, WILD_DRAW_FOUR};
};

struct Cards {
    Color color;
    Type type;
    bool matches(const Card& other) const {
        return color == other.color || value == other.value|| color == WILDS || other.color == WILD;
    }
};

class Deck {
    std::vector<Card> cards;
    void shuffle();
    Card draw();
};

class Player {
    std::vector<Card> hand;
    bool canPlay(Card topCard);
    Card playCard(int index);
};

class Game {
    Deck deck;
    std::vector<Player> players;
    std::stack<Card> discardPile;
    void playTurn();
    bool checkWinner();
};




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
                pixels[row * imgWidth + col] = (Color) {
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
}*/


//AI code only use to reference the raylib

#include <raylib.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <stack>
#include <string>
#include <vector>
#include <random>
using namespace std;

// Enums for card properties
enum CardColor { COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW, COLOR_WILD };
enum CardType {
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
    SKIP, REVERSE, DRAW_TWO, WILD, WILD_DRAW_FOUR
};

// Card class representing a single UNO card
class Card {
public:
    CardColor color;
    CardType type;

    Card() : color(COLOR_RED), type(ZERO) {}
    Card(CardColor c, CardType t) : color(c), type(t) {}

    // Check if this card can be played on top of another card
    bool canPlayOn(const Card& other) const {
        // Wild cards can be played on anything
        if (color == COLOR_WILD) return true;
        if (other.color == COLOR_WILD) return true;

        // Can play if color matches OR type matches
        return (color == other.color) || (type == other.type);
    }

    // Get color as Raylib Color for rendering
    Color getRaylibColor() const {
        switch(color) {
            case COLOR_RED: return RED;
            case COLOR_BLUE: return BLUE;
            case COLOR_GREEN: return GREEN;
            case COLOR_YELLOW: return YELLOW;
            case COLOR_WILD: return BLACK;
            default: return WHITE;
        }
    }

    // Get card type as string
    string getTypeString() const {
        if (type >= ZERO && type <= NINE) {
            return to_string(type);
        }
        switch(type) {
            case SKIP: return "SKIP";
            case REVERSE: return "REV";
            case DRAW_TWO: return "+2";
            case WILD: return "WILD";
            case WILD_DRAW_FOUR: return "+4";
            default: return "?";
        }
    }

    // Get color as string
    string getColorString() const {
        switch(color) {
            case COLOR_RED: return "Red";
            case COLOR_BLUE: return "Blue";
            case COLOR_GREEN: return "Green";
            case COLOR_YELLOW: return "Yellow";
            case COLOR_WILD: return "Wild";
            default: return "Unknown";
        }
    }
};

// Deck class manages the draw pile
class Deck {
private:
    vector<Card> cards;

public:
    Deck() {
        initializeDeck();
    }

    void initializeDeck() {
        cards.clear();

        // Add numbered cards (0-9) for each color
        // 0 appears once, 1-9 appear twice
        CardColor colors[] = {COLOR_RED, COLOR_BLUE, COLOR_GREEN, COLOR_YELLOW};

        for (CardColor color : colors) {
            // Add one 0 card
            cards.push_back(Card(color, ZERO));

            // Add two of each 1-9
            for (int i = 1; i <= 9; i++) {
                cards.push_back(Card(color, static_cast<CardType>(i)));
                cards.push_back(Card(color, static_cast<CardType>(i)));
            }

            // Add two of each action card (Skip, Reverse, Draw Two)
            cards.push_back(Card(color, SKIP));
            cards.push_back(Card(color, SKIP));
            cards.push_back(Card(color, REVERSE));
            cards.push_back(Card(color, REVERSE));
            cards.push_back(Card(color, DRAW_TWO));
            cards.push_back(Card(color, DRAW_TWO));

        }

        // Add 4 Wild cards and 4 Wild Draw Four cards
        for (int i = 0; i < 4; i++) {
            cards.push_back(Card(COLOR_WILD, WILD));
            cards.push_back(Card(COLOR_WILD, WILD_DRAW_FOUR));
        }
    }

    void shuffle() {
        random_device rd;
        mt19937 g(rd());
        std::shuffle(cards.begin(), cards.end(), g);
    }

    Card draw() {
        if (cards.empty()) {
            cout << "Warning: Deck is empty!" << endl;
            return Card(COLOR_RED, ZERO);
        }
        Card card = cards.back();
        cards.pop_back();
        return card;
    }

    bool isEmpty() const {
        return cards.empty();
    }

    int size() const {
        return cards.size();
    }

    void addCard(Card card) {
        cards.push_back(card);
    }
};

// Player class
class Player {
public:
    vector<Card> hand;
    string name;
    bool isAI;

    Player(string n = "Player", bool ai = false) : name(n), isAI(ai) {}

    void addCard(Card card) {
        hand.push_back(card);
    }

    Card playCard(int index) {
        if (index < 0 || index >= hand.size()) {
            return Card(); // Return default card
        }
        Card card = hand[index];
        hand.erase(hand.begin() + index);
        return card;
    }

    bool hasWon() const {
        return hand.empty();
    }

    // Find playable cards
    vector<int> getPlayableCardIndices(const Card& topCard) const {
        vector<int> playable;
        for (int i = 0; i < hand.size(); i++) {
            if (hand[i].canPlayOn(topCard)) {
                playable.push_back(i);
            }
        }
        return playable;
    }

    // Simple AI logic
    int chooseCardToPlay(const Card& topCard) {
        vector<int> playable = getPlayableCardIndices(topCard);
        if (playable.empty()) return -1;

        // Simple strategy: play first playable card
        return playable[0];
    }
};

// Main Game class
class Game {
private:
    Deck deck;
    vector<Player> players;
    stack<Card> discardPile;
    int currentPlayerIndex;
    bool clockwise;
    int drawPending; // For stacking Draw Two/Draw Four
    CardColor wildChosenColor;

public:
    Game() : currentPlayerIndex(0), clockwise(true), drawPending(0), wildChosenColor(COLOR_RED) {
        // Initialize with 2 players (1 human, 1 AI)
        players.push_back(Player("You", false));
        players.push_back(Player("AI", true));
    }

    void startGame() {
        // Initialize and shuffle deck
        deck.initializeDeck();
        deck.shuffle();

        // Deal 7 cards to each player
        for (int i = 0; i < 7; i++) {
            for (auto& player : players) {
                player.addCard(deck.draw());
            }
        }

        // Place first card on discard pile (make sure it's not a wild)
        Card firstCard;
        do {
            firstCard = deck.draw();
        } while (firstCard.color == COLOR_WILD);

        discardPile.push(firstCard);
    }

    Card getTopCard() const {
        if (discardPile.empty()) {
            return Card(COLOR_RED, ZERO);
        }
        return discardPile.top();
    }

    Player& getCurrentPlayer() {
        return players[currentPlayerIndex];
    }

    void nextPlayer() {
        if (clockwise) {
            currentPlayerIndex = (currentPlayerIndex + 1) % players.size();
        } else {
            currentPlayerIndex = (currentPlayerIndex - 1 + players.size()) % players.size();
        }
    }

    bool playCard(int cardIndex) {
        Player& player = getCurrentPlayer();

        if (cardIndex < 0 || cardIndex >= player.hand.size()) {
            return false;
        }

        Card card = player.hand[cardIndex];
        Card topCard = getTopCard();

        // Check if card can be played
        if (!card.canPlayOn(topCard) && drawPending == 0) {
            return false;
        }

        // Handle special case: Draw Two/Draw Four stacking
        if (drawPending > 0) {
            if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
                // Allow stacking
            } else {
                return false; // Must draw or play another draw card
            }
        }

        // Play the card
        Card playedCard = player.playCard(cardIndex);
        discardPile.push(playedCard);

        // Handle special cards
        handleSpecialCard(playedCard);

        // Check for winner
        if (player.hasWon()) {
            cout << player.name << " wins!" << endl;
            return true;
        }

        // Move to next player
        nextPlayer();

        return true;
    }

    void handleSpecialCard(const Card& card) {
        switch(card.type) {
            case SKIP:
                nextPlayer(); // Skip next player
                break;

            case REVERSE:
                if (players.size() == 2) {
                    // In 2-player game, reverse acts like skip
                    nextPlayer();
                } else {
                    clockwise = !clockwise;
                }
                break;

            case DRAW_TWO:
                drawPending += 2;
                break;

            case WILD_DRAW_FOUR:
                drawPending += 4;
                // Need to choose color (will be handled in UI)
                break;

            case WILD:
                // Need to choose color (will be handled in UI)
                break;

            default:
                break;
        }
    }

    void drawCard() {
        Player& player = getCurrentPlayer();

        if (drawPending > 0) {
            // Draw penalty cards
            for (int i = 0; i < drawPending; i++) {
                if (!deck.isEmpty()) {
                    player.addCard(deck.draw());
                }
            }
            drawPending = 0;
            nextPlayer();
        } else {
            // Normal draw
            if (!deck.isEmpty()) {
                player.addCard(deck.draw());
            }
            // After drawing, player can try to play or pass
        }

        // Reshuffle discard pile into deck if needed
        if (deck.isEmpty() && discardPile.size() > 1) {
            reshuffleDiscardIntoDeck();
        }
    }

    void reshuffleDiscardIntoDeck() {
        Card topCard = discardPile.top();
        discardPile.pop();

        while (!discardPile.empty()) {
            deck.addCard(discardPile.top());
            discardPile.pop();
        }

        deck.shuffle();
        discardPile.push(topCard);
    }

    // Getters
    const vector<Player>& getPlayers() const { return players; }
    int getCurrentPlayerIndex() const { return currentPlayerIndex; }
    int getDeckSize() const { return deck.size(); }
    int getDrawPending() const { return drawPending; }
};

// Main function with Raylib
int main() {
    const int screenWidth = 1200;
    const int screenHeight = 800;

    InitWindow(screenWidth, screenHeight, "UNO Game");
    SetTargetFPS(60);

    // Initialize game
    Game game;
    game.startGame();

    // Card display parameters
    const int cardWidth = 80;
    const int cardHeight = 120;
    const int cardSpacing = 90;

    int selectedCard = -1;

    // Main game loop
    while (!WindowShouldClose()) {
        // Update logic
        Player& currentPlayer = game.getCurrentPlayer();

        // Handle mouse input for player (not AI)
        if (!currentPlayer.isAI) {
            Vector2 mousePos = GetMousePosition();

            // Check if clicking on player's cards
            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON)) {
                int startX = screenWidth / 2 - (currentPlayer.hand.size() * cardSpacing) / 2;
                int cardY = screenHeight - cardHeight - 50;

                for (int i = 0; i < currentPlayer.hand.size(); i++) {
                    int cardX = startX + i * cardSpacing;
                    Rectangle cardRect = {(float)cardX, (float)cardY, (float)cardWidth, (float)cardHeight};

                    if (CheckCollisionPointRec(mousePos, cardRect)) {
                        if (game.playCard(i)) {
                            selectedCard = -1;
                        }
                        break;
                    }
                }

                // Check if clicking draw pile
                Rectangle drawPileRect = {50, screenHeight/2 - cardHeight/2, (float)cardWidth, (float)cardHeight};
                if (CheckCollisionPointRec(mousePos, drawPileRect)) {
                    game.drawCard();
                }
            }
        } else {
            // AI turn
            static float aiTimer = 0;
            aiTimer += GetFrameTime();

            if (aiTimer > 1.0f) { // AI waits 1 second before playing
                aiTimer = 0;

                Card topCard = game.getTopCard();
                int cardToPlay = currentPlayer.chooseCardToPlay(topCard);

                if (cardToPlay >= 0) {
                    game.playCard(cardToPlay);
                } else {
                    game.drawCard();
                }
            }
        }

        // Drawing
        BeginDrawing();
        ClearBackground(DARKGREEN);

        // Draw title
        DrawText("UNO GAME", screenWidth/2 - 100, 20, 40, WHITE);

        // Draw current player indicator
        string turnText = game.getCurrentPlayer().name + "'s Turn";
        DrawText(turnText.c_str(), screenWidth/2 - 80, 70, 20, YELLOW);

        // Draw draw pile
        DrawRectangle(50, screenHeight/2 - cardHeight/2, cardWidth, cardHeight, DARKGRAY);
        DrawRectangleLines(50, screenHeight/2 - cardHeight/2, cardWidth, cardHeight, WHITE);
        DrawText("DRAW", 60, screenHeight/2 - 10, 20, WHITE);
        DrawText(TextFormat("%d", game.getDeckSize()), 70, screenHeight/2 + 20, 20, WHITE);

        // Draw discard pile (top card)
        Card topCard = game.getTopCard();
        int discardX = 200;
        int discardY = screenHeight/2 - cardHeight/2;
        DrawRectangle(discardX, discardY, cardWidth, cardHeight, topCard.getRaylibColor());
        DrawRectangleLines(discardX, discardY, cardWidth, cardHeight, BLACK);
        DrawText(topCard.getTypeString().c_str(), discardX + 20, discardY + 50, 20, WHITE);

        // Draw current player's hand (bottom of screen)
        const Player& player = game.getPlayers()[0];
        int startX = screenWidth / 2 - (player.hand.size() * cardSpacing) / 2;
        int playerCardY = screenHeight - cardHeight - 50;

        for (int i = 0; i < player.hand.size(); i++) {
            const Card& card = player.hand[i];
            int cardX = startX + i * cardSpacing;

            // Draw card
            DrawRectangle(cardX, playerCardY, cardWidth, cardHeight, card.getRaylibColor());
            DrawRectangleLines(cardX, playerCardY, cardWidth, cardHeight, BLACK);

            // Draw card text
            string cardText = card.getTypeString();
            DrawText(cardText.c_str(), cardX + 20, playerCardY + 50, 20, WHITE);

            // Highlight if playable
            if (card.canPlayOn(topCard)) {
                DrawRectangleLines(cardX - 2, playerCardY - 2, cardWidth + 4, cardHeight + 4, YELLOW);
            }
        }

        // Draw opponent's hand (top of screen) - back of cards
        const Player& opponent = game.getPlayers()[1];
        int opponentStartX = screenWidth / 2 - (opponent.hand.size() * cardSpacing) / 2;
        int opponentCardY = 120;

        DrawText(TextFormat("%s: %d cards", opponent.name.c_str(), (int)opponent.hand.size()),
                 opponentStartX, opponentCardY - 30, 20, WHITE);

        for (int i = 0; i < opponent.hand.size(); i++) {
            int cardX = opponentStartX + i * cardSpacing;
            DrawRectangle(cardX, opponentCardY, cardWidth, cardHeight, DARKBLUE);
            DrawRectangleLines(cardX, opponentCardY, cardWidth, cardHeight, WHITE);
            DrawText("UNO", cardX + 15, opponentCardY + 50, 20, WHITE);
        }

        // Draw instructions
        DrawText("Click cards to play | Click DRAW to draw", 10, screenHeight - 30, 20, LIGHTGRAY);

        EndDrawing();
    }

    CloseWindow();
    return 0;
}