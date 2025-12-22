// tooth_model_core.cpp
// Constructor and initialization functions for ToothModel

#include "tooth_model.hpp"

//=============================================================================
// ToothModel Constructor and Core Initialization
//=============================================================================

ToothModel::ToothModel() :
    numCells(0), numCellsTotal(0), numZLevels(4), radius(0),
    timeStep(0), stepNumber(1),
    diffThresholdSet(0.0), diffThresholdInt(0.0),
    epithelialGrowthRate(0.0), mesenchymalGrowthRate(0.0), basalMesenchymalRate(0.0),
    activatorAutoActivation(0.0), ectodinRate(0.0), not3Rate(0.0),
    activatorInhibition(0.0), growthFactorSecretion(0.0), degradationRate(0.0),
    initialActivator(0.0),
    stiffness(0.0), neighborTraction(0.0), nucleusTraction(0.0), sharpnessMax(0.0),
    borderDistance(0.0), borderWidth(0.0),
    biasPosterior(0.0), biasAnterior(0.0), biasLingual(0.0), biasBuccal(0.0),
    biasCenterRadius(0.0), biasFactor(0.0),
    currentCellIndex(0), centerCellIndex(0), centerCell(0), numBorderCells(0),
    currX(0.0), currY(0.0), newX(0.0), newY(0.0),
    sinDir0(0.0), cosDir0(0.0), sinDir60(0.0), cosDir60(0.0),
    sinDir120(0.0), cosDir120(0.0), sinDir180(0.0), cosDir180(0.0),
    sinDir240(0.0), cosDir240(0.0), sinDir300(0.0), cosDir300(0.0),
    numNewCells(0), numAnteriorMarkers(0), numPosteriorMarkers(0),
    panicFlag(0.0),
    iterationIndex(0), totalIterations(0)
{
    diffusionCoeffs3D.resize(NUM_3D_QUANTITIES, 0.0);
    diffusionCoeffs2D.resize(NUM_2D_QUANTITIES, 0.0);
}

void ToothModel::initializeDefaults() {
    // Core values
    numZLevels = 4;
    timeStep = 0;
    stepNumber = 1;
    panicFlag = 0;

    diffusionCoeffs3D.resize(NUM_3D_QUANTITIES, 0.0);
    diffusionCoeffs2D.resize(NUM_2D_QUANTITIES, 0.0);
}

void ToothModel::reinitialize() {
    // Core values
    timeStep = 0;
    stepNumber = 1;
    panicFlag = 0;

    allocateAndInit();
}

