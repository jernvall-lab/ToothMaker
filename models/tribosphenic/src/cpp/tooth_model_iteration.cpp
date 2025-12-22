// tooth_model_iteration.cpp
// Main simulation iteration loop

#include "tooth_model.hpp"

void ToothModel::runIteration(int numSteps) {
    for (int ite = 0; ite < numSteps; ite++) {
        panicFlag = 0;

        // Reset position deltas
        for (auto& h : positionDeltas) h.fill(0.0);

        reactionDiffusion();
        if (panicFlag == 1) return;

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

        // Write progress to file
        std::ofstream progressFile(progressFilename, std::ios::app);
        if (progressFile.is_open()) {
            progressFile << (totalIterations * (iterationIndex - 1) + timeStep) << std::endl;
            progressFile.close();
        }
    }
}
