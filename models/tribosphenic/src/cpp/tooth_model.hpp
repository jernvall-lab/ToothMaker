// tooth_model.hpp
// Tooth development simulation model
// Ported from Fortran 90 to modern C++
//
// ToothModel: the simulation itself (Fortran module coreop2d). One iteration runs
//   reactionDiffusion      diffusion and reaction of the signalling molecules
//   applyBuccalLingualBias BMP4 concentration in the buccal and lingual borders
//   updateDifferentiation  differentiation state of each cell
//   calculateGrowthPushing pushing from epithelial and border growth
//   calculateBuoyancy      pushing from the stellate reticulum (buoyancy)
//   checkNonNeighborRepulsion  repulsion between non-neighbours that get too close
//   calculateNeighborRepulsion repulsion/traction between neighbouring cells
//   calculateNucleusTraction   pull of the cell nucleus towards its borders
//   updatePositions        move the cells
//   addNewCells            cell division where neighbours have drifted apart
//   calculateMargins       shape (margin positions) of every cell
// FileIO: parameter-file input and .dad/.off/.txt output (Fortran module esclec).

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

// Constants
constexpr int MAX_NEIGHBORS = 30;       // Maximum number of neighbors per cell
constexpr int NUM_3D_QUANTITIES = 5;    // Number of 3D quantities (act, inh, fgf, ect, p)
constexpr int NUM_2D_QUANTITIES = 4;    // Number of 2D quantities
constexpr int NUM_Z_LEVELS = 4;         // Depth levels of the 3D quantities (Fortran ncz)
constexpr int NUM_PARAMETERS = 32;      // Size of the Fortran parameter array parap
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
    // Neighbour lists hold 1-based cell indices (0 = empty slot), as in the
    // Fortran and in the .dad files. The value numCellsTotal marks the virtual
    // boundary cell surrounding the tooth.
    std::vector<std::array<double, 3>> cellPositions;      // Node positions (x, y, z)
    std::vector<std::array<int, MAX_NEIGHBORS>> neighbors; // Neighbor indices for each cell
    std::vector<int> knotMarkers;                          // Knot markers (1 = knot, 0 = not)

    // Per neighbour slot: the margin (cell corner) position x, y, z, recomputed
    // every iteration, and the rest length of the connection, fixed when it is made.
    static constexpr int MARGIN_REST = 3;
    using MarginSlot = std::array<double, 4>;
    using CellMargins = std::array<MarginSlot, MAX_NEIGHBORS>;
    std::vector<CellMargins> cellMargins;

    //=========================================================================
    // Concentration/quantity data
    //=========================================================================
    std::vector<std::array<double, NUM_2D_QUANTITIES>> quantities2D;  // 2D quantities per cell
    std::vector<std::vector<std::array<double, NUM_3D_QUANTITIES>>> quantities3D;  // 3D quantities [cell][z][quantity]
    std::vector<double> diffusionCoeffs3D;    // Diffusion coefficients for 3D quantities
    std::vector<double> diffusionCoeffs2D;    // Diffusion coefficients for 2D quantities
                                              // NOTE: [1] is Boy (buoyancy), not a diffusion coeff!

    //=========================================================================
    // Working arrays (reused each iteration)
    //=========================================================================
    std::vector<std::array<double, 3>> positionDeltas;     // Position changes per iteration
    std::vector<std::array<double, 3>> prevPositions;      // Staging area for calculateNucleusTraction
    std::vector<std::array<double, MAX_NEIGHBORS>> rdWeight;    // Diffusion weights
    std::vector<std::array<double, MAX_NEIGHBORS>> rdAreaProj;  // Area projections
    std::vector<std::vector<std::array<double, NUM_3D_QUANTITIES>>> rdDeltaQ3D;  // 3D quantity deltas

    //=========================================================================
    // Cell counts and grid parameters
    //=========================================================================
    int numCells;           // Number of actual cells
    int numCellsTotal;      // Number of cells including the virtual boundary cell
    int numBorderCells;     // Border cells are indices 0..numBorderCells-1 (Fortran ncils-1)
    int numZLevels;         // Number of depth z levels for quantity calculation
    int radius;             // Initial grid radius
    int timeStep;           // Current time step

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
    double borderWidth;              // Dff (tadif) - NOTE: Actually differentiation rate, not border width!
    // The four biases are named after the Fortran variables (bip, bia, bil, bib),
    // whose names do not match the GUI parameter that feeds them: parameters are
    // passed positionally, and -- as the Fortran source notes itself ("NOTE: Abi,
    // Pbi, Bbi, Lbi") -- Abi lands in bip, Pbi in bia, Lbi in bib and Bbi in bil.
    // Kept as is: existing users and parameter files rely on this order.
    double biasPosterior;            // bip = GUI Abi; scales x-growth of border cells with x < 0
    double biasAnterior;             // bia = GUI Pbi; scales x-growth of border cells with x > 0
    double biasLingual;              // bil = GUI Bbi; activator floor on the y < -Swi side
    double biasBuccal;               // bib = GUI Lbi; activator floor on the y > Swi side
    double biasCenterRadius;         // radibii - radius of center where AP bias is applied
                                     // NOTE: GUI "Bwi" maps here due to XML position error
    double biasFactor;               // Bgr (fac) - bias factor

    //=========================================================================
    // Run state
    //=========================================================================
    bool panicFlag;                  // Set on numerical overflow or a failed division

    // Iteration state (for progress reporting)
    int iterationIndex, totalIterations;
    std::string progressFilename;

    //=========================================================================
    // Constructor and initialization
    //=========================================================================
    ToothModel();
    void initializeDefaults();       // Set up default initial conditions
    void allocateAndInit();          // Allocate everything and build the initial hexagonal grid
    void initializeActivator();      // Set the initial activator concentration (Ina)

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
    void calculateNucleusTraction();       // Calculate nucleus traction by cell borders
    void updatePositions();                // Update cell positions
    void addNewCells();                    // Add new cells where division occurs
    void runIteration(int numSteps);       // Main iteration loop

    //=========================================================================
    // Mesh generation
    //=========================================================================
    std::vector<std::array<int, 3>> getTriangles() const;  // Get triangle mesh from cell neighbors

