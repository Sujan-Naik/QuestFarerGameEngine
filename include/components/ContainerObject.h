#ifndef QUESTFARERGAMEENGINE_CONTAINEROBJECT_H
#define QUESTFARERGAMEENGINE_CONTAINEROBJECT_H

#include "Component.h"

class ContainerObject
{
public:
    void send(int message)
    {
        for (int i = 0; i < MAX_COMPONENTS; i++)
        {
            if (components_[i] != nullptr)
            {
                components_[i]->receive(message);
            }
        }
    }

private:
    static const int MAX_COMPONENTS = 10;
    Component* components_[MAX_COMPONENTS];
};


#endif //QUESTFARERGAMEENGINE_CONTAINEROBJECT_H
