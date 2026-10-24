// tooth_model_geometry.cpp
// Cell margin/shape calculation functions

#include "tooth_model.hpp"

// Records, for every occupied neighbour slot, which cells' positions make up its
// margin point (Fortran calculmarges): the cell itself, then the cells visited
// while walking around it from that neighbour. The walk is decided by the
// neighbour lists alone, so it is recorded once per topology and re-summed by
// calculateMargins() every iteration. The control flow below is the Fortran's,
// including its fall-backs for a walk that does not close (kl > 100).
void ToothModel::rebuildMarginWalks() {
    walkCell.clear();
    walkSlot.clear();
    walkOffset.clear();
    walkVertex.clear();

    // Upper bound for the neighbour-slot loops. They still skip empty slots
    // below it, so the bound is exact whether or not a list has holes.
    slotEnd.assign(numCells, 0);
    for (int i = 0; i < numCells; i++) {
        for (int j = MAX_NEIGHBORS - 1; j >= 0; j--) {
            if (neighbors[i][j] != 0) {
                slotEnd[i] = j + 1;
                break;
            }
        }
    }

    for (int i = 0; i < numCells; i++) {
        int kl = 0;

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] != 0) {
                walkCell.push_back(i);
                walkSlot.push_back(j);
                walkOffset.push_back(static_cast<int>(walkVertex.size()));

                int iii = i;
                int ii = neighbors[i][j] - 1;  // Convert to 0-based index

                if (ii >= numCells) {
                    // Boundary cell - look for previous non-boundary neighbor
                    for (int jj = j - 1; jj >= 0; jj--) {
                        if (neighbors[i][jj] != 0) {
                            if (neighbors[i][jj] <= numCells) {
                                walkVertex.push_back(neighbors[i][jj] - 1);
                                goto done_searching;
                            } else {
                                goto done_searching;
                            }
                        }
                    }
                    // Try the other direction
                    for (int jj = MAX_NEIGHBORS - 1; jj > j; jj--) {
                        if (neighbors[i][jj] != 0) {
                            if (neighbors[i][jj] <= numCells) {
                                walkVertex.push_back(neighbors[i][jj] - 1);
                                goto done_searching;
                            } else {
                                goto done_searching;
                            }
                        }
                    }
                    goto done_searching;
                }

                // Walk around the cell to collect the corners of the margin region
                {
                    int walkLimit = 100;
                    int walkCount = 0;

                    while (ii != i && walkCount < walkLimit) {
                        walkCount++;
                        kl++;

                        if (kl > 100) {
                            // Exceeded limit - fall back to simple neighbor average
                            for (int jj = j - 1; jj >= 0; jj--) {
                                if (neighbors[i][jj] != 0 && neighbors[i][jj] <= numCells) {
                                    walkVertex.push_back(neighbors[i][jj] - 1);
                                    goto done_searching;
                                }
                            }
                            for (int jj = MAX_NEIGHBORS - 1; jj > j; jj--) {
                                if (neighbors[i][jj] != 0 && neighbors[i][jj] <= numCells) {
                                    walkVertex.push_back(neighbors[i][jj] - 1);
                                    goto done_searching;
                                }
                            }
                            goto done_searching;
                        }

                        walkVertex.push_back(ii);

                        // Find which slot points back to iii
                        int jjj = -1;
                        for (int jj = 0; jj < MAX_NEIGHBORS; jj++) {
                            if (neighbors[ii][jj] == iii + 1) {
                                jjj = jj;
                                break;
                            }
                        }

                        // Find next neighbor in rotation
                        bool found = false;
                        for (int jj = jjj + 1; jj < MAX_NEIGHBORS; jj++) {
                            if (neighbors[ii][jj] != 0) {
                                if (neighbors[ii][jj] > numCells) {
                                    goto done_searching;
                                }
                                iii = ii;
                                ii = neighbors[iii][jj] - 1;
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            for (int jj = 0; jj < jjj; jj++) {
                                if (neighbors[ii][jj] != 0) {
                                    if (neighbors[ii][jj] > numCells) {
                                        goto done_searching;
                                    }
                                    iii = ii;
                                    ii = neighbors[iii][jj] - 1;
                                    break;
                                }
                            }
                        }
                    }
                }

            done_searching:;
            }
        }
    }

    walkOffset.push_back(static_cast<int>(walkVertex.size()));
    marginWalksValid = true;
}

