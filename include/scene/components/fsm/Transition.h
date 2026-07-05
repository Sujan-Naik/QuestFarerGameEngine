#ifndef QUESTFARERGAMEENGINE_TRANSITION_H
#define QUESTFARERGAMEENGINE_TRANSITION_H

namespace scene::components::fsm {

    class Transition {
    public:
        virtual ~Transition() = default;
        virtual bool ShouldTransition() = 0;
    };
}

#endif