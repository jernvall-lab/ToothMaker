/**
 * @class ScanWindow
 * @brief Parameter scanning window.
 */

#include "misc/scanlist.h"


ScanList::ScanList()
{
    currentScanItem = 0;
    viewMode = 0;
    baseParameters = NULL;
}



/**
 * @brief Removes scan items from the scan list.
 */
void ScanList::resetScanQueue()
{
    currentScanItem=0;
    scanQueue.clear();
}



/**
 * @brief Removes all items in scan list and sets it init. state.
 */
void ScanList::reset()
{
    scanItems.clear();
    itemNames.clear();
    itemValues.clear();
    resetScanQueue();
    currentScanItem=0;
    if (baseParameters!=NULL) {
        delete baseParameters;
        baseParameters = NULL;
    }
    viewMode = 0;
    orientations.clear();
}



/**
 * @brief Adds a scan item to scan list.
 *        If a scan item with the same parameter name already exists, replaces
 *        the old item with the new one.
 * @param item      Item to add.
 */
void ScanList::addScanItem(ScanItem *item)
{
    removeScanItem(item->getParName());
    scanItems.push_back(item);
}



/**
 * @brief Removes scan item from the scan items vector.
 * @param item      Item to remove.
 */
void ScanList::removeScanItem(std::string parName)
{
    unsigned int i;

    for (i=0; i<scanItems.size(); i++) {
        if (!parName.compare(scanItems.at(i)->getParName())) {
            scanItems.erase(scanItems.begin()+i);
        }
    }
}



/**
 * @brief Gets a set of parameters by index from scan queue.
 * @param i     Item index.
 * @return      Parameters object.
 */
Parameters *ScanList::getScanItem(int i)
{
    if (scanQueue.size() <= (unsigned int)i) return NULL;
    return scanQueue.at(i);
}



/**
 * @brief Gets a set of parameters next in the scan queue.
 * @return      Parameters object.
 */
Parameters *ScanList::getNextScanJob()
{
    if (scanQueue.size() <= (unsigned int)currentScanItem) return NULL;
    currentScanItem++;
    return scanQueue.at(currentScanItem-1);
}



/**
 * @brief Gives the next permutation of a set of numbers defined by their max. values.
 * @param curr      Current permutation.
 * @param max       Sizes of sets.
 * @param calcPerm  If 1 calculates all permutations.
 */
void ScanList::updatePerm(std::vector<int> *curr, std::vector<int> *max, int calcPerm)
{
    if (calcPerm) {
        for (uint32_t i=0; i<curr->size(); i++) {
            if (curr->at(i) < max->at(i)-1) {
                curr->at(i)++;
                auto j = i;
                while (j > 0) {
                    curr->at(j-1) = 0;
                    j--;
                }
                break;
            }
        }
    }

    else {
        for (uint32_t i=0; i<curr->size(); i++) {
            if (curr->at(i)==max->at(i)-1 && i<curr->size()-1) {
                curr->at(i)=-1;
                curr->at(i+1)=0;
                break;
            }
            if (curr->at(i)>-1) {
                curr->at(i)++;
                break;
            }
        }
    }
}



/**
 * @brief Returns the number of jobs given the current scan items.
 * @param comb      If 1, calculates all combinations.
 */
unsigned long ScanList::getNofJobs(int comb)
{
    if (scanItems.size() == 0) {
        return 0;
    }

    unsigned long nperm=0;
    if (comb==1) {
        nperm=1;
    }

    for (uint32_t i=0; i<scanItems.size(); i++) {
        ScanItem* item = scanItems.at(i);
        auto div = item->getStep();
        if (div==0.0) {
            div=1.0;
        }
        auto n = lround((item->getMaxValue() - item->getMinValue())/div + 1.0);
        if (comb==1) {
            nperm = nperm*n;
        }
        else {
            nperm = nperm+n;
        }
    }

    return nperm;
}


/**
 * @brief Populates the scan queue based on a user-defined scan list.
 * - Linear and permutation scanning are treated separately.
 *
 * @param parlist   File to write queue/scanning info.
 * @param calcPerm  If 1 calculates all parameter combinations.
 */
int ScanList::populateScanQueue(std::string parlist, int calcPerm)
{
    FILE* output = fopen(parlist.c_str(), "w");
    if (output==NULL) {
        fprintf(stderr, "Error: Can't open file '%s' for writing. Aborting.\n",
                parlist.c_str());
        return -1;
    }

    std::vector<int> nSteps;
    std::vector<int> currSteps;

    // Init the step list. For non-permutations -1 indicates base value.
    for (uint32_t i=0; i<scanItems.size(); i++) {
        ScanItem* item = scanItems.at(i);
        auto n = lround((item->getMaxValue() - item->getMinValue())/item->getStep() + 1.0);
        nSteps.push_back(n);
        if (calcPerm) {
            currSteps.push_back(0);
        }
        else {
            currSteps.push_back(-1);
        }
    }
    if (!calcPerm) {
        currSteps.at(0)=0;
    }

    long nperm = getNofJobs(calcPerm);
    fprintf(stderr, "Number of jobs generated: %ld\n", nperm);
    if (nperm>100000) {
        fprintf(stderr, "Congratulations! Chances are you'll be waiting for an eternity while I work on these!\n");
    }

    long done=0;

    while (done<nperm) {
        fprintf(output, "i:%ld --- ", done);
        std::stringstream id;

        for (uint32_t i=0; i<currSteps.size(); i++) {
            if (currSteps.at(i)==-1) {
                fprintf(output, "X ");
                id << "X";
            }
            else {
                fprintf(output, "%d ", currSteps.at(i));
                id << currSteps.at(i);
            }
        }
        fprintf(output, "\n");
        Parameters *par = new Parameters(baseParameters);
        par->setID(id.str());
        for (uint32_t j=0; j<scanItems.size(); j++) {
            if (!calcPerm && currSteps.at(j)==-1) {
                continue;
            }
            ScanItem* item = scanItems.at(j);

            std::string name = item->getParName();
            double value = currSteps.at(j)*item->getStep() + item->getMinValue();
            fprintf( output, "par: %s, val: %f\n", name.c_str(), value );
            par->setParameterValue( name, value );

            if (!calcPerm && currSteps.at(j)>-1) {
                break;
            }
        }
        scanQueue.push_back(par);

        updatePerm(&currSteps, &nSteps, calcPerm);
        fprintf(output, "\n");
        done++;
    }
    fprintf(output, "\n");
    fclose(output);

    return 0;
}