private:
    // Initial grid: connect `center` to the cell at (x, y), creating it if needed.
    // `numPlaced` counts the cells created so far.
    void placeCell(int center, double x, double y, int& numPlaced);
    // Remove the zeros from every neighbour list (Fortran "treiem els zeros").
    void compactNeighborLists();
};

// File I/O class (originally esclec module)
class FileIO {
public:
    ToothModel* model;  // Reference to the main model

    int exitFlag;       // Set when the parameter file could not be read

    // The Fortran parameter array parap(:,1), 0-based: [0] time step, [1] cell
    // count, [2..31] the parameters in file order (see storeParameters()).
    std::array<double, NUM_PARAMETERS> parameters;
    std::array<std::string, NUM_PARAMETERS> parameterNames;

    std::vector<double> materialValues;   // Material values for OFF colouring
    double materialMax, materialMin;

    std::string argInputFile;             // Parameter file name

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
    void saveNeighbors(std::ostream& out);
    void saveMargins(std::ostream& out);
    void saveAsOFF(std::ostream& out);
    void writeParametersText(std::ostream& out);

    //=========================================================================
    // Load functions
    //=========================================================================
    void readParametersText(std::istream& in);           // Humppa format (value name)
    void readParametersToothMaker(std::istream& in);     // ToothMaker format (name==value)
    bool isToothMakerFormat(const std::string& text);    // Detect format (normalised text)
    int getParameterIndex(const std::string& name);      // Map parameter name to index
    void readInitialParameters();                        // Read the parameter file into the model

    //=========================================================================
    // Parameter transfer functions
    //=========================================================================
    void storeParameters();    // Copy parameters from model to `parameters`
    void loadParameters();     // Copy `parameters` to the model

    //=========================================================================
    // Visualization helpers
    //=========================================================================
    void calculateMaterial();         // Calculate material values for visualization
    void getColorMapping(double val, double minVal, double maxVal, std::array<double, 4>& color);
};

#endif // TOOTH_MODEL_HPP
