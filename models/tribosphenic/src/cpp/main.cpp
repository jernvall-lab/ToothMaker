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
    std::string iterStr = argv[3];
    std::string stepsStr = argv[4];

    // Parse iteration count and steps
    int totalIterations = std::stoi(iterStr);
    int numSteps = std::stoi(stepsStr);

    // Create model and file I/O handler
    ToothModel model;
    FileIO fileIO(&model);

    // Store command line arguments
    fileIO.argInputFile = inputFile;
    fileIO.argOutputFile = outputFile;

    // Initialize model
    model.numZLevels = 4;
    model.initializeDefaults();
    fileIO.readInitialParameters();
    model.allocateAndInit();

    // Store initial parameters
    int savedNumCells = model.numCells;
    fileIO.loadParameters(0);
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
            fileIO.storeParameters(0);
            fileIO.saveParameters(dadFile);
            fileIO.saveNeighbors(dadFile, model.neighbors);
            fileIO.saveMargins(dadFile, model.neighbors);
            fileIO.saveKnots(dadFile);
            fileIO.saveMorphology(dadFile);
            fileIO.saveConcentrations(dadFile);
            fileIO.saveExtraData(dadFile);
            dadFile.close();
        }

        // Write OFF output file (mesh format)
        std::ofstream offFile(offFilename);
        if (offFile.is_open()) {
            fileIO.saveAsOFF(offFile, model.neighbors);
            offFile.close();
        }

        // Write TXT output file (readable parameters)
        std::ofstream txtFile(txtFilename);
        if (txtFile.is_open()) {
            fileIO.loadParameters(0);
            fileIO.writeParametersText(txtFile);
            txtFile.close();
        }
    }

    // Restore initial parameters
    fileIO.loadParameters(0);

    std::cout << "Simulation complete." << std::endl;

    return 0;
}
