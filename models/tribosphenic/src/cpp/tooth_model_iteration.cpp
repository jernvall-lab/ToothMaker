// tooth_model_iteration.cpp
// Main simulation iteration loop

#include "tooth_model.hpp"

void ToothModel::runIteration(int numSteps) {
    // Open progress file once for the entire iteration batch
    std::ofstream progressFile;
    if (!progressFilename.empty()) {
        progressFile.open(progressFilename, std::ios::app);
    }

    for (int ite = 0; ite < numSteps; ite++) {
        panicFlag = false;

        reactionDiffusion();
        if (panicFlag) return;   // numerical overflow; a failed division does not stop the run

        applyBuccalLingualBias();
        updateDifferentiation();
        calculateGrowthPushing();
        calculateBuoyancy();
        checkNonNeighborRepulsion();
        calculateNeighborRepulsion();
        calculateNucleusTraction();
        updatePositions();
        addNewCells();
        calculateMargins();

        timeStep++;

        // Write progress
        if (progressFile.is_open()) {
            progressFile << (totalIterations * (iterationIndex - 1) + timeStep) << "\n";
        }
    }
}
