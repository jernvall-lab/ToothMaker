// file_io.cpp
// File I/O operations for ToothModel

#include "tooth_model.hpp"

//=============================================================================
// FileIO Implementation
//=============================================================================

FileIO::FileIO(ToothModel* modelPtr) :
    model(modelPtr),
    snapshotIndex(0), exitFlag(0), passFlag(0),
    materialMax(0.0), materialMin(0.0)
{
    positionHistory.resize(MAX_SNAPSHOTS);
    parameterHistory.resize(MAX_SNAPSHOTS);
    for (auto& p : parameterHistory) p.fill(0.0);
    parameterNames.resize(32);
    knotsHistory.resize(MAX_SNAPSHOTS);
    neighborHistory.resize(MAX_SNAPSHOTS);
}

void FileIO::saveParameters(std::ostream& out) {
    parameterHistory[snapshotIndex][0] = model->totalIterations * (model->iterationIndex - 1) + model->timeStep;

    out << std::fixed << std::setprecision(6);
    for (int i = 0; i < 5; i++) out << std::setw(15) << parameterHistory[snapshotIndex][i];
    out << "\n";
    for (int i = 5; i < 10; i++) out << std::setw(15) << parameterHistory[snapshotIndex][i];
    out << "\n";
    for (int i = 10; i < 15; i++) out << std::setw(15) << parameterHistory[snapshotIndex][i];
    out << "\n";
    for (int i = 15; i < 20; i++) out << std::setw(15) << parameterHistory[snapshotIndex][i];
    out << "\n";
    for (int i = 20; i < 25; i++) out << std::setw(15) << parameterHistory[snapshotIndex][i];
    out << "\n";
    for (int i = 25; i < 30; i++) out << std::setw(15) << parameterHistory[snapshotIndex][i];
    out << "\n";
    out << model->numZLevels << " " << model->numBorderCells << "\n";
}

void FileIO::saveMorphology(std::ostream& out) {
    out << model->timeStep << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        out << model->cellPositions[i][0] << " "
            << model->cellPositions[i][1] << " "
            << model->cellPositions[i][2] << "\n";
    }
}

void FileIO::saveConcentrations(std::ostream& out) {
    out << NUM_3D_QUANTITIES << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        for (int j = 0; j < model->numZLevels; j++) {
            for (int k = 0; k < NUM_3D_QUANTITIES; k++) {
                out << model->quantities3D[i][j][k] << " ";
            }
            out << "\n";
        }
    }
}

void FileIO::saveExtraData(std::ostream& out) {
    out << NUM_2D_QUANTITIES << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        for (int k = 0; k < NUM_2D_QUANTITIES; k++) {
            out << model->quantities2D[i][k] << " ";
        }
        out << "\n";
    }
}

void FileIO::saveKnots(std::ostream& out) {
    out << model->timeStep << " " << model->numCells << "\n";
    int knotCount = 0;
    for (int i = 0; i < model->numCells; i++) {
        if (model->knotMarkers[i] == 1) knotCount++;
    }
    out << knotCount << "\n";
    for (int i = 0; i < model->numCells; i++) {
        if (model->knotMarkers[i] == 1) {
            out << (i + 1) << "\n";  // 1-based index for compatibility
        }
    }
}

void FileIO::saveNeighbors(std::ostream& out, const std::vector<std::array<int, MAX_NEIGHBORS>>& cellNeighbors) {
    out << model->timeStep << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        int count = 0;
        std::array<int, MAX_NEIGHBORS> compactNeighbors;
        compactNeighbors.fill(0);

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (cellNeighbors[i][j] != 0) {
                compactNeighbors[count++] = cellNeighbors[i][j];
            }
        }
        out << count << "\n";
        for (int j = 0; j < count; j++) {
            out << compactNeighbors[j] << " ";
        }
        out << "\n";
    }
}

