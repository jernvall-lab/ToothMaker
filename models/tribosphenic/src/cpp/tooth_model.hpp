// tooth_model.hpp
// Tooth development simulation model
// Ported from Fortran 90 to modern C++
//
// FileIO class: Handles saving and loading parameter files and tooth morphology files
// ToothModel class: The main model containing the following routines:
//   - initializeDefaults: Specifies the default initial conditions
//   - allocateAndInit: Allocates matrices for initial conditions and sets them
//   - placeCell: Specifies the position of cells in the initial conditions
//   - calculateMargins: Calculates the shape of each epithelial cell (position of its margins)
//   - reactionDiffusion: Calculates diffusion of all molecules between cells
//   - updateDifferentiation: Updates cells differentiation values
//   - calculateGrowthPushing: Calculates pushing between cells from epithelial/border growth
//   - calculateBuoyancy: Calculates pushing between cells from buoyancy
//   - calculateNeighborRepulsion: Calculates repulsion between neighboring cells
//   - checkNonNeighborRepulsion: Checks if non-neighbor cells get too close and applies repulsion
//   - applyBuccalLingualBias: Applies BMP4 concentration in buccal and lingual borders
//   - calculateNucleusTraction: Calculates the nucleus traction by the cell borders
//   - updatePositions: Updates cell positions
//   - addNewCells: Calculates where cell divisions occur and adds new cells
//   - markBorderCells: Identifies which added cells are on the tooth border
//   - runIteration: Determines the order in which subroutines are called

#ifndef TOOTH_MODEL_HPP
#define TOOTH_MODEL_HPP

#include <vector>
#include <string>
#include <cmath>
#include <array>
#include <algorithm>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

// Forward declarations
class ToothModel;
class FileIO;

// Constants
constexpr int MAX_NEIGHBORS = 30;       // Maximum number of neighbors per cell
constexpr int NUM_3D_QUANTITIES = 5;    // Number of 3D quantities (act, inh, fgf, ect, p)
constexpr int NUM_2D_QUANTITIES = 4;    // Number of 2D quantities
constexpr double BASE_DISTANCE = 1.0;   // Original distance between nodes
constexpr double MAX_DIVISION_DIST = 2.0;  // Maximum distance before cell division
constexpr double TIME_DELTA = 0.05;     // Time step delta (Fortran: 0.005D1 = 0.05)
constexpr double PI = 3.141592653589793;

// Main model class (originally coreop2d module)
class ToothModel {
public:
    //=========================================================================
    // Cell geometry data
    //=========================================================================
    std::vector<std::array<double, 3>> cellPositions;      // Node positions (x, y, z)
    std::vector<std::vector<std::array<double, 8>>> cellMargins;  // Internode positions for each cell
    std::vector<std::array<int, MAX_NEIGHBORS>> neighbors; // Neighbor indices for each cell
    std::vector<int> knotMarkers;                          // Knot markers (1 = knot, 0 = not)
    std::vector<int> neighborCount;                        // Number of neighbors per cell

    //=========================================================================
    // Concentration/quantity data
    //=========================================================================
    std::vector<std::array<double, NUM_2D_QUANTITIES>> quantities2D;  // 2D quantities per cell
    std::vector<std::vector<std::array<double, NUM_3D_QUANTITIES>>> quantities3D;  // 3D quantities [cell][z][quantity]
    std::vector<double> diffusionCoeffs3D;    // Diffusion coefficients for 3D quantities
    std::vector<double> diffusionCoeffs2D;    // Diffusion coefficients for 2D quantities

    //=========================================================================
    // Working arrays for position updates
    //=========================================================================
    std::vector<std::array<double, 3>> positionDeltas;     // Position changes per iteration

    //=========================================================================
    // Working arrays for reaction-diffusion (reused each iteration)
    //=========================================================================
    std::vector<std::array<double, MAX_NEIGHBORS>> rdWeight;    // Diffusion weights
    std::vector<std::array<double, MAX_NEIGHBORS>> rdAreaProj;  // Area projections
    std::vector<std::vector<std::array<double, NUM_3D_QUANTITIES>>> rdDeltaQ3D;  // 3D quantity deltas

