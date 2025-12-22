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
    std::vector<std::array<double, 3>> pushForce(MAX_NEIGHBORS);

    for (int i = 0; i < numCells; i++) {
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        for (auto& p : pushForce) p.fill(0.0);

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            if (k > 0 && k <= numCells) {
                k--;

                double ux = cellPositions[k][0] - ua;
                double uy = cellPositions[k][1] - ub;
                double uz = cellPositions[k][2] - uc;

                // Clean up tiny values
                if (std::abs(ux) < 1e-15) ux = 0.0;
                if (std::abs(uy) < 1e-15) uy = 0.0;
                if (std::abs(uz) < 1e-15) uz = 0.0;

                double dist = std::sqrt(ux * ux + uy * uy + uz * uz);
                double restDist = cellMargins[i][j][4];

                if (dist < 1e-8) dist = 0.0;
                if (restDist < 1e-8) restDist = 0.0;

                if (knotMarkers[i] == 1 && knotMarkers[k] == 1) {
                    // Both are knots - always apply force
                    double d = dist - restDist;
                    if (dist > 0) {
                        double dr = d / dist;
                        pushForce[j][0] = ux * dr;
                        pushForce[j][1] = uy * dr;
                        pushForce[j][2] = uz * dr;
                    }
                } else {
                    if (dist < restDist) {
                        // Too close - repel
                        double d = dist - restDist;
                        if (dist > 0) {
                            double dr = d / dist;
                            pushForce[j][0] = ux * dr;
                            pushForce[j][1] = uy * dr;
                            pushForce[j][2] = uz * dr;
                        }
                    } else {
                        // Not too close - apply traction for interior cells
                        if (i >= numBorderCells) {
                            pushForce[j][0] = ux * neighborTraction;
                            pushForce[j][1] = uy * neighborTraction;
                            pushForce[j][2] = uz * neighborTraction;
                        }
                    }
                }
            }
        }

        // Sum forces with stiffness coefficient
        double c = stiffness;
        if (c > 1) c = 1;

        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            sumX += pushForce[j][0];
            sumY += pushForce[j][1];
            sumZ += pushForce[j][2];
        }

        positionDeltas[i][0] += sumX * c;
        positionDeltas[i][1] += sumY * c;
        positionDeltas[i][2] += sumZ * c;
    }
}

void ToothModel::checkNonNeighborRepulsion() {
    // Check if non-neighboring cells get too close
    const int espai = 20;
    std::vector<std::array<double, 3>> pushForce(espai);

    for (int i = 0; i < numCells; i++) {
        double ua = cellPositions[i][0];
        double ub = cellPositions[i][1];
        double uc = cellPositions[i][2];

        for (auto& p : pushForce) p.fill(0.0);
        int conta = 0;

        for (int ii = 0; ii < numCells; ii++) {
            if (ii == i) continue;

            // Check if ii is already a neighbor
            bool isNeighbor = false;
            for (int j = 0; j < MAX_NEIGHBORS; j++) {
                if (neighbors[i][j] == ii + 1) {
                    isNeighbor = true;
                    break;
                }
            }
            if (isNeighbor) continue;

            double ux = cellPositions[ii][0] - ua;
            if (ux > 1.4) continue;
            double uy = cellPositions[ii][1] - ub;
            if (uy > 1.4) continue;
            double uz = cellPositions[ii][2] - uc;
            if (uz > 1.4) continue;

            if (std::abs(ux) < 1e-15) ux = 0.0;
            if (std::abs(uy) < 1e-15) uy = 0.0;
            if (std::abs(uz) < 1e-15) uz = 0.0;

            double d = std::sqrt(ux * ux + uy * uy + uz * uz);

            if (d < 1.4) {
                if (conta >= static_cast<int>(pushForce.size())) {
                    pushForce.resize(pushForce.size() + 20);
                }

                // Soft repulsion force that falls off with distance
                // Use manual multiplication instead of std::pow for exact match with Fortran
                double dp1 = d + 1.0;
                double dp1_2 = dp1 * dp1;
                double dp1_4 = dp1_2 * dp1_2;
                double dp1_8 = dp1_4 * dp1_4;
                double dd = 1.0 / dp1_8;
                d = dd / d;
                d = std::trunc(d * 1e8) * 1e-8;  // aint() in Fortran truncates

                pushForce[conta][0] = -ux * d;
                pushForce[conta][1] = -uy * d;
                pushForce[conta][2] = -uz * d;
                conta++;
            }
        }

        // Sum forces (only iterate over actually used elements)
        double sumX = 0.0, sumY = 0.0, sumZ = 0.0;
        for (int j = 0; j < conta; j++) {
            sumX += pushForce[j][0];
            sumY += pushForce[j][1];
            sumZ += pushForce[j][2];
        }

        double c = stiffness;
        if (c > 1) c = 1;

        positionDeltas[i][0] += sumX * stiffness;
        positionDeltas[i][1] += sumY * stiffness;
        positionDeltas[i][2] += sumZ * stiffness;
    }
}

void ToothModel::calculateNucleusTraction() {
    // Nucleus traction toward cell borders (averaging effect)
    std::vector<std::array<double, 3>> prevPositions = cellPositions;

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

    // Prevent upward movement (from stellate pressure)
    for (int i = 0; i < numCells; i++) {
        if (positionDeltas[i][2] < 0) {
            positionDeltas[i][2] = 0.0;
        }
    }

    // Lock z position for knots
    for (int i = 0; i < numCells; i++) {
        if (knotMarkers[i] == 1) {
            positionDeltas[i][2] = 0.0;
        }
    }

    // Apply position updates
    for (int i = 0; i < numCells; i++) {
        cellPositions[i][0] += TIME_DELTA * positionDeltas[i][0];
        cellPositions[i][1] += TIME_DELTA * positionDeltas[i][1];
        cellPositions[i][2] += TIME_DELTA * positionDeltas[i][2];
    }
}