void FileIO::saveMargins(std::ostream& out, const std::vector<std::array<int, MAX_NEIGHBORS>>& cellNeighbors) {
    out << model->timeStep << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        int count = 0;
        std::vector<std::array<double, 8>> compactMargins;

        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (cellNeighbors[i][j] != 0) {
                compactMargins.push_back(model->cellMargins[i][j]);
                count++;
            }
        }
        out << count << " cell shape\n";
        for (int k = 0; k < count; k++) {
            out << compactMargins[k][0] << " "
                << compactMargins[k][1] << " "
                << compactMargins[k][2] << "\n";
        }
    }
}

void FileIO::readParametersText(std::istream& in) {
    std::string line;
    // Fortran reads parap(3:32), C++ uses [2:31] (inclusive)
    // Fortran parap(n) -> C++ [n-1]
    for (int i = 2; i <= 31; i++) {
        double value;
        std::string name;
        if (in >> value >> name) {
            parameterHistory[snapshotIndex][i] = value;
            parameterNames[i] = name;
        } else {
            break;
        }
    }
}

bool FileIO::isToothMakerFormat(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;

    std::string line;
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Check for ToothMaker format identifier: "model==Tribosphenic tooth"
        // Case-insensitive for "model", case-sensitive for model name
        size_t pos = line.find("==");
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 2);

            // Convert key to lowercase for comparison
            std::string keyLower = key;
            for (char& c : keyLower) c = std::tolower(c);

            if (keyLower == "model" && value == "Tribosphenic tooth") {
                file.close();
                return true;
            }
        }
        // If first non-comment line doesn't match, it's humppa format
        break;
    }
    file.close();
    return false;
}

int FileIO::getParameterIndex(const std::string& name) {
    // Map ToothMaker parameter names to parameterHistory indices
    // Based on what loadParameters() expects at each index
    // See README.md "Parameter Naming Confusion" section for details

    // Convert to lowercase for case-insensitive comparison
    std::string nameLower = name;
    for (char& c : nameLower) c = std::tolower(c);

    if (nameLower == "egr") return 2;
    if (nameLower == "mgr") return 3;
    if (nameLower == "rep") return 4;
    if (nameLower == "swi") return 5;
    if (nameLower == "adh") return 6;
    if (nameLower == "act") return 7;
    if (nameLower == "inh") return 8;
    if (nameLower == "not2") return 9;
    if (nameLower == "sec") return 10;
    if (nameLower == "not3") return 11;
    if (nameLower == "da") return 12;    // diffusionCoeffs3D[0]
    if (nameLower == "di") return 13;    // diffusionCoeffs3D[1]
    if (nameLower == "ds") return 14;    // diffusionCoeffs3D[2]
    if (nameLower == "not4") return 15;  // diffusionCoeffs3D[3]
    if (nameLower == "int") return 16;
    if (nameLower == "set") return 17;
    if (nameLower == "boy") return 18;   // diffusionCoeffs2D[1] - buoyancy
    if (nameLower == "dff") return 19;   // borderWidth (misnaming) - actually differentiation rate
    if (nameLower == "bgr") return 20;   // biasFactor
    if (nameLower == "pbi") return 21;
    if (nameLower == "abi") return 22;
    if (nameLower == "bbi") return 23;
    if (nameLower == "lbi") return 24;
    if (nameLower == "rad") return 25;
    if (nameLower == "deg") return 26;
    if (nameLower == "dgr") return 27;
    if (nameLower == "ntr") return 28;
    if (nameLower == "bwi") return 29;   // biasCenterRadius (XML mapping issue, preserved)
    if (nameLower == "ina") return 30;
    if (nameLower == "umgr") return 31;  // basalMesenchymalRate

    return -1;  // Unknown parameter
}

