#include "FunctionalSpaceCSVLoader.h"
#include "FunctionalSpaceOptimizer.h"
#include "FunctionalSpace.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;


// ============================================================
// CONFIGURATION
// ============================================================

struct PredictionTest {

    std::size_t scope;

    std::string name;
};


// ============================================================
// CONSOLE LOGGING
// ============================================================

enum class LogLevel {

    INFO,
    DEBUG,
    WARNING,
    ERROR
};


static std::string timestamp() {

    const auto now =
        std::chrono::system_clock::now();

    const std::time_t time =
        std::chrono::system_clock::to_time_t(now);

    std::tm localTime{};

#ifdef _WIN32

    localtime_s(
        &localTime,
        &time
    );

#else

    localtime_r(
        &time,
        &localTime
    );

#endif

    std::ostringstream stream;

    stream
        << std::put_time(
            &localTime,
            "%Y-%m-%d %H:%M:%S"
        );

    return stream.str();
}


static std::string levelName(
    LogLevel level
) {

    switch (level) {

        case LogLevel::INFO:
            return "INFO";

        case LogLevel::DEBUG:
            return "DEBUG";

        case LogLevel::WARNING:
            return "WARN";

        case LogLevel::ERROR:
            return "ERROR";
    }

    return "UNKNOWN";
}


static void logMessage(
    LogLevel level,
    const std::string& message
) {

    std::cout
        << "["
        << timestamp()
        << "] ["
        << levelName(level)
        << "] "
        << message
        << "\n";
}


// ============================================================
// TIMER
// ============================================================

class ScopedTimer {

private:

    std::string operation_;

    std::chrono::steady_clock::time_point start_;


public:

    ScopedTimer(
        const std::string& operation
    )
        : operation_(operation),
          start_(
              std::chrono::steady_clock::now()
          )
    {

        logMessage(
            LogLevel::INFO,
            "START: " + operation_
        );
    }


    ~ScopedTimer() {

        const auto end =
            std::chrono::steady_clock::now();

        const auto milliseconds =
            std::chrono::duration_cast<
                std::chrono::milliseconds
            >(
                end - start_
            ).count();

        std::ostringstream message;

        message
            << "COMPLETE: "
            << operation_
            << " | elapsed="
            << milliseconds
            << " ms";

        logMessage(
            LogLevel::INFO,
            message.str()
        );
    }
};


// ============================================================
// METRICS
// ============================================================

struct ComparisonMetrics {

    double mae = 0.0;

    double mse = 0.0;

    double rmse = 0.0;

    double maxAbsoluteError = 0.0;
};


// ============================================================
// CSV DATA
// ============================================================

struct CSVData {

    std::vector<std::string> headers;

    std::vector<std::vector<double>> rows;
};


// ============================================================
// READ CSV
// ============================================================

CSVData readCSV(
    const std::string& filename
) {

    std::ifstream input(
        filename
    );

    if (
        !input.is_open()
    ) {

        throw std::runtime_error(
            "Unable to open CSV: " +
            filename
        );
    }

    CSVData data;

    std::string line;


    // --------------------------------------------------------
    // HEADER
    // --------------------------------------------------------

    if (
        !std::getline(
            input,
            line
        )
    ) {

        throw std::runtime_error(
            "CSV is empty: " +
            filename
        );
    }


    {
        std::stringstream stream(
            line
        );

        std::string field;

        while (
            std::getline(
                stream,
                field,
                ','
            )
        ) {

            data.headers.push_back(
                field
            );
        }
    }


    // --------------------------------------------------------
    // DATA
    // --------------------------------------------------------

    while (
        std::getline(
            input,
            line
        )
    ) {

        if (
            line.empty()
        ) {

            continue;
        }

        std::stringstream stream(
            line
        );

        std::string field;

        std::vector<double> row;


        while (
            std::getline(
                stream,
                field,
                ','
            )
        ) {

            try {

                row.push_back(
                    std::stod(field)
                );

            }
            catch (...) {

                throw std::runtime_error(
                    "Invalid numeric value in CSV: " +
                    field
                );
            }
        }


        if (
            !row.empty()
        ) {

            data.rows.push_back(
                row
            );
        }
    }


    input.close();

    return data;
}


