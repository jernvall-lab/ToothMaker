// file_io.cpp
// Parameter-file input and .dad/.off/.txt output for ToothModel

#include "tooth_model.hpp"

#include <cctype>
#include <limits>

namespace {

// The Fortran writes its data with list-directed output, i.e. 17 significant
// digits for real*8, which reproduces every double exactly. Match that for
// everything except the fixed-format parameter header.
void fullPrecision_(std::ostream& out)
{
    out << std::defaultfloat << std::setprecision(std::numeric_limits<double>::max_digits10);
}

// Strips leading and trailing whitespace. Removes only what is actually there,
// so a string with no surrounding spaces comes through unchanged.
std::string trim_(const std::string& s)
{
    size_t begin = 0;
    size_t end = s.size();
    while (begin < end && std::isspace(static_cast<unsigned char>(s[begin]))) {
        begin++;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        end--;
    }
    return s.substr(begin, end - begin);
}

// Reads a whole text file, converting every line ending to '\n': CRLF ("\r\n")
// from Windows, lone CR ("\r") from classic Mac OS, and LF ("\n") all come out
// as LF. Everything downstream can then assume Unix line endings regardless of
// which platform wrote the file.
//
// This is the C++ equivalent of Python's universal newlines, which the standard
// library does not provide: std::getline() splits on a single delimiter
// character, and text-mode translation only handles the host's own convention
// (it is a no-op outside Windows). Neither copes with a file that arrived from
// another platform, so parameter files are normalised here, once, at the point
// where their bytes enter the program.
//
// Returns false if the file could not be opened.
bool readFileNormalized_(const std::string& filename, std::string& out)
{
    // Binary mode on purpose: do the translation here rather than inherit
    // whatever the platform's text mode happens to do.
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }

    std::ostringstream buffer;
    buffer << file.rdbuf();
    const std::string raw = buffer.str();

    out.clear();
    out.reserve(raw.size());
    for (size_t i = 0; i < raw.size(); i++) {
        if (raw[i] == '\r') {
            if (i + 1 < raw.size() && raw[i + 1] == '\n') {
                i++;            // CRLF: consume the LF as well
            }
            out += '\n';        // lone CR becomes LF too
        }
        else {
            out += raw[i];
        }
    }

    return true;
}

}   // END namespace

//=============================================================================
// FileIO Implementation
//=============================================================================

FileIO::FileIO(ToothModel* modelPtr) :
    model(modelPtr), exitFlag(0), materialMax(0.0), materialMin(0.0)
{
    parameters.fill(0.0);
}

//=============================================================================
// Save functions
//=============================================================================

void FileIO::saveParameters(std::ostream& out) {
    parameters[0] = model->totalIterations * (model->iterationIndex - 1) + model->timeStep;

    // Fortran: six records of 5F15.6, then list-directed output for the rest of
    // the file. Restore full precision so that the header format does not leak
    // into the data that follows.
    out << std::fixed << std::setprecision(6);
    for (int row = 0; row < 6; row++) {
        for (int i = 5 * row; i < 5 * row + 5; i++) {
            out << std::setw(15) << parameters[i];
        }
        out << "\n";
    }
    out << model->numZLevels << " " << model->numBorderCells << "\n";
    fullPrecision_(out);
}

void FileIO::saveMorphology(std::ostream& out) {
    fullPrecision_(out);
    out << model->timeStep << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        out << model->cellPositions[i][0] << " "
            << model->cellPositions[i][1] << " "
            << model->cellPositions[i][2] << "\n";
    }
}

void FileIO::saveConcentrations(std::ostream& out) {
    fullPrecision_(out);
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
    fullPrecision_(out);
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

void FileIO::saveNeighbors(std::ostream& out) {
    out << model->timeStep << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        const auto& list = model->neighbors[i];
        int count = 0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (list[j] != 0) count++;
        }
        out << count << "\n";
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (list[j] != 0) out << list[j] << " ";
        }
        out << "\n";
    }
}

void FileIO::saveMargins(std::ostream& out) {
    fullPrecision_(out);
    out << model->timeStep << " " << model->numCells << "\n";
    for (int i = 0; i < model->numCells; i++) {
        const auto& list = model->neighbors[i];
        int count = 0;
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (list[j] != 0) count++;
        }
        out << count << " cell shape\n";
        for (int j = 0; j < MAX_NEIGHBORS; j++) {
            if (list[j] != 0) {
                const auto& m = model->cellMargins[i][j];
                out << m[0] << " " << m[1] << " " << m[2] << "\n";
            }
        }
    }
}

void FileIO::writeParametersText(std::ostream& out) {
    fullPrecision_(out);
    // Fortran writes parap(3:32), C++ uses [2:31] (inclusive)
    for (int i = 2; i <= 31; i++) {
        out << parameters[i] << " " << parameterNames[i] << "\n";
    }
    out << model->numCells << " number of cells\n";
    out << model->timeStep << " number of iterations\n";
}