    //=========================================================================
    // Cell counts and grid parameters
    //=========================================================================
    int numCells;           // Number of actual cells
    int numCellsTotal;      // Number of cells including virtual border cells
    int numZLevels;         // Number of depth z levels for quantity calculation
    int radius;             // Grid radius
    int timeStep;           // Current time step
    int stepNumber;         // Step counter

    //=========================================================================
    // Model parameters (true parameters)
    //=========================================================================
    // Differentiation thresholds
    double diffThresholdSet;    // Set threshold (ud) - growth factor threshold
    double diffThresholdInt;    // Int threshold (us) - initial inhibitor threshold

    // Growth rates
    double epithelialGrowthRate;     // Egr (tacre) - epithelial proliferation rate
    double mesenchymalGrowthRate;    // Mgr (tahor) - mesenchymal proliferation rate
    double basalMesenchymalRate;     // umgr - basal mesenchymal rate (independent of Sec)

    // Activation/inhibition parameters
    double activatorAutoActivation;  // Act (acac) - activator auto-activation
    double ectodinRate;              // Not2 (acec) - ectodin rate (not in use)
    double not3Rate;                 // Not3 (acaca) - not in use
    double activatorInhibition;      // Inh (ihac) - inhibition of activator
    double growthFactorSecretion;    // Sec (ih) - growth factor secretion rate
    double degradationRate;          // Deg (mu) - protein degradation rate
    double initialActivator;         // Ina - initial activator concentration

    // Mechanical parameters
    double stiffness;                // Rep (elas) - Young's modulus/stiffness
    double neighborTraction;         // Adh (crema) - traction between neighbors
    double nucleusTraction;          // Ntr (radibi) - mechanical traction from borders to nucleus
    double sharpnessMax;             // Dgr (tazmax) - sharpness maxima (epithelium pull-down)

    // Border/bias parameters
    double borderDistance;           // Swi (tadi) - distance from 0 where borders are defined
    double borderWidth;              // Bwi (tadif) - width of border
    double biasPosterior;            // Pbi (bip) - posterior bias
    double biasAnterior;             // Abi (bia) - anterior bias
    double biasLingual;              // Lbi (bil) - lingual bias
    double biasBuccal;               // Bbi (bib) - buccal bias
    double biasCenterRadius;         // radibii - radius of center where AP bias is applied
    double biasFactor;               // fac - bias factor

    //=========================================================================
    // Implementation state variables
    //=========================================================================
    int currentCellIndex;            // nca - current cell being processed
    int centerCellIndex;             // icentre - center cell index during iteration
    int centerCell;                  // centre - center cell
    int numBorderCells;              // ncils - number of border cells

    // Current position being processed
    double currX, currY;             // x, y - current position
    double newX, newY;               // xx, yy - new position being calculated

    // Precomputed sin/cos values for hexagonal grid directions
    double sinDir0, cosDir0;         // Direction 0 (0 degrees)
    double sinDir60, cosDir60;       // Direction 60 degrees
    double sinDir120, cosDir120;     // Direction 120 degrees
    double sinDir180, cosDir180;     // Direction 180 degrees
    double sinDir240, cosDir240;     // Direction 240 degrees
    double sinDir300, cosDir300;     // Direction 300 degrees

    // New cells tracking
    int numNewCells;                 // nnous - number of new cells added
    int numAnteriorMarkers;          // nmaa
    int numPosteriorMarkers;         // nmap
    std::vector<int> anteriorMarkers;   // mmaa - anterior marker cells
    std::vector<int> posteriorMarkers;  // mmap - posterior marker cells

    // Panic flag for overflow detection
    double panicFlag;

    // Iteration state (for progress reporting)
    int iterationIndex, totalIterations;
    std::string progressFilename;

    //=========================================================================
    // Constructor
    //=========================================================================
    ToothModel();

