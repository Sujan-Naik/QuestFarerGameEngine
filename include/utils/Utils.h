#ifndef QUESTFARERGAMEENGINE_UTILS_H
#define QUESTFARERGAMEENGINE_UTILS_H

#include "glm/vec3.hpp"

/**
 * @brief Calculates the average of vectors
 * @param vectors A vector of vectars
 * @return The average of inputted vectors
 */
glm::vec3 getVectorsAverage(const std::vector<glm::vec3>& vectors);

bool getRandomBool(int percentageSuccess);

float getRandomFloat(float min, float max);


#endif //QUESTFARERGAMEENGINE_UTILS_H