void ToothModel::allocateAndInit() {
    // Calculate cell counts based on radius
    int j = 0;
    for (int i = 1; i <= radius; i++) {
        j += i;
    }
    numCellsTotal = 6 * j + 1;

    j = 0;
    for (int i = 1; i <= radius - 1; i++) {
        j += i;
    }
    numCells = 6 * j + 1;

    // Precompute sin/cos for hexagonal directions
    double angleStep = PI * 2.0 / 360.0;
    sinDir0   = std::sin(0 * angleStep);    cosDir0   = std::cos(0 * angleStep);
    sinDir60  = std::sin(60 * angleStep);   cosDir60  = std::cos(60 * angleStep);
    sinDir120 = std::sin(120 * angleStep);  cosDir120 = std::cos(120 * angleStep);
    sinDir180 = std::sin(180 * angleStep);  cosDir180 = std::cos(180 * angleStep);
    sinDir240 = std::sin(240 * angleStep);  cosDir240 = std::cos(240 * angleStep);
    sinDir300 = std::sin(300 * angleStep);  cosDir300 = std::cos(300 * angleStep);

    // Allocate all arrays
    cellPositions.resize(numCellsTotal, {0.0, 0.0, 0.0});
    neighbors.resize(numCellsTotal);
    for (auto& n : neighbors) n.fill(0);
    positionDeltas.resize(numCellsTotal, {0.0, 0.0, 0.0});

    cellMargins.resize(numCellsTotal);
    for (auto& m : cellMargins) {
        m.resize(MAX_NEIGHBORS);
        for (auto& arr : m) arr.fill(0.0);
    }

    knotMarkers.resize(numCellsTotal, 0);
    neighborCount.resize(numCellsTotal, 0);

    quantities2D.resize(numCellsTotal);
    for (auto& q : quantities2D) q.fill(0.0);

    quantities3D.resize(numCellsTotal);
    for (auto& cell : quantities3D) {
        cell.resize(numZLevels);
        for (auto& level : cell) level.fill(0.0);
    }

    posteriorMarkers.resize(radius, 0);
    anteriorMarkers.resize(radius, 0);

    // Initialize all arrays to zero (already done by resize)
    std::fill(neighborCount.begin(), neighborCount.end(), 0);

    // Initial values
    cellPositions[0] = {0.0, 0.0, 1.0};
    currentCellIndex = 1;
    std::fill(neighborCount.begin(), neighborCount.end(), 6);

    // Build initial grid by iterating through cells and placing neighbors
    // Temporary storage for building the grid
    std::vector<std::array<int, MAX_NEIGHBORS>> tempNeighbors(numCellsTotal);
    for (auto& n : tempNeighbors) n.fill(0);
    std::vector<std::array<double, 3>> tempPositions(numCellsTotal, {0.0, 0.0, 0.0});

    for (centerCellIndex = 0; centerCellIndex < numCells; centerCellIndex++) {
        currX = cellPositions[centerCellIndex][0];
        currY = cellPositions[centerCellIndex][1];

        // Place neighbors in all 6 hexagonal directions
        newX = currX + sinDir0 * BASE_DISTANCE;
        newY = currY + cosDir0 * BASE_DISTANCE;
        placeCell();

        newX = currX + sinDir60 * BASE_DISTANCE;
        newY = currY + cosDir60 * BASE_DISTANCE;
        placeCell();

        newX = currX + sinDir120 * BASE_DISTANCE;
        newY = currY + cosDir120 * BASE_DISTANCE;
        placeCell();

        newX = currX + sinDir180 * BASE_DISTANCE;
        newY = currY + cosDir180 * BASE_DISTANCE;
        placeCell();

        newX = currX + sinDir240 * BASE_DISTANCE;
        newY = currY + cosDir240 * BASE_DISTANCE;
        placeCell();

        newX = currX + sinDir300 * BASE_DISTANCE;
        newY = currY + cosDir300 * BASE_DISTANCE;
        placeCell();
    }

    // Mark neighbors beyond numCells as boundary (numCellsTotal)
    for (int i = 1; i < numCells; i++) {
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            if (neighbors[i][jIdx] > numCells) {
                neighbors[i][jIdx] = numCellsTotal;
            }
        }
    }

    // Remove duplicate boundary markers
    for (int k = 0; k < 3; k++) {
        for (int i = 1; i < numCells; i++) {
            for (int jIdx = 0; jIdx < MAX_NEIGHBORS - 1; jIdx++) {
                if (neighbors[i][jIdx] == numCellsTotal && neighbors[i][jIdx + 1] == numCellsTotal) {
                    for (int jj = jIdx; jj < MAX_NEIGHBORS - 1; jj++) {
                        neighbors[i][jj] = neighbors[i][jj + 1];
                    }
                }
            }
        }
    }

    // Remove trailing boundary markers after the first one
    for (int i = 1; i < numCells; i++) {
        int foundFirst = 0;
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            if (neighbors[i][jIdx] == numCellsTotal && foundFirst == 0) {
                foundFirst = 1;
                continue;
            }
            if (neighbors[i][jIdx] == numCellsTotal && foundFirst == 1) {
                neighbors[i][jIdx] = 0;
                break;
            }
        }
    }

    // Round positions to avoid floating point issues
    for (int i = 0; i < numCells; i++) {
        for (int jIdx = 0; jIdx < 3; jIdx++) {
            cellPositions[i][jIdx] = std::round(cellPositions[i][jIdx] * 1e14) * 1e-14;
            if (std::abs(cellPositions[i][jIdx]) < 1e-14) {
                cellPositions[i][jIdx] = 0.0;
            }
        }
    }

    // Invert order so border cells come first
    std::vector<std::array<int, MAX_NEIGHBORS>> cvei = neighbors;
    std::vector<std::array<double, 3>> cmalla = cellPositions;

    for (int i = numCells - 1; i >= 0; i--) {
        neighbors[i] = cvei[numCells - i - 1];
        cellPositions[i] = cmalla[numCells - i - 1];
    }

    // Update neighbor references after inversion
    cvei = neighbors;
    for (int i = numCells - 1; i >= 0; i--) {
        int ii = numCells - i - 1;
        for (int jj = 0; jj < numCells; jj++) {
            for (int jjj = 0; jjj < MAX_NEIGHBORS; jjj++) {
                if (cvei[jj][jjj] == i + 1) {  // Fortran 1-based to 0-based adjustment
                    neighbors[jj][jjj] = ii + 1;
                }
            }
        }
    }

    calculateMargins();
    std::fill(neighborCount.begin(), neighborCount.end(), 3);
    neighborCount[0] = 6;

    // Initialize margin distances
    for (int i = 0; i < numCells; i++) {
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            cellMargins[i][jIdx][3] = BASE_DISTANCE;
            cellMargins[i][jIdx][4] = BASE_DISTANCE;
        }
    }

    centerCell = numCells;
    // In Fortran, ncils = (radi-1)*6+1 gives the 1-based index of the first interior cell
    // In C++ with 0-based indexing, we need the count of border cells = ncils - 1
    numBorderCells = (radius - 1) * 6;  // ncils-1 in 0-based (Fortran ncils=(radi-1)*6+1)

    // Set up anterior and posterior marker arrays
    std::fill(anteriorMarkers.begin(), anteriorMarkers.end(), 0);
    std::fill(posteriorMarkers.begin(), posteriorMarkers.end(), 0);

    for (int i = 0; i < radius; i++) {
        posteriorMarkers[i] = i + 1;
    }

    int ii = 0;
    for (int i = numBorderCells / 2; i < numBorderCells / 2 + radius; i++) {
        anteriorMarkers[ii] = i + 1;
        ii++;
    }

    numAnteriorMarkers = radius;
    numPosteriorMarkers = radius;

    calculateMargins();

    // Initialize quantities to zero
    for (auto& cell : quantities3D) {
        for (auto& level : cell) {
            level.fill(0.0);
        }
    }
}

