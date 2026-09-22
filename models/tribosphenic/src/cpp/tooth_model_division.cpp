// tooth_model_division.cpp
// Cell division: addNewCells (Fortran afegircel)

#include "tooth_model.hpp"

namespace {

// First slot of `list` holding `value` (a 1-based cell index), or -1.
int findSlot(const std::array<int, MAX_NEIGHBORS>& list, int value) {
    for (int j = 0; j < MAX_NEIGHBORS; j++) {
        if (list[j] == value) return j;
    }
    return -1;
}

}   // namespace

// Removes the zeros from every neighbour list, keeping the order of the rest.
// Deliberately the Fortran's single pass, which leaves one zero of a run of two.
void ToothModel::compactNeighborLists() {
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
}

void ToothModel::addNewCells() {
    // Translated statement for statement from the Fortran subroutine afegircel,
    // quirks included (see TODO.md 12.2 F1 and F4). Neighbour lists hold 1-based
    // indices throughout; local cell indices are 0-based.

    // 1. Neighbour pairs that have drifted further apart than the division
    //    distance get a new cell between them. A pair of two border cells makes
    //    a border cell ("external").
    std::vector<std::array<int, 2>> newNodePairs;
    std::vector<int> isExternal;
    for (int i = 0; i < numCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int k = neighbors[i][j];
            // Fortran: if (k/=0.and.k>i.and.k<=ncels)
            if (k != 0 && k > i + 1 && k <= numCells) {
                k--;
                double ux = cellPositions[k][0] - cellPositions[i][0];
                double uy = cellPositions[k][1] - cellPositions[i][1];
                double uz = cellPositions[k][2] - cellPositions[i][2];
                double dist = std::sqrt(ux * ux + uy * uy + uz * uz);
                dist = std::round(dist * 1e9) * 1e-9;
                if (dist > MAX_DIVISION_DIST) {
                    // Not in the Fortran: create each pair once even if a
                    // neighbour list holds the same cell twice.
                    bool isDuplicate = false;
                    for (const auto& p : newNodePairs) {
                        if ((p[0] == i && p[1] == k) || (p[0] == k && p[1] == i)) {
                            isDuplicate = true;
                            break;
                        }
                    }
                    if (!isDuplicate) {
                        newNodePairs.push_back({i, k});
                        isExternal.push_back((i < numBorderCells && k < numBorderCells) ? 1 : 0);
                    }
                }
            }
        }
    }
    const int numNewCells = static_cast<int>(newNodePairs.size());
    if (numNewCells == 0) return;

    compactNeighborLists();

    const int newNumCellsTotal = numCellsTotal + numNewCells;
    const int newNumCells = numCells + numNewCells;
    const int newNumCellsBound = newNumCells + 1;    // 1-based values below this are real cells

    // 2. Rebuild every per-cell array with room for the new cells, which go
    //    between the old real cells and the virtual boundary cell.
    std::vector<std::array<double, 3>> newPositions(newNumCellsTotal, {0.0, 0.0, 0.0});
    std::vector<std::array<int, MAX_NEIGHBORS>> newNeighbors(newNumCellsTotal, std::array<int, MAX_NEIGHBORS>{});
    std::vector<std::array<double, NUM_2D_QUANTITIES>> newQ2D(newNumCellsTotal, std::array<double, NUM_2D_QUANTITIES>{});
    std::vector<std::vector<std::array<double, NUM_3D_QUANTITIES>>> newQ3D(newNumCellsTotal,
        std::vector<std::array<double, NUM_3D_QUANTITIES>>(numZLevels, std::array<double, NUM_3D_QUANTITIES>{}));
    std::vector<int> newKnots(newNumCellsTotal, 0);
    std::vector<CellMargins> newMargins(newNumCellsTotal, CellMargins{});
    std::vector<std::array<int, MAX_NEIGHBORS>> tempNewNeighbors(numNewCells, std::array<int, MAX_NEIGHBORS>{});  // Fortran ccvei

    // The boundary cells (indices >= numCells) shift up by numNewCells.
    for (int i = newNumCellsTotal - 1; i >= newNumCells; i--) {
        int oldIdx = i - numNewCells;
        newPositions[i] = cellPositions[oldIdx];
        newNeighbors[i] = neighbors[oldIdx];
        newQ2D[i] = quantities2D[oldIdx];
        newQ3D[i] = quantities3D[oldIdx];
        newKnots[i] = knotMarkers[oldIdx];
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            newMargins[i][j][MARGIN_REST] = cellMargins[oldIdx][j][MARGIN_REST];
        }
    }
    // The real cells stay where they are.
    for (int i = 0; i < numCells; i++) {
        newPositions[i] = cellPositions[i];
        newNeighbors[i] = neighbors[i];
        newQ2D[i] = quantities2D[i];
        newQ3D[i] = quantities3D[i];
        newKnots[i] = knotMarkers[i];
        newMargins[i] = cellMargins[i];
    }
    // References to the boundary cell get its new index.
    for (int i = 0; i < newNumCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (newNeighbors[i][j] > numCells) newNeighbors[i][j] = newNumCellsTotal;
        }
    }

    // 3. Each new cell starts out connected to its two parents, placed at their
    //    mean radius in the plane and mean height, with the mean of their
    //    quantities.
    for (int n = 0; n < numNewCells; n++) {
        const int ii = newNodePairs[n][0];
        const int kk = newNodePairs[n][1];
        const int jj = numCells + n;

        newNeighbors[jj][0] = ii + 1;
        newNeighbors[jj][1] = kk + 1;

        double a = newPositions[ii][0] + newPositions[kk][0];
        double b = newPositions[ii][1] + newPositions[kk][1];
        double d = std::sqrt(a * a + b * b);
        a = a / d;
        b = b / d;
        d = d / 2.0;                            // Fortran: d=d/0.20D1
        d = std::round(d * 1e10) * 1e-10;
        newPositions[jj][0] = d * a;
        newPositions[jj][1] = d * b;
        newPositions[jj][2] = (cellPositions[ii][2] + cellPositions[kk][2]) * 0.5;   // Fortran: *0.05D1

        for (int kz = 0; kz < numZLevels; kz++) {
            for (int q = 0; q < NUM_3D_QUANTITIES; q++) {
                newQ3D[jj][kz][q] = (newQ3D[ii][kz][q] + newQ3D[kk][kz][q]) * 0.5;
            }
        }
        for (int q = 0; q < NUM_2D_QUANTITIES; q++) {
            newQ2D[jj][q] = (newQ2D[ii][q] + newQ2D[kk][q]) * 0.5;
        }
    }

    // The parents now point at the new cell instead of at each other.
    for (int n = 0; n < numNewCells; n++) {
        const int ii = newNodePairs[n][0];
        const int kk = newNodePairs[n][1];
        const int jj = numCells + n;
        int s = findSlot(newNeighbors[ii], kk + 1);
        if (s >= 0) newNeighbors[ii][s] = jj + 1;
        s = findSlot(newNeighbors[kk], ii + 1);
        if (s >= 0) newNeighbors[kk][s] = jj + 1;
    }

    // 4. The remaining neighbours of each new cell: walk around it from one
    //    parent (ini) to the other (fi) through the cells adjacent to both,
    //    following each cell's neighbour list in order. Fortran lines 1207-1404.

    // First real cell (value below newNumCellsBound) in `cell`'s list after slot
    // `afterSlot`, wrapping around to the slots before it; -1 if there is none.
    auto nextRealNeighbor = [&](int cell, int afterSlot) -> int {
        for (int j = afterSlot + 1; j < MAX_NEIGHBORS; j++) {
            int v = newNeighbors[cell][j];
            if (v != 0 && v < newNumCellsBound) return v - 1;
        }
        for (int j = 0; j < afterSlot; j++) {
            int v = newNeighbors[cell][j];
            if (v != 0 && v < newNumCellsBound) return v - 1;
        }
        return -1;
    };

    // Insert `value` between slots ij < ji of a neighbour list. Consecutive slots:
    // shift the tail right by one (the rest lengths move with it; slot 29 is
    // lost). Otherwise the gap slot ji+1 is used -- without a bounds check when
    // ji == 29, exactly as in the Fortran (TODO.md 12.2 F1).
    auto insertBetween = [](std::array<int, MAX_NEIGHBORS>& list, CellMargins& margins,
                            int ij, int ji, int value) {
        if (ji - ij == 1) {
            for (int s = MAX_NEIGHBORS - 1; s > ji; s--) {
                list[s] = list[s - 1];
                margins[s][MARGIN_REST] = margins[s - 1][MARGIN_REST];
            }
            list[ji] = value;
        } else {
            list[ji + 1] = value;
        }
    };

    for (int n = 0; n < numNewCells; n++) {
        std::array<int, MAX_NEIGHBORS> pillats{};   // cells caught on the way round (1-based)
        const int ii = newNodePairs[n][0];
        const int kk = newNodePairs[n][1];
        const int jj = numCells + n;

        // Start from the parent that has a real (non-boundary) cell after the
        // other parent in its original neighbour list; end at the other one.
        int jjj = findSlot(neighbors[ii], kk + 1);
        int ini = ii;
        int fi = kk;
        {
            bool found = false;
            for (int j = jjj + 1; j < MAX_NEIGHBORS; j++) {
                if (neighbors[ii][j] > 0) {
                    if (neighbors[ii][j] < numCells + 1) { ini = ii; fi = kk; } else { ini = kk; fi = ii; }
                    found = true;
                    break;
                }
            }
            if (!found) {
                for (int j = 0; j < jjj; j++) {
                    if (neighbors[ii][j] > 0) {
                        if (neighbors[ii][j] < numCells + 1) { ini = ii; fi = kk; } else { ini = kk; fi = ii; }
                        break;
                    }
                }
            }
        }

        // First step: from ini to its first real neighbour after the new cell.
        int iii = ini;
        int cj = 0;
        pillats[cj] = iii + 1;

        jjj = findSlot(newNeighbors[iii], jj + 1);
        int iiii = nextRealNeighbor(iii, jjj);

        cj++;
        if (cj >= MAX_NEIGHBORS) { panicFlag = true; return; }
        pillats[cj] = iiii + 1;

        int jjjj = findSlot(newNeighbors[iiii], iii + 1);   // slot pointing back to ini

        // Main traversal (Fortran label 88)
        while (true) {
            iii = iiii;
            jjj = jjjj;

            int next = nextRealNeighbor(iii, jjj);
            if (next >= 0) iiii = next;             // Fortran: iiii keeps its value if none is found

            cj++;
            if (cj >= MAX_NEIGHBORS) { panicFlag = true; return; }
            pillats[cj] = iiii + 1;

            int s = findSlot(newNeighbors[iiii], iii + 1);
            if (s >= 0) jjjj = s;                    // Fortran: jjjj keeps its value if not found

            // "Equinox": arrived at the other parent. Continue from the slot after
            // the second non-zero entry following jjjj -- unless that entry is the
            // boundary, in which case the walk is complete (jump back to ini).
            if (iiii == fi) {
                int kkk = 0;
                int sjj = -1;
                for (int kkkk = jjjj + 1; kkkk < MAX_NEIGHBORS; kkkk++) {
                    if (newNeighbors[iiii][kkkk] != 0 && kkk == 1) {
                        if (newNeighbors[iiii][kkkk] > newNumCells) {
                            iiii = ini;
                            kkk = 2;
                            cj++;
                            break;
                        }
                        sjj = kkkk;
                        kkk = 2;
                        break;
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
                            }
                            sjj = kkkk;
                            break;
                        }
                        if (newNeighbors[iiii][kkkk] != 0 && kkk == 0) {
                            kkk = 1;
                            sjj = kkkk;
                        }
                    }
                }
                if (sjj >= 0) jjjj = sjj - 1;
            }

            if (iiii != ini) continue;              // goto 88

            // Back at ini: the ring is complete.
            if (cj >= MAX_NEIGHBORS) { panicFlag = true; return; }
            pillats[cj] = 0;                         // drop the repeated ini
            cj--;
            std::reverse(pillats.begin(), pillats.begin() + cj + 1);

            int newCellCount = 0;
            for (int idx = 0; idx <= cj; idx++) {
                int v = pillats[idx];
                if (v > numCells && v <= newNumCells) newCellCount++;
            }

            if (newCellCount == 0) {
                // Every caught cell becomes a neighbour of the new cell, and the
                // new cell is inserted into each of their lists between the two
                // caught cells that flank it there.
                tempNewNeighbors[n] = pillats;
                for (int j = 0; j < MAX_NEIGHBORS; j++) {
                    int k = tempNewNeighbors[n][j];
                    if (k == 0 || k == ii + 1 || k == kk + 1 || k > newNumCells) continue;
                    int k0 = k - 1;
                    // The first two slots of k's list that hold a caught cell
                    int ij = -1, ji = -1;
                    for (int kkN = 0; kkN < MAX_NEIGHBORS && ji < 0; kkN++) {
                        for (int kkkN = 0; kkkN <= cj; kkkN++) {
                            if (newNeighbors[k0][kkN] != 0 && newNeighbors[k0][kkN] == pillats[kkkN]) {
                                if (ij < 0) ij = kkN; else ji = kkN;
                                break;
                            }
                        }
                    }
                    if (ij >= 0 && ji >= 0) insertBetween(newNeighbors[k0], newMargins[k0], ij, ji, jj + 1);
                }
            } else {
                // Other new cells among the caught ones: keep only ini, fi and
                // the new cells. The Fortran skips the insertion into the other
                // caught cells' lists here (goto 899).
                tempNewNeighbors[n].fill(0);
                tempNewNeighbors[n][0] = ini + 1;
                int count = 0;
                for (int idx = 0; idx <= cj; idx++) {
                    int v = pillats[idx];
                    if (v == fi + 1) {
                        count++;
                        tempNewNeighbors[n][count] = fi + 1;
                    } else if (v > numCells) {
                        count++;
                        tempNewNeighbors[n][count] = v;
                    }
                }
            }

            // If both parents touch the boundary, so does the new cell: the
            // boundary goes between the parents in its list.
            bool iiExternal = false, kkExternal = false;
            for (int j = 0; j < MAX_NEIGHBORS; j++) {
                if (newNeighbors[ii][j] > newNumCells) iiExternal = true;
                if (newNeighbors[kk][j] > newNumCells) kkExternal = true;
            }
            if (iiExternal && kkExternal) {
                int ij = findSlot(tempNewNeighbors[n], ii + 1);
                int ji = findSlot(tempNewNeighbors[n], kk + 1);
                if (ij >= 0 && ji >= 0) {
                    if (ij > ji) std::swap(ij, ji);
                    insertBetween(tempNewNeighbors[n], newMargins[jj], ij, ji, newNumCellsTotal);
                }
            }
            break;
        }
    }

    for (int n = 0; n < numNewCells; n++) {
        newNeighbors[numCells + n] = tempNewNeighbors[n];
    }

    // 5. Rest lengths of the new connections: the basal (x, y) distance at creation.
    auto setRestLength = [&](int i, int j, int other) {
        double ux = newPositions[other][0] - newPositions[i][0];
        double uy = newPositions[other][1] - newPositions[i][1];
        if (std::abs(ux) < 1e-13) ux = 0.0;
        if (std::abs(uy) < 1e-13) uy = 0.0;
        newMargins[i][j][MARGIN_REST] = std::sqrt(ux * ux + uy * uy);
    };
    for (int i = numCells; i < newNumCells; i++) {          // new cells: every real neighbour
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int other = newNeighbors[i][j];
            if (other > 0 && other < newNumCellsBound) setRestLength(i, j, other - 1);
        }
    }
    for (int i = 0; i < numCells; i++) {                    // old cells: their new neighbours
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int other = newNeighbors[i][j];
            if (other > numCells && other < newNumCellsBound) setRestLength(i, j, other - 1);
        }
    }

    // 6. Swap in the new arrays.
    const int oldNumCells = numCells;
    numCellsTotal = newNumCellsTotal;
    numCells = newNumCells;
    cellPositions = std::move(newPositions);
    neighbors = std::move(newNeighbors);
    quantities2D = std::move(newQ2D);
    quantities3D = std::move(newQ3D);
    knotMarkers = std::move(newKnots);
    cellMargins = std::move(newMargins);
    positionDeltas.resize(numCellsTotal, {0.0, 0.0, 0.0});

    compactNeighborLists();

    // 7. A new cell born between two border cells is a border cell: swap it with
    //    the first interior cell so that the border cells stay at the front, and
    //    renumber the two in every neighbour list.
    for (int n = 0; n < numNewCells; n++) {
        if (!isExternal[n]) continue;
        const int ii = oldNumCells + n;
        const int b = numBorderCells;
        std::swap(neighbors[ii], neighbors[b]);
        std::swap(cellPositions[ii], cellPositions[b]);
        std::swap(quantities2D[ii], quantities2D[b]);
        std::swap(quantities3D[ii], quantities3D[b]);
        std::swap(cellMargins[ii], cellMargins[b]);
        std::swap(knotMarkers[ii], knotMarkers[b]);
        for (int c = 0; c < numCells; c++) {
            for (int j = 0; j < MAX_NEIGHBORS; j++) {
                int& v = neighbors[c][j];
                if (v == ii + 1) v = b + 1;
                else if (v == b + 1) v = ii + 1;
            }
        }
        numBorderCells++;
    }
}
