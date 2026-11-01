#ifndef FUNCTIONAL_SPACE_OPTI_OBSERVER_H
#define FUNCTIONAL_SPACE_OPTI_OBSERVER_H

#include <iostream>
#include <string>
#include <vector>
#include <cmath>
#include <random>
#include <numeric>
#include <limits>
#include <cstddef>

#include "Function.h"
#include "FunctionalSpace.h"

class FunctionalSpaceOptiObserver {

private:

    // The sequence of functional spaces
    std::vector<FunctionalSpace> trajectory;


    // ============================================================
    // STATISTICS
    // ============================================================

    double mean(const std::vector<double>& values) const {

        if (values.empty())
            return 0.0;

        double sum = std::accumulate(
            values.begin(),
            values.end(),
            0.0
        );

        return sum / values.size();
    }


    double standardDeviation(
        const std::vector<double>& values
    ) const {

        if (values.size() < 2)
            return 0.0;

        double mu = mean(values);

        double variance = 0.0;

        for (double x : values) {
            variance += std::pow(x - mu, 2);
        }

        variance /= values.size();

        return std::sqrt(variance);
    }


    // ============================================================
    // FLATTEN FUNCTIONAL SPACE
    // ============================================================

    /*
        evaluateSpace() produces:

        [
            [f1(x1), f1(x2), f1(x3), ...],
            [f2(x1), f2(x2), f2(x3), ...],
            [f3(x1), f3(x2), f3(x3), ...]
        ]

        We flatten this into one distribution.
    */

    std::vector<double> flatten(
        const std::vector<std::vector<double>>& space
    ) const {

        std::vector<double> values;

        for (const auto& functionSpace : space) {

            for (double value : functionSpace) {
                values.push_back(value);
            }
        }

        return values;
    }


    // ============================================================
    // NORMAL DISTRIBUTION LOSS
    // ============================================================

    /*
        Target:

            N(0, 1)

        Therefore:

            target mean = 0
            target std  = 1

        Loss:

            L = mean^2 + (std - 1)^2
    */

    double normalLoss(
        const std::vector<double>& values
    ) const {

        if (values.empty())
            return INFINITY;

        double mu = mean(values);
        double sigma = standardDeviation(values);

        double meanLoss =
            std::pow(mu, 2);

        double varianceLoss =
            std::pow(sigma - 1.0, 2);

        return meanLoss + varianceLoss;
    }


    // ============================================================
    // RANDOM FUNCTION PERTURBATION
    // ============================================================

    /*
        This is intentionally left abstract.

        The idea is:

            current function
                    ↓
              perturb function
                    ↓
              evaluate space
                    ↓
              calculate loss
                    ↓
            keep/reject change
    */

    Function perturbFunction(
        const Function& function,
        double magnitude
    ) {

        static std::random_device rd;
        static std::mt19937 generator(rd());

        std::normal_distribution<double>
            noise(0.0, magnitude);


        // Randomly modify the function parameters.

        double scale =
            1.0 + noise(generator);

        double shift =
            noise(generator);


        std::string oldName =
            function.getName();


        return Function(
            oldName + "*",

            [scale, shift, function](double x) {

                return
                    scale *
                    function.evaluate(x)
                    + shift;
            },
            "f'(x) = " + std::to_string(scale) + "*f(x) + " + std::to_string(shift)
        );
    }


public:


    // ============================================================
    // CONSTRUCTOR
    // ============================================================

    FunctionalSpaceOptiObserver() = default;


    // ============================================================
    // ADD STATE
    // ============================================================

    void addState(
        const FunctionalSpace& space
    ) {

        trajectory.push_back(space);
    }


    // ============================================================
    // GET CURRENT STATE
    // ============================================================

    FunctionalSpace& currentState() {

        return trajectory.back();
    }


    const FunctionalSpace& currentState() const {

        return trajectory.back();
    }


    // ============================================================
    // EVALUATE CURRENT SPACE
    // ============================================================

    std::vector<std::vector<double>>
    evaluateCurrentSpace() const {

        if (trajectory.empty())
            return {};

        return trajectory.back().evaluateSpace();
    }


    // ============================================================
    // CALCULATE CURRENT LOSS
    // ============================================================

    double currentLoss() const {

        if (trajectory.empty())
            return INFINITY;

        auto space =
            trajectory.back().evaluateSpace();

        auto values =
            flatten(space);

        return normalLoss(values);
    }


    // ============================================================
    // OPTIMIZE ONE TRAJECTORY STEP
    // ============================================================

