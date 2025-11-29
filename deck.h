#ifndef DECK_H
#define DECK_H

#include <vector>
#include <string>
#include <random>
#include <map>

//it is the card colors in UNO
enum cardColor {
    REDS, BLUES, GREENS, YELLOWS, WILDS
};

//it is the card values/types in UNO
enum cardValue {
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE, SKIP, REVERSE, DRAW_TWO, WILD, WILD_DRAW_FOUR
};

//it is the game states
enum GameState {
    GAME_MENU,
    GAME_PLAYING,
    WAITING_FOR_COLOR_CHOICE,
    GAME_OVER
};

//it is the card structure
struct Card {
    cardColor color;
    cardValue type;

    //it checks if this card matches the top card
    bool matches(const Card& other) const {
        return color == other.color || type == other.type || color == WILDS;
    }

    //it checks if this card is a wild card
    bool isWild() const {
        return type == WILD || type == WILD_DRAW_FOUR;
    }

    //it checks if this card is an action card
    bool isActionCard() const {
        return type == SKIP || type == REVERSE || type == DRAW_TWO || isWild();
    }

    //it changes the color of a wild card
    void colorChange(cardColor newColor) {
        color = newColor;
    }

    //it gets the point value of the card for UNO scoring
    int getPointValue() const {
        if (type >= ZERO && type <= NINE) return type;
        if (type == SKIP || type == REVERSE || type == DRAW_TWO) return 20;
        if (type == WILD || type == WILD_DRAW_FOUR) return 50;
        return 0;
    }
};

//it is the card score structure for evaluating cards
struct CardScore {
    double attackingValue;
    double defendingValue;
    double strategicValue;
    double lpOptimalValue;
};

//it tracks opponent behavior for modeling
struct OpponentModel {
    std::map<cardColor, int> colorsPlayed;      //it counts how many times each color was played
    std::map<cardColor, int> colorsAvoided;     //it counts how many times each color was avoided
    int totalTurnsObserved;                      //it counts total turns to calculate probabilities
    int turnsWithoutPlaying;                     //it counts consecutive turns where opponent drew

    //it initializes the opponent model
    OpponentModel() : totalTurnsObserved(0), turnsWithoutPlaying(0) {
        for (int c = REDS; c <= YELLOWS; c++) {
            colorsPlayed[static_cast<cardColor>(c)] = 0;
            colorsAvoided[static_cast<cardColor>(c)] = 0;
        }
    }

    //it estimates probability opponent has a specific color
    double getProbabilityHasColor(cardColor color) const {
        if (totalTurnsObserved == 0) return 0.25; //it defaults to even distribution

        int played = colorsPlayed.at(color);
        int avoided = colorsAvoided.at(color);

        if (avoided > 3) return 0.1; //it assumes they probably dont have this color
        if (played > 3) return 0.8;  //it assumes they probably have this color

        return 0.25 + (played - avoided) * 0.1; //it adjusts based on behavior
    }
};

//it is the multi-turn plan structure
struct TurnPlan {
    std::vector<int> cardSequence;  //it stores indices of cards to play in order
    double expectedUtility;          //it stores the expected value of this plan
    int expectedHandSize;            //it stores expected hand size after plan
};

//it is the linear programming optimizer
class LPOptimizer {
public:
    //it calculates the utility of a card
    static double getCardUtility(const Card& card, int handSize, int opponentHandSize);

    //it solves LP to find the best card for a single turn
    static int solveLPForBestCard(const std::vector<Card>& hand, const Card& topCard,
                                   int handSize, int opponentHandSize);

    //it solves LP with multi-turn planning
    static int solveLPMultiTurn(const std::vector<Card>& hand, const Card& topCard,
                                int handSize, int opponentHandSize,
                                const OpponentModel& opponentModel, int turnsAhead);

    //it creates a multi-turn plan for the next N turns
    static TurnPlan planNextTurns(const std::vector<Card>& hand, const Card& topCard,
                                   int opponentHandSize, const OpponentModel& opponentModel,
                                   int numTurns);