void ToothModel::calculateMargins() {
    if (!marginWalksValid) rebuildMarginWalks();

    // The Fortran zeroes every slot's margin here (marge(:,:,1:3)=0). Occupied
    // slots are overwritten below, and of the empty slots only slot 0 is ever
    // read (reactionDiffusion's wrap-around edge), so that is the only one that
    // has to be reset.
    for (int i = 0; i < numCells; i++) {
        if (neighbors[i][0] == 0) {
            cellMargins[i][0][0] = 0.0;
            cellMargins[i][0][1] = 0.0;
            cellMargins[i][0][2] = 0.0;
        }
    }

    // Each margin point is the mean of the cell's position and the recorded
    // corner cells' positions, summed in the recorded (Fortran) order.
    const int nWalks = static_cast<int>(walkCell.size());
    for (int w = 0; w < nWalks; w++) {
        const int i = walkCell[w];
        double a = cellPositions[i][0];
        double b = cellPositions[i][1];
        double c = cellPositions[i][2];
        const int first = walkOffset[w];
        const int last = walkOffset[w + 1];
        for (int p = first; p < last; p++) {
            const int ii = walkVertex[p];
            a += cellPositions[ii][0];
            b += cellPositions[ii][1];
            c += cellPositions[ii][2];
        }
        const double cont = 1.0 + (last - first);
        MarginSlot& m = cellMargins[i][walkSlot[w]];
        m[0] = a / cont;
        m[1] = b / cont;
        m[2] = c / cont;
    }
}

std::vector<std::array<int, 3>> ToothModel::getTriangles() const {
    // Build clean neighbor lists (0-indexed, no boundary markers)
    std::vector<std::vector<int>> nlist(numCells);
    for (int i = 0; i < numCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            int n = neighbors[i][j];
            if (n > 0 && n <= numCells) {
                nlist[i].push_back(n - 1);  // Convert to 0-based
            }
        }
        std::sort(nlist[i].begin(), nlist[i].end());
    }

    // Find triangles using set intersection approach
    // Triangle exists when k is a neighbor of both i and j
    std::vector<std::array<int, 3>> triangles;

    // Reusable buffers for set operations (avoid per-iteration heap allocation)
    std::vector<int> common;
    std::vector<int> jOnly;

    for (int i = 0; i < numCells; i++) {
        for (int j : nlist[i]) {
            if (j <= i) continue;  // Avoid duplicates

            // Find common neighbors of i and j (set intersection)
            common.clear();
            std::set_intersection(
                nlist[i].begin(), nlist[i].end(),
                nlist[j].begin(), nlist[j].end(),
                std::back_inserter(common));

            for (int k : common) {
                if (k <= j) continue;  // Avoid duplicates
                triangles.push_back({i, j, k});
            }
        }
    }

    // Find quads (4-cycles without diagonals) and convert them to triangles.
    // Same quads as dad_to_polygons; only the splitting diagonal may differ.

    // True if a and b share a neighbour other than i and k.
    auto shareThirdNeighbour = [&nlist](int a, int b, int i, int k) {
        const auto& la = nlist[a];
        const auto& lb = nlist[b];
        size_t p = 0, q = 0;
        while (p < la.size() && q < lb.size()) {
            if (la[p] < lb[q]) {
                p++;
            } else if (lb[q] < la[p]) {
                q++;
            } else {
                if (la[p] != i && la[p] != k) return true;
                p++;
                q++;
            }
        }
        return false;
    };

    for (int i = 0; i < numCells; i++) {
        for (int j : nlist[i]) {
            if (j <= i) continue;

            // Find neighbors of j that are NOT neighbors of i
            jOnly.clear();
            std::set_difference(
                nlist[j].begin(), nlist[j].end(),
                nlist[i].begin(), nlist[i].end(),
                std::back_inserter(jOnly));

            for (int k : jOnly) {
                if (k == i) continue;

                // Find common neighbors of k and i (potential 4th vertex)
                common.clear();
                std::set_intersection(
                    nlist[k].begin(), nlist[k].end(),
                    nlist[i].begin(), nlist[i].end(),
                    std::back_inserter(common));

                for (int w : common) {
                    if (w == j) continue;

                    // Check that w and j are NOT connected (making it a quad, not 2 triangles)
                    bool wjConnected = std::binary_search(nlist[w].begin(), nlist[w].end(), j);
                    if (wjConnected) continue;

                    // dad_to_polygons' crossing check: a cell inside the cycle rejects the
                    // quad only if neither pair of opposite corners is clean
                    if (shareThirdNeighbour(w, j, i, k) && shareThirdNeighbour(i, k, j, w)) continue;

                    // Valid quad: i-j-k-w, split into two triangles
                    // Only add if this is the canonical ordering to avoid duplicates
                    int minVertex = std::min({i, j, k, w});
                    if (i == minVertex) {
                        triangles.push_back({i, j, k});
                        triangles.push_back({i, k, w});
                    }
                }
            }
        }
    }

    // Remove duplicate triangles (sort vertices in each triangle, then unique)
    for (auto& tri : triangles) {
        std::sort(tri.begin(), tri.end());
    }
    std::sort(triangles.begin(), triangles.end());
    triangles.erase(std::unique(triangles.begin(), triangles.end()), triangles.end());

    return triangles;
}
