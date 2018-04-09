
#include <stdlib.h>
#include <stdio.h>
#if !defined(__APPLE__)
#include <malloc.h>
#endif

#include "parameters.h"


/**
 * @brief Constructs Parameters with list of parameter names.
 * @param names     Vector containing the model parameter names.
 */
Parameters::Parameters(std::vector<std::string> *names)
{
    if (names != NULL) {
        for (size_t i=0; i<names->size(); i++) {
            parNames.push_back(names->at(i));
            parValues.push_back(0.0);
        }
    }

    modelName="";
    id="";

    // Reserved keyword. NOTE: These must not be tampered with, as the rest of
    // the program assumes these precise names!
    keywords.push_back(PARKEY_MODEL);
    keyvalues.push_back("");
    keywords.push_back(PARKEY_VIEWTHRESH);
    keyvalues.push_back("");
    keywords.push_back(PARKEY_VIEWMODE);
    keyvalues.push_back("");
    keywords.push_back(PARKEY_ITER);
    keyvalues.push_back("");
}



/**
 * @brief Copy-constructor.
 * @param old       Parameters to copy.
 */
Parameters::Parameters(Parameters *old)
{
    std::string tmp;

    for (size_t i=0; i<old->getParNames()->size(); i++) {
        parNames.push_back(old->getParNames()->at(i));
    }
    for (size_t i=0; i<old->getHiddenParameters()->size(); i++) {
        hiddenParameters.push_back(old->getHiddenParameters()->at(i));
    }
    for (size_t i=0; i<old->getParNames()->size(); i++) {
        parValues.push_back(old->getParValues()->at(i));
    }
    modelName=old->getModelName();

    for (size_t i=0; i<old->getKeywords()->size(); i++) {
        tmp = old->getKey(old->getKeywords()->at(i));
        keywords.push_back(old->getKeywords()->at(i));
        keyvalues.push_back(tmp);
    }
    id = old->getID();
}



/**
 * @brief Sets parameters value.
 * @param name      Parameter name.
 * @param value     Floating-point value of the parameter.
 */
void Parameters::setParameter(std::string name, double value)
{

    if (parNames.size() != parValues.size()) {
        if (DEBUG_MODE)
            fprintf(stderr, "Error: %s(): parNames, parValues differ in size! (%lu != %lu)\n",
                    __FUNCTION__, parNames.size(), parValues.size());
        return;
    }

    bool found = false;
    for (size_t i=0; i<parNames.size(); i++) {
        if (!name.compare(parNames.at(i))) {
            parValues.at(i) = value;
            found = true;
        }
    }
    if (!found) {
        parNames.push_back(name);
        parValues.push_back(value);
    }
}



/**
 * @brief Returns a single parameter value by name.
 * @param name      Parameter name.
 * @return
 */
double Parameters::getParameter(std::string name)
{
    if (parNames.size() != parValues.size()) {
        if (DEBUG_MODE)
            fprintf(stderr, "Error: %s(): parNames, parValues differ in size! (%lu != %lu)\n",
                    __FUNCTION__, parNames.size(), parValues.size());
        return 0.0;
    }

    for (size_t i=0; i<parNames.size(); i++) {
        if (!name.compare(parNames.at(i))) return parValues.at(i);
    }

    return 0.0;
}



/**
 * @brief Tells if the given parameter is set as hidden.
 * @param par       Parameter name.
 * @return          True or False.
 */
bool Parameters::isParameterHidden(std::string name)
{
    for (size_t i=0; i<hiddenParameters.size(); i++) {
        if (!name.compare(hiddenParameters.at(i))) return true;
    }
    return false;
}



/**
 * @brief Sets key variable value.
 * @param key   Key name.
 * @param val   Key value.
 */
void Parameters::setKey(std::string key, std::string value)
{
    for (size_t i=0; i<keywords.size(); i++) {
        if (!key.compare(keywords.at(i))) {
            keyvalues.at(i) = value;
        }
    }
}



/**
 * @brief Returns the key value.
 * @param key       Key name.
 * @return          Key value or empty.
 */
std::string Parameters::getKey(std::string key)
{
    for (size_t i=0; i<keywords.size(); i++) {
        if (!key.compare(keywords.at(i))) {
            return keyvalues.at(i);
        }
    }

    return "";
}



/**
 * @brief Returns true if the given variable name is a keyword.
 * @param key       Variable name.
 * @return          True or False.
 */
bool Parameters::isKeyword( std::string name )
{
    for (size_t i=0; i<keywords.size(); i++) {
        if (!name.compare(keywords.at(i))) {
            return true;
        }
    }

    return false;
}



/**
 * @brief Gets model file name by index.
 * @param i     File name index.
 * @return
 */
std::string Parameters::getModelFile(int i)
{
    if (i<0 || (unsigned int)i>=modelFiles.size()) return "";
    return modelFiles.at(i);
}
