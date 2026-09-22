// tooth_model_mechanics.cpp
// Mechanical calculations: growth, buoyancy, repulsion, traction, position updates

#include "tooth_model.hpp"

namespace {

// Direction angle in [0, 2*pi) from its cosine, mirrored into the lower half
// plane when the y component is negative -- the Fortran's acos plus 2*pi
// correction, used for the border-growth geometry.
inline double directionAngle(double cosine, double y) {
    double angle = std::acos(cosine);
    if (y < 0) angle = 2 * PI - angle;
    return angle;
}

}   // namespace

void ToothModel::calculateGrowthPushing() {
    // Reset deltas
    for (int i = 0; i < numCells; i++) {
        positionDeltas[i].fill(0.0);
    }

    // Interior cells (from numBorderCells to numCells)
    for (int i = numBorderCells; i < numCells; i++) {
        if (knotMarkers[i] == 1) continue;

        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        double aa = 0.0, bb = 0.0, cc = 0.0;

        for (int j = 0; j < slotEnd[i]; j++) {
            int k = neighbors[i][j];
            if (k == 0 || k > numCells) continue;
            k--;  // Convert to 0-based

            double zDiff = uc - cellPositions[k][2];
            if (zDiff < -1e-4) {
                // Neighbor is higher - calculate push direction
                double uux = ua - cellPositions[k][0];
                double uuy = ub - cellPositions[k][1];
                double uuz = uc - cellPositions[k][2];
                double d = std::sqrt(uux * uux + uuy * uuy + uuz * uuz);

                if (d > 0) {
                    d = 1.0 / d;
                    aa -= uux * d;
                    bb -= uuy * d;
                    cc -= uuz * d;
                }
            }
        }

        double d = std::sqrt(aa * aa + bb * bb + cc * cc);
        if (d > 0) {
            d = epithelialGrowthRate / d;
            double a = 1.0 - quantities2D[i][0];
            if (a < 0) a = 0.0;
            d *= a;

            // Equation (5): Growth pushing
            positionDeltas[i][0] = aa * d;
            positionDeltas[i][1] = bb * d;
            positionDeltas[i][2] = cc * d;
        }
    }

    // Border cells (from 0 to numBorderCells-1)
    for (int i = 0; i < numBorderCells; i++) {
        double aa = 0.0, bb = 0.0;
        double a = -0.3, b = 0.0, c = 0.0;
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];

        double uuux = 0.0, uuuy = 0.0, duux = 0.0, duuy = 0.0;
        double uaa = 0.0, ubb = 0.0;
        double dd = 0.0, ddd = 0.0;

        for (int j = 0; j < slotEnd[i]; j++) {
            int k = neighbors[i][j];
            if (k < 1 || k > numCells) continue;
            k--;

            double uux = ua - cellPositions[k][0];
            double uuy = ub - cellPositions[k][1];
            double d = std::sqrt(uux * uux + uuy * uuy);

            if (k >= numBorderCells) {
                // Neighbor is interior cell
                if (d > 0) {
                    c = directionAngle(uux / d, uuy);
                }
            } else {
                // Neighbor is border cell
                if (d > 0) {
                    // First border neighbour: its direction a and the
                    // perpendicular (uuux, uuuy) at angle uaa. Second: b, ubb.
                    if (a == -0.3) {
                        a = directionAngle(uux / d, uuy);
                        dd = 1.0 / d;
                        uuux = -uuy * dd;
                        uuuy = uux * dd;
                        uaa = directionAngle(uuux, uuuy);
                    } else {
                        b = directionAngle(uux / d, uuy);
                        dd = 1.0 / d;
                        duux = -uuy * dd;
                        duuy = uux * dd;
                        ubb = directionAngle(duux, duuy);
                    }
                }
            }
        }

        // Ensure a > b
        if (a < b) {
            std::swap(a, b);
        }

        // Determine which side is inside/outside
        if (c < a && c > b) {
            if (uaa < a && uaa > b) {
                uuux = -uuux;
                uuuy = -uuuy;
            }
            if (ubb < a && ubb > b) {
                duux = -duux;
                duuy = -duuy;
            }
        } else {
            if (uaa > a || uaa < b) {
                uuux = -uuux;
                uuuy = -uuuy;
            }
            if (ubb > a || ubb < b) {
                duux = -duux;
                duuy = -duuy;
            }
        }

        aa = -uuux - duux;
        bb = -uuuy - duuy;

        // Ensure direction is outward
        double testA = ua + aa;
        double testB = ub + bb;
        double testC = ua - aa;
        double testD = ub - bb;
        dd = std::sqrt(testA * testA + testB * testB);
        ddd = std::sqrt(testC * testC + testD * testD);
        if (ddd > dd) {
            aa = -aa;
            bb = -bb;
        }

        // Add mesenchymal traction downward
        double d = std::sqrt(aa * aa + bb * bb);
        if (d > 0) {
            // Equation (12) + basal mesenchymal rate
            d = (d + mesenchymalGrowthRate * quantities3D[i][0][2] + basalMesenchymalRate) / d;
            aa *= d;
            bb *= d;
        }

        double cc = sharpnessMax;  // Downward pull (sharpness)
        d = std::sqrt(aa * aa + bb * bb + cc * cc);
        if (d > 0) {
            d = epithelialGrowthRate / d;
            double diffState = 1.0 - quantities2D[i][0];
            if (diffState < 0) diffState = 0.0;
            d *= diffState;

            positionDeltas[i][0] = aa * d;
            positionDeltas[i][1] = bb * d;
            positionDeltas[i][2] = cc * d;
        }
    }
}

