// main.cpp
// Entry point for tooth development simulation

#include "tooth_model.hpp"

int main(int argc, char* argv[]) {
    if (argc < 5) {
        std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <iterations> <steps>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string outputFile = argv[2];

    // Parse iteration count and steps
    int totalIterations = std::stoi(argv[3]);
    int numSteps = std::stoi(argv[4]);

    // Create model and file I/O handler
    ToothModel model;
    FileIO fileIO(&model);
    fileIO.argInputFile = inputFile;

    // Initialize model
    model.initializeDefaults();
    fileIO.readInitialParameters();
    if (fileIO.exitFlag != 0) {
        return 1;
    }

    // allocateAndInit() derives numBorderCells = (radius-1)*6, which goes
    // negative for radius < 1. The border/interior loops then start at a
    // negative index and read cellPositions[] out of bounds. Any parameter
    // file that fails to parse leaves every parameter at zero and lands here
    // with radius == 0, so catch it before allocating anything.
    if (model.radius < 1) {
        std::cerr << "Error: invalid initial radius (Rad = " << model.radius
                  << ") from '" << inputFile << "'." << std::endl;
        std::cerr << "Either the parameter file could not be parsed, or it does "
                     "not set Rad." << std::endl;
        return 1;
    }

    model.allocateAndInit();

    // loadParameters() also copies the cell-count slot, which the parameter file
    // leaves at zero, so keep the count allocateAndInit() derived.
    int savedNumCells = model.numCells;
    fileIO.loadParameters();
    model.numCells = savedNumCells;

    // Set initial activator concentration
    model.initializeActivator();

    // Main simulation loop
    for (model.iterationIndex = 1; model.iterationIndex <= std::abs(numSteps); model.iterationIndex++) {
        // Generate output filenames
        std::ostringstream iterSuffix;
        iterSuffix << model.iterationIndex * totalIterations;

        std::string baseFilename = iterSuffix.str() + "_" + outputFile;
        // Replace spaces with underscores
        for (char& c : baseFilename) {
            if (c == ' ') c = '_';
        }

        std::string dadFilename = baseFilename.substr(0, 26) + "_.dad";
        std::string offFilename = baseFilename.substr(0, 26) + "_.off";
        std::string txtFilename = baseFilename.substr(0, 26) + "_.txt";

        model.progressFilename = outputFile.substr(0, 15) + "_progressbar.txt";
        for (char& c : model.progressFilename) {
            if (c == ' ') c = '_';
        }

        model.timeStep = 0;
        model.totalIterations = totalIterations;
        model.runIteration(totalIterations);

        // Write DAD output file
        std::ofstream dadFile(dadFilename);
        if (dadFile.is_open()) {
            fileIO.storeParameters();
            fileIO.saveParameters(dadFile);
            fileIO.saveNeighbors(dadFile);
            fileIO.saveMargins(dadFile);
            fileIO.saveKnots(dadFile);
            fileIO.saveMorphology(dadFile);
            fileIO.saveConcentrations(dadFile);
            fileIO.saveExtraData(dadFile);
            dadFile.close();
        }

        // Write OFF output file (mesh format)
        std::ofstream offFile(offFilename);
        if (offFile.is_open()) {
            fileIO.saveAsOFF(offFile);
            offFile.close();
        }

        // Write TXT output file (readable parameters)
        std::ofstream txtFile(txtFilename);
        if (txtFile.is_open()) {
            fileIO.loadParameters();
            fileIO.writeParametersText(txtFile);
            txtFile.close();
        }
    }

    // Restore initial parameters
    fileIO.loadParameters();

    std::cout << "Simulation complete." << std::endl;

    return 0;
}
