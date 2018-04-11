/**
 * @class ReadXML
 * @brief Interface XML reader.
 */

#include <iostream>
#include <QLabel>
#include <QCheckBox>
#include <QFile>
#include <QXmlStreamReader>

#include "utils/readxml.h"
#include "model.h"


namespace {

/**
 * @brief Parse more general info.
 * @param xml       XML object.
 * @param m         Model object.
 * @return          0.
 */
int parse_general_info_(QXmlStreamReader *xml, Model &m)
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
int parse_binary_info_(QXmlStreamReader *xml, Model &m)
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

int parse_file_dialog_(QXmlStreamReader *xml, ParameterWindow &pwin)
{
    QStringList list;
    int buttonx=0, buttony=0, validButton;
    int cnt=0;
    QString name;

    validButton=0;

    while (xml->tokenType() != QXmlStreamReader::EndElement || xml->name() != "FileDialog") {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                name = xml->text().toString();
                validButton++;
            }
            if (xml->name() == "Button") {
                xml->readNext();
                list = xml->text().toString().split(",");
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
        pwin.addFileDialog( name, buttonx, buttony );
    }

    return 0;
}


/**
 * @brief Parse a model view mode.
 * @return
 */
int parse_view_mode_( QXmlStreamReader *xml, Model &m )
{
    model::view_mode mode;

    while (xml->tokenType() != QXmlStreamReader::EndElement || xml->name() != "ViewMode") {
        if (xml->tokenType() == QXmlStreamReader::StartElement) {

            if (xml->name() == "Name") {
                xml->readNext();
                mode.name = xml->text().toString().toStdString();
            }
            if (xml->name() == "Shape") {
                xml->readNext();
                QStringList list = xml->text().toString().split(",");
                if (list.size() != 2)
                    continue;               // TODO: Add a warning/error.
                mode.shapes.push_back( {list[0].toInt(), list[1].toInt()} );
            }
            if (xml->name() == "Content") {
                xml->readNext();
                mode.shapes.push_back( {xml->text().toString().toInt(), -1} );
            }
            xml->readNext();
        }
        xml->readNext();
    }

    m.addViewMode(mode);

    return 0;

}


/**
 * @brief Reads an orientation tag.
 * @return
 */
int parse_orientation_( QXmlStreamReader *xml, Model& model )
{
    model::orientation orient;
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
                if (list.size() != 2)
                    continue;               // TODO: Add a warning/error.
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
int parse_controls_( QXmlStreamReader *xml, Model &model )
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
                parse_view_mode_(xml, model);
            }
            if (xml->name() == "Orientation") {
                parse_orientation_(xml, model);
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
 *
 *  The order of parameters in XML files defines the order in exported/model
 *  parameters files, which is important for some models.
 *
 * @param xml       XML-file object.
 * @param par_type  Parameter type: "Field"/"Parameter" or "CheckBox".
 * @return          0 if OK, else -1.
 */
int parse_parameter_( QXmlStreamReader *xml, QString par_type, Parameters& par )
{
    QString name, note;
    std::pair<int, int> position;
    bool hidden = true;
    double value = 0.0;

    int validItem = 0;
    int cnt = 0;

    while (xml->tokenType() != QXmlStreamReader::EndElement
           || xml->name() != par_type) {

        if (xml->tokenType() == QXmlStreamReader::StartElement) {
            if (xml->name() == "Name") {
                xml->readNext();
                name = xml->text().toString();
                validItem++;
            }
            if (xml->name() == "Value") {
                xml->readNext();
                value = xml->text().toString().toDouble();
            }
            if (xml->name() == "Position") {
                xml->readNext();
                QStringList list = xml->text().toString().split(",");
                if (list.size() != 2) {
                    // TODO: Add a warning/error.
                    continue;
                }
                position = { list.at(0).toInt(), list.at(1).toInt() };
                validItem++;
            }
            if (xml->name() == "Description") {
                xml->readNext();
                note = xml->text().toString();
                validItem++;
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
            std::cerr << "Error: Malformed XML file (MAX_XML_COUNT exceeded)."
                      << std::endl;
            return -1;
        }

    }

    // If all required information is given, add the parameter with data to the
    // parameters object.
    if (validItem == 3) {
        int type = PARTYPE_FIELD;
        if (par_type == "CheckBox")
            type = PARTYPE_CHECKBOX;

        parameter p = { name.toStdString(), note.toStdString(), type,
                        position, hidden, value };
        par.addParameter(p);
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
            auto name = xml.name();

            if (name == "Binary") {
                if (parse_binary_info_(&xml, m)) {
                    err=1;
                    break;
                }
            }
            if (name == "General") {
                if (parse_general_info_(&xml, m)) {
                    err=1;
                    break;
                }
            }
            if (name == "Controls") {
                if (parse_controls_(&xml, m)) {
                    err=1;
                    break;
                }
            }
            if (name == "Parameters") {
                continue;
            }
            if (name == "Parameter" || name == "Field" || name == "CheckBox") {
                if (parse_parameter_(&xml, name.toString(), par)) {
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
                if (parse_file_dialog_(&xml, pwin)) {
                    err=1;
                    break;
                }
            }
        }
    }

    pwin.setParameters(par);

    xml.clear();
    if (err) {
        return -1;
    }
    return 0;
}
