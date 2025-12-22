// tooth_model_division.cpp
// Cell division: addNewCells, markBorderCells, increaseZDepth, checkSymmetry

#include "tooth_model.hpp"

void ToothModel::addNewCells() {
    // This function handles cell division when cells get too far apart.
    // Faithfully translated from Fortran subroutine afegircel.

    numNewCells = 0;
    std::vector<std::array<int, 2>> newNodePairs(numCells * MAX_NEIGHBORS);
    std::vector<int> isExternal(numCells * MAX_NEIGHBORS, 0);

    double maxDist = 0.0;

    // First identify pairs of cells that need division (nousnodes in Fortran)
    // Fortran: lines 1119-1138
    for (int i = 0; i < numCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            // Fortran: if (k/=0.and.k>i.and.k<=ncels)
            if (k != 0 && k > i + 1 && k <= numCells) {
                k--;  // Convert to 0-based

                double ua = cellPositions[i][0];
                double ub = cellPositions[i][1];
                double uc = cellPositions[i][2];
                double ux = cellPositions[k][0] - ua;
                double uy = cellPositions[k][1] - ub;
                double uz = cellPositions[k][2] - uc;

                double dist = std::sqrt(ux * ux + uy * uy + uz * uz);
                dist = std::round(dist * 1e9) * 1e-9;

                if (dist > maxDist) {
                    maxDist = dist;
                }

                if (dist > MAX_DIVISION_DIST) {
                    // Check for duplicate pair (shouldn't happen but let's detect it)
                    bool isDuplicate = false;
                    for (int d = 0; d < numNewCells; d++) {
                        if ((newNodePairs[d][0] == i && newNodePairs[d][1] == k) ||
                            (newNodePairs[d][0] == k && newNodePairs[d][1] == i)) {
                            isDuplicate = true;
                            break;
                        }
                    }
                    if (!isDuplicate) {
                        newNodePairs[numNewCells][0] = i;
                        newNodePairs[numNewCells][1] = k;
                        // Fortran: if (i<ncils.and.k<ncils) then ; externsa(nnous)=1
                        if (i < numBorderCells && k < numBorderCells) {
                            isExternal[numNewCells] = 1;
                        }
                        numNewCells++;
                    }
                }
            }
        }
    }

    if (numNewCells == 0) return;

    // Remove zero entries from neighbor lists (compact them)
    // Fortran: lines 1141-1149
    for (int i = 0; i < numCellsTotal; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] == 0) {
                for (int jj = j; jj < MAX_NEIGHBORS - 1; jj++) {
                    neighbors[i][jj] = neighbors[i][jj + 1];
                }
                neighbors[i][MAX_NEIGHBORS - 1] = 0;
            }
        }
    }

    int newNumCellsTotal = numCellsTotal + numNewCells;
    const int newNumCells = numCells + numNewCells;  // Pre-compute for loop bounds
    const int newNumCellsBound = newNumCells + 1;    // 1-based boundary check

    // Create new expanded arrays
    // Fortran: lines 1153-1157
    std::vector<std::array<double, 3>> newPositions(newNumCellsTotal, {0.0, 0.0, 0.0});
    std::vector<std::array<int, MAX_NEIGHBORS>> newNeighbors(newNumCellsTotal);
    for (auto& n : newNeighbors) n.fill(0);
    std::vector<int> newNeighborCount(newNumCellsTotal, 0);
    std::vector<std::array<double, NUM_2D_QUANTITIES>> newQ2D(newNumCellsTotal);
    for (auto& q : newQ2D) q.fill(0.0);
    std::vector<std::vector<std::array<double, NUM_3D_QUANTITIES>>> newQ3D(newNumCellsTotal);
    for (auto& cell : newQ3D) {
        cell.resize(numZLevels);
        for (auto& level : cell) level.fill(0.0);
    }
    std::vector<int> newKnots(newNumCellsTotal, 0);
    std::vector<std::vector<std::array<double, 8>>> newMargins(newNumCellsTotal);
    for (auto& m : newMargins) {
        m.resize(MAX_NEIGHBORS);
        for (auto& arr : m) arr.fill(0.0);
    }

    // Temporary neighbor array for new cells (ccvei in Fortran)
    std::vector<std::array<int, MAX_NEIGHBORS>> tempNewNeighbors(numNewCells);
    for (auto& n : tempNewNeighbors) n.fill(0);

    // Copy existing cells that are beyond current numCells (virtual boundary cells)
    // Shift them up to make room for new cells
    // Fortran: lines 1165-1170
    for (int i = newNumCellsTotal - 1; i >= newNumCells; i--) {
        int oldIdx = i - numNewCells;
        newPositions[i] = cellPositions[oldIdx];
        newNeighbors[i] = neighbors[oldIdx];
        newNeighborCount[i] = neighborCount[oldIdx];
        newQ2D[i] = quantities2D[oldIdx];
        newQ3D[i] = quantities3D[oldIdx];
        newKnots[i] = knotMarkers[oldIdx];
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            for (int kIdx = 3; kIdx < 8; kIdx++) {
                newMargins[i][j][kIdx] = cellMargins[oldIdx][j][kIdx];
            }
        }
    }

    // Copy existing cells (0 to numCells-1)
    // Fortran: lines 1172-1174
    for (int i = 0; i < numCells; i++) {
        newPositions[i] = cellPositions[i];
        newNeighbors[i] = neighbors[i];
        newNeighborCount[i] = neighborCount[i];
        newQ2D[i] = quantities2D[i];
        newQ3D[i] = quantities3D[i];
        newKnots[i] = knotMarkers[i];
        newMargins[i] = cellMargins[i];
    }

    // Update references to virtual boundary cells
    // Fortran: line 1176
    for (int i = 0; i < newNumCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (newNeighbors[i][j] > numCells) {
                newNeighbors[i][j] = newNumCellsTotal;
            }
        }
    }

    // Clear neighbors for the slot where new cells will go
    // Fortran: line 1178
    for (int i = numCells; i < newNumCells; i++) {
        newNeighbors[i].fill(0);
    }

    // Create new cells between dividing pairs
    // Set initial neighbors (parent cells) and position
    // Fortran: lines 1180-1196
    for (int i = 0; i < numNewCells; i++) {
        int ii = newNodePairs[i][0];  // 0-based parent 1
        int kk = newNodePairs[i][1];  // 0-based parent 2
        int jj = numCells + i;        // 0-based index of new cell

        // New cell initially connected to both parents (1-based in array)
        newNeighbors[jj][0] = ii + 1;
        newNeighbors[jj][1] = kk + 1;

        // Position: midpoint projected to same radial distance
        // Fortran: a=cmalla(ii,1)+cmalla(kk,1) ; b=cmalla(ii,2)+cmalla(kk,2)
        double a = newPositions[ii][0] + newPositions[kk][0];
        double b = newPositions[ii][1] + newPositions[kk][1];
        double d = std::sqrt(a * a + b * b);
        a = a / d;
        b = b / d;
        d = d / 2.0;  // Fortran: d=d/0.20D1 = d/2.0
        d = std::round(d * 1e10) * 1e-10;
        a = d * a;
        b = d * b;

        newPositions[jj][0] = a;
        newPositions[jj][1] = b;
        // Fortran: cmalla(jj,3)=(malla(ii,3)+malla(kk,3))*0.05D1 = *0.5
        newPositions[jj][2] = (cellPositions[ii][2] + cellPositions[kk][2]) * 0.5;

        // Average quantities from parents
        // Fortran: cq3d(jj,:,:)=(cq3d(ii,:,:)+cq3d(kk,:,:))*0.05D1
        for (int kz = 0; kz < numZLevels; kz++) {
            for (int q = 0; q < NUM_3D_QUANTITIES; q++) {
                newQ3D[jj][kz][q] = (newQ3D[ii][kz][q] + newQ3D[kk][kz][q]) * 0.5;
            }
        }
        for (int q = 0; q < NUM_2D_QUANTITIES; q++) {
            newQ2D[jj][q] = (newQ2D[ii][q] + newQ2D[kk][q]) * 0.5;
        }
    }

    // Update parent cell neighbors to point to new cells instead of each other
    // Fortran: lines 1200-1205
    for (int i = 0; i < numNewCells; i++) {
        int ii = newNodePairs[i][0];
        int kk = newNodePairs[i][1];
        int jj = numCells + i;

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (newNeighbors[ii][j] == kk + 1) {
                newNeighbors[ii][j] = jj + 1;
                break;
            }
        }
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (newNeighbors[kk][j] == ii + 1) {
                newNeighbors[kk][j] = jj + 1;
                break;
            }
        }
    }

    // ========================================================================
    // NEIGHBOR-FINDING ALGORITHM
    // This is the critical part that traverses around each new cell to find
    // all cells that should be its neighbors.
    // Fortran: lines 1207-1404
    // ========================================================================
    for (int i = 0; i < numNewCells; i++) {
        std::array<int, MAX_NEIGHBORS> pillats;  // "caught" cells
        pillats.fill(0);
        std::array<int, MAX_NEIGHBORS> cpillats;
        cpillats.fill(0);

        int ii = newNodePairs[i][0];  // 0-based parent 1
        int kk = newNodePairs[i][1];  // 0-based parent 2
        int jj = numCells + i;        // 0-based new cell index

        // Determine which parent to start from (ini) and which to end at (fi)
        // We look at the neighbors of cell ii beyond position where kk is found
        // If we find a real cell (not external), we start from ii
        // Fortran: lines 1211-1233
        int jjj = -1;  // Position where ii has kk as neighbor (in original vei)
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[ii][j] == kk + 1) {
                jjj = j;
                break;
            }
        }

        int ini = ii;  // ini = start parent (default to ii if no other neighbors found)
        int fi = kk;   // fi = end parent (default to kk if no other neighbors found)
        int kkk = 0;
        // Look in j+1 direction
        for (int jjjj = jjj + 1; jjjj < MAX_NEIGHBORS; jjjj++) {
            if (neighbors[ii][jjjj] > 0) {
                if (neighbors[ii][jjjj] < numCells + 1) {
                    ini = ii;
                    fi = kk;
                    kkk = 1;
                    break;
                } else {
                    ini = kk;
                    fi = ii;
                    kkk = 1;
                    break;
                }
            }
        }
        if (kkk == 0) {
            // Wrap around to beginning
            for (int jjjj = 0; jjjj < jjj; jjjj++) {
                if (neighbors[ii][jjjj] > 0) {
                    if (neighbors[ii][jjjj] < numCells + 1) {
                        ini = ii;
                        fi = kk;
                        break;
                    } else {
                        ini = kk;
                        fi = ii;
                        break;
                    }
                }
            }
        }

        // Start traversal from ini
        // Fortran: lines 1234-1249
        int iii = ini;
        int cj = 0;  // Count of caught cells (0-based, Fortran uses 1-based)
        pillats[cj] = iii + 1;  // Store 1-based like Fortran

        // Find position where iii has jj (the new cell) as neighbor
        jjj = -1;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (newNeighbors[iii][j] == jj + 1) {
                jjj = j;
                break;
            }
        }

        // Find next neighbor in j+1 direction (following the chain around the new cell)
        kkk = 0;
        int iiii = -1;  // Next cell in chain
        for (int j = jjj + 1; j < MAX_NEIGHBORS; j++) {
            int jji = newNeighbors[iii][j];
            if (jji != 0 && jji < newNumCellsBound) {
                iiii = jji - 1;  // Convert to 0-based
                kkk = 1;
                break;
            }
        }
        if (kkk == 0) {
            // Wrap around
            for (int j = 0; j < jjj; j++) {
                int jji = newNeighbors[iii][j];
                if (jji != 0 && jji < newNumCellsBound) {
                    iiii = jji - 1;
                    kkk = 1;
                    break;
                }
            }
        }

        cj++;
        if (cj >= MAX_NEIGHBORS) {
            // PANIC: Traversal exceeded max neighbors
            panicFlag = 1.0;
            return;
        }
        pillats[cj] = iiii + 1;  // Store 1-based like Fortran

        // Find position in iiii where iii is a neighbor
        int jjjj = -1;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (newNeighbors[iiii][j] == iii + 1) {
                jjjj = j;
                break;
            }
        }

        // Main traversal loop (Fortran label 88)
        // Fortran: lines 1250-1403
        while (true) {
            iii = iiii;
            jjj = jjjj;

            // Find next neighbor in j+1 direction
            kkk = 0;
            for (int j = jjj + 1; j < MAX_NEIGHBORS; j++) {
                int jji = newNeighbors[iii][j];
                if (jji != 0 && jji < newNumCellsBound) {
                    iiii = jji - 1;
                    kkk = 1;
                    break;
                }
            }
            if (kkk == 0) {
                // Wrap around
                for (int j = 0; j < jjj; j++) {
                    int jji = newNeighbors[iii][j];
                    if (jji != 0 && jji < newNumCellsBound) {
                        iiii = jji - 1;
                        kkk = 1;
                        break;
                    }
                }
            }

            cj++;
            if (cj >= MAX_NEIGHBORS) {
                // PANIC: Traversal loop exceeded max neighbors
                panicFlag = 1.0;
                return;
            }
            pillats[cj] = iiii + 1;  // Store 1-based like Fortran

            // Find position in iiii where iii is a neighbor
            // Fortran: do j=1,nvmax ; if (cvei(iiii,j)==iii) then ; jjjj=j ; exit ; end if ; end do
            // Note: In Fortran, jjjj keeps its previous value if not found
            for (int j = 0; j < MAX_NEIGHBORS; j++) {
                if (newNeighbors[iiii][j] == iii + 1) {
                    jjjj = j;
                    break;
                }
            }

            // Check for "equinox" - when we reach the other parent (fi)
            // Fortran: lines 1259-1286
            if (iiii == fi) {
                kkk = 0;
                int sjj = -1;
                for (int kkkk = jjjj + 1; kkkk < MAX_NEIGHBORS; kkkk++) {
                    if (newNeighbors[iiii][kkkk] != 0 && kkk == 1) {
                        if (newNeighbors[iiii][kkkk] > newNumCells) {
                            iiii = ini;
                            kkk = 2;
                            cj++;
                            break;
                        } else {
                            sjj = kkkk;
                            kkk = 2;
                            break;
                        }
                    }
                    if (newNeighbors[iiii][kkkk] != 0 && kkk == 0) {
                        kkk = 1;
                        sjj = kkkk;
                    }
                }
                if (kkk < 2) {
                    for (int kkkk = 0; kkkk < jjjj; kkkk++) {
                        if (newNeighbors[iiii][kkkk] != 0 && kkk == 1) {
                            if (newNeighbors[iiii][kkkk] > newNumCells) {
                                iiii = ini;
                                cj++;
                                break;
                            } else {
                                sjj = kkkk;
                                break;
                            }
                        }
                        if (newNeighbors[iiii][kkkk] != 0 && kkk == 0) {
                            kkk = 1;
                            sjj = kkkk;
                        }
                    }
                }
                if (sjj >= 0) {
                    jjjj = sjj - 1;
                }
            }

            // Check if we've completed the loop (back at ini)
            // Fortran: lines 1289-1401
            if (iiii == ini) {
                cpillats.fill(0);
                if (cj >= MAX_NEIGHBORS) {
                    // PANIC: Traversal exceeded max neighbors
                    panicFlag = 1.0;
                    return;
                }
                pillats[cj] = 0;  // Remove last (it's ini again)
                cj--;

                // Reverse the pillats array
                for (int jjjIdx = 0; jjjIdx <= cj; jjjIdx++) {
                    cpillats[cj - jjjIdx] = pillats[jjjIdx];
                }
                for (int j = 0; j <= cj; j++) {
                    pillats[j] = cpillats[j];
                }

                // Count how many new cells are in pillats
                // pillats stores 1-based: original cells are 1 to numCells, new cells are numCells+1 to numCells+numNewCells
                int newCellCount = 0;
                for (int kkkIdx = 0; kkkIdx <= cj; kkkIdx++) {
                    int kkkk = pillats[kkkIdx];  // 1-based
                    if (kkkk > numCells && kkkk <= newNumCells) {
                        newCellCount++;
                    }
                }

                if (newCellCount == 0) {
                    // No new cells among neighbors - connect all caught cells
                    // Fortran: lines 1304-1330
                    tempNewNeighbors[i] = pillats;  // pillats is now 1-based

                    for (int j = 0; j < MAX_NEIGHBORS; j++) {
                        int k = tempNewNeighbors[i][j];  // 1-based
                        if (k != 0) {
                            int iiP = newNodePairs[i][0];   // 0-based
                            int iiiiP = newNodePairs[i][1]; // 0-based
                            int k0 = k - 1;  // Convert to 0-based for array access
                            // Check if k is not one of the parents and is a real cell
                            if (k != iiP + 1 && k != iiiiP + 1 && k <= newNumCells) {
                                // Find where to insert new cell in k's neighbor list
                                int kkkkN = 0;
                                int ij = -1, ji = -1;
                                for (int kkN = 0; kkN < MAX_NEIGHBORS && ji < 0; kkN++) {
                                    for (int kkkN = 0; kkkN <= cj; kkkN++) {
                                        if (newNeighbors[k0][kkN] != 0 &&
                                            newNeighbors[k0][kkN] == pillats[kkkN] && kkkkN == 1) {
                                            ji = kkN;
                                            break;
                                        }
                                        if (newNeighbors[k0][kkN] != 0 &&
                                            newNeighbors[k0][kkN] == pillats[kkkN] && kkkkN == 0) {
                                            kkkkN = 1;
                                            ij = kkN;
                                            break;
                                        }
                                    }
                                }

                                if (ij >= 0 && ji >= 0) {
                                    // Insert new cell between ij and ji
                                    if (ji - ij == 1) {
                                        // Shift neighbors to make room
                                        for (int kkN = MAX_NEIGHBORS - 1; kkN > ji; kkN--) {
                                            newNeighbors[k0][kkN] = newNeighbors[k0][kkN - 1];
                                            for (int mIdx = 3; mIdx < 8; mIdx++) {
                                                newMargins[k0][kkN][mIdx] = newMargins[k0][kkN - 1][mIdx];
                                            }
                                        }
                                        newNeighbors[k0][ji] = jj + 1;
                                    } else {
                                        newNeighbors[k0][ji + 1] = jj + 1;
                                    }
                                }
                            }
                        }
                    }
                    // No conversion needed - tempNewNeighbors already has 1-based values from pillats
                } else {
                    // There are new cells among neighbors
                    // Only keep ini, fi, and other new cells
                    // Fortran: lines 1331-1370
                    tempNewNeighbors[i].fill(0);
                    tempNewNeighbors[i][0] = ini + 1;
                    int kkkkN = 0;
                    for (int kkkIdx = 0; kkkIdx <= cj; kkkIdx++) {
                        int jjjjP = pillats[kkkIdx];  // 1-based
                        if (jjjjP == fi + 1) {  // fi is 0-based, jjjjP is 1-based
                            kkkkN++;
                            tempNewNeighbors[i][kkkkN] = fi + 1;
                        } else if (jjjjP > numCells) {  // New cells are numCells+1 to numCells+numNewCells in 1-based
                            kkkkN++;
                            tempNewNeighbors[i][kkkkN] = jjjjP;  // Already 1-based
                        }
                    }
                    // Note: Fortran has "goto 899" which skips the connection logic
                    // when there are new cells. We follow the same pattern.
                }

                // Add external cell connections if both parents have external neighbors
                // Fortran: lines 1372-1400
                int iiExt = newNodePairs[i][0];
                int kkExt = newNodePairs[i][1];
                kkk = 0;
                for (int j = 0; j < MAX_NEIGHBORS; j++) {
                    if (newNeighbors[iiExt][j] > newNumCells) {
                        kkk = 1;
                        break;
                    }
                }
                for (int j = 0; j < MAX_NEIGHBORS; j++) {
                    if (newNeighbors[kkExt][j] > newNumCells) {
                        kkk++;
                        break;
                    }
                }
                if (kkk == 2) {
                    // Both parents have external neighbors - add external to new cell
                    int ij = -1, jiExt = -1;
                    for (int j = 0; j < MAX_NEIGHBORS; j++) {
                        if (tempNewNeighbors[i][j] == iiExt + 1) {
                            ij = j;
                            break;
                        }
                    }
                    for (int j = 0; j < MAX_NEIGHBORS; j++) {
                        if (tempNewNeighbors[i][j] == kkExt + 1) {
                            jiExt = j;
                            break;
                        }
                    }
                    if (ij >= 0 && jiExt >= 0) {
                        if (ij > jiExt) {
                            int temp = ij;
                            ij = jiExt;
                            jiExt = temp;
                        }
                        if (jiExt - ij == 1) {
                            for (int kkN = MAX_NEIGHBORS - 1; kkN > jiExt; kkN--) {
                                tempNewNeighbors[i][kkN] = tempNewNeighbors[i][kkN - 1];
                                for (int mIdx = 3; mIdx < 8; mIdx++) {
                                    newMargins[numCells + i][kkN][mIdx] =
                                        newMargins[numCells + i][kkN - 1][mIdx];
                                }
                            }
                            tempNewNeighbors[i][jiExt] = newNumCellsTotal;
                        } else {
                            tempNewNeighbors[i][jiExt + 1] = newNumCellsTotal;
                        }
                    }
                }

                break;  // Exit the while loop - we've completed this new cell
            }
            // Continue the loop (goto 88 in Fortran)
        }
    }

    // Copy temporary neighbors to new cell positions
    // Fortran: line 1409
    for (int i = 0; i < numNewCells; i++) {
        newNeighbors[numCells + i] = tempNewNeighbors[i];
    }

    // Calculate distances for new cells
    // Fortran: lines 1416-1433
    for (int i = numCells; i < newNumCells; i++) {
        double ua = newPositions[i][0];
        double ub = newPositions[i][1];
        double uc = newPositions[i][2];

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int ii = newNeighbors[i][j];
            if (ii > 0 && ii < newNumCellsBound) {
                ii--;
                double ux = newPositions[ii][0] - ua;
                double uy = newPositions[ii][1] - ub;
                double uz = newPositions[ii][2] - uc;

                if (std::abs(ux) < 1e-13) ux = 0.0;
                if (std::abs(uy) < 1e-13) uy = 0.0;
                if (std::abs(uz) < 1e-13) uz = 0.0;

                double dBasal = std::sqrt(ux * ux + uy * uy);
                double d3d = std::sqrt(ux * ux + uy * uy + uz * uz);
                newMargins[i][j][4] = dBasal;
                newMargins[i][j][3] = d3d;
                newMargins[i][j][5] = ux;
                newMargins[i][j][6] = uy;
                newMargins[i][j][7] = uz;
            }
        }
    }

    // Calculate distances for old cells with new neighbors
    // Fortran: lines 1436-1453
    for (int i = 0; i < numCells; i++) {
        double ua = newPositions[i][0];
        double ub = newPositions[i][1];
        double uc = newPositions[i][2];

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int ii = newNeighbors[i][j];
            if (ii > numCells && ii < newNumCellsBound) {
                ii--;
                double ux = newPositions[ii][0] - ua;
                double uy = newPositions[ii][1] - ub;
                double uz = newPositions[ii][2] - uc;

                if (std::abs(ux) < 1e-13) ux = 0.0;
                if (std::abs(uy) < 1e-13) uy = 0.0;
                if (std::abs(uz) < 1e-13) uz = 0.0;

                double dBasal = std::sqrt(ux * ux + uy * uy);
                double d3d = std::sqrt(ux * ux + uy * uy + uz * uz);
                newMargins[i][j][4] = dBasal;
                newMargins[i][j][3] = d3d;
                newMargins[i][j][5] = ux;
                newMargins[i][j][6] = uy;
                newMargins[i][j][7] = uz;
            }
        }
    }

    // Update counts
    // Fortran: lines 1455-1457
    int oldNumCells = numCells;
    numCellsTotal = newNumCellsTotal;
    numCells = newNumCells;

    // Replace arrays
    cellPositions = std::move(newPositions);
    neighbors = std::move(newNeighbors);
    neighborCount = std::move(newNeighborCount);
    quantities2D = std::move(newQ2D);
    quantities3D = std::move(newQ3D);
    knotMarkers = std::move(newKnots);
    cellMargins = std::move(newMargins);

    // Resize other arrays
    positionDeltas.resize(numCellsTotal, {0.0, 0.0, 0.0});

    // Remove zeros from neighbor lists
    // Fortran: lines 1475-1483
    for (int i = 0; i < numCellsTotal; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] == 0) {
                for (int jj = j; jj < MAX_NEIGHBORS - 1; jj++) {
                    neighbors[i][jj] = neighbors[i][jj + 1];
                }
                neighbors[i][MAX_NEIGHBORS - 1] = 0;
            }
        }
    }

    // Update neighbor counts
    // Fortran: lines 1484-1490
    for (int i = 0; i < numCellsTotal; i++) {
        int count = 0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] > 0) count++;
        }
        neighborCount[i] = count;
    }

    // Handle external (border) cells - swap new border cells to front
    // Fortran: lines 1493-1546
    for (int iii = 0; iii < numNewCells; iii++) {
        if (isExternal[iii] == 1) {
            int ii = oldNumCells + iii;
            if (numBorderCells == centerCell) {
                centerCell = ii;
            }

            // Swap new cell with cell at numBorderCells position
            std::swap(neighbors[ii], neighbors[numBorderCells]);
            std::swap(neighborCount[ii], neighborCount[numBorderCells]);
            std::swap(cellPositions[ii], cellPositions[numBorderCells]);
            std::swap(quantities2D[ii], quantities2D[numBorderCells]);
            std::swap(quantities3D[ii], quantities3D[numBorderCells]);
            std::swap(cellMargins[ii], cellMargins[numBorderCells]);
            std::swap(knotMarkers[ii], knotMarkers[numBorderCells]);

            // Create temporary copy
            auto tempNeighbors = neighbors;

            // Update references: ii -> numBorderCells
            for (int iCell = 0; iCell < numCells; iCell++) {
                for (int j = 0; j < MAX_NEIGHBORS; j++) {
                    if (tempNeighbors[iCell][j] == ii + 1) {
                        neighbors[iCell][j] = numBorderCells + 1;
                    }
                }
            }

            // Update references: numBorderCells -> ii
            for (int iCell = 0; iCell < numCells; iCell++) {
                for (int j = 0; j < MAX_NEIGHBORS; j++) {
                    if (tempNeighbors[iCell][j] == numBorderCells + 1) {
                        neighbors[iCell][j] = ii + 1;
                    }
                }
            }

            numBorderCells++;
        }
    }

    if (numNewCells > 0) {
        markBorderCells();
    }
}

