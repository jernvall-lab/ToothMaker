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
        if (list.size()>=2 && !list[0].isEmpty() && !list[1].isEmpty()) {
            if (par->isKeyword(list[0].toLower().toStdString())) {
                par->setKey(list[0].toLower().toStdString(), list[1].toStdString());
            }
            else {
                par->setParameterValue( list[0].toStdString(), list[1].toDouble() );
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

        if (!list[0].toLower().compare("model")) {
            // TODO: Implement reading model name from the scan list.
            // std::string model = list[1].toStdString();
        }
        else if (!list[0].toLower().compare("viewmode")) {
            if (!list[1].compare("BW") || !list[1].compare("1")
                || !list[1].toLower().compare("differentiation")) {
                scanList->setViewMode(1);
            }
            else if (!list[1].compare("2") || !list[1].toLower().compare("activator")) {
                scanList->setViewMode(2);
            }
            else if (!list[1].compare("3") || !list[1].toLower().compare("inhibitor")) {
                scanList->setViewMode(3);
            }
            else if (!list[1].compare("4") || !list[1].toLower().compare("fgf")) {
                scanList->setViewMode(4);
            }
            else {
                scanList->setViewMode(0);
            }
        }
        else if (!list[0].toLower().compare("orientation")) {
            QStringList orientations = list[1].split(",");
            orientations.removeDuplicates();
            for (auto orient : orientations) {
                scanList->addOrientation( orient.trimmed().toStdString() );
            }
        }
        else {
            ScanItem *item = new ScanItem();

            item->setParName(list[0].toStdString());
            list = list[1].split(":");
            if (list.length() < 3) {
                fclose(input);
                return NULL;
            }
            item->setMinValue(list[0].toDouble());
            item->setStep(list[1].toDouble());
            item->setMaxValue(list[2].toDouble());

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