void FileIO::readParametersToothMaker(std::istream& in) {
    // Initialize all parameters to zero
    for (int i = 2; i <= 31; i++) {
        parameterHistory[snapshotIndex][i] = 0.0;
        parameterNames[i] = "";
    }

    // Set default parameter names (for writeParametersText compatibility)
    parameterNames[2] = "Egr"; parameterNames[3] = "Mgr"; parameterNames[4] = "Rep";
    parameterNames[5] = "Swi"; parameterNames[6] = "Adh"; parameterNames[7] = "Act";
    parameterNames[8] = "Inh"; parameterNames[9] = "Not2"; parameterNames[10] = "Sec";
    parameterNames[11] = "Not3"; parameterNames[12] = "Da"; parameterNames[13] = "Di";
    parameterNames[14] = "Ds"; parameterNames[15] = "Not4"; parameterNames[16] = "Int";
    parameterNames[17] = "Set"; parameterNames[18] = "Boy"; parameterNames[19] = "Dff";
    parameterNames[20] = "Bgr"; parameterNames[21] = "Pbi"; parameterNames[22] = "Abi";
    parameterNames[23] = "Bbi"; parameterNames[24] = "Lbi"; parameterNames[25] = "Rad";
    parameterNames[26] = "Deg"; parameterNames[27] = "Dgr"; parameterNames[28] = "Ntr";
    parameterNames[29] = "Bwi"; parameterNames[30] = "Ina"; parameterNames[31] = "uMgr";

    std::string line;
    while (std::getline(in, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Parse name==value format
        size_t pos = line.find("==");
        if (pos == std::string::npos) continue;

        std::string name = line.substr(0, pos);
        std::string valueStr = line.substr(pos + 2);

        // Trim whitespace from name and value
        while (!name.empty() && std::isspace(name.back())) name.pop_back();
        while (!valueStr.empty() && std::isspace(valueStr.front())) valueStr.erase(0, 1);

        // Skip non-parameter keywords (model, viewthresh, viewmode, iter)
        std::string nameLower = name;
        for (char& c : nameLower) c = std::tolower(c);
        if (nameLower == "model" || nameLower == "viewthresh" ||
            nameLower == "viewmode" || nameLower == "iter") {
            continue;
        }

        // Get parameter index and store value
        int index = getParameterIndex(name);
        if (index >= 0) {
            try {
                double value = std::stod(valueStr);
                parameterHistory[snapshotIndex][index] = value;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse value for parameter " << name << std::endl;
            }
        } else {
            std::cerr << "Warning: Unknown parameter name: " << name << std::endl;
        }
    }
}

void FileIO::writeParametersText(std::ostream& out) {
    // Fortran writes parap(3:32), C++ uses [2:31] (inclusive)
    for (int i = 2; i <= 31; i++) {
        out << parameterHistory[snapshotIndex][i] << " " << parameterNames[i] << "\n";
    }
    out << model->numCells << " number of cells\n";
    out << model->timeStep << " number of iterations\n";
}

// NOTE: Part of unimplemented DAD file import feature. Not called from CLI, untested.
void FileIO::readParametersBinary(std::istream& in) {
    std::cout << "Reading parameters..." << std::endl;
    for (int i = 0; i < 5; i++) in >> parameterHistory[snapshotIndex][i];
    for (int i = 5; i < 10; i++) in >> parameterHistory[snapshotIndex][i];
    for (int i = 10; i < 15; i++) in >> parameterHistory[snapshotIndex][i];
    for (int i = 15; i < 20; i++) in >> parameterHistory[snapshotIndex][i];
    for (int i = 20; i < 25; i++) in >> parameterHistory[snapshotIndex][i];
    for (int i = 25; i < 30; i++) in >> parameterHistory[snapshotIndex][i];
    in >> parameterHistory[snapshotIndex][30];
    in >> parameterHistory[snapshotIndex][31];
}

// NOTE: Part of unimplemented DAD file import feature. Not called from CLI, untested.
void FileIO::readMorphology(std::istream& in) {
    int tempTime, tempCells;
    if (!(in >> tempTime >> tempCells)) {
        std::cerr << "Error reading morphology header" << std::endl;
        exitFlag = 1;
        return;
    }
    model->timeStep = tempTime;
    model->numCells = tempCells;

    for (int i = 0; i < model->numCells; i++) {
        if (!(in >> model->cellPositions[i][0]
                 >> model->cellPositions[i][1]
                 >> model->cellPositions[i][2])) {
            std::cerr << "Error reading morphology data" << std::endl;
            exitFlag = 1;
            return;
        }
    }
}

// NOTE: Part of unimplemented DAD file import feature. Not called from CLI, untested.
void FileIO::readExtraData(std::istream& in) {
    int tempTime, tempCells;
    if (!(in >> tempTime >> tempCells)) {
        std::cerr << "Error reading extra data header" << std::endl;
        exitFlag = 1;
        return;
    }

    for (int i = 0; i < model->numCells; i++) {
        for (int k = 0; k < NUM_3D_QUANTITIES; k++) {
            if (!(in >> model->quantities3D[i][0][k])) {
                std::cerr << "Error reading extra data" << std::endl;
                exitFlag = 1;
                return;
            }
        }
    }
}

// NOTE: Part of unimplemented DAD file import feature. Not called from CLI, untested.
void FileIO::readKnots(std::istream& in) {
    int tempTime, tempCells;
    if (!(in >> tempTime >> tempCells)) {
        std::cerr << "Error reading knots header" << std::endl;
        exitFlag = 1;
        return;
    }

    int knotCount;
    if (!(in >> knotCount)) {
        std::cerr << "Error reading knot count" << std::endl;
        exitFlag = 1;
        return;
    }

    for (int i = 0; i < knotCount; i++) {
        int knotIdx;
        if (!(in >> knotIdx)) {
            std::cerr << "Error reading knot index" << std::endl;
            exitFlag = 1;
            return;
        }
        model->knotMarkers[knotIdx - 1] = 1;  // Convert to 0-based
    }
}

// NOTE: Part of unimplemented DAD file import feature. Not called from CLI, untested.
void FileIO::readNeighbors(std::istream& in) {
    int tempTime, tempCells;
    if (!(in >> tempTime >> tempCells)) {
        std::cerr << "Error reading neighbors header" << std::endl;
        exitFlag = 1;
        return;
    }

    for (auto& n : model->neighbors) n.fill(0);

    for (int i = 0; i < model->numCells; i++) {
        int count;
        if (!(in >> count)) {
            std::cerr << "Error reading neighbor count" << std::endl;
            exitFlag = 1;
            return;
        }
        for (int j = 0; j < count; j++) {
            if (!(in >> model->neighbors[i][j])) {
                std::cerr << "Error reading neighbor data" << std::endl;
                exitFlag = 1;
                return;
            }
        }
    }
}

void FileIO::storeParameters(int snapshotIdx) {
    // C++ uses 0-based indexing, Fortran parap uses 1-based
    // Fortran parap(1:30) maps to C++ [0:29]
    // saveParameters writes [0..29] in groups of 5, matching Fortran parap(1:5), (6:10), etc.
    parameterHistory[snapshotIdx][0] = model->timeStep;                  // parap(1) = temps
    parameterHistory[snapshotIdx][1] = model->numCells;                  // parap(2) = ncels
    parameterHistory[snapshotIdx][2] = model->epithelialGrowthRate;      // parap(3) = tacre
    parameterHistory[snapshotIdx][3] = model->mesenchymalGrowthRate;     // parap(4) = tahor
    parameterHistory[snapshotIdx][4] = model->stiffness;                 // parap(5) = elas
    parameterHistory[snapshotIdx][5] = model->borderDistance;            // parap(6) = tadi
    parameterHistory[snapshotIdx][6] = model->neighborTraction;          // parap(7) = crema
    parameterHistory[snapshotIdx][7] = model->activatorAutoActivation;   // parap(8) = acac
    parameterHistory[snapshotIdx][8] = model->activatorInhibition;       // parap(9) = ihac
    parameterHistory[snapshotIdx][9] = model->ectodinRate;               // parap(10) = acec
    parameterHistory[snapshotIdx][10] = model->growthFactorSecretion;    // parap(11) = ih
    parameterHistory[snapshotIdx][11] = model->not3Rate;                 // parap(12) = acaca

    // Diffusion coefficients: parap(13-17) = difq3d(1-5) -> C++ [12-16]
    for (int j = 0; j < NUM_3D_QUANTITIES; j++) {
        parameterHistory[snapshotIdx][12 + j] = model->diffusionCoeffs3D[j];
    }
    // 2D diffusion coefficients: parap(18-21) = difq2d(1-4) -> C++ [17-20]
    // NOTE: difq2d(2) at [18] is GUI "Boy" (buoyancy), not a diffusion coefficient!
    for (int j = 0; j < NUM_2D_QUANTITIES; j++) {
        parameterHistory[snapshotIdx][17 + j] = model->diffusionCoeffs2D[j];
    }

    // Note: Original Fortran has overlapping indices - these are preserved
    parameterHistory[snapshotIdx][16] = model->diffThresholdInt;         // parap(17) = us (overlaps difq3d(5))
    parameterHistory[snapshotIdx][17] = model->diffThresholdSet;         // parap(18) = ud (overlaps difq2d(1))
    parameterHistory[snapshotIdx][21] = model->biasPosterior;            // parap(22) = bip
    parameterHistory[snapshotIdx][22] = model->biasAnterior;             // parap(23) = bia
    parameterHistory[snapshotIdx][23] = model->biasBuccal;               // parap(24) = bib
    parameterHistory[snapshotIdx][24] = model->biasLingual;              // parap(25) = bil
    parameterHistory[snapshotIdx][25] = model->radius;                   // parap(26) = radi
    parameterHistory[snapshotIdx][26] = model->degradationRate;          // parap(27) = mu
    parameterHistory[snapshotIdx][27] = model->sharpnessMax;             // parap(28) = tazmax
    parameterHistory[snapshotIdx][28] = model->nucleusTraction;          // parap(29) = radibi
    parameterHistory[snapshotIdx][19] = model->borderWidth;              // parap(20) = tadif - GUI "Dff" (differentiation rate!)
    parameterHistory[snapshotIdx][20] = model->biasFactor;               // parap(21) = fac (overlaps difq2d(4))
    parameterHistory[snapshotIdx][29] = model->biasCenterRadius;         // parap(30) = radibii - GUI "Bwi" maps here (XML error)
    parameterHistory[snapshotIdx][30] = model->initialActivator;         // parap(31) = ina
    parameterHistory[snapshotIdx][31] = model->basalMesenchymalRate;     // parap(32) = umgr

    // Fortran agafarparap overwrites parap(12) with ncals at the end (line 1918)
    // This is intentional in the original code - ncals replaces acaca
    // parap(12) -> C++ [11]
    parameterHistory[snapshotIdx][11] = model->numCellsTotal;
}

void FileIO::loadParameters(int snapshotIdx) {
    // C++ uses 0-based indexing, Fortran parap uses 1-based
    // Fortran parap(1:30) maps to C++ [0:29]
    model->timeStep = static_cast<int>(parameterHistory[snapshotIdx][0]);   // parap(1)
    model->numCells = static_cast<int>(parameterHistory[snapshotIdx][1]);   // parap(2)
    model->epithelialGrowthRate = parameterHistory[snapshotIdx][2];         // parap(3) = tacre
    model->mesenchymalGrowthRate = parameterHistory[snapshotIdx][3];        // parap(4) = tahor
    model->stiffness = parameterHistory[snapshotIdx][4];                    // parap(5) = elas
    model->borderDistance = parameterHistory[snapshotIdx][5];               // parap(6) = tadi
    model->neighborTraction = parameterHistory[snapshotIdx][6];             // parap(7) = crema
    model->activatorAutoActivation = parameterHistory[snapshotIdx][7];      // parap(8) = acac
    model->activatorInhibition = parameterHistory[snapshotIdx][8];          // parap(9) = ihac
    model->ectodinRate = parameterHistory[snapshotIdx][9];                  // parap(10) = acec
    model->growthFactorSecretion = parameterHistory[snapshotIdx][10];       // parap(11) = ih
    model->not3Rate = parameterHistory[snapshotIdx][11];                    // parap(12) = acaca

    // Diffusion coefficients: parap(13-17) = difq3d(1-5) -> C++ [12-16]
    for (int j = 0; j < NUM_3D_QUANTITIES; j++) {
        model->diffusionCoeffs3D[j] = parameterHistory[snapshotIdx][12 + j];
    }
    // 2D diffusion coefficients: parap(18-21) = difq2d(1-4) -> C++ [17-20]
    // NOTE: difq2d(2) at [18] is GUI "Boy" (buoyancy), not a diffusion coefficient!
    for (int j = 0; j < NUM_2D_QUANTITIES; j++) {
        model->diffusionCoeffs2D[j] = parameterHistory[snapshotIdx][17 + j];
    }

    // Note: Original Fortran has overlapping indices - these are preserved
    model->diffThresholdInt = parameterHistory[snapshotIdx][16];            // parap(17) = us
    model->diffThresholdSet = parameterHistory[snapshotIdx][17];            // parap(18) = ud
    model->biasPosterior = parameterHistory[snapshotIdx][21];               // parap(22) = bip
    model->biasAnterior = parameterHistory[snapshotIdx][22];                // parap(23) = bia
    model->biasBuccal = parameterHistory[snapshotIdx][23];                  // parap(24) = bib
    model->biasLingual = parameterHistory[snapshotIdx][24];                 // parap(25) = bil
    model->radius = static_cast<int>(parameterHistory[snapshotIdx][25]);    // parap(26) = radi
    model->degradationRate = parameterHistory[snapshotIdx][26];             // parap(27) = mu
    model->sharpnessMax = parameterHistory[snapshotIdx][27];                // parap(28) = tazmax
    model->nucleusTraction = parameterHistory[snapshotIdx][28];             // parap(29) = radibi
    model->borderWidth = parameterHistory[snapshotIdx][19];                 // parap(20) = tadif - GUI "Dff" (differentiation rate!)
    model->biasFactor = parameterHistory[snapshotIdx][20];                  // parap(21) = fac
    model->biasCenterRadius = parameterHistory[snapshotIdx][29];            // parap(30) = radibii - GUI "Bwi" maps here (XML error)
    model->initialActivator = parameterHistory[snapshotIdx][30];            // parap(31) = ina
    model->basalMesenchymalRate = parameterHistory[snapshotIdx][31];        // parap(32) = umgr
}

// NOTE: Part of unimplemented DAD file import feature. Not called from CLI, untested.
// Corresponds to Fortran 'llegir' subroutine for loading saved simulations.
void FileIO::readDataFile() {
    if (passFlag == 0) {
        neighborHistory.resize(MAX_SNAPSHOTS);
        exitFlag = 0;
        passFlag = 1;
    }

    // Clear history arrays
    for (auto& p : positionHistory) p.clear();
    for (auto& p : parameterHistory) p.fill(0.0);
    for (auto& n : neighborHistory) n.clear();
    for (auto& k : knotsHistory) k.clear();

    std::ifstream file(argInputFile);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << argInputFile << std::endl;
        return;
    }

    int oldNumCells = model->numCells;

    for (snapshotIndex = 0; snapshotIndex < MAX_SNAPSHOTS; snapshotIndex++) {
        oldNumCells = model->numCells;
        readParametersText(file);
        loadParameters(snapshotIndex);

        if (exitFlag == 1) break;

        if (model->numCells != oldNumCells) {
            model->numCellsTotal = model->numCells;
            model->cellPositions.resize(model->numCells, {0.0, 0.0, 0.0});
            model->neighbors.resize(model->numCells);
            for (auto& n : model->neighbors) n.fill(0);
            model->knotMarkers.resize(model->numCells, 0);
        }

        readNeighbors(file);
        neighborHistory[snapshotIndex].resize(model->numCells);
        for (int i = 0; i < model->numCells; i++) {
            neighborHistory[snapshotIndex][i] = model->neighbors[i];
        }
        if (exitFlag == 1) break;

        readKnots(file);
        knotsHistory[snapshotIndex].resize(model->numCells);
        for (int i = 0; i < model->numCells; i++) {
            knotsHistory[snapshotIndex][i] = model->knotMarkers[i];
        }
        if (exitFlag == 1) break;

        readMorphology(file);
        positionHistory[snapshotIndex].resize(model->numCells);
        for (int i = 0; i < model->numCells; i++) {
            positionHistory[snapshotIndex][i] = model->cellPositions[i];
        }
        if (exitFlag == 1) break;
    }

    file.close();

    snapshotIndex = 0;
    loadParameters(snapshotIndex);

    model->deallocateAll();
    model->numCellsTotal = model->numCells;
    model->reallocate();

    for (int i = 0; i < model->numCells; i++) {
        model->knotMarkers[i] = knotsHistory[snapshotIndex][i];
        model->cellPositions[i] = positionHistory[snapshotIndex][i];
        model->neighbors[i] = neighborHistory[snapshotIndex][i];
    }

    // Determine number of border cells
    // Count cells that have at least one boundary neighbor (matches Fortran lines 2012-2017)
    model->numBorderCells = 0;
    for (int i = 0; i < model->numCells; i++) {
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (model->neighbors[i][j] >= model->numCellsTotal) {
                model->numBorderCells++;
                break;
            }
        }
    }

    passFlag = 1;
}