    //=========================================================================
    // Core initialization functions
    //=========================================================================
    void initializeDefaults();       // Set up default initial conditions
    void reinitialize();             // Reinitialize the model
    void allocateAndInit();          // Allocate and initialize all matrices
    void reallocate();               // Reallocate matrices (not in use)
    void reset();                    // Reset and reinitialize
    void deallocateAll();            // Deallocate all memory
    void placeCell();                // Position a cell in initial conditions

    //=========================================================================
    // Core simulation functions
    //=========================================================================
    void calculateMargins();         // Calculate cell margins/shapes
    void reactionDiffusion();        // Reaction-diffusion calculations
    void updateDifferentiation();    // Update differentiation values
    void calculateGrowthPushing();   // Calculate pushing from epithelial/border growth
    void calculateBuoyancy();        // Calculate pushing from buoyancy (stellate effect)
    void calculateNeighborRepulsion();     // Calculate repulsion between neighbors
    void checkNonNeighborRepulsion();      // Check and apply repulsion for non-neighbors
    void applyBuccalLingualBias();         // Apply BMP4 concentration in buccal/lingual borders
    void initializeActivator();            // Initialize activator concentration
    void calculateNucleusTraction();       // Calculate nucleus traction by cell borders
    void updatePositions();                // Update cell positions
    void addNewCells();                    // Add new cells where division occurs
    void markBorderCells();                // Mark new border cells
    void increaseZDepth();                 // Increase z depth for quantities
    void runIteration(int numSteps);       // Main iteration loop
};

// File I/O class (originally esclec module)
class FileIO {
public:
    ToothModel* model;  // Reference to the main model

    int snapshotIndex, exitFlag, passFlag;

    static constexpr int MAX_SNAPSHOTS = 5000;

    // Storage arrays for multiple snapshots
    std::vector<std::vector<std::array<double, 3>>> positionHistory;   // Position snapshots
    std::vector<std::array<double, 32>> parameterHistory;              // Parameter snapshots
    std::vector<std::string> parameterNames;                           // Parameter names
    std::vector<std::vector<int>> knotsHistory;                        // Knots snapshots
    std::vector<std::vector<std::array<int, MAX_NEIGHBORS>>> neighborHistory;  // Neighbor snapshots
    std::vector<double> materialValues;                                // Material values for visualization
    double materialMax, materialMin;

    // Command line argument storage
    std::string argInputFile, argOutputFile, argIterations;

    //=========================================================================
    // Constructor
    //=========================================================================
    FileIO(ToothModel* modelPtr);

    //=========================================================================
    // Save functions
    //=========================================================================
    void saveParameters(std::ostream& out);
    void saveMorphology(std::ostream& out);
    void saveConcentrations(std::ostream& out);
    void saveExtraData(std::ostream& out);
    void saveKnots(std::ostream& out);
    void saveNeighbors(std::ostream& out, const std::vector<std::array<int, MAX_NEIGHBORS>>& cellNeighbors);
    void saveMargins(std::ostream& out, const std::vector<std::array<int, MAX_NEIGHBORS>>& cellNeighbors);
    void saveAsOFF(std::ostream& out, const std::vector<std::array<int, MAX_NEIGHBORS>>& cellNeighbors);

    //=========================================================================
    // Load functions
    //=========================================================================
    void readParametersText(std::istream& in);
    void writeParametersText(std::ostream& out);
    void readParametersBinary(std::istream& in);
    void readMorphology(std::istream& in);
    void readExtraData(std::istream& in);
    void readKnots(std::istream& in);
    void readNeighbors(std::istream& in);

    //=========================================================================
    // Parameter transfer functions
    //=========================================================================
    void storeParameters(int snapshotIdx);    // Copy parameters from model to storage
    void loadParameters(int snapshotIdx);     // Copy parameters from storage to model

    //=========================================================================
    // High-level I/O functions
    //=========================================================================
    void readDataFile();              // Main read function
    void readInitialParameters();     // Read initial parameters only

    //=========================================================================
    // Visualization helpers
    //=========================================================================
    void calculateMaterial();         // Calculate material values for visualization
    void getColorMapping(double val, double minVal, double maxVal, std::array<double, 4>& color);
};

#endif // TOOTH_MODEL_HPP
