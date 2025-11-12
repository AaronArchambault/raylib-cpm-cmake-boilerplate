#ifndef DECK_H
#define DECK_H

#include <vector>
#include <random>
#include <cmath>
#include <algorithm>

enum cardColor { REDS, BLUES, GREENS, YELLOWS, WILDS };
enum cardValue {
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
    REVERSE, SKIP, DRAW_TWO, WILD, WILD_DRAW_FOUR
};

struct Card {
    cardColor color;
    cardValue type;

    bool matches(const Card& other) const {
        return color == other.color || type == other.type ||
               color == WILDS || other.color == WILDS;
    }

    bool operator==(const Card& other) const {
        return color == other.color && type == other.type;
    }

    void colorChange(cardColor target) {
        color = target;
    }

    bool isWild() const {
        return type == WILD || type == WILD_DRAW_FOUR;
    }

    bool isActionCard() const {
        return type == SKIP || type == REVERSE || type == DRAW_TWO || type == WILD || type == WILD_DRAW_FOUR;
    }
};

class Deck {
private:
    std::vector<Card> cards;
    std::mt19937 rng;
public:
    void shuffle();
    Card draw();
    bool isEmpty() const;
    int size() const;
    void addCard(const Card& card);
    void initinialize();
};

class Player {
private:
    std::vector<Card> hand;
    bool isAI;
    std::string name;

public:
    Player(bool ai = false, const std::string& playerName = "Player") //: isAI(ai) {}

    bool canPlay(const Card& topCard) const;
    int chooseOptimalCard(const Card& topCard, int opponentHandSize);
    int chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze = 3);
    Card playCard(int index);
    void addCard(const Card& card);
    int getHandSize() const;
    const std::vector<Card>& getHand() const;
    bool getISAI() const {return isAI;}
    std::string getName() const {return name;}
    cardColor chooseBestColor(const Card& topCard) const;
};

struct CardScore {
    double attackingValue;
    double defendingValue;
    double strategicValue;
};

class LPOptimizer {
public:
    static CardScore calcCard(const Card& card, const Card& topCard,
                              int handSize, int opponentHandSize);
private:
    static double calcAttackingValue(const Card& card, int handSize);
    static double calcDefendingValue(const Card& card, int opponentHandSize);
};

enum GameState {
    GAME_MENU,
    GAME_PLAYING,
    GAME_OVER,
    WAITING_FOR_COLOR_CHOICE
};

class Game {
    Deck deck;
    std::vector<Player> players;
    Card discardPile;
    Card topCard;
    int currentPlayer;
    bool clockwise;
    int drawStack;
    GameState state;
    int winner;

public:
    Game();
    void initialize(int numPlayers, int numAI);
    void playTurn(int cardIndex = -1);
    bool checkWinner();
    void skipPlayer();
    void reverseDirection();
    void drawCards(int playerIndex, int count);

    const Card& getTopCard() const { return topCard; }
    const Player& getCurrentPlayer() const { return players[currentPlayer]; }
    Player& getCurrentPlayer() { return players[currentPlayer]; }
    const std::vector<Player>& getPlayers() const { return players; }
    int getCurrentPlayerIndex() const { return currentPlayer; }
    GameState getState() const { return state; }
    void setState(GameState newState) { state = newState; }
    int getWinner() const { return winner; }
    bool isClockwise() const { return clockwise; }
    int getDrawStack() const { return drawStack; }

    void chooseColorForWild(cardColor color);
    void nextPlayer();
};

#endif