void FileIO::readInitialParameters() {
    // Detect file format and use appropriate reader
    bool useToothMakerFormat = isToothMakerFormat(argInputFile);

    std::ifstream file(argInputFile);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << argInputFile << std::endl;
        return;
    }

    snapshotIndex = 0;

    if (useToothMakerFormat) {
        readParametersToothMaker(file);
    } else {
        readParametersText(file);
    }

    loadParameters(snapshotIndex);
    file.close();
}

// NOTE: This function differs from the original Fortran 'guardaveinsoff' in two key respects:
// 1) Fixes a bug with missing faces that existed in the original routine
// 2) Writes each face TWICE (both orientations) to simplify visualization with renderers
//    that expect consistently oriented faces
void FileIO::saveAsOFF(std::ostream& out, const std::vector<std::array<int, MAX_NEIGHBORS>>& cellNeighbors) {
    // Calculate material values for coloring
    calculateMaterial();

    // Get triangles from model
    auto triangles = model->getTriangles();

    // Write OFF header (output both orientations since triangles are not oriented)
    out << "COFF\n";
    out << model->numCells << " " << triangles.size() * 2 << " " << model->numCells << "\n\n";

    // Write vertices with colors
    for (int i = 0; i < model->numCells; i++) {
        std::array<double, 4> color;
        getColorMapping(materialValues[i], materialMin, materialMax, color);
        out << model->cellPositions[i][0] << " "
            << model->cellPositions[i][1] << " "
            << model->cellPositions[i][2] << " "
            << color[0] << " " << color[1] << " " << color[2] << " " << color[3] << "\n";
    }

    out << "\n";

    // Write faces (both orientations since triangles are not oriented)
    for (const auto& tri : triangles) {
        out << "3 " << tri[0] << " " << tri[1] << " " << tri[2] << "\n";
        out << "3 " << tri[0] << " " << tri[2] << " " << tri[1] << "\n";
    }
}

