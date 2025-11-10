#include <vector>
#include <random>
#include <cmath>

enum cardColor { REDS, BLUES, GREENS, YELLOWS, WILDS };
enum cardValue {
    ZERO, ONE, TWO, THREE, FOUR, FIVE, SIX, SEVEN, EIGHT, NINE,
    REVERSE, SKIP, DRAW_TWO, WILD, WILD_DRAW_FOUR
};



struct Card
{
    cardColor color;
    cardValue value;

    bool matches(const Card& other);
    bool operator==(const Card& other);
    void colorChange(cardColor target);
};

class Player
{
private:
    vector<Card> hand;

public:

};