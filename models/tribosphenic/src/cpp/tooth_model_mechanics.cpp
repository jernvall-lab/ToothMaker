// tooth_model_mechanics.cpp
// Mechanical calculations: growth, buoyancy, repulsion, traction, position updates

#include "tooth_model.hpp"


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

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
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

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            if (k < 1 || k > numCells) continue;
            k--;

            double uux = ua - cellPositions[k][0];
            double uuy = ub - cellPositions[k][1];
            double d = std::sqrt(uux * uux + uuy * uuy);

            if (k >= numBorderCells) {
                // Neighbor is interior cell
                if (d > 0) {
                    c = std::acos(uux / d);
                    if (uuy < 0) c = 2 * PI - c;
                }
            } else {
                // Neighbor is border cell
                if (d > 0) {
                    if (a == -0.3) {
                        a = std::acos(uux / d);
                        if (uuy < 0) a = 2 * PI - a;
                        dd = 1.0 / d;
                        uuux = -uuy * dd;
                        uuuy = uux * dd;
                        uaa = std::acos(uuux);
                        if (uuuy < 0) uaa = 2 * PI - uaa;
                    } else {
                        b = std::acos(uux / d);
                        if (uuy < 0) b = 2 * PI - b;
                        dd = 1.0 / d;
                        duux = -uuy * dd;
                        duuy = uux * dd;
                        ubb = std::acos(duux);
                        if (duuy < 0) ubb = 2 * PI - ubb;
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

                    ax *= dd * diffState;
                    ay *= dd * diffState;
                    d *= dd * diffState;

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

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
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
            double restDist = cellMargins[i][j][4];

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

void ToothModel::checkNonNeighborRepulsion() {
    constexpr double distThreshold = 1.4;
    constexpr double distThresholdSq = distThreshold * distThreshold;

    if (numCells < 2) return;

    constexpr double gridCellSize = distThreshold;
    constexpr double invGridCellSize = 1.0 / gridCellSize;

    double minX = cellPositions[0][0], maxX = minX;
    double minY = cellPositions[0][1], maxY = minY;
    double minZ = cellPositions[0][2], maxZ = minZ;
    for (int i = 1; i < numCells; i++) {
        double x = cellPositions[i][0], y = cellPositions[i][1], z = cellPositions[i][2];
        if (x < minX) minX = x; else if (x > maxX) maxX = x;
        if (y < minY) minY = y; else if (y > maxY) maxY = y;
        if (z < minZ) minZ = z; else if (z > maxZ) maxZ = z;
    }

    int gridX = static_cast<int>((maxX - minX) * invGridCellSize) + 2;
    int gridY = static_cast<int>((maxY - minY) * invGridCellSize) + 2;
    int gridZ = static_cast<int>((maxZ - minZ) * invGridCellSize) + 2;
    int gridYZ = gridY * gridZ;
    int gridTotal = gridX * gridYZ;

    // Persistent scratch: avoids re-allocating five vectors every iteration.
    static std::vector<int> binCount, cellBin, binOffset, binCells, binPos;
    static std::vector<int> stamp;      // neighbour membership stamp
    static std::vector<int> survivors;
    static int stampGen = 0;

    binCount.assign(gridTotal, 0);
    cellBin.resize(numCells);
    binOffset.resize(gridTotal + 1);
    binCells.resize(numCells);
    binPos.assign(gridTotal, 0);
    if (static_cast<int>(stamp.size()) < numCellsTotal + 1) stamp.assign(numCellsTotal + 1, 0);

    for (int i = 0; i < numCells; i++) {
        int gx = static_cast<int>((cellPositions[i][0] - minX) * invGridCellSize);
        int gy = static_cast<int>((cellPositions[i][1] - minY) * invGridCellSize);
        int gz = static_cast<int>((cellPositions[i][2] - minZ) * invGridCellSize);
        int bin = gx * gridYZ + gy * gridZ + gz;
        cellBin[i] = bin;
        binCount[bin]++;
    }

    binOffset[0] = 0;
    for (int b = 0; b < gridTotal; b++) binOffset[b + 1] = binOffset[b] + binCount[b];

    for (int i = 0; i < numCells; i++) {
        int bin = cellBin[i];
        binCells[binOffset[bin] + binPos[bin]] = i;
        binPos[bin]++;
    }

    for (int i = 0; i < numCells; i++) {
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        // O(1) neighbour membership via generation stamp.
        stampGen++;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int n = neighbors[i][j];
            if (n > 0) stamp[n] = stampGen;
        }

        int gx = static_cast<int>((ua - minX) * invGridCellSize);
        int gy = static_cast<int>((ub - minY) * invGridCellSize);
        int gz = static_cast<int>((uc - minZ) * invGridCellSize);

        // Collect only the cells that actually contribute, then sort those.
        // Ordering the *contributors* by index reproduces the original
        // ascending-index accumulation exactly; rejected cells cannot matter.
        survivors.clear();
        int xlo = gx - 1 < 0 ? 0 : gx - 1, xhi = gx + 1 >= gridX ? gridX - 1 : gx + 1;
        int ylo = gy - 1 < 0 ? 0 : gy - 1, yhi = gy + 1 >= gridY ? gridY - 1 : gy + 1;
        int zlo = gz - 1 < 0 ? 0 : gz - 1, zhi = gz + 1 >= gridZ ? gridZ - 1 : gz + 1;
        for (int nx = xlo; nx <= xhi; nx++) {
            for (int ny = ylo; ny <= yhi; ny++) {
                int rowBase = nx * gridYZ + ny * gridZ;
                for (int nz = zlo; nz <= zhi; nz++) {
                    int nbin = rowBase + nz;
                    for (int p = binOffset[nbin]; p < binOffset[nbin + 1]; p++) {
                        int ii = binCells[p];
                        if (ii == i) continue;
                        // cheap axis rejections first
                        double ux = cellPositions[ii][0] - ua;
                        if (ux > distThreshold || ux < -distThreshold) continue;
                        double uy = cellPositions[ii][1] - ub;
                        if (uy > distThreshold || uy < -distThreshold) continue;
                        double uz = cellPositions[ii][2] - uc;
                        if (uz > distThreshold || uz < -distThreshold) continue;
                        if (stamp[ii + 1] == stampGen) continue;   // is a neighbour
                        if (std::abs(ux) < 1e-15) ux = 0.0;
                        if (std::abs(uy) < 1e-15) uy = 0.0;
                        if (std::abs(uz) < 1e-15) uz = 0.0;
                        if (ux * ux + uy * uy + uz * uz < distThresholdSq) survivors.push_back(ii);
                    }
                }
            }
        }

        std::sort(survivors.begin(), survivors.end());

        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        for (int ii : survivors) {
            double ux = cellPositions[ii][0] - ua;
            double uy = cellPositions[ii][1] - ub;
            double uz = cellPositions[ii][2] - uc;
            if (std::abs(ux) < 1e-15) ux = 0.0;
            if (std::abs(uy) < 1e-15) uy = 0.0;
            if (std::abs(uz) < 1e-15) uz = 0.0;

            double d = std::sqrt(ux * ux + uy * uy + uz * uz);
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
    // Nucleus traction toward cell borders (averaging effect)
    prevPositions.resize(cellPositions.size());
    std::copy(cellPositions.begin(), cellPositions.end(), prevPositions.begin());

    // Interior cells
    for (int i = numBorderCells; i < numCells; i++) {
        if (quantities2D[i][0] == 1.0) continue;

        double a = 0.0, b = 0.0, c = 0.0;
        double n = 0;

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            if (k != 0 && k <= numCells) {
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

    // Border cells
    for (int i = 0; i < numBorderCells; i++) {
        if (quantities2D[i][0] == 1.0) continue;

        double a = 0.0, b = 0.0, c = 0.0;
        double n = 0;

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            // Only consider other border cells for border averaging
            // Fortran: if (k>0.and.k<ncils.and.k<ncels+1)
            // k is 1-based here, ncils = numBorderCells + 1
            if (k > 0 && k <= numBorderCells && k <= numCells) {
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

    cellPositions = prevPositions;
}

void ToothModel::updatePositions() {
    // Apply anterior/posterior bias to border cells
    for (int i = 0; i < numBorderCells; i++) {
        if (std::abs(cellPositions[i][1]) < biasCenterRadius) {
            if (cellPositions[i][0] > 0) {
                // Anterior side
                positionDeltas[i][0] *= biasAnterior;
                positionDeltas[i][2] *= biasFactor;
            }
            if (cellPositions[i][0] < 0) {
                // Posterior side
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
