// tooth_model_geometry.cpp
// Cell margin/shape calculation functions

#include "tooth_model.hpp"

void ToothModel::calculateMargins() {
    // Reset margins to zero
    for (int i = 0; i < numCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            cellMargins[i][j][0] = 0.0;
            cellMargins[i][j][1] = 0.0;
            cellMargins[i][j][2] = 0.0;
        }
    }

    for (int i = 0; i < numCells; i++) {
        int kl = 0;

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (neighbors[i][j] != 0) {
                double a = 0.0, b = 0.0, c = 0.0;
                double cont = 0.0;

                int iii = i;
                a = cellPositions[i][0];
                b = cellPositions[i][1];
                c = cellPositions[i][2];
                cont = 1.0;

                int ii = neighbors[i][j] - 1;  // Convert to 0-based index

                if (ii >= numCells) {
                    // Boundary cell - look for previous non-boundary neighbor
                    for (int jj = j - 1; jj >= 0; jj--) {
                        if (neighbors[i][jj] != 0) {
                            if (neighbors[i][jj] <= numCells) {
                                ii = neighbors[i][jj] - 1;
                                a += cellPositions[ii][0];
                                b += cellPositions[ii][1];
                                c += cellPositions[ii][2];
                                cont += 1.0;
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
                                ii = neighbors[i][jj] - 1;
                                a += cellPositions[ii][0];
                                b += cellPositions[ii][1];
                                c += cellPositions[ii][2];
                                cont += 1.0;
                                goto done_searching;
                            } else {
                                goto done_searching;
                            }
                        }
                    }
                    goto done_searching;
                }

                // Walk around the cell to compute centroid of the margin region
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
                                    ii = neighbors[i][jj] - 1;
                                    a += cellPositions[ii][0];
                                    b += cellPositions[ii][1];
                                    c += cellPositions[ii][2];
                                    cont += 1.0;
                                    goto done_searching;
                                }
                            }
                            for (int jj = MAX_NEIGHBORS - 1; jj > j; jj--) {
                                if (neighbors[i][jj] != 0 && neighbors[i][jj] <= numCells) {
                                    ii = neighbors[i][jj] - 1;
                                    a += cellPositions[ii][0];
                                    b += cellPositions[ii][1];
                                    c += cellPositions[ii][2];
                                    cont += 1.0;
                                    goto done_searching;
                                }
                            }
                            goto done_searching;
                        }

                        a += cellPositions[ii][0];
                        b += cellPositions[ii][1];
                        c += cellPositions[ii][2];
                        cont += 1.0;

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

            done_searching:
                cellMargins[i][j][0] = a / cont;
                cellMargins[i][j][1] = b / cont;
                cellMargins[i][j][2] = c / cont;
            }
        }
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

    for (int i = 0; i < numCells; i++) {
        for (int j : nlist[i]) {
            if (j <= i) continue;  // Avoid duplicates

            // Find common neighbors of i and j (set intersection)
            std::vector<int> common;
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

    // Find quads (4 cells where opposite pairs aren't directly connected)
    // and convert them to triangles
    for (int i = 0; i < numCells; i++) {
        for (int j : nlist[i]) {
            if (j <= i) continue;

            // Find neighbors of j that are NOT neighbors of i
            std::vector<int> jOnly;
            std::set_difference(
                nlist[j].begin(), nlist[j].end(),
                nlist[i].begin(), nlist[i].end(),
                std::back_inserter(jOnly));

            for (int k : jOnly) {
                if (k == i) continue;

                // Find common neighbors of k and i (potential 4th vertex)
                std::vector<int> common;
                std::set_intersection(
                    nlist[k].begin(), nlist[k].end(),
                    nlist[i].begin(), nlist[i].end(),
                    std::back_inserter(common));

                for (int w : common) {
                    if (w == j) continue;

                    // Check that w and j are NOT connected (making it a quad, not 2 triangles)
                    bool wjConnected = std::binary_search(nlist[w].begin(), nlist[w].end(), j);
                    if (wjConnected) continue;

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
