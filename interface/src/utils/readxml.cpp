/**
 * @class ReadXML
 * @brief Interface XML reader.
 */

#include <QLabel>
#include <QFile>
#include <QXmlStreamReader>

#include <utils/readxml.h>
#include <model.h>


namespace {

/**
 * @brief Parse more general info.
 * @param xml       XML object.
 * @param m         Model object.
 * @return          0.
 */
int _parse_general_info(QXmlStreamReader *xml, Model &m)
{
    QString key = xml->name().toString();

    while (xml->tokenType()!=QXmlStreamReader::EndElement || xml->name()!=key) {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                m.setModelName( xml->text().toString().toStdString() );
            }
           if (xml->name() == "DefaultParameters") {
                xml->readNext();
                m.setExampleParameters( xml->text().toString().toStdString() );
            }
            xml->readNext();
        }
        xml->readNext();
    }

    return 0;
}


/**
 * @brief Reads binary definitions.
 * @param xml       XML-file object.
 * @return          0.
 */
int _parse_binary_info(QXmlStreamReader *xml, Model &m)
{
    QString key = xml->name().toString();

    QString bin_info[3] = {"", "", ""};
    std::vector<QString> output_parsers, result_parsers;

    while (xml->tokenType() != QXmlStreamReader::EndElement || xml->name() != key) {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {

#if defined(__APPLE__)
            if (xml->name() == "BinaryOSX") {
                xml->readNext();
                bin_info[0] = xml->text().toString();
            }
#elif defined(__linux__)
            if (xml->name() == "BinaryLinux") {
                xml->readNext();
                bin_info[0] = xml->text().toString();
            }
#else
            if (xml->name() == "BinaryWindows") {
                xml->readNext();
                bin_info[0] = xml->text().toString();
            }
#endif
            if (xml->name() == "InputStyle") {
                xml->readNext();
                bin_info[1] = xml->text().toString();
            }
            if (xml->name() == "OutputStyle") {
                xml->readNext();
                bin_info[2] = xml->text().toString();
            }
            if (xml->name() == "OutputParser") {
                xml->readNext();
                output_parsers.push_back( xml->text().toString() );
            }
            if (xml->name() == "ResultParser") {
                xml->readNext();
                result_parsers.push_back( xml->text().toString() );
            }
            xml->readNext();
        }
        xml->readNext();
    }

    m.setBinaryInfo( bin_info[0], bin_info[1], bin_info[2],
                       output_parsers, result_parsers );

    return 0;
}



/**
 * @brief Reads a file dialog definition.
 * @return
 */

int _parse_file_dialog(QXmlStreamReader *xml, ParameterWindow &pwin)
{
    QString name;
    int buttonx=0, buttony=0;
    int validButton = 0;

    int cnt = 0;
    while (xml->tokenType() != QXmlStreamReader::EndElement || xml->name() != "FileDialog") {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                name = xml->text().toString();
                validButton++;
            }
            if (xml->name() == "Button") {
                xml->readNext();
                QStringList list = xml->text().toString().split(",");
                if (list.size() != 2) {
                    // TODO: Add a warning/error.
                    continue;
                }
                buttonx = list.at(0).toInt();
                buttony = list.at(1).toInt();

                validButton++;
                // if (DEBUG_MODE) fprintf(stderr, "Button: %d, %d\n", buttonx, buttony);
            }
            xml->readNext();
        }
        xml->readNext();
        cnt++;
        if (cnt>MAX_XML_COUNT) {
            fprintf(stderr, "Error: Malformed XML file (MAX_XML_COUNT exceeded)\n");
            return -1;
        }
    }

    if (validButton==2) {
        pwin.add_file_dialog( name, buttonx, buttony );
    }

    return 0;
}


/**
 * @brief Parse a model view mode.
 * @param xml
 * @param m
 * @return
 */
int _parse_view_mode( QXmlStreamReader *xml, Model &m )
{
    std::pair<std::string,std::string> vmode("", "");

    while (xml->tokenType() != QXmlStreamReader::EndElement || xml->name() != "ViewMode") {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                vmode.first = xml->text().toString().toStdString();
            }
            if (xml->name() == "Content") {
                xml->readNext();
                vmode.second = xml->text().toString().toStdString();
            }
            xml->readNext();
        }
        xml->readNext();
    }

    m.addViewMode(vmode);

    return 0;

}


/**
 * @brief Reads an orientation tag.
 * @return
 */
int _parse_orientation( QXmlStreamReader *xml, Model& model )
{
    orientation orient;
    orient.name = "";
    orient.roty = 0.0;
    orient.rotx = 0.0;

    int valid_orient = 0;

    while (xml->tokenType() != QXmlStreamReader::EndElement ||
           xml->name() != "Orientation") {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                orient.name = xml->text().toString().toStdString();
                valid_orient++;
            }
            if (xml->name() == "Rotate") {
                xml->readNext();
                QStringList list = xml->text().toString().split(",");
                if (list.size() != 2) {
                    // TODO: Add a warning/error.
                    continue;
                }
                orient.rotx = list.at(0).toFloat();
                orient.roty = list.at(1).toFloat();
                valid_orient++;
            }
            xml->readNext();
        }
        xml->readNext();
    }

    if (valid_orient==2) {
        model.addOrientation(orient);
    }

    return 0;
}



/**
 * @brief Reads <Controls>
 * @param xml       XML-file object.
 * @param model     Model object
 * @return          0
 */
