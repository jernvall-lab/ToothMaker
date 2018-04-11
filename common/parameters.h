#pragma once

/**
 *  @class Parameters
 *  @brief General-purpose model parameter container.
 *
 *  Contains run-time properties of a model, including model parameters,
 *  parameter notes, model name, view threshold and view mode. The following
 *  keywords are reserved: 'model', 'viewthresh', 'viewmode'. These keywords
 *  are common to all models, thus no model parameter can be named any of those.
 *
 *  Checkboxes are internally treated as floats such that values >0.5 indicate
 *  checked, while values <=0.5 unchecked.
 *
 *  NOTE: Current structure assumes that parameters proper are floating point
 *  numbers. Keywords are stored as strings, thus they may be anything.
 *
 */

#include <string>
#include <vector>
#include "morphomaker.h"

#define PARKEY_MODEL        "model"
#define PARKEY_VIEWTHRESH   "viewthresh"
#define PARKEY_VIEWMODE     "viewmode"
#define PARKEY_ITER         "iter"
enum { PARTYPE_FIELD, PARTYPE_CHECKBOX };



struct parameter {
    std::string name;                   // name both in GUI and internally
    std::string description;            // GUI parameter description
    int type;                           // PARTYPE_FIELD or PARTYPE_CHECKBOX
    std::pair<int, int> position;       // GUI coordinates
    bool hidden;                        // GUI visibility
    double value;                       // parameter value
};



class Parameters
{

public:
    // Constructs Parameters with list of parameter names.
    Parameters(std::vector<std::string>* names = NULL);

    // Copy constructor.
    Parameters(Parameters *);

    ~Parameters()   {}

    // Adds a parameter or updates the value of an existing one.
    void addParameter( parameter& );

    // Get a single parameter by name, or all parameters.
    double getParameter( const std::string& name ) const;
    std::vector<parameter>& getParameters()         { return m_parameters; }

    // Sets the value of an existing parameter.
    void setParameterValue( std::string, double );

    // Set model plain text name.
    void setModelName( std::string& name )          { m_modelName = name; }
    std::string getModelName()                      { return m_modelName; }

    // Set parameters object ID.
    void setID( const std::string& s )              { m_id = s; }
    std::string getID()                             { return m_id; }

    // Set a model key variable.
    void setKey( const std::string& key, const std::string& value );
    std::string getKey( const std::string& key );

    // Returns list of model keywords.
    std::vector<std::string>* getKeywords()         { return &m_keywords; }

    // Returns true if the given variable name is a keyword.
    bool isKeyword( const std::string& name );

    // Adds a model input file (e.g., prepatterns).
    void addModelFile( const std::string& file )    { m_modelFiles.push_back(file); }
    std::string getModelFile(int);



private:
    std::vector<parameter>      m_parameters;

    std::vector<std::string>    m_modelFiles;   // names of files passed to the model
    std::string                 m_modelName;    // model name
    std::string                 m_viewmode;     // current model view mode
    std::string                 m_id;           // parameter object ID

    std::vector<std::string>    m_keywords;     // model keywords (e.g. model, iter)
    std::vector<std::string>    m_keyvalues;    // keyword values
};