void ToothModel::calculateBuoyancy() {
    // Stellate/buoyancy effect - pushes perpendicular to growth direction
    // NOTE: diffusionCoeffs2D[1] is GUI "Boy" (buoyancy strength), not a diffusion coefficient!
    for (int i = 0; i < numCells; i++) {
        double ax = positionDeltas[i][0];
        double ay = positionDeltas[i][1];
        double d = std::sqrt(ax * ax + ay * ay);

        if (d != 0) {
            double c = positionDeltas[i][2];
            if (d > 0) {
                double a = std::sqrt(ax * ax + ay * ay + c * c);
                a = -c / a;
                ax *= a;
                ay *= a;

                double dd = std::sqrt(ax * ax + ay * ay + d * d);
                // diffusionCoeffs2D[1] = GUI "Boy" parameter (buoyancy strength)
                dd = diffusionCoeffs2D[1] * quantities3D[i][0][2] / dd;

                if (dd > 0) {
                    double diffState = 1.0 - quantities2D[i][0];
                    if (diffState < 0) diffState = 0.0;

                    // Fortran: ax=ax*dd*a, i.e. (ax*dd)*a. Written as `ax *= dd*a`
                    // this would round dd*a first and drift from the Fortran.
                    ax = ax * dd * diffState;
                    ay = ay * dd * diffState;
                    d = d * dd * diffState;

                    positionDeltas[i][0] -= ax;
                    positionDeltas[i][1] -= ay;
                    positionDeltas[i][2] -= d;
                }
            }
        }
    }
}

