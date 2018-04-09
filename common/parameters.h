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


typedef std::vector<std::pair<int,int> > pvector;


class Parameters
{

public:
    // Constructs Parameters with list of parameter names.
    Parameters(std::vector<std::string>* names = NULL);

    // Copy constructor.
    Parameters(Parameters *);

    ~Parameters()   {}

    // Sets model parameter.
    void setParameter( std::string name, double value );
    double getParameter( std::string name );

    // Return parameter values, names, notes.
    std::vector<double> *getParValues()         { return &parValues; }
    std::vector<std::string> *getParNames()     { return &parNames; }
    std::vector<std::string> *getParNotes()     { return &parNotes; }

    // Set model plain text name.
    void setModelName( std::string& name )      { modelName = name; }
    std::string getModelName()                  { return modelName; }

    // Marks a parameter as hidden from the GUI.
    void hideParameter( std::string name )      { hiddenParameters.push_back(name); }
    bool isParameterHidden( std::string name );

    // Returns hidden parameter names.
    std::vector<std::string> *getHiddenParameters()     { return &hiddenParameters; }

    // Sets parameters object ID.
    void setID( std::string s )                 { id = s; }
    std::string getID()                         { return id; }

    // Sets a model key variable.
    void setKey( std::string key, std::string value );
    std::string getKey( std::string key );

    // Returns list of model keywords.
    std::vector<std::string> *getKeywords()     { return &keywords; }

    // Returns true if the given variable name is a keyword.
    bool isKeyword( std::string name );

    // Adds a model input file (e.g., prepatterns).
    void addModelFile(std::string file )        { modelFiles.push_back(file); }
    std::string getModelFile(int);

    // Set button width in the GUI.
    void setButtonWidth( const int width )      { buttonWidth.push_back(width); }
    std::vector<int>& getButtonWidths()         { return buttonWidth; }

    // Set parameter button position coordinates (x,y).
    void setButtonLocation( const std::pair<int,int>& loc ) { buttonLoc.push_back(loc); }
    pvector& getButtonLocations()                           { return buttonLoc; }

    // Set parameter field position coordinates (x,y).
    void setFieldLocation( const std::pair<int,int>& loc )  { fieldLoc.push_back(loc); }

    // Assigns a note/description to a parameter button.
    void setButtonNote( const std::string& note )           { parNotes.push_back(note); }



private:
    std::vector<std::string> modelFiles;        // Names of files passed to the model
    std::vector<std::string> parNames;          // Paramter names
    std::vector<std::string> parNotes;          // Parameter descriptions
    std::vector<double> parValues;              // Parameter values
    std::string modelName;                      // Model name
    std::string viewmode;                       // Current model view mode
    std::vector<std::string> hiddenParameters;  // Names parameters hidden from GUI
    std::string id;                             // Unique model ID

    std::vector<std::string> keywords;          // Model keywords (e.g. model, iter)
    std::vector<std::string> keyvalues;         // Keyword values

    pvector buttonLoc;                          // Parameter button locations in GUI
    std::vector<int> buttonWidth;               // Button widths
    pvector fieldLoc;                           // Parameter field locations in GUI
};