    void optimizeStep(
        double perturbationMagnitude = 0.01
    ) {

        if (trajectory.empty())
            return;


        FunctionalSpace current =
            trajectory.back();


        double currentLoss =
            normalLoss(
                flatten(
                    current.evaluateSpace()
                )
            );


        FunctionalSpace candidate =
            current;


        // --------------------------------------------------------
        // Perturb every function
        // --------------------------------------------------------

        for (
            std::size_t i = 0;
            i < candidate.functions.size();
            ++i
        ) {

            candidate.functions[i] =
                perturbFunction(
                    candidate.functions[i],
                    perturbationMagnitude
                );
        }


        // --------------------------------------------------------
        // Evaluate candidate
        // --------------------------------------------------------

        auto candidateValues =
            flatten(
                candidate.evaluateSpace()
            );


        double candidateLoss =
            normalLoss(candidateValues);


        // --------------------------------------------------------
        // Accept if better
        // --------------------------------------------------------

        if (candidateLoss < currentLoss) {

            trajectory.push_back(candidate);

        } else {

            // Keep trajectory explicit even when
            // optimization fails to improve.

            trajectory.push_back(current);
        }
    }


    // ============================================================
    // RUN OPTIMIZATION
    // ============================================================

    void optimize(
        std::size_t iterations,
        double perturbationMagnitude = 0.01
    ) {

        for (
            std::size_t i = 0;
            i < iterations;
            ++i
        ) {

            optimizeStep(
                perturbationMagnitude
            );
        }
    }


    // ============================================================
    // TRAJECTORY ACCESS
    // ============================================================

    const std::vector<FunctionalSpace>&
    getTrajectory() const {

        return trajectory;
    }


    // ============================================================
    // FINAL DISTRIBUTION
    // ============================================================

    std::vector<double> finalDistribution() const {

        if (trajectory.empty())
            return {};

        return flatten(
            trajectory.back().evaluateSpace()
        );
    }


    // ============================================================
    // PRINT CURRENT STATISTICS
    // ============================================================

    void printStatistics() const {

        auto values =
            finalDistribution();

        double mu =
            mean(values);

        double sigma =
            standardDeviation(values);

        std::cout
            << "Mean: "
            << mu
            << "\n";

        std::cout
            << "Std Dev: "
            << sigma
            << "\n";

        std::cout
            << "Loss: "
            << normalLoss(values)
            << "\n";
    }

    void observeFunctionFamily() const {

        if (trajectory.empty()) {
            std::cout << "No states recorded.\n";
            return;
        }

        std::cout << "\n========================================\n";
        std::cout << "       FUNCTIONAL SPACE TRAJECTORY\n";
        std::cout << "========================================\n";

        for (std::size_t i = 0; i < trajectory.size(); ++i) {

            const auto& space = trajectory[i];

            std::cout << "\nState " << i + 1 << ":\n\n";

            // --------------------------------------------------------
            // Family of Functions
            // --------------------------------------------------------

            std::cout << "Family of Functions: {";

            for (std::size_t j = 0;
                j < space.functions.size();
                ++j) {

                std::cout
                    << space.functions[j].getName()
                    << ": "
                    << space.functions[j].getFormula();

                if (j < space.functions.size() - 1) {
                    std::cout << ", ";
                }
            }

            std::cout << "}\n";


            // --------------------------------------------------------
            // State information
            // --------------------------------------------------------

            std::cout
                << "Number of Functions: "
                << space.functions.size()
                << "\n";

            std::cout
                << "Number of Observations: "
                << space.v.size()
                << "\n";

            std::cout
                << "State Size: "
                << space.size()
                << "\n";


            // --------------------------------------------------------
            // Distribution statistics
            // --------------------------------------------------------

            std::vector<double> values =
                flatten(space.evaluateSpace());

            double stateMean =
                mean(values);

            double stateStd =
                standardDeviation(values);

            double stateLoss =
                normalLoss(values);

            std::cout
                << "Mean: "
                << stateMean
                << "\n";

            std::cout
                << "Standard Deviation: "
                << stateStd
                << "\n";

            std::cout
                << "Normal Distribution Loss: "
                << stateLoss
                << "\n";


            // --------------------------------------------------------
            // State separator
            // --------------------------------------------------------

            std::cout
                << "\n----------------------------------------\n";
        }
    }
};

#endif // FUNCTIONAL_SPACE_OPTI_OBSERVER_H