// ============================================================
// SAVE FUNCTIONAL SPACE CSV
// ============================================================

void saveFunctionalSpaceCSV(
    const FunctionalSpace& space,
    const std::string& filename
) {

    logMessage(
        LogLevel::INFO,
        "Saving FunctionalSpace CSV: " +
        filename
    );


    std::ofstream output(
        filename
    );


    if (
        !output.is_open()
    ) {

        throw std::runtime_error(
            "Unable to create CSV: " +
            filename
        );
    }


    output
        << std::setprecision(15);


    // --------------------------------------------------------
    // HEADER
    // --------------------------------------------------------

    output
        << "Vector";


    for (
        const auto& function :
        space.functions
    ) {

        output
            << ","
            << function.getName();
    }


    output
        << "\n";


    // --------------------------------------------------------
    // VALUES
    // --------------------------------------------------------

    for (
        std::size_t row = 0;
        row < space.v.size();
        ++row
    ) {

        output
            << space.v[row];


        for (
            std::size_t functionIndex = 0;
            functionIndex < space.functions.size();
            ++functionIndex
        ) {

            output
                << ","
                << space.functions[
                    functionIndex
                ].evaluate(
                    space.v[row]
                );
        }


        output
            << "\n";
    }


    output.close();


    std::ostringstream message;

    message
        << "CSV saved successfully: "
        << filename
        << " | rows="
        << space.v.size()
        << " | functions="
        << space.functionCount();


    logMessage(
        LogLevel::INFO,
        message.str()
    );
}


// ============================================================
// EXTRACT VECTOR FROM CSV
// ============================================================

std::vector<double>
extractCSVVector(
    const CSVData& data
) {

    if (
        data.rows.empty()
    ) {

        throw std::runtime_error(
            "CSV contains no rows."
        );
    }


    std::vector<double> vector;


    vector.reserve(
        data.rows.size()
    );


    for (
        const auto& row :
        data.rows
    ) {

        if (
            row.empty()
        ) {

            throw std::runtime_error(
                "CSV contains an empty row."
            );
        }


        vector.push_back(
            row[0]
        );
    }


    return vector;
}


// ============================================================
// EXTRACT SOLVED VECTOR
// ============================================================

std::vector<double>
extractSolvedVector(
    const CSVData& solvedCSV
) {

    return extractCSVVector(
        solvedCSV
    );
}


// ============================================================
// BUILD HYBRID VECTOR
// ============================================================
//
// Given:
//
//     n = total rows
//     y = prediction scope
//     j = n - y
//
// Construct:
//
//     source_vector[0 ... j-1]
//     +
//     solved_vector[j ... n-1]
//
// ============================================================

