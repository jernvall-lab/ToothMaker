#pragma once

/**
 * @class ToothLife
 * @brief Container for a complete model run.
 *
 * - Container for a set of Tooth objects.
 */

#include <vector>
#include "tooth.h"
#include "parameters.h"


class ToothLife
{

public:
    // Construct Tooth for model i with run ID j.
    ToothLife( int i=0, int j=0 ) : parameters(NULL)
    {
        currentModel = i;
        id = j;
    }

    ~ToothLife()
    {
        for (size_t i=0; i<teeth.size(); i++)
            delete teeth.at(i);
        teeth.clear();
        if (parameters != NULL)
            delete parameters;
    }

    // Set current model parameters.
    void setParameters( Parameters *par )
    {
        if (parameters != NULL)
            delete parameters;
        parameters = new Parameters(par);
    }

    // Get current model parameters.
    Parameters *getParameters()             { return parameters; }

    // Add a tooth object.
    void addTooth( Tooth *tooth )           { teeth.push_back(tooth); }

    // Get a tooth object by index.
    Tooth *getTooth( int i )
    {
        if (i>=0 && i<(int)(teeth.size()))
            return teeth.at(i);
        return NULL;
    }

    // Return the number of tooth objects.
    int getLifeSize()                       { return teeth.size(); }

    // Get model index.
    int getCurrentModel()                   { return currentModel; }

    // Get model run ID.
    int getID()                             { return id; }



private:
    Parameters *parameters;         // Model parameters.
    unsigned int currentModel;      // Model index.
    std::vector<Tooth*> teeth;      // Vector of model states.
    int id;                         // Model run ID.

};
