#ifndef QUESTFARERGAMEENGINE_CLOUDSIMULATOR_H
#define QUESTFARERGAMEENGINE_CLOUDSIMULATOR_H

#include <vector>
#include "Cloud.h"

class CloudSimulator{

private:
    std::vector<Cloud> clouds;

public:
    CloudSimulator();

    std::vector<Cloud>& getClouds();
};

#endif //QUESTFARERGAMEENGINE_CLOUDSIMULATOR_H