void ToothModel::reallocate() {
    // Not commonly used - reallocate matrices with current sizes
    cellPositions.resize(numCellsTotal, {0.0, 0.0, 0.0});
    neighbors.resize(numCellsTotal);
    for (auto& n : neighbors) n.fill(0);
    positionDeltas.resize(numCellsTotal, {0.0, 0.0, 0.0});

    cellMargins.resize(numCellsTotal);
    for (auto& m : cellMargins) {
        m.resize(MAX_NEIGHBORS);
        for (auto& arr : m) arr.fill(0.0);
    }

    knotMarkers.resize(numCellsTotal, 0);
    neighborCount.resize(numCellsTotal, 0);

    quantities2D.resize(numCellsTotal);
    for (auto& q : quantities2D) q.fill(0.0);

    quantities3D.resize(numCellsTotal);
    for (auto& cell : quantities3D) {
        cell.resize(numZLevels);
        for (auto& level : cell) level.fill(0.0);
    }

    posteriorMarkers.resize(radius, 0);
    anteriorMarkers.resize(radius, 0);

    cellPositions[0] = {0.0, 0.0, 1.0};
    currentCellIndex = 1;
}

void ToothModel::reset() {
    deallocateAll();
    reinitialize();
}

void ToothModel::deallocateAll() {
    cellPositions.clear();
    quantities3D.clear();
    quantities2D.clear();
    cellMargins.clear();
    neighbors.clear();
    neighborCount.clear();
    positionDeltas.clear();
    knotMarkers.clear();
    anteriorMarkers.clear();
    posteriorMarkers.clear();
}

void ToothModel::placeCell() {
    // Check if a cell already exists at the new position
    for (int i = 0; i < currentCellIndex; i++) {
        if (i == centerCellIndex) continue;

        // Compare positions with tolerance
        if (std::round(1000000 * cellPositions[i][0]) == std::round(1000000 * newX) &&
            std::round(1000000 * cellPositions[i][1]) == std::round(1000000 * newY)) {

            // Cell already exists - check if neighbor connection exists
            for (int ii = 0; ii < MAX_NEIGHBORS; ii++) {
                if (neighbors[centerCellIndex][ii] == i + 1) {  // Fortran 1-based indexing
                    return;  // Already connected
                }
            }

            // Find the direction slot we're filling (j is the direction)
            // This needs the direction variables that were set before calling placeCell
            // For now, find first empty slot
            int j = 0;
            for (j = 0; j < MAX_NEIGHBORS; j++) {
                if (neighbors[centerCellIndex][j] == 0) break;
            }
            int jj = (j + 3) % 6;  // Opposite direction

            neighbors[centerCellIndex][j] = i + 1;
            neighbors[i][jj] = centerCellIndex + 1;
            neighborCount[i]++;
            neighborCount[centerCellIndex]++;
            return;
        }
    }

    // Cell doesn't exist - create new one
    neighborCount[centerCellIndex]++;
    currentCellIndex++;

    int j = 0;
    for (j = 0; j < MAX_NEIGHBORS; j++) {
        if (neighbors[centerCellIndex][j] == 0) break;
    }
    int jj = (j + 3) % 6;

    neighbors[centerCellIndex][j] = currentCellIndex;
    neighbors[currentCellIndex - 1][jj] = centerCellIndex + 1;
    cellPositions[currentCellIndex - 1][0] = newX;
    cellPositions[currentCellIndex - 1][1] = newY;
    cellPositions[currentCellIndex - 1][2] = 1.0;
    neighborCount[currentCellIndex - 1]++;
}