int _parse_controls( QXmlStreamReader *xml, Model &model )
{
    QString key = xml->name().toString();

    while (xml->tokenType()!=QXmlStreamReader::EndElement || xml->name()!=key) {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "CellConnections") {
                xml->readNext();

                model.setShowMeshAccess(true);
                model.setShowMesh(true);
                if (xml->text().toString().toLower() == "disable") {
                    model.setShowMeshAccess(false);
                    model.setShowMesh(false);
                }
            }
            if (xml->name() == "ViewMode") {
                _parse_view_mode(xml, model);
            }
            if (xml->name() == "Orientation") {
                _parse_orientation(xml, model);
            }
            if (xml->name() == "ParametersImage") {
                xml->readNext();
                model.setBackgroundImage( xml->text().toString() );
            }
            if (xml->name() == "ModelStepsize") {
                xml->readNext();
                model.setStepSize(xml->text().toString().toInt());
            }
            xml->readNext();
        }
        xml->readNext();
    }

    return 0;
}



/**
 * @brief Called by the XML reader for parsing a parameter.
 * - The order of parameters in XML files define the order in exported/model
 *   parameters files. This is required for the tooth models.
 *
 * @param xml   XML-file object.
 * @return      0 if OK, else -1.
 */
int _parse_parameter(QXmlStreamReader *xml, Parameters& par)
{
    QString name = "", note = "";
    int buttonx=0, buttony=0, width=0, valuex=0, valuey=0;
    bool hidden = true;
    int validButton = 0;
    double value = 0.0;

    int cnt=0;
    while (xml->tokenType() != QXmlStreamReader::EndElement || xml->name() != "Parameter") {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                name = xml->text().toString();
                validButton++;
            }
            if (xml->name() == "Value") {
                xml->readNext();
                value = xml->text().toString().toDouble();
            }
            if (xml->name() == "Button") {
                xml->readNext();
                QStringList list = xml->text().toString().split(",");
                if (list.size() != 2) {
                    // TODO: Add a warning/error.
                    continue;
                }
                buttonx = list.at(0).toInt();
                buttony = list.at(1).toInt();
                valuey = buttony;

                validButton++;
            }
            if (xml->name() == "ButtonWidth") {
                xml->readNext();
                width = xml->text().toString().toInt();
                valuex = buttonx+width-2;
                validButton++;
            }
            if (xml->name() == "Description") {
                xml->readNext();
                note = xml->text().toString();
                validButton++;
            }
            if (xml->name() == "Hidden") {
                xml->readNext();
                hidden = false;
                if (xml->text().toString().toLower() == "true") {
                    hidden = true;
                }
            }
            xml->readNext();
        }
        xml->readNext();

        cnt++;
        if (cnt>MAX_XML_COUNT) {
            fprintf(stderr, "Error: Malformed XML file (MAX_XML_COUNT exceeded)\n");
            return -1;
        }
    }

    // If all required information is given, add the parameter with data to the
    // parameters object.
    if (validButton==4) {
        std::pair<int, int> pos;
        pos.first = buttonx;
        pos.second = buttony;
        par.setButtonLocation(pos);
        pos.first = valuex;
        pos.second = valuey;
        par.setFieldLocation(pos);

        par.setButtonWidth(width);
        par.setButtonNote( note.toStdString() );

        par.setParameter(name.toStdString(), value);
        if (hidden) {
            par.hideParameter(name.toStdString());
        }
    }

    return 0;
}

}


/**
 * @brief Reads model binary definitions common to both GUI and CLI.
 * @param xmlfile       XML file.
 * @param m             Model object.
 * @return          0 if sucess, else -1.
 */
int morphomaker::Read_binary_definitions( const QString xmlfile, Model &m )
{
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cd(RESOURCES);
    QString fname = dir.path() + "/" + xmlfile;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }
    QXmlStreamReader xml(&file);
    Parameters &par = *m.getParameters();

    int err=0;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "Binary") {
                if (_parse_binary_info(&xml, m)) {
                    err=1;
                    break;
                }
            }
            if (xml.name() == "General") {
                if (_parse_general_info(&xml, m)) {
                    err=1;
                    break;
                }
            }
            if (xml.name() == "Controls") {
                if (_parse_controls(&xml, m)) {
                    err=1;
                    break;
                }
            }
            if (xml.name() == "Parameters") {
                continue;
            }
            if (xml.name() == "Parameter") {
                if (_parse_parameter(&xml, par)) {
                    err=1;
                    break;
                }
            }
        }
    }

    xml.clear();
    if (err) {
        return -1;
    }
    return 0;
}



/**
 * @brief Reads model GUI definitions.
 *
 * - TODO: This function is going extinct; should be combined with
 *   Read_bindary_definitions().
 *
 * @param xmlfile       XML file.
 * @param m             Model object.
 * @param pwin          ParameterWindow object.
 * @return          0 if success, else -1.
 */
int morphomaker::Read_GUI_definitions( const QString xmlfile, Model &m,
                                       ParameterWindow &pwin )
{
    QDir dir(QCoreApplication::applicationDirPath());
    dir.cd(RESOURCES);
    QString fname = dir.path() + "/" + xmlfile;

    QFile file(fname);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return -1;
    }
    QXmlStreamReader xml(&file);

    Parameters &par = *m.getParameters();

    int err=0;
    while (!xml.atEnd()) {
        QXmlStreamReader::TokenType token = xml.readNext();
        if (token == QXmlStreamReader::StartElement) {
            if (xml.name() == "Parameters") {
                continue;
            }
            if (xml.name() == "FileDialog") {
                if (_parse_file_dialog(&xml, pwin)) {
                    err=1;
                    break;
                }
            }
        }
    }

    pwin.set_parameter_buttons(par);

    xml.clear();
    if (err) {
        return -1;
    }
    return 0;
}
