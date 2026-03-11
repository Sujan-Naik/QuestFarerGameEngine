#include "../../include/cloud/CloudSimulator.h"
#include "glm/vec3.hpp"
#include "../../include/utils/Utils.h"
#include "../../include/globals.h"


std::vector<Cloud>& CloudSimulator::getClouds() {
    return clouds;
}


CloudSimulator::CloudSimulator() {
    clouds = std::vector<Cloud>();

    for (int x = -EFFECTIVE_SIDE_LENGTH/2; x <  EFFECTIVE_SIDE_LENGTH/2; x++){
        for (int z = -EFFECTIVE_SIDE_LENGTH/2; z < EFFECTIVE_SIDE_LENGTH ; z++) {
            if (getRandomBool(CLOUD_CHANCE)){
                Cloud cloud = {glm::vec3(x,HEIGHT_UPPER_BOUND * 1.5,z), {0,0,0}, 0.2};
                clouds.push_back( cloud);
            }

        }
    }
}