void ToothModel::markBorderCells() {
    // Mark new cells that are on the border between two border markers
    std::vector<int> newBorderCells(numBorderCells, 0);
    int numNew = 0;

    // Check for cells between anterior markers
    for (int i = 0; i < numBorderCells; i++) {
        // Skip certain indices (not scalable with radius - keeping original logic)
        if (i == 2 || i == 5) continue;

        // Check if already in marker list
        bool isMarker = false;
        for (int ii = 0; ii < numAnteriorMarkers; ii++) {
            if (anteriorMarkers[ii] == i + 1) {
                isMarker = true;
                break;
            }
        }
        if (isMarker) continue;

        int markerNeighborCount = 0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            // k is 1-based, ncils = numBorderCells + 1
            if (k > 0 && k <= numBorderCells) {
                for (int ii = 0; ii < numAnteriorMarkers; ii++) {
                    if (k == anteriorMarkers[ii]) {
                        markerNeighborCount++;
                        break;
                    }
                }
            }
        }

        if (markerNeighborCount >= 2) {
            newBorderCells[numNew++] = i + 1;
        }
    }

    // Add new anterior markers
    if (numNew > 0) {
        int oldCount = numAnteriorMarkers;
        anteriorMarkers.resize(numAnteriorMarkers + numNew);
        for (int i = 0; i < numNew; i++) {
            anteriorMarkers[oldCount + i] = newBorderCells[i];
        }
        numAnteriorMarkers += numNew;
    }

    // Same for posterior markers
    std::fill(newBorderCells.begin(), newBorderCells.end(), 0);
    numNew = 0;

    for (int i = 0; i < numBorderCells; i++) {
        if (i == 2 || i == 5) continue;

        bool isMarker = false;
        for (int ii = 0; ii < numPosteriorMarkers; ii++) {
            if (posteriorMarkers[ii] == i + 1) {
                isMarker = true;
                break;
            }
        }
        if (isMarker) continue;

        int markerNeighborCount = 0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            // k is 1-based, ncils = numBorderCells + 1
            if (k > 0 && k <= numBorderCells) {
                for (int ii = 0; ii < numPosteriorMarkers; ii++) {
                    if (k == posteriorMarkers[ii]) {
                        markerNeighborCount++;
                        break;
                    }
                }
            }
        }

        if (markerNeighborCount >= 2) {
            newBorderCells[numNew++] = i + 1;
        }
    }

    if (numNew > 0) {
        int oldCount = numPosteriorMarkers;
        posteriorMarkers.resize(numPosteriorMarkers + numNew);
        for (int i = 0; i < numNew; i++) {
            posteriorMarkers[oldCount + i] = newBorderCells[i];
        }
        numPosteriorMarkers += numNew;
    }
}

void ToothModel::increaseZDepth() {
    int oldZ = numZLevels;
    numZLevels++;

    for (int i = 0; i < numCellsTotal; i++) {
        quantities3D[i].resize(numZLevels);
        quantities3D[i][oldZ].fill(0.0);
    }
}
