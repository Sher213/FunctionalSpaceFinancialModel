#ifndef FUNCTIONAL_OPTIMIZER_H
#define FUNCTIONAL_OPTIMIZER_H

#include "FunctionalSpace.h"

#include <cmath>
#include <iostream>
#include <random>


struct OptimizationResult {

    FunctionalSpace space;

    double initialLoss = 0.0;

    double finalLoss = 0.0;

    int iterations = 0;

    bool converged = false;
};


class FunctionalOptimizer {

private:

    Vector vector_;

    double xMin_;

    double xMax_;

    int samples_;

    int gridWidth_;

    int gridHeight_;

    double perturbation_;

    int candidatesPerIteration_;

    int maxIterations_;

    double convergenceThreshold_;

    std::mt19937 generator_;


public:

    FunctionalOptimizer(
        Vector vector,
        double xMin,
        double xMax,
        int samples = 1000,
        int gridWidth = 10,
        int gridHeight = 10,
        double perturbation = 0.05,
        int candidatesPerIteration = 25,
        int maxIterations = 200,
        double convergenceThreshold = 1e-6
    )
        : vector_(vector),
          xMin_(xMin),
          xMax_(xMax),
          samples_(samples),
          gridWidth_(gridWidth),
          gridHeight_(gridHeight),
          perturbation_(perturbation),
          candidatesPerIteration_(
              candidatesPerIteration
          ),
          maxIterations_(
              maxIterations
          ),
          convergenceThreshold_(
              convergenceThreshold
          ),
          generator_(
              std::random_device{}()
          )
    {
    }


    // ============================================================
    // MAIN OPTIMIZATION
    // ============================================================

    OptimizationResult optimize(
        FunctionalSpace initialSpace
    ) {

        OptimizationResult result;

        result.space =
            initialSpace;


        double currentLoss =
            evaluate(
                result.space
            );


        result.initialLoss =
            currentLoss;


        std::cout
            << "Initial loss: "
            << currentLoss
            << "\n";


        for (
            int iteration = 0;
            iteration < maxIterations_;
            ++iteration
        ) {

            FunctionalSpace bestSpace =
                result.space;


            double bestLoss =
                currentLoss;


            // ----------------------------------------------------
            // Candidate generation
            // ----------------------------------------------------

            for (
                int candidate = 0;
                candidate < candidatesPerIteration_;
                ++candidate
            ) {

                FunctionalSpace candidateSpace =
                    perturb(
                        result.space
                    );


                double candidateLoss =
                    evaluate(
                        candidateSpace
                    );


                if (
                    candidateLoss <
                    bestLoss
                ) {

                    bestLoss =
                        candidateLoss;

                    bestSpace =
                        candidateSpace;
                }
            }


            // ----------------------------------------------------
            // No improvement
            // ----------------------------------------------------

            if (
                bestLoss >=
                currentLoss
            ) {

                result.converged =
                    true;

                result.iterations =
                    iteration;

                break;
            }


            // ----------------------------------------------------
            // Accept
            // ----------------------------------------------------

            result.space =
                bestSpace;


            currentLoss =
                bestLoss;


            result.iterations =
                iteration + 1;


            std::cout
                << "Iteration "
                << iteration + 1
                << " | Loss: "
                << currentLoss
                << "\n";


            if (
                currentLoss <
                convergenceThreshold_
            ) {

                result.converged =
                    true;

                break;
            }
        }


        result.finalLoss =
            currentLoss;


        return result;
    }


private:

    // ============================================================
    // OBJECTIVE
    // ============================================================

    double evaluate(
        const FunctionalSpace& space
    ) const {

        return
            space.intersectionLoss(
                vector_,
                xMin_,
                xMax_,
                samples_,
                gridWidth_,
                gridHeight_
            );
    }


    // ============================================================
    // PARAMETER PERTURBATION
    // ============================================================

    FunctionalSpace perturb(
        const FunctionalSpace& original
    ) {

        FunctionalSpace candidate =
            original;


        if (
            candidate.functions.empty()
        ) {

            return candidate;
        }


        std::uniform_int_distribution<int>
            functionDistribution(
                0,
                static_cast<int>(
                    candidate.functions.size()
                ) - 1
            );


        std::uniform_int_distribution<int>
            parameterDistribution(
                0,
                2
            );


        std::normal_distribution<double>
            noise(
                0.0,
                perturbation_
            );


        int functionIndex =
            functionDistribution(
                generator_
            );


        int parameter =
            parameterDistribution(
                generator_
            );


        Function& function =
            candidate.functions[
                functionIndex
            ];


        switch (parameter) {

            case 0:

                function.setVerticalOffset(
                    function.verticalOffset()
                    +
                    noise(generator_)
                );

                break;


            case 1:

                function.setHorizontalOffset(
                    function.horizontalOffset()
                    +
                    noise(generator_)
                );

                break;


            case 2:
            {

                double scale =
                    function.scale()
                    +
                    noise(generator_);


                if (
                    std::abs(scale)
                    <
                    0.01
                ) {

                    scale =
                        scale < 0.0
                        ?
                        -0.01
                        :
                        0.01;
                }


                function.setScale(
                    scale
                );

                break;
            }
        }


        return candidate;
    }
};

#endif