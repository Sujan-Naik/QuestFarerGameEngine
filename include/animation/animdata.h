#ifndef QUESTFARERGAMEENGINE_ANIMDATA_H
#define QUESTFARERGAMEENGINE_ANIMDATA_H


#include"glm/glm.hpp"

namespace animation {

    struct BoneInfo {
        /*id is index in finalBoneMatrices*/
        int id;

        /*offset matrix transforms vertex from model space to bone space*/
        glm::mat4 offset;

    };
}


#endif //QUESTFARERGAMEENGINE_ANIMDATA_H