void ToothModel::calculateNeighborRepulsion() {
    // Fortran zeroes persu(nvmax,3) per cell and then sums all 30 slots.
    // Slots that produce no force contribute exactly +0.0, so accumulating
    // directly in ascending j gives the same sum (bar signed zero) without
    // the 90-double scratch buffer.
    for (int i = 0; i < numCells; i++) {
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;

        for (int j = 0; j < slotEnd[i]; j++) {
            int k = neighbors[i][j];
            if (k <= 0 || k > numCells) continue;
            k--;

            double ux = cellPositions[k][0] - ua;
            double uy = cellPositions[k][1] - ub;
            double uz = cellPositions[k][2] - uc;

            if (std::abs(ux) < 1e-15) ux = 0.0;
            if (std::abs(uy) < 1e-15) uy = 0.0;
            if (std::abs(uz) < 1e-15) uz = 0.0;

            double dist = std::sqrt(ux * ux + uy * uy + uz * uz);
            double restDist = cellMargins[i][j][MARGIN_REST];

            if (dist < 1e-8) dist = 0.0;
            if (restDist < 1e-8) restDist = 0.0;

            if (knotMarkers[i] == 1 && knotMarkers[k] == 1) {
                double d = dist - restDist;
                if (dist > 0) {
                    double dr = d / dist;
                    double fx = ux * dr, fy = uy * dr, fz = uz * dr;
                    sumX += fx; sumY += fy; sumZ += fz;
                }
            } else if (dist < restDist) {
                double d = dist - restDist;
                if (dist > 0) {
                    double dr = d / dist;
                    double fx = ux * dr, fy = uy * dr, fz = uz * dr;
                    sumX += fx; sumY += fy; sumZ += fz;
                }
            } else if (i >= numBorderCells) {
                double fx = ux * neighborTraction;
                double fy = uy * neighborTraction;
                double fz = uz * neighborTraction;
                sumX += fx; sumY += fy; sumZ += fz;
            }
        }

        double c = stiffness;
        if (c > 1) c = 1;

        positionDeltas[i][0] += sumX * c;
        positionDeltas[i][1] += sumY * c;
        positionDeltas[i][2] += sumZ * c;
    }
}

namespace {

// Non-neighbour repulsion acts between cells closer than NN_RANGE (Fortran
// 0.14D1). The candidate lists are built for NN_RANGE + NN_SKIN and rebuilt once
// any cell has moved more than 0.45 * NN_SKIN: two cells can then have approached
// each other by less than NN_SKIN, so every pair now within range was within
// NN_RANGE + NN_SKIN when the lists were built.
constexpr double NN_RANGE = 1.4;
constexpr double NN_RANGE_SQ = NN_RANGE * NN_RANGE;
constexpr double NN_SKIN = 0.2;
constexpr double NN_MAX_DISP_SQ = (0.45 * NN_SKIN) * (0.45 * NN_SKIN);

}   // namespace

