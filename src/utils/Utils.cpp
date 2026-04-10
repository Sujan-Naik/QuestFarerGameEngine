#include "../../include/utils/Utils.h"
#include <cstdlib>
#include <random>

static std::random_device rd;
static std::mt19937 gen(rd());
namespace utils {


    glm::vec3 getVectorsAverage(const std::vector<glm::vec3> &vectors) {
        glm::vec3 average = {0, 0, 0};
        for (auto vec: vectors) {
            average += vec;
        }
        average /= glm::vec3{vectors.size(), vectors.size(), vectors.size()};
        return average;
    }

    float getRandomFloat(float min, float max) {

        std::uniform_real_distribution<> distrib(min, max);

        return distrib(gen);

    }


    bool getRandomBool(int percentageSuccess) {
        return rand() % 101 < percentageSuccess;
    }
}