// NOTE: This function differs from the original Fortran 'guardaveinsoff' in two key respects:
// 1) Fixes a bug with missing faces that existed in the original routine
// 2) Writes each face TWICE (both orientations) to simplify visualization with renderers
//    that expect consistently oriented faces
void FileIO::saveAsOFF(std::ostream& out) {
    fullPrecision_(out);

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

//=============================================================================
// Load functions
//=============================================================================

void FileIO::readParametersText(std::istream& in) {
    // Fortran reads parap(3:32), C++ uses [2:31] (inclusive)
    // Fortran parap(n) -> C++ [n-1]
    for (int i = 2; i <= 31; i++) {
        double value;
        std::string name;
        if (in >> value >> name) {
            parameters[i] = value;
            parameterNames[i] = name;
        } else {
            break;
        }
    }
}

// Takes the already newline-normalised file contents, not a file name.
bool FileIO::isToothMakerFormat(const std::string& text) {
    std::istringstream file(text);

    std::string raw;
    while (std::getline(file, raw)) {
        // Trim first, so that indented comments and whitespace-only lines are
        // recognised as such instead of ending the scan at the break below.
        const std::string line = trim_(raw);
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;

        // Check for ToothMaker format identifier: "model==Tribosphenic tooth"
        // Case-insensitive for "model", case-sensitive for model name
        size_t pos = line.find("==");
        if (pos != std::string::npos) {
            std::string key = trim_(line.substr(0, pos));
            std::string value = trim_(line.substr(pos + 2));

            // Convert key to lowercase for comparison
            std::string keyLower = key;
            for (char& c : keyLower) c = std::tolower(c);

            if (keyLower == "model" && value == "Tribosphenic tooth") {
                return true;
            }
        }
        // If first non-comment line doesn't match, it's humppa format
        break;
    }
    return false;
}

int FileIO::getParameterIndex(const std::string& name) {
    // Map ToothMaker parameter names to `parameters` indices
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
    // The biases are mapped as the Fortran variable names suggest (Pbi -> bip),
    // which is the reverse of the GUI's positional order (Abi -> bip, see
    // tooth_model.hpp): a ToothMaker-format file run directly through this
    // reader gives a different tooth than the same file run through the GUI.
    // Kept deliberately for now (TODO.md 12.1 B1).
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
        parameters[i] = 0.0;
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

    std::string raw;
    while (std::getline(in, raw)) {
        // Skip empty lines and comments
        const std::string line = trim_(raw);
        if (line.empty() || line[0] == '#') continue;

        // Parse name==value format
        size_t pos = line.find("==");
        if (pos == std::string::npos) continue;

        // Trim whitespace from name and value
        std::string name = trim_(line.substr(0, pos));
        std::string valueStr = trim_(line.substr(pos + 2));

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
                parameters[index] = value;
            } catch (const std::exception& e) {
                std::cerr << "Warning: Could not parse value for parameter " << name << std::endl;
            }
        } else {
            std::cerr << "Warning: Unknown parameter name: " << name << std::endl;
        }
    }
}

void FileIO::readInitialParameters() {
    std::string text;
    if (!readFileNormalized_(argInputFile, text)) {
        std::cerr << "Error opening file: " << argInputFile << std::endl;
        exitFlag = 1;
        return;
    }

    // Detect file format and use appropriate reader
    bool useToothMakerFormat = isToothMakerFormat(text);

    std::istringstream file(text);
    if (useToothMakerFormat) {
        readParametersToothMaker(file);
    } else {
        readParametersText(file);
    }

    loadParameters();
}

//=============================================================================
// Parameter transfer functions
//=============================================================================
// `parameters` is the Fortran parap(:,1) array, 0-based: Fortran parap(n) is
// parameters[n-1]. The Fortran assigns some slots twice (agafarparap); the last
// assignment wins, and that is what is reproduced here.

