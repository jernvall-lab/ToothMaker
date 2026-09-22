// tooth_model_diffusion.cpp
// Reaction-diffusion and differentiation functions

#include "tooth_model.hpp"

void ToothModel::reactionDiffusion() {
    // The delta buffer is kept between iterations and only grows.
    if (static_cast<int>(rdDeltaQ3D.size()) < numCellsTotal) {
        rdDeltaQ3D.resize(numCellsTotal, Q3DBlock{});
    }

    constexpr int lastZ = NUM_Z_LEVELS - 1;

    // DIFFUSION (Fortran reaccio_difusio, first part). Only the first four
    // quantities diffuse ("do k=1,4 !ng ATENCIO").
    for (int i = 0; i < numCells; i++) {
        const CellMargins& margins = cellMargins[i];
        const Q3DBlock& qi = quantities3D[i];
        Q3DBlock& di = rdDeltaQ3D[i];

        // Occupied neighbour slots, ascending. Empty slots would contribute
        // exactly +0.0 to every sum below, so leaving them out is exact.
        int slotJ[MAX_NEIGHBORS];
        int slotII[MAX_NEIGHBORS];
        int nSlots = 0;
        for (int j = 0; j < slotEnd[i]; j++) {
            if (neighbors[i][j] != 0) {
                slotJ[nSlots] = j;
                slotII[nSlots] = neighbors[i][j] - 1;
                nSlots++;
            }
        }

        // Diffusion weights from the cell geometry (Fortran pes, areap): for each
        // occupied slot, the length of the edge from its margin point to the next
        // occupied slot's, and the area of the triangle they span with the cell
        // centre. The last slot pairs with slot 0, occupied or not, as in the Fortran.
        double weight[MAX_NEIGHBORS];
        double areaProj[MAX_NEIGHBORS];
        const double ua = cellPositions[i][0];
        const double ub = cellPositions[i][1];
        const double uc = cellPositions[i][2];
        for (int s = 0; s < nSlots; s++) {
            const MarginSlot& mj = margins[slotJ[s]];
            const MarginSlot& mjj = (s + 1 < nSlots) ? margins[slotJ[s + 1]] : margins[0];

            weight[s] = std::sqrt(
                std::pow(mj[0] - mjj[0], 2) +
                std::pow(mj[1] - mjj[1], 2) +
                std::pow(mj[2] - mjj[2], 2));

            double ux = mj[0] - ua;
            double uy = mj[1] - ub;
            double uz = mj[2] - uc;
            double dx = mjj[0] - ua;
            double dy = mjj[1] - ub;
            double dz = mjj[2] - uc;

            areaProj[s] = 0.5 * std::sqrt(  // Fortran: 0.05D1 = 0.5
                std::pow(uy * dz - uz * dy, 2) +
                std::pow(uz * dx - ux * dz, 2) +
                std::pow(ux * dy - uy * dx, 2));
        }

        // Normalise: the cell's faces are its edges plus a floor and a ceiling.
        double areaBelow = 0.0;
        for (int s = 0; s < nSlots; s++) areaBelow += areaProj[s];

        double suma = 0.0;
        for (int s = 0; s < nSlots; s++) suma += weight[s];
        suma += 2 * areaBelow;
        const double originalSuma = suma;

        if (suma > 0) {
            areaBelow /= suma;
            for (int s = 0; s < nSlots; s++) weight[s] /= suma;
        }

        // Vertical (areaBelow) terms first: for every (kk,k) accumulator these are
        // the first additions. The middle levels start from zero and accumulate,
        // as the Fortran does (a plain assignment would differ only in the sign
        // of a zero).
        for (int k = 0; k < 4; k++) {
            for (int kk = 1; kk < lastZ; kk++) {
                di[kk][k] = 0.0;
                di[kk][k] += areaBelow * (qi[kk - 1][k] - qi[kk][k]);
                di[kk][k] += areaBelow * (qi[kk + 1][k] - qi[kk][k]);
            }
            di[lastZ][k]  = areaBelow * (-qi[lastZ][k] * 0.44);    // bottom: sink
            di[lastZ][k] += areaBelow * (qi[lastZ - 1][k] - qi[lastZ][k]);
        }

        // Neighbour terms, slot ascending -- the Fortran's per-accumulator order.
        // The virtual boundary cell is a sink.
        for (int s = 0; s < nSlots; s++) {
            const int ii = slotII[s];
            const double w = weight[s];
            if (ii == numCellsTotal - 1) {
                for (int kk = 1; kk <= lastZ; kk++)
                    for (int k = 0; k < 4; k++)
                        di[kk][k] += w * (-qi[kk][k] * 0.44);
            } else {
                const Q3DBlock& qn = quantities3D[ii];
                for (int kk = 1; kk <= lastZ; kk++)
                    for (int k = 0; k < 4; k++)
                        di[kk][k] += w * (qn[kk][k] - qi[kk][k]);
            }
        }

        // The top level has one vertical face instead of two, so its weights are
        // normalised without one areaBelow -- by scaling back and dividing again,
        // exactly as the Fortran does (pes=pes*suma ; areasota=areasota*suma ;
        // suma=suma-areasota ; pes=pes/suma).
        for (int s = 0; s < nSlots; s++) weight[s] *= originalSuma;
        areaBelow *= originalSuma;
        suma = originalSuma - areaBelow;
        if (suma > 0) {
            for (int s = 0; s < nSlots; s++) weight[s] /= suma;
            areaBelow /= suma;
        }

        for (int k = 0; k < 4; k++) {
            di[0][k] = areaBelow * (qi[1][k] - qi[0][k]);
        }
        for (int s = 0; s < nSlots; s++) {
            const int ii = slotII[s];
            const double w = weight[s];
            if (ii == numCellsTotal - 1) {
                for (int k = 0; k < 4; k++)
                    di[0][k] += w * (-qi[0][k] * 0.44);
            } else {
                const Q3DBlock& qn = quantities3D[ii];
                for (int k = 0; k < 4; k++)
                    di[0][k] += w * (qn[0][k] - qi[0][k]);
            }
        }
    }

    // Apply diffusion updates
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < numCells; i++) {
            for (int kk = 0; kk < NUM_Z_LEVELS; kk++) {
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
        panic("numerical overflow in the reaction terms (concentration change above 1e100)");
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
        for (int kk = 0; kk < NUM_Z_LEVELS; kk++) {
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
        // The bias is a floor: the activator is never lowered. Sides and names, see
        // the note in tooth_model.hpp.
        if (cellPositions[i][1] < -borderDistance) {
            // y < -Swi side: bil, fed by GUI Bbi
            if (quantities3D[i][0][0] < biasLingual) {
                quantities3D[i][0][0] = biasLingual;
            }
        } else if (cellPositions[i][1] > borderDistance) {
            // y > Swi side: bib, fed by GUI Lbi
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
