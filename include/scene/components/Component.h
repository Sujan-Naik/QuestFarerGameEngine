#ifndef QUESTFARERGAMEENGINE_COMPONENT_H
#define QUESTFARERGAMEENGINE_COMPONENT_H

class Component{

public:
    virtual ~Component(){}

    virtual void receive(int message) = 0;

    virtual void update() = 0;
};

#endif //QUESTFARERGAMEENGINE_COMPONENT_H
