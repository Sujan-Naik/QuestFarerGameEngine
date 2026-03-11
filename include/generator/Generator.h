#ifndef QUESTFARERGAMEENGINE_GENERATOR_H
#define QUESTFARERGAMEENGINE_GENERATOR_H


#include <memory>
#include "Grid.h"
#include "Square.h"
#include "../logger/Logger.h"

/**
 * @class Generator
 * @brief An implementation of a static Diamond-Square world generator combined with dynamic Rikitake noise
 */
class Generator {

private:
    int sideLength; // "2^N + 1"
    int size;
    Grid grid;


    /**
     * Handles a singular stage within the Diamond Square algorithm recursion.
     * @param i the number of the iteration
     */
    void performIteration(int i);
        std::vector<Square> squares;

public:
    /**
     * @brief Constructor - initialises a Generator
     * @param size details the size used to calculate how long the sides in, 2^size - 1
     * @param heightLowerBound The lowest height that a grid point can be displaced.
     * @param heightUpperBound The highest height that a grid point can be displayed
     */
    Generator(int size, float heightLowerBound, float heightUpperBound);

    /**
     * Initially generate the grid to begin with
     */
    void generateGrid();


    /**
     * Accesses the output of the Diamond Square Algorithm
     * @return A reference to the vector of final computed squares
     */
    [[nodiscard]] const vector<Square> &getSquares() const;

    /**
     * Displays information about the heightmap
     * @return A reference to the Grid structure
     */
    [[nodiscard]] const Grid &getGrid() const;

};


#endif //QUESTFARERGAMEENGINE_GENERATOR_H
