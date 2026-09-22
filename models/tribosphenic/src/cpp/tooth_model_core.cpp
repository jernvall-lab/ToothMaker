// tooth_model_core.cpp
// Constructor and initialization functions for ToothModel

#include "tooth_model.hpp"

//=============================================================================
// ToothModel Constructor and Core Initialization
//=============================================================================

ToothModel::ToothModel() :
    numCells(0), numCellsTotal(0), numBorderCells(0), numZLevels(NUM_Z_LEVELS), radius(0),
    timeStep(0),
    diffThresholdSet(0.0), diffThresholdInt(0.0),
    epithelialGrowthRate(0.0), mesenchymalGrowthRate(0.0), basalMesenchymalRate(0.0),
    activatorAutoActivation(0.0), ectodinRate(0.0), not3Rate(0.0),
    activatorInhibition(0.0), growthFactorSecretion(0.0), degradationRate(0.0),
    initialActivator(0.0),
    stiffness(0.0), neighborTraction(0.0), nucleusTraction(0.0), sharpnessMax(0.0),
    borderDistance(0.0), borderWidth(0.0),
    biasPosterior(0.0), biasAnterior(0.0), biasLingual(0.0), biasBuccal(0.0),
    biasCenterRadius(0.0), biasFactor(0.0),
    panicFlag(false),
    iterationIndex(0), totalIterations(0)
{
    diffusionCoeffs3D.resize(NUM_3D_QUANTITIES, 0.0);
    diffusionCoeffs2D.resize(NUM_2D_QUANTITIES, 0.0);
}

void ToothModel::initializeDefaults() {
    numZLevels = NUM_Z_LEVELS;
    timeStep = 0;
    panicFlag = false;

    diffusionCoeffs3D.resize(NUM_3D_QUANTITIES, 0.0);
    diffusionCoeffs2D.resize(NUM_2D_QUANTITIES, 0.0);
}

void ToothModel::allocateAndInit() {
    // Cell counts of a hexagonal grid of the given radius: 1 + 6*(1+2+...+r).
    // The real cells fill radius-1 rings; the outermost ring is the virtual
    // boundary, which every neighbour list refers to by the single index
    // numCellsTotal.
    int j = 0;
    for (int i = 1; i <= radius; i++) j += i;
    numCellsTotal = 6 * j + 1;
    j = 0;
    for (int i = 1; i <= radius - 1; i++) j += i;
    numCells = 6 * j + 1;

    cellPositions.assign(numCellsTotal, {0.0, 0.0, 0.0});
    neighbors.assign(numCellsTotal, std::array<int, MAX_NEIGHBORS>{});
    knotMarkers.assign(numCellsTotal, 0);
    cellMargins.assign(numCellsTotal, CellMargins{});
    quantities2D.assign(numCellsTotal, std::array<double, NUM_2D_QUANTITIES>{});
    quantities3D.assign(numCellsTotal,
        std::vector<std::array<double, NUM_3D_QUANTITIES>>(numZLevels, std::array<double, NUM_3D_QUANTITIES>{}));
    positionDeltas.assign(numCellsTotal, {0.0, 0.0, 0.0});

    // Build the grid: from every real cell, place a neighbour at unit distance in
    // each of the six hexagonal directions (0, 60, ..., 300 degrees).
    const double angleStep = PI * 2.0 / 360.0;
    double dirSin[6], dirCos[6];
    for (int k = 0; k < 6; k++) {
        dirSin[k] = std::sin((60 * k) * angleStep);
        dirCos[k] = std::cos((60 * k) * angleStep);
    }
    cellPositions[0] = {0.0, 0.0, 1.0};
    int numPlaced = 1;
    for (int center = 0; center < numCells; center++) {
        const double cx = cellPositions[center][0];
        const double cy = cellPositions[center][1];
        for (int k = 0; k < 6; k++) {
            placeCell(center, cx + dirSin[k] * BASE_DISTANCE, cy + dirCos[k] * BASE_DISTANCE, numPlaced);
        }
    }

    // Neighbours beyond the real cells are the boundary. Where the construction
    // left a cell with adjacent boundary entries, or with more than one in total,
    // keep a single one (exactly the Fortran's three passes).
    for (int i = 1; i < numCells; i++) {
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            if (neighbors[i][jIdx] > numCells) neighbors[i][jIdx] = numCellsTotal;
        }
    }
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
    for (int i = 1; i < numCells; i++) {
        bool foundFirst = false;
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            if (neighbors[i][jIdx] == numCellsTotal) {
                if (foundFirst) {
                    neighbors[i][jIdx] = 0;
                    break;
                }
                foundFirst = true;
            }
        }
    }

    // Snap the positions to 14 decimals so that symmetric cells are exactly symmetric
    for (int i = 0; i < numCells; i++) {
        for (int jIdx = 0; jIdx < 3; jIdx++) {
            cellPositions[i][jIdx] = std::round(cellPositions[i][jIdx] * 1e14) * 1e-14;
            if (std::abs(cellPositions[i][jIdx]) < 1e-14) cellPositions[i][jIdx] = 0.0;
        }
    }

    // Reverse the cell order so that the border cells come first, and renumber
    // the neighbour lists to match (boundary entries stay as they are).
    std::reverse(neighbors.begin(), neighbors.begin() + numCells);
    std::reverse(cellPositions.begin(), cellPositions.begin() + numCells);
    for (int i = 0; i < numCells; i++) {
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            int v = neighbors[i][jIdx];
            if (v >= 1 && v <= numCells) neighbors[i][jIdx] = numCells - v + 1;
        }
    }

    marginWalksValid = false;
    topologyVersion++;
    calculateMargins();
    for (int i = 0; i < numCells; i++) {
        for (int jIdx = 0; jIdx < MAX_NEIGHBORS; jIdx++) {
            cellMargins[i][jIdx][MARGIN_REST] = BASE_DISTANCE;
        }
    }

    // Fortran ncils = (radi-1)*6+1 is the 1-based index of the first interior
    // cell; the border cells are the indices below it.
    numBorderCells = (radius - 1) * 6;
}

void ToothModel::placeCell(int center, double x, double y, int& numPlaced) {
    auto firstFreeSlot = [this](int cell) {
        int j = 0;
        while (j < MAX_NEIGHBORS && neighbors[cell][j] != 0) j++;
        return j;
    };

    // Is there a cell at (x, y) already? Compare at 1e-6 to absorb rounding.
    for (int i = 0; i < numPlaced; i++) {
        if (i == center) continue;
        if (std::round(1000000 * cellPositions[i][0]) == std::round(1000000 * x) &&
            std::round(1000000 * cellPositions[i][1]) == std::round(1000000 * y)) {
            for (int ii = 0; ii < MAX_NEIGHBORS; ii++) {
                if (neighbors[center][ii] == i + 1) return;   // already connected
            }
            int j = firstFreeSlot(center);
            neighbors[center][j] = i + 1;
            neighbors[i][(j + 3) % 6] = center + 1;          // opposite direction
            return;
        }
    }

    // No: create it
    numPlaced++;
    int j = firstFreeSlot(center);
    neighbors[center][j] = numPlaced;
    neighbors[numPlaced - 1][(j + 3) % 6] = center + 1;
    cellPositions[numPlaced - 1] = {x, y, 1.0};
}