std::vector<double>
buildHybridVector(
    const std::vector<double>& sourceVector,
    const std::vector<double>& solvedVector,
    std::size_t predictionScope
) {

    if (
        sourceVector.size() !=
        solvedVector.size()
    ) {

        throw std::runtime_error(
            "Source and solved CSVs have "
            "different numbers of rows."
        );
    }


    const std::size_t n =
        sourceVector.size();


    if (
        predictionScope == 0
    ) {

        throw std::runtime_error(
            "Prediction scope must be greater than zero."
        );
    }


    if (
        predictionScope > n
    ) {

        throw std::runtime_error(
            "Prediction scope cannot exceed "
            "the number of rows."
        );
    }


    const std::size_t j =
        n - predictionScope;


    // --------------------------------------------------------
    // LOG CONSTRUCTION
    // --------------------------------------------------------

    std::ostringstream message;


    message
        << "Building hybrid vector | "
        << "n=" << n
        << " | scope=" << predictionScope
        << " | split=" << j;


    logMessage(
        LogLevel::INFO,
        message.str()
    );


    // --------------------------------------------------------
    // CREATE VECTOR
    // --------------------------------------------------------

    std::vector<double> hybrid;


    hybrid.reserve(
        n
    );


    // --------------------------------------------------------
    // ORIGINAL SOURCE PORTION
    // --------------------------------------------------------

    for (
        std::size_t i = 0;
        i < j;
        ++i
    ) {

        hybrid.push_back(
            sourceVector[i]
        );
    }


    // --------------------------------------------------------
    // SOLVED CSV PORTION
    // --------------------------------------------------------

    for (
        std::size_t i = j;
        i < n;
        ++i
    ) {

        hybrid.push_back(
            solvedVector[i]
        );
    }


    // --------------------------------------------------------
    // LOG RESULT
    // --------------------------------------------------------

    std::ostringstream resultMessage;


    resultMessage
        << "Hybrid vector constructed | "
        << "source rows=" << j
        << " | solved rows=" << predictionScope
        << " | total rows=" << hybrid.size();


    logMessage(
        LogLevel::DEBUG,
        resultMessage.str()
    );


    return hybrid;
}


// ============================================================
// CREATE HYBRID FUNCTIONAL SPACE
// ============================================================

FunctionalSpace
createHybridSpace(
    const std::vector<double>& hybridVector,
    const std::vector<Function>& solvedFunctionals
) {

    std::ostringstream message;


    message
        << "Creating hybrid FunctionalSpace | "
        << "vector size=" << hybridVector.size()
        << " | functions=" << solvedFunctionals.size();


    logMessage(
        LogLevel::INFO,
        message.str()
    );


    return FunctionalSpace(
        hybridVector,
        solvedFunctionals
    );
}


// ============================================================
// COMPARE CSV VALUES
// ============================================================

ComparisonMetrics compareCSV(
    const CSVData& sourceCSV,
    const CSVData& hybridCSV
) {

    logMessage(
        LogLevel::INFO,
        "Comparing source CSV against hybrid CSV."
    );


    if (
        sourceCSV.rows.size() !=
        hybridCSV.rows.size()
    ) {

        throw std::runtime_error(
            "Source and hybrid CSVs have "
            "different row counts."
        );
    }


    if (
        sourceCSV.rows.empty()
    ) {

        return {};
    }


    if (
        sourceCSV.rows[0].size() !=
        hybridCSV.rows[0].size()
    ) {

        throw std::runtime_error(
            "Source and hybrid CSVs have "
            "different column counts."
        );
    }


    ComparisonMetrics metrics;


    double absoluteErrorSum =
        0.0;


    double squaredErrorSum =
        0.0;


    std::size_t count =
        0;


    // --------------------------------------------------------
    // IGNORE COLUMN ZERO
    // --------------------------------------------------------

    for (
        std::size_t row = 0;
        row < sourceCSV.rows.size();
        ++row
    ) {

        if (
            sourceCSV.rows[row].size() !=
            hybridCSV.rows[row].size()
        ) {

            throw std::runtime_error(
                "CSV row column count mismatch at row " +
                std::to_string(row)
            );
        }


        for (
            std::size_t column = 1;
            column < sourceCSV.rows[row].size();
            ++column
        ) {

            const double error =
                hybridCSV.rows[row][column] -
                sourceCSV.rows[row][column];


            const double absoluteError =
                std::abs(
                    error
                );


            absoluteErrorSum +=
                absoluteError;


            squaredErrorSum +=
                error * error;


            metrics.maxAbsoluteError =
                std::max(
                    metrics.maxAbsoluteError,
                    absoluteError
                );


            ++count;
        }
    }


    if (
        count > 0
    ) {

        metrics.mae =
            absoluteErrorSum /
            static_cast<double>(
                count
            );


        metrics.mse =
            squaredErrorSum /
            static_cast<double>(
                count
            );


        metrics.rmse =
            std::sqrt(
                metrics.mse
            );
    }


    // --------------------------------------------------------
    // LOG METRICS
    // --------------------------------------------------------

    std::ostringstream message;


    message
        << "Comparison complete | "
        << "MAE=" << metrics.mae
        << " | RMSE=" << metrics.rmse
        << " | MaxError=" << metrics.maxAbsoluteError;


    logMessage(
        LogLevel::INFO,
        message.str()
    );


    return metrics;
}