void FileIO::calculateMaterial() {
    materialValues.resize(model->numCells);
    std::fill(materialValues.begin(), materialValues.end(), 0.0);

    for (int i = 0; i < model->numCells; i++) {
        if (model->knotMarkers[i] == 1) {
            materialValues[i] = 1.0;
        } else {
            if (model->quantities2D[i][0] > model->diffThresholdInt) {
                materialValues[i] = 0.1;
            }
            if (model->quantities2D[i][0] > model->diffThresholdSet) {
                materialValues[i] = 1.0;
            }
        }
    }

    materialMax = *std::max_element(materialValues.begin(), materialValues.end());
    materialMin = *std::min_element(materialValues.begin(), materialValues.end());
}

void FileIO::getColorMapping(double val, double minVal, double maxVal, std::array<double, 4>& color) {
    double f;
    if (maxVal > minVal) {
        f = (val - minVal) / (maxVal - minVal);
    } else {
        f = 0.5;
    }

    if (f < 0.07) {
        color = {0.6, 0.6, 0.6, 0.8};
    } else if (f < 0.2) {
        color = {1.0, f, 0.0, 0.5};
    } else if (f < 1.0) {
        color = {1.0, f * 3, 0.0, 1.0};
    } else {
        color = {1.0, 1.0, 0.0, 1.0};
    }
}