void ToothModel::rebuildNonNeighborCandidates() {
    constexpr double range = NN_RANGE + NN_SKIN;
    constexpr double rangeSq = range * range;
    constexpr double invBinSize = 1.0 / range;

    // Bin the cells into a grid with bins of the candidate range, so that all
    // candidates of a cell are in the 27 bins around it.
    double minX = cellPositions[0][0], maxX = minX;
    double minY = cellPositions[0][1], maxY = minY;
    double minZ = cellPositions[0][2], maxZ = minZ;
    for (int i = 1; i < numCells; i++) {
        double x = cellPositions[i][0], y = cellPositions[i][1], z = cellPositions[i][2];
        if (x < minX) minX = x; else if (x > maxX) maxX = x;
        if (y < minY) minY = y; else if (y > maxY) maxY = y;
        if (z < minZ) minZ = z; else if (z > maxZ) maxZ = z;
    }
    int gridX = static_cast<int>((maxX - minX) * invBinSize) + 2;
    int gridY = static_cast<int>((maxY - minY) * invBinSize) + 2;
    int gridZ = static_cast<int>((maxZ - minZ) * invBinSize) + 2;
    int gridYZ = gridY * gridZ;
    int gridTotal = gridX * gridYZ;

    nnBinCount.assign(gridTotal, 0);
    nnCellBin.resize(numCells);
    nnBinOffset.resize(gridTotal + 1);
    nnBinCells.resize(numCells);
    nnBinPos.assign(gridTotal, 0);
    if (static_cast<int>(nnStamp.size()) < numCellsTotal + 1) nnStamp.assign(numCellsTotal + 1, 0);

    for (int i = 0; i < numCells; i++) {
        int gx = static_cast<int>((cellPositions[i][0] - minX) * invBinSize);
        int gy = static_cast<int>((cellPositions[i][1] - minY) * invBinSize);
        int gz = static_cast<int>((cellPositions[i][2] - minZ) * invBinSize);
        int bin = gx * gridYZ + gy * gridZ + gz;
        nnCellBin[i] = bin;
        nnBinCount[bin]++;
    }
    nnBinOffset[0] = 0;
    for (int b = 0; b < gridTotal; b++) nnBinOffset[b + 1] = nnBinOffset[b] + nnBinCount[b];
    for (int i = 0; i < numCells; i++) {
        int bin = nnCellBin[i];
        nnBinCells[nnBinOffset[bin] + nnBinPos[bin]] = i;
        nnBinPos[bin]++;
    }

    nnCandOffset.assign(numCells + 1, 0);
    nnCand.clear();
    for (int i = 0; i < numCells; i++) {
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        // O(1) neighbour membership via generation stamp.
        nnStampGen++;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int n = neighbors[i][j];
            if (n > 0) nnStamp[n] = nnStampGen;
        }

        int gx = static_cast<int>((ua - minX) * invBinSize);
        int gy = static_cast<int>((ub - minY) * invBinSize);
        int gz = static_cast<int>((uc - minZ) * invBinSize);
        int xlo = gx - 1 < 0 ? 0 : gx - 1, xhi = gx + 1 >= gridX ? gridX - 1 : gx + 1;
        int ylo = gy - 1 < 0 ? 0 : gy - 1, yhi = gy + 1 >= gridY ? gridY - 1 : gy + 1;
        int zlo = gz - 1 < 0 ? 0 : gz - 1, zhi = gz + 1 >= gridZ ? gridZ - 1 : gz + 1;
        const size_t first = nnCand.size();
        for (int nx = xlo; nx <= xhi; nx++) {
            for (int ny = ylo; ny <= yhi; ny++) {
                int rowBase = nx * gridYZ + ny * gridZ;
                for (int nz = zlo; nz <= zhi; nz++) {
                    int nbin = rowBase + nz;
                    for (int p = nnBinOffset[nbin]; p < nnBinOffset[nbin + 1]; p++) {
                        int ii = nnBinCells[p];
                        if (ii == i) continue;
                        if (nnStamp[ii + 1] == nnStampGen) continue;   // is a neighbour
                        double ux = cellPositions[ii][0] - ua;
                        double uy = cellPositions[ii][1] - ub;
                        double uz = cellPositions[ii][2] - uc;
                        if (ux * ux + uy * uy + uz * uz < rangeSq) nnCand.push_back(ii);
                    }
                }
            }
        }
        // Ascending index order reproduces the Fortran's accumulation order exactly.
        std::sort(nnCand.begin() + first, nnCand.end());
        nnCandOffset[i + 1] = static_cast<int>(nnCand.size());
    }

    nnRefPositions.assign(cellPositions.begin(), cellPositions.begin() + numCells);
    nnCandVersion = topologyVersion;
}

void ToothModel::checkNonNeighborRepulsion() {
    if (numCells < 2) return;

    bool rebuild = nnCandVersion != topologyVersion ||
                   static_cast<int>(nnRefPositions.size()) != numCells;
    if (!rebuild) {
        double maxDispSq = 0.0;
        for (int i = 0; i < numCells; i++) {
            double dx = cellPositions[i][0] - nnRefPositions[i][0];
            double dy = cellPositions[i][1] - nnRefPositions[i][1];
            double dz = cellPositions[i][2] - nnRefPositions[i][2];
            double dispSq = dx * dx + dy * dy + dz * dz;
            if (!(dispSq <= maxDispSq)) maxDispSq = dispSq;     // also catches NaN
        }
        rebuild = !(maxDispSq <= NN_MAX_DISP_SQ);
    }
    if (rebuild) rebuildNonNeighborCandidates();

    for (int i = 0; i < numCells; i++) {
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        for (int p = nnCandOffset[i]; p < nnCandOffset[i + 1]; p++) {
            int ii = nnCand[p];
            double ux = cellPositions[ii][0] - ua;
            double uy = cellPositions[ii][1] - ub;
            double uz = cellPositions[ii][2] - uc;
            if (std::abs(ux) < 1e-15) ux = 0.0;
            if (std::abs(uy) < 1e-15) uy = 0.0;
            if (std::abs(uz) < 1e-15) uz = 0.0;

            double dSq = ux * ux + uy * uy + uz * uz;
            if (!(dSq < NN_RANGE_SQ)) continue;

            double d = std::sqrt(dSq);
            double dp1 = d + 1.0;
            double dp1_2 = dp1 * dp1;
            double dp1_4 = dp1_2 * dp1_2;
            double dp1_8 = dp1_4 * dp1_4;
            double dd = 1.0 / dp1_8;
            d = dd / d;
            d = std::trunc(d * 1e8) * 1e-8;

            sumX -= ux * d;
            sumY -= uy * d;
            sumZ -= uz * d;
        }

        // NOTE: unlike calculateNeighborRepulsion, this applies stiffness
        // unclamped. Fortran pushingnovei computes a clamped c=elas and then
        // uses raw elas anyway (humppa_translate.f90:953-960); that quirk is
        // reproduced deliberately. Clamping here would change the output.
        positionDeltas[i][0] += sumX * stiffness;
        positionDeltas[i][1] += sumY * stiffness;
        positionDeltas[i][2] += sumZ * stiffness;
    }
}