// ============================================================
// SAVE COMPARISON REPORT
// ============================================================

void saveComparisonReport(
    const std::string& filename,
    const PredictionTest& test,
    std::size_t n,
    const OptimizationResult& sourceOptimization,
    const OptimizationResult& hybridOptimization,
    const ComparisonMetrics& metrics
) {

    logMessage(
        LogLevel::INFO,
        "Saving comparison report: " +
        filename
    );


    std::ofstream output(
        filename
    );


    if (
        !output.is_open()
    ) {

        throw std::runtime_error(
            "Unable to create report: " +
            filename
        );
    }


    const std::size_t j =
        n - test.scope;


    output
        << std::fixed
        << std::setprecision(10);


    // --------------------------------------------------------
    // HEADER
    // --------------------------------------------------------

    output
        << "FUNCTIONAL SPACE PREDICTION TEST\n"
        << "=================================\n\n";


    output
        << "Test: "
        << test.name
        << "\n";


    output
        << "Total rows: "
        << n
        << "\n";


    output
        << "Prediction scope (y): "
        << test.scope
        << "\n";


    output
        << "Split index (j): "
        << j
        << "\n\n";


    // --------------------------------------------------------
    // DATA CONSTRUCTION
    // --------------------------------------------------------

    output
        << "DATA CONSTRUCTION\n"
        << "-----------------\n";


    if (
        j > 0
    ) {

        output
            << "source_vector[0 ... "
            << j - 1
            << "]\n";
    }


    output
        << "+\n";


    output
        << "solved_vector["
        << j
        << " ... "
        << n - 1
        << "]\n\n";


    // --------------------------------------------------------
    // SOURCE OPTIMIZATION
    // --------------------------------------------------------

    output
        << "SOURCE FUNCTIONAL SPACE OPTIMIZATION\n"
        << "-------------------------------------\n";


    output
        << "Initial loss: "
        << sourceOptimization.initialLoss
        << "\n";


    output
        << "Final loss:   "
        << sourceOptimization.finalLoss
        << "\n";


    output
        << "Iterations:   "
        << sourceOptimization.iterations
        << "\n";


    output
        << "Converged:    "
        << std::boolalpha
        << sourceOptimization.converged
        << "\n\n";


    // --------------------------------------------------------
    // HYBRID OPTIMIZATION
    // --------------------------------------------------------

    output
        << "HYBRID FUNCTIONAL SPACE OPTIMIZATION\n"
        << "-------------------------------------\n";


    output
        << "Initial loss: "
        << hybridOptimization.initialLoss
        << "\n";


    output
        << "Final loss:   "
        << hybridOptimization.finalLoss
        << "\n";


    output
        << "Iterations:   "
        << hybridOptimization.iterations
        << "\n";


    output
        << "Converged:    "
        << std::boolalpha
        << hybridOptimization.converged
        << "\n\n";


    // --------------------------------------------------------
    // COMPARISON
    // --------------------------------------------------------

    output
        << "SOURCE VS HYBRID\n"
        << "----------------\n";


    output
        << "MAE:               "
        << metrics.mae
        << "\n";


    output
        << "MSE:               "
        << metrics.mse
        << "\n";


    output
        << "RMSE:              "
        << metrics.rmse
        << "\n";


    output
        << "Maximum Abs Error: "
        << metrics.maxAbsoluteError
        << "\n\n";


    // --------------------------------------------------------
    // PREDICTION REGION
    // --------------------------------------------------------

    output
        << "PREDICTION REGION\n"
        << "-----------------\n";


    output
        << "Rows "
        << j
        << " through "
        << n - 1
        << " are the predicted region.\n";


    output
        << "Number of predicted rows: "
        << test.scope
        << "\n";


    output.close();


    logMessage(
        LogLevel::INFO,
        "Comparison report saved successfully."
    );
}


