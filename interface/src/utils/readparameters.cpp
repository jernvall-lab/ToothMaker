/**
 * @file readparameters.cpp
 * @brief Reads model parameters, scanlist.
 *
 * User calls Import_parameters().
 *
 * Supported file formats: MorphoMaker.
 *
 */

#include <stdio.h>
#include <QString>
#include <QStringList>

#include "utils/readparameters.h"



/**
 * @brief Reads parameters file.
 * - Any line beginning with # is ignored.
 * - Data is to be formatted as [tag]==[value], white spaces allowed for strings.
 *
 * @param file      File name.
 * @param par       Parameters object to store the parameters.
 * @return          0 OK
 * @return          -1 Error
 */
int morphomaker::Import_parameters(std::string file, Parameters *par)
{
    FILE* input = fopen(file.c_str(), "r");
    if (input==NULL) {
        fprintf(stderr, "%s(): Can't open file '%s'. Aborted.\n", __FUNCTION__,
                file.c_str());
        return -1;
    }

    #if defined(__linux__)
    char *oldloc = setlocale(LC_ALL, "C");
    #endif

    while (!feof(input) && !ferror(input)) {
        char line[256];
        strcpy(line, "");
        if (fgets(line, 255, input) == nullptr) continue;
        QString str = QString(line);
        if (line[0]=='#' || !strcmp(line, "") || !strcmp(line, "\n")) continue;

        QStringList list = str.split("\n");
        list = list[0].split("==");
        if (list.size()>=2 && !list[0].trimmed().isEmpty() && !list[1].trimmed().isEmpty()) {
            QString key = list[0].trimmed();
            QString value = list[1].trimmed();
            if (par->isKeyword(key.toLower().toStdString())) {
                par->setKey(key.toLower().toStdString(), value.toStdString());
            }
            else {
                par->setParameterValue( key.toStdString(), value.toDouble() );
            }
        }
    }

    #if defined(__linux__)
    setlocale(LC_ALL, oldloc);
    #endif

    fclose(input);
    return 0;
}



/**
 * @brief Reads the scanlist provided at the command line.
 * @param file      File name.
 * @return          ScanList object.
 */
ScanList* morphomaker::Read_scanlist(std::string file)
{
    FILE* input = fopen(file.c_str(), "r");
    if (input==NULL) {
        fprintf(stderr, "%s(): Can't open file '%s'. Aborted.\n", __FUNCTION__,
                file.c_str());
        return NULL;
    }

    #if defined(__linux__)
    char *oldloc = setlocale(LC_ALL, "C");
    #endif

    ScanList *scanList = new ScanList();

    while (!feof(input) && !ferror(input)) {
        char line[256];
        strcpy(line, "");
        if (fgets(line, 255, input) == nullptr) continue;
        if (line[0]=='#' || !strcmp(line, "") || !strcmp(line, "\n")) continue;
        QString str = QString(line);
        QStringList list = str.split("\n");
        list = list[0].split("==");
        if (list.size() < 2) continue;
        QString key = list[0].trimmed();
        QString value = list[1].trimmed();

        if (!key.toLower().compare("model")) {
            // TODO: Implement reading model name from the scan list.
            // std::string model = value.toStdString();
        }
        else if (!key.toLower().compare("viewmode")) {
            if (!value.compare("BW") || !value.compare("1")
                || !value.toLower().compare("differentiation")) {
                scanList->setViewMode(1);
            }
            else if (!value.compare("2") || !value.toLower().compare("activator")) {
                scanList->setViewMode(2);
            }
            else if (!value.compare("3") || !value.toLower().compare("inhibitor")) {
                scanList->setViewMode(3);
            }
            else if (!value.compare("4") || !value.toLower().compare("fgf")) {
                scanList->setViewMode(4);
            }
            else {
                scanList->setViewMode(0);
            }
        }
        else if (!key.toLower().compare("orientation")) {
            QStringList orientations = value.split(",");
            orientations.removeDuplicates();
            for (auto orient : orientations) {
                scanList->addOrientation( orient.trimmed().toStdString() );
            }
        }
        else {
            ScanItem *item = new ScanItem();

            item->setParName(key.toStdString());
            list = value.split(":");
            if (list.length() < 3) {
                fclose(input);
                delete item;
                return NULL;
            }
            item->setMinValue(list[0].trimmed().toDouble());
            item->setStep(list[1].trimmed().toDouble());
            item->setMaxValue(list[2].trimmed().toDouble());

            scanList->addScanItem(item);
            fprintf(stderr, "name: %s, %lf:%lf:%lf\n", item->getParName().c_str(),
                    item->getMinValue(), item->getStep(), item->getMaxValue());  // DEBUG
       }
    }

    #if defined(__linux__)
    setlocale(LC_ALL, oldloc);
    #endif

    fclose(input);
    return scanList;
}
