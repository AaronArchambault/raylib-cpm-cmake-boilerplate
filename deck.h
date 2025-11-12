#ifndef DECK_H
#define DECK_H

#include <vector>
#include <random>
#include <cmath>

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
};

class Deck {
    std::vector<Card> cards;
public:
    void shuffle();
    Card draw();
};

class Player {
private:
    std::vector<Card> hand;
    bool isAI;

public:
    Player(bool ai = false) : isAI(ai) {}

    bool canPlay(const Card& topCard);
    int chooseOptimalCard(const Card& topCard, int opponentHandSize);
    int chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze = 3);
    Card playCard(int index);
    void addCard(Card card);
    int getHandSize() const;
    const std::vector<Card>& getHand() const;
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

class Game {
    Deck deck;
    std::vector<Player> players;
    Card discardPile;
public:
    void playTurn();
    bool checkWinner();
};

#endif