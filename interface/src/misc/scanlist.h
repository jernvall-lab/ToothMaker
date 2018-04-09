#pragma once

#include <string>
#include <vector>
#include <stdio.h>
#include <sstream>
#include <cmath>
#include <string.h>
#include "parameters.h"
#include "morphomaker.h"



class ScanItem;

class ScanList
{
    public:
        ScanList();

        // Removes scan items from the scan list.
        void resetScanQueue();

        // Full reset of the scanner.
        void reset();

        // Adds, removes, gets a scan item.
        void addScanItem( ScanItem* item );
        void removeScanItem( std::string parName );
        Parameters *getScanItem( int i );

        // Returns the index of the current scan item.
        int getCurrentScanItem()                    { return currentScanItem; }

        // Set model view mode.
        void setViewMode( int mode )                { viewMode = mode; }
        int getViewMode()                           { return viewMode; }

        // Returns the number of scan items in the queue.
        int getScanQueueSize()                      { return scanQueue.size(); }

        // Add a model view orientation for rendering output.
        void addOrientation( std::string name )     { orientations.push_back(name); }
        std::vector<std::string>& getOrientations() { return orientations; }

        // Sets the base model parameters that are varied during scanning.
        void setBaseParameters( Parameters *par )   { baseParameters = new Parameters(par); }

        // Gets a set of parameters next in the scan queue.
        Parameters *getNextScanJob();

        // Gives the next permutation of a set of numbers defined by their max. values.
        void updatePerm(std::vector<int> *, std::vector<int> *, int calcPerm=1);

        // Returns the number of jobs given the current scan items.
        unsigned long getNofJobs(int);

        // Populates the scan queue based on a user-defined scan list.
        int populateScanQueue(std::string, int calcPerm=1);



    private:
        std::vector<ScanItem*> scanItems;
        std::vector<std::string> itemNames;
        std::vector<double> itemValues;
        std::vector<Parameters*> scanQueue;
        int currentScanItem;
        Parameters *baseParameters;
        int viewMode;
        std::vector<std::string> orientations;
};


class ScanItem
{
    public:
        ScanItem()      {}
        ~ScanItem()     {}

        void setParName( std::string s )    { parName = s; }
        void setMinValue( double val )      { minValue = val; }
        void setMaxValue( double val )      { maxValue = val; }
        void setStep( double val )          { step = val; }
        std::string getParName()            { return parName; }
        double getMinValue()                { return minValue; }
        double getMaxValue()                { return maxValue; }
        double getStep()                    { return step; }


    private:
        // Name of the parameter whose values are scanned.
        std::string parName;

        // Parameter values to be scanned; values in range [minValue, maxValue]
        // with increments of step.
        double minValue;
        double maxValue;
        double step;
};