    //it calculates expected utility of a card sequence
    static double evaluateSequence(const std::vector<Card>& hand,
                                    const std::vector<int>& sequence,
                                    const Card& topCard, int opponentHandSize,
                                    const OpponentModel& opponentModel);

    //it gets the versatility score of a card (how many situations it can be played in)
    static double getCardVersatility(const Card& card, const std::vector<Card>& hand);

    //it calculates opponent blocking probability
    static double getBlockingProbability(const Card& cardToPlay, const OpponentModel& model);

    //legacy functions for compatibility
    static CardScore calcCard(const Card& card, const Card& topCard, int handSize, int opponentHandSize);
    static double calcAttackingValue(const Card& card, int handSize);
    static double calcDefendingValue(const Card& card, int opponentHandSize);
};

//it is the player class
class Player {
private:
    std::vector<Card> hand;
    bool isAI;
    std::string name;
    OpponentModel opponentModel; //it tracks opponent behavior

public:
    Player(bool ai = false, const std::string& playerName = "Player");

    //it checks if player can play any card
    bool canPlay(const Card& topCard) const;

    //it chooses the optimal card using simple evaluation
    int chooseOptimalCard(const Card& topCard, int opponentHandSize);

    //it chooses the optimal card with multi-turn planning
    int chooseOptimalCardMultiTurn(const Card& topCard, int opponentHandSize, int turnsToAnalyze) const;

    //it chooses the optimal card with advanced LP and opponent modeling
    int chooseOptimalCardAdvanced(const Card& topCard, int opponentHandSize, int turnsAhead) const;

    //it plays a card and returns it
    Card playCard(int index);

    //it adds a card to the hand
    void addCard(const Card& card);

    //it returns the hand size
    int getHandSize() const;

    //it returns the hand
    const std::vector<Card>& getHand() const;

    //it chooses the best color for a wild card
    cardColor chooseBestColor(const Card& topCard) const;

    //it sorts the hand using radix sort
    void sortHand();

    //it checks if this is an AI player
    bool getISAI() const { return isAI; }

    //it gets the player name
    std::string getName() const { return name; }

    //it updates the opponent model based on observed play
    void updateOpponentModel(const Card& playedCard, bool opponentDrew);

    //it gets the opponent model for analysis
    const OpponentModel& getOpponentModel() const { return opponentModel; }
};

//it is the deck class
class Deck {
private:
    std::vector<Card> cards;
    std::mt19937 rng;

public:
    Deck();

    //it initializes a full UNO deck
    void initinialize();

    //it shuffles the deck
    void shuffle();

    //it draws a card from the deck
    Card draw();

    //it checks if the deck is empty
    bool isEmpty() const;

    //it returns the size of the deck
    int size() const;

    //it adds a card to the deck
    void addCard(const Card& card);
};

//it is the game class
class Game {
private:
    std::vector<Player> players;
    Deck deck;
    Deck discardPile;
    Card topCard;
    int currentPlayer;
    bool clockwise;
    int drawStack;
    GameState state;
    int winner;
    Card lastPlayedCard; //it tracks the last played card for opponent modeling

public:
    Game();

    //it initializes a new game
    void initialize(int numPlayers, int numAI);

    //it plays a turn
    void playTurn(int cardIndex);

    //it moves to the next player
    void nextPlayer();

    //it reverses the direction
    void reverseDirection();

    //it skips a player
    void skipPlayer();

    //it forces a player to draw cards
    void drawCards(int playerIndex, int count);

    //it checks if there is a winner
    bool checkWinner();

    //it handles wild card color selection
    void chooseColorForWild(cardColor color);

    //getters
    const std::vector<Player>& getPlayers() const { return players; }
    const Player& getCurrentPlayer() const { return players[currentPlayer]; }
    Player& getCurrentPlayer() { return players[currentPlayer]; }
    int getCurrentPlayerIndex() const { return currentPlayer; }
    const Card& getTopCard() const { return topCard; }
    GameState getState() const { return state; }
    int getWinner() const { return winner; }
    int getDrawStack() const { return drawStack; }
    bool isClockwise() const { return clockwise; }
};

#endif