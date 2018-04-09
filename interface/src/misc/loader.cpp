/**
 * @class Loader
 * @brief Loads and registers the available models (libraries, binaries, scripts).
 *
 * Used by both Hampu (GUI) and CMDAppCore (CLI).
 *
 */

#include <iostream>
#include <QDir>
#include <QLibrary>

#include "model.h"
#include "misc/binaryhandler.h"
#include "misc/loader.h"
#include "utils/readxml.h"


namespace {

/**
 * @brief Returns the list of detected interface XMLs.
 * @return      Vector of strings.
 */
std::vector<QString> _get_model_interfaces()
{
    QDir qdir(QCoreApplication::applicationDirPath());
    qdir.cd(RESOURCES);

    QStringList filters, files;
    filters << "*.xml";
    files = qdir.entryList(filters);

    std::vector<QString> vec;
    for (auto& f : files) {
        vec.push_back(f);
    }

    return vec;
}

}



/**
 * @brief Returns the list of available models.
 * @param models        Vector of model objects.
 */
void morphomaker::Load_models( std::vector<Model*> &models )
{
    QDir qdir(QCoreApplication::applicationDirPath());
    qdir.cd(RESOURCES);
    qdir.cd("bin");

    std::vector<QString> xmls = _get_model_interfaces();
    Model model, *modelp;
    typedef Model* create_m();

    std::cout << "Looking for available models..." << std::endl;

    for (auto& f : xmls) {
        // Just getting the model binary file name here; assigning the binary
        // info later.
        morphomaker::Read_binary_definitions(f, model);
        auto name = model.getBinaryName();

        // Load library models.
        if (QLibrary::isLibrary(name)) {
            QString path = qdir.path() + "/" + name;
            QLibrary library(path);
            if (!library.load()) {
                std::cerr << library.errorString().toStdString() << std::endl;
                continue;
            }

            create_m* mm = (create_m*)library.resolve(LOAD_NAME);
            if (!mm) {
                if (DEBUG_MODE) std::cerr << "Cannot load '" << path.toStdString()
                                          << "': Unknown error."<< std::endl;
                continue;
            }
            modelp = mm();
            std::cout << " * Library '" << name.toStdString() << "' loaded ("
                      << f.toStdString() << ")." << std::endl;
        }

        // Load binary models.
        else {
            modelp = new BinaryHandler();
            if (modelp==NULL) {
                continue;
            }
            std::cout << " * Binary '" << name.toStdString() << "' loaded ("
                      << f.toStdString() << ")." << std::endl;
        }

        models.push_back(modelp);
        morphomaker::Read_binary_definitions( f, *modelp );
        modelp->setInterfaceXML(f);
    }

}

