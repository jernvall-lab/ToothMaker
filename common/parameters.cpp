
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
        for (uint32_t i=0; i<names->size(); i++) {
            parameter p = { names->at(i), "", 0, {}, false, 0.0 };
        }
    }

    m_modelName = "";
    m_id = "";

    // Reserved keyword. NOTE: These must not be tampered with, as the rest of
    // the program assumes these precise names!
    m_keywords.push_back(PARKEY_MODEL);
    m_keyvalues.push_back("");
    m_keywords.push_back(PARKEY_VIEWTHRESH);
    m_keyvalues.push_back("");
    m_keywords.push_back(PARKEY_VIEWMODE);
    m_keyvalues.push_back("");
    m_keywords.push_back(PARKEY_ITER);
    m_keyvalues.push_back("");
}



/**
 * @brief Copy-constructor.
 * @param old       Parameters to copy.
 */
Parameters::Parameters( Parameters* old )
{
    for (auto& p : old->getParameters())
        m_parameters.push_back(p);

    m_modelName=old->getModelName();

    for (size_t i=0; i<old->getKeywords()->size(); i++) {
        std::string tmp = old->getKey( old->getKeywords()->at(i) );
        m_keywords.push_back( old->getKeywords()->at(i) );
        m_keyvalues.push_back( tmp );
    }
    m_id = old->getID();
}



/**
 * @brief Adds a parameter or updates the value of an existing one.
 * @param par
 */
void Parameters::addParameter( parameter& par )
{
    bool found = false;
    for (auto& p : m_parameters) {
        if (!par.name.compare( p.name )) {
            p.value = par.value;
            found = true;
        }
    }
    if (!found)
        m_parameters.push_back( par );
}



/**
 * @brief Returns a single parameter value by name.
 * @param name      Parameter name.
 * @return
 */
double Parameters::getParameter( const std::string& name ) const
{
    for (auto& p : m_parameters) {
        if (!name.compare( p.name ))
            return p.value;
    }
    return 0.0;
}



/**
 * @brief Sets the value of an existing parameter.
 */
void Parameters::setParameterValue( std::string name, double value )
{
    for (auto& p : m_parameters) {
        if (!name.compare( p.name )) {
            p.value = value;
        }
    }
}



/**
 * @brief Sets key variable value.
 * @param key   Key name.
 * @param val   Key value.
 */
void Parameters::setKey( const std::string& key, const std::string& val)
{
    for (size_t i=0; i<m_keywords.size(); i++) {
        if (!key.compare(m_keywords.at(i))) {
            m_keyvalues.at(i) = val;
        }
    }
}



/**
 * @brief Returns the key value.
 * @param key       Key name.
 * @return          Key value or empty.
 */
std::string Parameters::getKey( const std::string& key )
{
    for (size_t i=0; i<m_keywords.size(); i++) {
        if (!key.compare(m_keywords.at(i))) {
            return m_keyvalues.at(i);
        }
    }

    return "";
}



/**
 * @brief Tells if the given variable name is a keyword.
 * @param key       Variable name.
 * @return          True or False.
 */
bool Parameters::isKeyword( const std::string& key )
{
    for (size_t i=0; i<m_keywords.size(); i++) {
        if (!key.compare(m_keywords.at(i))) {
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
    if (i<0 || (unsigned int)i>=m_modelFiles.size())
        return "";
    return m_modelFiles.at(i);
}