void FileIO::storeParameters() {
    parameters[0] = model->timeStep;                  // parap(1) = temps
    parameters[1] = model->numCells;                  // parap(2) = ncels
    parameters[2] = model->epithelialGrowthRate;      // parap(3) = tacre
    parameters[3] = model->mesenchymalGrowthRate;     // parap(4) = tahor
    parameters[4] = model->stiffness;                 // parap(5) = elas
    parameters[5] = model->borderDistance;            // parap(6) = tadi
    parameters[6] = model->neighborTraction;          // parap(7) = crema
    parameters[7] = model->activatorAutoActivation;   // parap(8) = acac
    parameters[8] = model->activatorInhibition;       // parap(9) = ihac
    parameters[9] = model->ectodinRate;               // parap(10) = acec
    parameters[10] = model->growthFactorSecretion;    // parap(11) = ih
    for (int j = 0; j < 4; j++) {
        parameters[12 + j] = model->diffusionCoeffs3D[j];   // parap(13..16) = difq3d(1..4)
    }
    parameters[16] = model->diffThresholdInt;         // parap(17) = us
    parameters[17] = model->diffThresholdSet;         // parap(18) = ud
    parameters[18] = model->diffusionCoeffs2D[1];     // parap(19) = difq2d(2) = GUI "Boy" (buoyancy)
    parameters[19] = model->borderWidth;              // parap(20) = tadif = GUI "Dff" (differentiation rate)
    parameters[20] = model->biasFactor;               // parap(21) = fac
    parameters[21] = model->biasPosterior;            // parap(22) = bip (GUI Abi)
    parameters[22] = model->biasAnterior;             // parap(23) = bia (GUI Pbi)
    parameters[23] = model->biasBuccal;               // parap(24) = bib (GUI Lbi)
    parameters[24] = model->biasLingual;              // parap(25) = bil (GUI Bbi)
    parameters[25] = model->radius;                   // parap(26) = radi
    parameters[26] = model->degradationRate;          // parap(27) = mu
    parameters[27] = model->sharpnessMax;             // parap(28) = tazmax
    parameters[28] = model->nucleusTraction;          // parap(29) = radibi
    parameters[29] = model->biasCenterRadius;         // parap(30) = radibii = GUI "Bwi" (XML mapping error)
    parameters[30] = model->initialActivator;         // parap(31) = ina
    parameters[31] = model->basalMesenchymalRate;     // parap(32) = umgr

    // parap(12) holds acaca (Not3) but the Fortran overwrites it with ncals at the
    // end of agafarparap; loadParameters() then reads Not3 back from it. Kept
    // for compatibility (TODO.md 12.2 F2).
    parameters[11] = model->numCellsTotal;
}

void FileIO::loadParameters() {
    model->timeStep = static_cast<int>(parameters[0]);          // parap(1)
    model->numCells = static_cast<int>(parameters[1]);          // parap(2)
    model->epithelialGrowthRate = parameters[2];                // parap(3) = tacre
    model->mesenchymalGrowthRate = parameters[3];               // parap(4) = tahor
    model->stiffness = parameters[4];                           // parap(5) = elas
    model->borderDistance = parameters[5];                      // parap(6) = tadi
    model->neighborTraction = parameters[6];                    // parap(7) = crema
    model->activatorAutoActivation = parameters[7];             // parap(8) = acac
    model->activatorInhibition = parameters[8];                 // parap(9) = ihac
    model->ectodinRate = parameters[9];                         // parap(10) = acec
    model->growthFactorSecretion = parameters[10];              // parap(11) = ih
    model->not3Rate = parameters[11];                           // parap(12) = acaca (see storeParameters)
    for (int j = 0; j < NUM_3D_QUANTITIES; j++) {
        model->diffusionCoeffs3D[j] = parameters[12 + j];       // parap(13..17) = difq3d(1..5)
    }
    for (int j = 0; j < NUM_2D_QUANTITIES; j++) {
        model->diffusionCoeffs2D[j] = parameters[17 + j];       // parap(18..21) = difq2d(1..4)
    }
    model->diffThresholdInt = parameters[16];                   // parap(17) = us
    model->diffThresholdSet = parameters[17];                   // parap(18) = ud
    model->borderWidth = parameters[19];                        // parap(20) = tadif
    model->biasFactor = parameters[20];                         // parap(21) = fac
    model->biasPosterior = parameters[21];                      // parap(22) = bip (GUI Abi)
    model->biasAnterior = parameters[22];                       // parap(23) = bia (GUI Pbi)
    model->biasBuccal = parameters[23];                         // parap(24) = bib (GUI Lbi)
    model->biasLingual = parameters[24];                        // parap(25) = bil (GUI Bbi)
    model->radius = static_cast<int>(parameters[25]);           // parap(26) = radi
    model->degradationRate = parameters[26];                    // parap(27) = mu
    model->sharpnessMax = parameters[27];                       // parap(28) = tazmax
    model->nucleusTraction = parameters[28];                    // parap(29) = radibi
    model->biasCenterRadius = parameters[29];                   // parap(30) = radibii
    model->initialActivator = parameters[30];                   // parap(31) = ina
    model->basalMesenchymalRate = parameters[31];               // parap(32) = umgr
}

//=============================================================================
// Visualization helpers
//=============================================================================

void FileIO::calculateMaterial() {
    materialValues.assign(model->numCells, 0.0);

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

    // Color mapping matches dad_to_polygons output:
    // - Undifferentiated (f < 0.07): gray
    // - Knots (f >= 1.0): yellow
    // - Differentiated: white
    // Note: Original Fortran used red/orange gradient for differentiation.
    if (f < 0.07) {
        color = {0.5, 0.5, 0.5, 1.0};  // Gray (undifferentiated)
    } else if (f >= 1.0) {
        color = {1.0, 1.0, 0.0, 1.0};  // Yellow (knots)
    } else {
        color = {1.0, 1.0, 1.0, 1.0};  // White (differentiated)
    }
}