// ============================================================
// PRINT TEST SUMMARY
// ============================================================

void printTestSummary(
    const PredictionTest& test,
    std::size_t n,
    const OptimizationResult& sourceOptimization,
    const OptimizationResult& hybridOptimization,
    const ComparisonMetrics& metrics
) {

    const std::size_t j =
        n - test.scope;


    std::cout
        << "\n\n========================================\n"
        << "TEST: "
        << test.name
        << "\n"
        << "========================================\n";


    std::cout
        << "n = "
        << n
        << "\n";


    std::cout
        << "y = "
        << test.scope
        << "\n";


    std::cout
        << "j = n - y = "
        << j
        << "\n";


    std::cout
        << "\nSource optimization:\n"
        << "  Initial loss = "
        << sourceOptimization.initialLoss
        << "\n"
        << "  Final loss   = "
        << sourceOptimization.finalLoss
        << "\n"
        << "  Iterations   = "
        << sourceOptimization.iterations
        << "\n"
        << "  Converged    = "
        << std::boolalpha
        << sourceOptimization.converged
        << "\n";


    std::cout
        << "\nHybrid optimization:\n"
        << "  Initial loss = "
        << hybridOptimization.initialLoss
        << "\n"
        << "  Final loss   = "
        << hybridOptimization.finalLoss
        << "\n"
        << "  Iterations   = "
        << hybridOptimization.iterations
        << "\n"
        << "  Converged    = "
        << std::boolalpha
        << hybridOptimization.converged
        << "\n";


    std::cout
        << "\nPrediction accuracy:\n"
        << "  MAE       = "
        << metrics.mae
        << "\n"
        << "  MSE       = "
        << metrics.mse
        << "\n"
        << "  RMSE      = "
        << metrics.rmse
        << "\n"
        << "  Max error = "
        << metrics.maxAbsoluteError
        << "\n";
}


// ============================================================
// MAIN
// ============================================================

