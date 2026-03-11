#include "../../include/utils/Utils.h"
#include "../../include/generator/Generator.h"
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/gtx/string_cast.hpp"
#include <cmath>
#include <utility>
#include "../include/globals.h"

// size is  the N "2^N + 1"
Generator::Generator(int size,  float heightLowerBound, float heightUpperBound): size(size),
        sideLength(pow(2,size)+1) {


    grid.heightLowerBound = heightLowerBound;
    grid.heightUpperBound = heightUpperBound;

}

void Generator::performIteration(int i) {

    std::vector<Square> tempSquares;
    unsigned int newSquares = squares.size() * 4;
    tempSquares.reserve(newSquares);

    float range = (grid.heightUpperBound - grid.heightLowerBound) * pow(0.5f, i);

    for (const Square& square: squares) {
        glm::vec3 bl = square.getBottomLeft(grid);
        glm::vec3 tl = square.getTopLeft(grid);
        glm::vec3 br = square.getBottomRight(grid);
        glm::vec3 tr = square.getTopRight(grid);

        glm::ivec2 mIndex = (square.bottomLeft + square.topRight) / 2;
        glm::ivec2 mlIndex = (square.bottomLeft + square.topLeft) / 2;
        glm::ivec2 mrIndex = (square.bottomRight + square.topRight) / 2;
        glm::ivec2 tmIndex = (square.topLeft + square.topRight) / 2;
        glm::ivec2 bmIndex = (square.bottomLeft + square.bottomRight) / 2;


        glm::vec3 m = getVectorsAverage({bl, tl, br, tr});
        m.y = getRandomFloat(-range, range);
        grid.heightmap[mIndex.x][ mIndex.y].position = m;
        grid.heightmap[mIndex.x][ mIndex.y].basePosition = m;

        glm::vec3 ml = getVectorsAverage({bl, tl});
        ml.y += getRandomFloat(-range, range);
        grid.heightmap[mlIndex.x][ mlIndex.y].position = ml;
        grid.heightmap[ mlIndex.x][ mlIndex.y].basePosition = ml;

        glm::vec3 mr = getVectorsAverage({br, tr});
        mr.y += getRandomFloat(-range, range);
        grid.heightmap[ mrIndex.x][ mrIndex.y].position = mr;
        grid.heightmap[ mrIndex.x][ mrIndex.y].basePosition = mr;

        glm::vec3 tm = getVectorsAverage({tl, tr});
        tm.y += getRandomFloat(-range, range);
        grid.heightmap[ tmIndex.x][ tmIndex.y].position = tm;
        grid.heightmap[ tmIndex.x][ tmIndex.y].basePosition = tm;

        glm::vec3 bm = getVectorsAverage({bl, br});
        bm.y += getRandomFloat(-range, range);
        grid.heightmap[ bmIndex.x][ bmIndex.y].position = bm;
        grid.heightmap[ bmIndex.x][ bmIndex.y].basePosition = bm;


        Square bottomLeftSquare{};
        bottomLeftSquare.bottomLeft = square.bottomLeft;
        bottomLeftSquare.topLeft = mlIndex;
        bottomLeftSquare.bottomRight = bmIndex;
        bottomLeftSquare.topRight = mIndex;
        tempSquares.push_back(bottomLeftSquare);

        Square topLeftSquare{};
        topLeftSquare.bottomLeft = mlIndex;
        topLeftSquare.topLeft = square.topLeft;
        topLeftSquare.bottomRight = mIndex;
        topLeftSquare.topRight = tmIndex;
        tempSquares.push_back(topLeftSquare);


        Square bottomRightSquare{};
        bottomRightSquare.bottomLeft = bmIndex;
        bottomRightSquare.topLeft = mIndex;
        bottomRightSquare.bottomRight = square.bottomRight;
        bottomRightSquare.topRight = mrIndex;
        tempSquares.push_back(bottomRightSquare);


        Square topRightSquare{};
        topRightSquare.bottomLeft = mIndex;
        topRightSquare.topLeft = tmIndex;
        topRightSquare.bottomRight = mrIndex;
        topRightSquare.topRight = square.topRight;
        tempSquares.push_back(topRightSquare);
    }
    squares = tempSquares;
}

void Generator::generateGrid() {
    grid.heightmap = std::vector<std::vector<GridPoint>>
            (sideLength, std::vector<GridPoint>(sideLength));

    for (int x = 0; x <  sideLength; x++){
        for (int z = 0; z < sideLength ; z++) {

            grid.heightmap[x][z] = GridPoint(glm::vec3(x, 0, z));
        }
    }

    float range = (grid.heightUpperBound - grid.heightLowerBound)/2;

    grid.heightmap[0][0].position.y = getRandomFloat(-range, range);
    grid.heightmap[0][0].basePosition = grid.heightmap[0][0].position;

    grid.heightmap[0][sideLength-1].position.y = getRandomFloat(-range, range);
    grid.heightmap[0][sideLength-1].basePosition =grid.heightmap[0][sideLength-1].position;

    grid.heightmap[sideLength-1][0].position.y = getRandomFloat(-range, range);
    grid.heightmap[sideLength-1][0].basePosition = grid.heightmap[sideLength-1][0].position;

    grid.heightmap[sideLength-1][sideLength-1].position.y = getRandomFloat(-range, range);
    grid.heightmap[sideLength-1][sideLength-1].basePosition = grid.heightmap[sideLength-1][sideLength-1].position;

    Square square{};
    square.bottomLeft = glm::ivec2(0,0);
    square.topLeft = glm::ivec2(0, sideLength-1);
    square.bottomRight = glm::ivec2(sideLength-1, 0 );
    square.topRight = glm::ivec2(sideLength-1, sideLength-1 );
    squares = {square};

    for (int i = 0 ; i < size ; i++){
        performIteration(i);
    }
}



const vector<Square> &Generator::getSquares() const {
    return squares;
}

const Grid &Generator::getGrid() const {
    return grid;
}


















