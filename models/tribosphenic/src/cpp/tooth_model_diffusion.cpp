// tooth_model_diffusion.cpp
// Reaction-diffusion and differentiation functions

#include "tooth_model.hpp"

void ToothModel::reactionDiffusion() {
    // Ensure working arrays are properly sized (only reallocates if size changed)
    if (static_cast<int>(rdWeight.size()) < numCellsTotal) {
        rdWeight.resize(numCellsTotal);
        rdAreaProj.resize(numCellsTotal);
        rdDeltaQ3D.resize(numCellsTotal);
        for (auto& cell : rdDeltaQ3D) {
            cell.resize(numZLevels);
        }
    }

    // Only the middle z-levels are accumulated with +=; kk=0 and kk=numZLevels-1
    // are assigned with = further down, and rdWeight/rdAreaProj are cleared
    // per-cell inside the loop below. Cells >= numCells are never read.
    for (int i = 0; i < numCells; i++) {
        for (int kk = 1; kk < numZLevels - 1; kk++) {
            for (int k = 0; k < 4; k++) rdDeltaQ3D[i][kk][k] = 0.0;
        }
    }

    // Calculate diffusion weights based on cell geometry
    for (int i = 0; i < numCells; i++) {
        rdWeight[i].fill(0.0);
        rdAreaProj[i].fill(0.0);

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] != 0) {
                double ua = cellPositions[i][0];
                double ub = cellPositions[i][1];
                double uc = cellPositions[i][2];

                // Find next non-zero neighbor for edge calculation
                for (int jj = j + 1; jj < MAX_NEIGHBORS; jj++) {
                    if (neighbors[i][jj] != 0) {
                        // Edge length between margin points
                        rdWeight[i][j] = std::sqrt(
                            std::pow(cellMargins[i][j][0] - cellMargins[i][jj][0], 2) +
                            std::pow(cellMargins[i][j][1] - cellMargins[i][jj][1], 2) +
                            std::pow(cellMargins[i][j][2] - cellMargins[i][jj][2], 2));

                        // Triangle area calculation using cross product
                        double ux = cellMargins[i][j][0] - ua;
                        double uy = cellMargins[i][j][1] - ub;
                        double uz = cellMargins[i][j][2] - uc;
                        double dx = cellMargins[i][jj][0] - ua;
                        double dy = cellMargins[i][jj][1] - ub;
                        double dz = cellMargins[i][jj][2] - uc;

                        rdAreaProj[i][j] = 0.5 * std::sqrt(  // Fortran: 0.05D1 = 0.5
                            std::pow(uy * dz - uz * dy, 2) +
                            std::pow(uz * dx - ux * dz, 2) +
                            std::pow(ux * dy - uy * dx, 2));
                        goto next_neighbor;
                    }
                }

                // Wrap around to first neighbor if no next found
                rdWeight[i][j] = std::sqrt(
                    std::pow(cellMargins[i][j][0] - cellMargins[i][0][0], 2) +
                    std::pow(cellMargins[i][j][1] - cellMargins[i][0][1], 2) +
                    std::pow(cellMargins[i][j][2] - cellMargins[i][0][2], 2));

                double ux = cellMargins[i][j][0] - cellPositions[i][0];
                double uy = cellMargins[i][j][1] - cellPositions[i][1];
                double uz = cellMargins[i][j][2] - cellPositions[i][2];
                double dx = cellMargins[i][0][0] - cellPositions[i][0];
                double dy = cellMargins[i][0][1] - cellPositions[i][1];
                double dz = cellMargins[i][0][2] - cellPositions[i][2];

                rdAreaProj[i][j] = 0.5 * std::sqrt(  // Fortran: 0.05D1 = 0.5
                    std::pow(uy * dz - uz * dy, 2) +
                    std::pow(uz * dx - ux * dz, 2) +
                    std::pow(ux * dy - uy * dx, 2));
            }
        next_neighbor:;
        }

        // Normalize weights
        double areaBelow = 0.0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            areaBelow += rdAreaProj[i][j];
        }

        double suma = 0.0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            suma += rdWeight[i][j];
        }
        suma += 2 * areaBelow;
        double originalSuma = suma;  // Save for later restoration

        if (suma > 0) {
            areaBelow /= suma;
            for (int j = 0; j < MAX_NEIGHBORS; j++) {
                rdWeight[i][j] /= suma;
            }
        }

        // Compact list of occupied neighbour slots. The slot list does not
        // change across k/kk, so hoist it out of the loop nest. Slots with
        // neighbors[i][j]==0 contribute nothing, so skipping them is exact.
        int slotJ[MAX_NEIGHBORS];
        int slotII[MAX_NEIGHBORS];
        int nSlots = 0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] != 0) {
                slotJ[nSlots] = j;
                slotII[nSlots] = neighbors[i][j] - 1;
                nSlots++;
            }
        }
        const int lastZ = numZLevels - 1;

        // Vertical (areaBelow) terms first, exactly as before: for every
        // (kk,k) accumulator these are the first one or two additions.
        for (int k = 0; k < 4; k++) {
            for (int kk = 1; kk < lastZ; kk++) {
                rdDeltaQ3D[i][kk][k] += areaBelow * (quantities3D[i][kk - 1][k] - quantities3D[i][kk][k]);
                rdDeltaQ3D[i][kk][k] += areaBelow * (quantities3D[i][kk + 1][k] - quantities3D[i][kk][k]);
            }
            rdDeltaQ3D[i][lastZ][k]  = areaBelow * (-quantities3D[i][lastZ][k] * 0.44);
            rdDeltaQ3D[i][lastZ][k] += areaBelow * (quantities3D[i][lastZ - 1][k] - quantities3D[i][lastZ][k]);
        }

        // Neighbour terms, j ascending -- same per-accumulator order as before.
        for (int s = 0; s < nSlots; s++) {
            const int j  = slotJ[s];
            const int ii = slotII[s];
            const double w = rdWeight[i][j];
            if (ii == numCellsTotal - 1) {
                for (int kk = 1; kk < lastZ; kk++)
                    for (int k = 0; k < 4; k++)
                        rdDeltaQ3D[i][kk][k] += w * (-quantities3D[i][kk][k] * 0.44);
                for (int k = 0; k < 4; k++)
                    rdDeltaQ3D[i][lastZ][k] += w * (-quantities3D[i][lastZ][k] * 0.44);
            } else {
                for (int kk = 1; kk < lastZ; kk++)
                    for (int k = 0; k < 4; k++)
                        rdDeltaQ3D[i][kk][k] += w * (quantities3D[ii][kk][k] - quantities3D[i][kk][k]);
                for (int k = 0; k < 4; k++)
                    rdDeltaQ3D[i][lastZ][k] += w * (quantities3D[ii][lastZ][k] - quantities3D[i][lastZ][k]);
            }
        }

        // Re-normalize for top layer (different boundary condition)
        // Fortran: pes(i,:)=pes(i,:)*suma ; areasota=areasota*suma ; suma=suma-areasota
        // First restore original weights, then re-normalize excluding areaBelow
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            rdWeight[i][j] *= originalSuma;  // Restore original weights
        }
        areaBelow *= originalSuma;  // Restore original areaBelow
        suma = originalSuma - areaBelow;  // New normalization factor
        if (suma > 0) {
            for (int j = 0; j < MAX_NEIGHBORS; j++) {
                rdWeight[i][j] /= suma;
            }
            areaBelow /= suma;
        }

        // Top z level (index 0)
        for (int k = 0; k < 4; k++) {
            rdDeltaQ3D[i][0][k] = areaBelow * (quantities3D[i][1][k] - quantities3D[i][0][k]);
        }
        for (int s = 0; s < nSlots; s++) {
            const int j  = slotJ[s];
            const int ii = slotII[s];
            const double w = rdWeight[i][j];
            if (ii == numCellsTotal - 1) {
                for (int k = 0; k < 4; k++)
                    rdDeltaQ3D[i][0][k] += w * (-quantities3D[i][0][k] * 0.44);
            } else {
                for (int k = 0; k < 4; k++)
                    rdDeltaQ3D[i][0][k] += w * (quantities3D[ii][0][k] - quantities3D[i][0][k]);
            }
        }
    }

    // Apply diffusion updates
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < numCells; i++) {
            for (int kk = 0; kk < numZLevels; kk++) {
                quantities3D[i][kk][k] += TIME_DELTA * diffusionCoeffs3D[k] * rdDeltaQ3D[i][kk][k];
            }
        }
    }

    // REACTION equations - clear deltas for reaction phase
    for (int i = 0; i < numCells; i++) {
        for (int k = 0; k < 4; k++) rdDeltaQ3D[i][0][k] = 0.0;
    }

    for (int i = 0; i < numCells; i++) {
        // Mark knots based on activator concentration
        if (quantities3D[i][0][0] > 1.0) {
            if (i >= numBorderCells) {
                knotMarkers[i] = 1;
            }
        }

        // Equation (14): Activator dynamics
        double a = activatorAutoActivation * quantities3D[i][0][0] - quantities3D[i][0][3];
        if (a < 0) a = 0.0;
        rdDeltaQ3D[i][0][0] = a / (1 + activatorInhibition * quantities3D[i][0][1]) -
                             degradationRate * quantities3D[i][0][0];

        // Equation (17): Inhibitor dynamics
        if (quantities2D[i][0] > diffThresholdInt) {
            rdDeltaQ3D[i][0][1] = quantities3D[i][0][0] * quantities2D[i][0] -
                                 degradationRate * quantities3D[i][0][1];
        } else {
            if (knotMarkers[i] == 1) {
                rdDeltaQ3D[i][0][1] = quantities3D[i][0][0] - degradationRate * quantities3D[i][0][1];
            }
        }

        // Equation (18): Growth factor (FGF) dynamics
        if (quantities2D[i][0] > diffThresholdSet) {
            a = growthFactorSecretion * quantities2D[i][0] - degradationRate * quantities3D[i][0][2];
            if (a < 0.0) a = 0.0;
            rdDeltaQ3D[i][0][2] = a;
        } else {
            if (knotMarkers[i] > diffThresholdSet) {
                a = growthFactorSecretion - degradationRate * quantities3D[i][0][2];
                if (a < 0.0) a = 0.0;
                rdDeltaQ3D[i][0][2] = a;
            }
        }

        // Ectodin dynamics (Not2, Not3)
        a = ectodinRate * quantities3D[i][0][0] - degradationRate * quantities3D[i][0][3] -
            not3Rate * quantities3D[i][0][2];
        if (a < 0.0) a = 0.0;
        rdDeltaQ3D[i][0][3] = a;
    }

    // Check for overflow
    double maxDelta = 0.0;
    for (int i = 0; i < numCells; i++) {
        maxDelta = std::max(maxDelta, std::abs(rdDeltaQ3D[i][0][0]));
        maxDelta = std::max(maxDelta, std::abs(rdDeltaQ3D[i][0][1]));
    }

    if (maxDelta > 1e100) {
        std::cerr << "PANIC: numerical overflow, maxDelta=" << maxDelta << std::endl;
        panicFlag = 1;
        return;
    }

    // Apply reaction updates (explicit Euler)
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < numCells; i++) {
            quantities3D[i][0][k] += TIME_DELTA * rdDeltaQ3D[i][0][k];
        }
    }

    // Clamp negative values to zero
    for (int i = 0; i < numCells; i++) {
        for (int kk = 0; kk < numZLevels; kk++) {
            for (int k = 0; k < NUM_3D_QUANTITIES; k++) {
                if (quantities3D[i][kk][k] < 0.0) {
                    quantities3D[i][kk][k] = 0.0;
                }
            }
        }
    }
}

void ToothModel::updateDifferentiation() {
    for (int i = 0; i < numCells; i++) {
        // borderWidth is GUI "Dff" (differentiation rate) - controls response to growth factor
        // NOTE: Variable name is misleading; this is NOT border width but differentiation rate!
        quantities2D[i][0] += borderWidth * quantities3D[i][0][2];
        if (quantities2D[i][0] > 1.0) {
            quantities2D[i][0] = 1.0;
        }
    }
}

void ToothModel::applyBuccalLingualBias() {
    for (int i = 0; i < numBorderCells; i++) {
        if (cellPositions[i][1] < -borderDistance) {
            // Lingual side - won't reset activator if already higher than bias
            if (quantities3D[i][0][0] < biasLingual) {
                quantities3D[i][0][0] = biasLingual;
            }
        } else if (cellPositions[i][1] > borderDistance) {
            // Buccal side
            if (quantities3D[i][0][0] < biasBuccal) {
                quantities3D[i][0][0] = biasBuccal;
            }
        }
    }
}

void ToothModel::initializeActivator() {
    // Add initial activator concentration in each cell
    for (int i = 0; i < numCells; i++) {
        quantities3D[i][0][0] = initialActivator;
    }
}