void ToothModel::calculateNucleusTraction() {
    // Every cell that is not fully differentiated is pulled towards the centroid
    // of its neighbours; a border cell only averages over the other border cells
    // (Fortran: k<ncils). New positions are computed from the old ones, so they
    // are staged in prevPositions and swapped in at the end.
    prevPositions.assign(cellPositions.begin(), cellPositions.end());

    for (int i = 0; i < numCells; i++) {
        if (quantities2D[i][0] == 1.0) continue;

        const int maxNeighbor = (i < numBorderCells) ? numBorderCells : numCells;  // 1-based, inclusive
        double a = 0.0, b = 0.0, c = 0.0;
        double n = 0;

        for (int j = 0; j < slotEnd[i]; j++) {
            int k = neighbors[i][j];
            if (k > 0 && k <= maxNeighbor) {
                k--;
                a += cellPositions[k][0];
                b += cellPositions[k][1];
                c += cellPositions[k][2];
                n += 1.0;
            }
        }

        if (n > 0) {
            n = 1.0 / n;
            a = (a * n) - cellPositions[i][0];
            b = (b * n) - cellPositions[i][1];
            c = (c * n) - cellPositions[i][2];

            prevPositions[i][0] = cellPositions[i][0] + TIME_DELTA * nucleusTraction * a;
            prevPositions[i][1] = cellPositions[i][1] + TIME_DELTA * nucleusTraction * b;

            if (knotMarkers[i] == 0) {
                double diffState = 1.0 - quantities2D[i][0];
                if (diffState < 0) diffState = 0.0;
                prevPositions[i][2] = cellPositions[i][2] + TIME_DELTA * nucleusTraction * c * diffState;
            }
        }
    }

    std::swap(cellPositions, prevPositions);
}

void ToothModel::updatePositions() {
    // Apply anterior/posterior bias to border cells
    for (int i = 0; i < numBorderCells; i++) {
        if (std::abs(cellPositions[i][1]) < biasCenterRadius) {
            if (cellPositions[i][0] > 0) {
                // x > 0 side: bia, fed by GUI Pbi (see the note in tooth_model.hpp)
                positionDeltas[i][0] *= biasAnterior;
                positionDeltas[i][2] *= biasFactor;
            }
            if (cellPositions[i][0] < 0) {
                // x < 0 side: bip, fed by GUI Abi
                positionDeltas[i][0] *= biasPosterior;
                positionDeltas[i][2] *= biasFactor;
            }
        }
    }

    // Clamp z deltas, lock knot z, and apply position updates (single pass)
    for (int i = 0; i < numCells; i++) {
        if (positionDeltas[i][2] < 0) positionDeltas[i][2] = 0.0;
        if (knotMarkers[i] == 1) positionDeltas[i][2] = 0.0;
        cellPositions[i][0] += TIME_DELTA * positionDeltas[i][0];
        cellPositions[i][1] += TIME_DELTA * positionDeltas[i][1];
        cellPositions[i][2] += TIME_DELTA * positionDeltas[i][2];
    }
}
