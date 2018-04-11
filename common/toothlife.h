#pragma once

/**
 * @class ToothLife
 * @brief Container for a complete model run.
 *
 * A model run consists of model parameters, a set of Tooth objects (one per
 * step) and a run ID to distinguish between ToothLife objects.
 *
 */

#include <vector>
#include "tooth.h"
#include "parameters.h"


class ToothLife
{

public:
    // Construct Tooth for model i with run ID j.
    ToothLife( int i=0, int j=0 ) : m_parameters(nullptr)
    {
        m_currentModel = i;
        m_id = j;
    }

    ~ToothLife()
    {
        for (size_t i=0; i<m_teeth.size(); i++)
            delete m_teeth.at(i);
        m_teeth.clear();
        if (m_parameters != nullptr)
            delete m_parameters;
    }

    // Set current model parameters.
    void setParameters( Parameters *par )
    {
        if (m_parameters != nullptr)
            delete m_parameters;
        m_parameters = new Parameters(par);
    }

    // Get current model parameters.
    Parameters *getParameters()             { return m_parameters; }

    // Add a tooth object.
    void addTooth( Tooth *tooth )           { m_teeth.push_back(tooth); }

    // Get a tooth object by index.
    Tooth *getTooth( int i )
    {
        if (i>=0 && i<(int)(m_teeth.size()))
            return m_teeth.at(i);
        return nullptr;
    }

    // Return the number of tooth objects.
    int getLifeSize()                       { return m_teeth.size(); }

    // Get model index.
    int getCurrentModel()                   { return m_currentModel; }

    // Get model run ID.
    int getID()                             { return m_id; }



private:
    Parameters* m_parameters;           // model parameters
    unsigned int m_currentModel;        // model index
    std::vector<Tooth*> m_teeth;        // vector of model states
    int m_id;                           // model run ID

};