int main() {

    try {

        // ====================================================
        // PATHS
        // ====================================================

        const std::string sourceFilename =
            "../data/raw/daily_IBM.csv";


        const std::string processedDirectory =
            "../data/processed";


        fs::create_directories(
            processedDirectory
        );


        // ====================================================
        // START
        // ====================================================

        logMessage(
            LogLevel::INFO,
            "========================================"
        );


        logMessage(
            LogLevel::INFO,
            "FUNCTIONAL SPACE PREDICTOR"
        );


        logMessage(
            LogLevel::INFO,
            "========================================"
        );


        logMessage(
            LogLevel::INFO,
            "Pipeline initialized."
        );


        logMessage(
            LogLevel::INFO,
            "Source CSV: " +
            sourceFilename
        );


        logMessage(
            LogLevel::INFO,
            "Processed directory: " +
            processedDirectory
        );


        // ====================================================
        // PREDICTION TESTS
        // ====================================================

        const std::vector<PredictionTest> tests = {

            {10, "scope_10"},

            {20, "scope_20"},

            {30, "scope_30"},

            {50, "scope_50"}
        };


        logMessage(
            LogLevel::INFO,
            "Configured prediction tests: " +
            std::to_string(
                tests.size()
            )
        );


        // ====================================================
        // LOAD ORIGINAL FUNCTIONAL SPACE
        // ====================================================

        logMessage(
            LogLevel::INFO,
            "Loading source FunctionalSpace."
        );


        FunctionalSpace sourceSpace =
            FunctionalSpaceCSVLoader::load(
                sourceFilename
            );


        if (
            sourceSpace.v.empty() ||
            sourceSpace.functions.empty()
        ) {

            throw std::runtime_error(
                "The source FunctionalSpace is empty."
            );
        }


        const std::size_t n =
            sourceSpace.v.size();


        logMessage(
            LogLevel::INFO,
            "Source FunctionalSpace loaded successfully."
        );


        logMessage(
            LogLevel::INFO,
            "Rows: " +
            std::to_string(n)
        );


        logMessage(
            LogLevel::INFO,
            "Functions: " +
            std::to_string(
                sourceSpace.functionCount()
            )
        );


        // ====================================================
        // SOURCE FUNCTION NAMES
        // ====================================================

        for (
            std::size_t i = 0;
            i < sourceSpace.functionCount();
            ++i
        ) {

            logMessage(
                LogLevel::DEBUG,
                "Source function[" +
                std::to_string(i) +
                "]: " +
                sourceSpace.getFunction(i).getName()
            );
        }


        // ====================================================
        // SAVE SOURCE CSV
        // ====================================================

        const std::string sourceOutput =
            processedDirectory +
            "/source.csv";


        saveFunctionalSpaceCSV(
            sourceSpace,
            sourceOutput
        );


        // ====================================================
        // READ ORIGINAL CSV
        // ====================================================

        logMessage(
            LogLevel::INFO,
            "Reading original source CSV for validation."
        );


        CSVData sourceCSV =
            readCSV(
                sourceFilename
            );


        std::vector<double> sourceVector =
            extractCSVVector(
                sourceCSV
            );


        if (
            sourceVector.size() != n
        ) {

            throw std::runtime_error(
                "The CSV row count and "
                "FunctionalSpace vector size differ."
            );
        }


        {
            std::ostringstream message;

            message
                << "Source CSV validated | rows="
                << sourceCSV.rows.size()
                << " | columns="
                << sourceCSV.headers.size();

            logMessage(
                LogLevel::INFO,
                message.str()
            );
        }


        // ====================================================
        // OPTIMIZATION VECTOR
        // ====================================================

        Vector optimizationVector(
            0.5,
            0.0
        );


        logMessage(
            LogLevel::INFO,
            "Optimization vector initialized."
        );


        // ====================================================
        // STEP 1
        // OPTIMIZE SOURCE FUNCTIONAL SPACE
        // ====================================================

        logMessage(
            LogLevel::INFO,
            "========================================"
        );


        logMessage(
            LogLevel::INFO,
            "STEP 1: SOLVE SOURCE FUNCTIONAL SPACE"
        );


        logMessage(
            LogLevel::INFO,
            "========================================"
        );


        FunctionalOptimizer sourceOptimizer(

            optimizationVector,

            0.0,
            1.0,

            1000,

            10,
            10,

            0.05,

            25,

            200,

            1e-6
        );


        OptimizationResult sourceOptimization;


        {

            ScopedTimer timer(
                "Source FunctionalSpace optimization"
            );


            sourceOptimization =
                sourceOptimizer.optimize(
                    sourceSpace
                );
        }


        logMessage(
            LogLevel::INFO,
            "Source optimization finished."
        );


        logMessage(
            LogLevel::INFO,
            "Initial loss: " +
            std::to_string(
                sourceOptimization.initialLoss
            )
        );


        logMessage(
            LogLevel::INFO,
            "Final loss: " +
            std::to_string(
                sourceOptimization.finalLoss
            )
        );


        logMessage(
            LogLevel::INFO,
            "Iterations: " +
            std::to_string(
                sourceOptimization.iterations
            )
        );


        logMessage(
            LogLevel::INFO,
            "Converged: " +
            std::string(
                sourceOptimization.converged
                    ? "true"
                    : "false"
            )
        );


        // ====================================================
        // STEP 2
        // CREATE SOLVED SPACE
        // ====================================================

        FunctionalSpace solvedSpace =
            sourceOptimization.space;


        logMessage(
            LogLevel::INFO,
            "Solved FunctionalSpace created."
        );


        // ====================================================
        // SAVE SOLVED CSV
        // ====================================================

        const std::string solvedOutput =
            processedDirectory +
            "/solved.csv";


        saveFunctionalSpaceCSV(
            solvedSpace,
            solvedOutput
        );


        // ====================================================
        // STEP 3
        // READ SOLVED CSV
        // ====================================================

        logMessage(
            LogLevel::INFO,
            "Reading solved CSV."
        );


        CSVData solvedCSV =
            readCSV(
                solvedOutput
            );


        std::vector<double> solvedVector =
            extractSolvedVector(
                solvedCSV
            );


        if (
            solvedVector.size() != n
        ) {

            throw std::runtime_error(
                "Solved CSV contains a different "
                "number of rows than the source CSV."
            );
        }


        logMessage(
            LogLevel::INFO,
            "Solved CSV validated | rows=" +
            std::to_string(
                solvedCSV.rows.size()
            )
        );


        // ====================================================
        // COLLECT SOLVED FUNCTIONALS
        // ====================================================

        std::vector<Function> solvedFunctionals =
            solvedSpace.functions;


        logMessage(
            LogLevel::INFO,
            "Collected solved functionals: " +
            std::to_string(
                solvedFunctionals.size()
            )
        );


        for (
            std::size_t i = 0;
            i < solvedFunctionals.size();
            ++i
        ) {

            const Function& function =
                solvedFunctionals[i];


            std::ostringstream message;


            message
                << "Solved functional["
                << i
                << "] "
                << function.getName()
                << " | vertical="
                << function.verticalOffset()
                << " | horizontal="
                << function.horizontalOffset()
                << " | scale="
                << function.scale();


            logMessage(
                LogLevel::DEBUG,
                message.str()
            );
        }


        // ====================================================
        // STEP 4
        // RUN EACH PREDICTION TEST
        // ====================================================

        for (
            const PredictionTest& test :
            tests
        ) {

            logMessage(
                LogLevel::INFO,
                "========================================"
            );


            logMessage(
                LogLevel::INFO,
                "STARTING TEST: " +
                test.name
            );


            // ------------------------------------------------
            // VALIDATE SCOPE
            // ------------------------------------------------

            if (
                test.scope == 0 ||
                test.scope > n
            ) {

                logMessage(
                    LogLevel::WARNING,
                    "Skipping " +
                    test.name +
                    ": invalid scope."
                );


                continue;
            }


            const std::size_t j =
                n - test.scope;


            logMessage(
                LogLevel::INFO,
                "Prediction scope y=" +
                std::to_string(
                    test.scope
                )
            );


            logMessage(
                LogLevel::INFO,
                "Total rows n=" +
                std::to_string(
                    n
                )
            );


            logMessage(
                LogLevel::INFO,
                "Split index j=" +
                std::to_string(
                    j
                )
            );


            {
                std::ostringstream message;

                message
                    << "Hybrid construction: source[0.."
                    << (j > 0 ? j - 1 : 0)
                    << "] + solved["
                    << j
                    << ".."
                    << n - 1
                    << "]";

                logMessage(
                    LogLevel::INFO,
                    message.str()
                );
            }


            // ------------------------------------------------
            // BUILD HYBRID VECTOR
            // ------------------------------------------------

            std::vector<double> hybridVector;


            {

                ScopedTimer timer(
                    "Build hybrid vector: " +
                    test.name
                );


                hybridVector =
                    buildHybridVector(
                        sourceVector,
                        solvedVector,
                        test.scope
                    );
            }


            // ------------------------------------------------
            // CREATE HYBRID SPACE
            // ------------------------------------------------

            FunctionalSpace hybridSpace =
                createHybridSpace(
                    hybridVector,
                    solvedFunctionals
                );


            // ------------------------------------------------
            // OPTIMIZE HYBRID SPACE
            // ------------------------------------------------

            logMessage(
                LogLevel::INFO,
                "Optimizing hybrid FunctionalSpace."
            );


            FunctionalOptimizer hybridOptimizer(

                optimizationVector,

                0.0,
                1.0,

                1000,

                10,
                10,

                0.05,

                25,

                200,

                1e-6
            );


            OptimizationResult hybridOptimization;


            {

                ScopedTimer timer(
                    "Hybrid FunctionalSpace optimization: " +
                    test.name
                );


                hybridOptimization =
                    hybridOptimizer.optimize(
                        hybridSpace
                    );
            }


            FunctionalSpace predictedSpace =
                hybridOptimization.space;


            logMessage(
                LogLevel::INFO,
                "Hybrid optimization complete."
            );


            logMessage(
                LogLevel::INFO,
                "Hybrid initial loss: " +
                std::to_string(
                    hybridOptimization.initialLoss
                )
            );


            logMessage(
                LogLevel::INFO,
                "Hybrid final loss: " +
                std::to_string(
                    hybridOptimization.finalLoss
                )
            );


            logMessage(
                LogLevel::INFO,
                "Hybrid iterations: " +
                std::to_string(
                    hybridOptimization.iterations
                )
            );


            logMessage(
                LogLevel::INFO,
                "Hybrid converged: " +
                std::string(
                    hybridOptimization.converged
                        ? "true"
                        : "false"
                )
            );


            // ------------------------------------------------
            // SAVE HYBRID CSV
            // ------------------------------------------------

            const std::string hybridOutput =
                processedDirectory +
                "/" +
                test.name +
                "_hybrid.csv";


            saveFunctionalSpaceCSV(
                predictedSpace,
                hybridOutput
            );


            // ------------------------------------------------
            // READ HYBRID CSV
            // ------------------------------------------------

            logMessage(
                LogLevel::INFO,
                "Reloading generated hybrid CSV."
            );


            CSVData hybridCSV =
                readCSV(
                    hybridOutput
                );


            {
                std::ostringstream message;

                message
                    << "Hybrid CSV loaded | rows="
                    << hybridCSV.rows.size()
                    << " | columns="
                    << hybridCSV.headers.size();

                logMessage(
                    LogLevel::INFO,
                    message.str()
                );
            }


            // ------------------------------------------------
            // COMPARE
            // ------------------------------------------------

            ComparisonMetrics metrics;


            {

                ScopedTimer timer(
                    "Compare source vs hybrid: " +
                    test.name
                );


                metrics =
                    compareCSV(
                        sourceCSV,
                        hybridCSV
                    );
            }


            // ------------------------------------------------
            // SAVE REPORT
            // ------------------------------------------------

            const std::string reportOutput =
                processedDirectory +
                "/" +
                test.name +
                "_comparison.txt";


            saveComparisonReport(
                reportOutput,
                test,
                n,
                sourceOptimization,
                hybridOptimization,
                metrics
            );


            // ------------------------------------------------
            // PRINT SUMMARY
            // ------------------------------------------------

            printTestSummary(
                test,
                n,
                sourceOptimization,
                hybridOptimization,
                metrics
            );


            logMessage(
                LogLevel::INFO,
                "TEST COMPLETE: " +
                test.name
            );
        }


        // ====================================================
        // COMPLETE
        // ====================================================

        logMessage(
            LogLevel::INFO,
            "========================================"
        );


        logMessage(
            LogLevel::INFO,
            "PREDICTION PIPELINE COMPLETE"
        );


        logMessage(
            LogLevel::INFO,
            "========================================"
        );


        logMessage(
            LogLevel::INFO,
            "Processed directory: " +
            processedDirectory
        );


        return 0;
    }


    catch (
        const std::exception& exception
    ) {

        std::cerr
            << "\nERROR: "
            << exception.what()
            << "\n";

        std::cout << "\nExiting due to error.\n";

        return 1;
    }
}