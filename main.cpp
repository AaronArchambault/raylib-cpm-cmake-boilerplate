#include <raylib.h>
#include <cstdint>
#include <iostream>
#include <sstream>
#include <algorithm>
//#include <assert.h>
#include <stack>
#include <string>
#include "deck.h"

using namespace std;

bool Player::canPlay(const Card& topCard) {
    for (const auto& card : hand) {
        if (card.matches(topCard)) {
            return true;
        }
    }
    return false;
}

int Player::chooseOptimalCard(const Card& topCard, int opponentHandSize) {
    if (!isAI) {
        return -1;
    }
    //find all the playable cards
    vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }
    if (playableIndices.empty()) {
        return -1;
    }

    //score each playable card using the linear programming
    int bestIndex = -1;
    double bestScore = -1.0;

    for (int idx : playableIndices) {
        CardScore score = LPOptimizer::calcCard(hand[idx], topCard,
                                                hand.size(), opponentHandSize);
        //apply the optimization
        if (score.strategicValue > bestScore) {
            bestScore = score.strategicValue;
            bestIndex = idx;
        }
    }
    return bestIndex;
}

int Player::chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze) {
    if (!isAI) {
        return -1;
    }
    vector<int> playableIndices;
    for (int i = 0; i < hand.size(); i++) {
        if (hand[i].matches(topCard)) {
            playableIndices.push_back(i);
        }
    }
    if (playableIndices.empty()) {
        return -1;
    }
    int bestIndex = -1;
    double bestScore = -1.0;
    for (int idx : playableIndices) {
        double multiTurnValues = 0.0;

        for (int turn = 0; turn < turnsToAnalyze; turn++) {
            CardScore score = LPOptimizer::calcCard(hand[idx], topCard,
                                                    hand.size() - turn,
                                                    opponentHandSize);

            double discountFactor = pow(0.9, turn);
            multiTurnValues += score.strategicValue * discountFactor;
        }
        if (multiTurnValues > bestScore) {
            bestScore = multiTurnValues;
            bestIndex = idx;
        }
    }
    return bestIndex;
}


/*
struct CardScore;
class LPOptimizer;

struct Card {
    cardColor color;
    cardValue type;
    bool matches(const Card& other) const {
        return color == other.color || type == other.type ||
               color == WILDS || other.color == WILDS;
    }
};

class Deck {
    std::vector<Card> cards;
public:
    void shuffle();
    Card draw();
};
*/

    Card Player::playCard(int index) {
        Card played = hand[index];
        hand.erase(hand.begin() + index);
        return played;
    }

    void Player::addCard(Card card) {
        hand.push_back(card);
    }

    int Player::getHandSize() const {
        return hand.size();
    }

    const std::vector<Card>& Player::getHand() const {
        return hand;
    }
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
    //helps to get the value of each card
    CardScore LPOptimizer::calcCard(const Card& card, const Card& topCard,int handSize, int opponentHandSize) {
        CardScore score;

        //gets the attacking value/getting rid of cards
        score.attackingValue = calcAttackingValue(card, handSize);

        //gets the defensive value of the cards that are good to keep
        score.defendingValue = calcDefendingValue(card, opponentHandSize);

        //it gets the combined strategic value of the card
        score.strategicValue = 0.6 * score.attackingValue +
                              0.4 * score.defendingValue;

        return score;
    }

//private:
    double LPOptimizer::calcAttackingValue(const Card& card, int handSize) {
        double value = 0.0;

        //the base values for different card types
        switch (card.type) {
            case WILD:
                value = 0.5;
                break;
            case WILD_DRAW_FOUR:
                value = 0.7;
                break;
            case DRAW_TWO:
                value = 0.4;
                break;
            case REVERSE:
                value = 0.35;
                break;
            case SKIP:
                value = 0.35;
                break;
            default:
                value = 0.2 + (card.type * 0.01);
                break;
        }
        //adjust based on hand size
        if (handSize <= 3) {
            value *= 1.5;
        }
        else if (handSize > 7) {
            if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
                value *= 1.3;
            }
        }
        return value;
    }

     double LPOptimizer::calcDefendingValue(const Card& card, int opponentHandSize) {
        double value = 0.0;

        //gets/decides if it should save the card
        if (card.type == WILD || card.type == WILD_DRAW_FOUR) {
            value = 0.8;
        }
        else if (card.type == DRAW_TWO) {
            value = 0.6;
        }
        else {
            value = 0.2;
        }

        //test if the enemy is close to winning
        if (opponentHandSize <= 2) {
            if (card.type == DRAW_TWO || card.type == WILD_DRAW_FOUR) {
                value *= 1.5;
            }
        }
        return value;
    }
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


