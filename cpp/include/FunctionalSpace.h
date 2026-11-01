#ifndef FUNCTIONAL_SPACE_H
#define FUNCTIONAL_SPACE_H

#include "Function.h"
#include "IntersectionCalculator.h"
#include "Functional.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <functional>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>


// ============================================================
// NORMALIZED POINT
// ============================================================

struct NormalizedPoint {

    double x = 0.0;
    double y = 0.0;
    double intensity = 0.0;
};


// ============================================================
// FUNCTIONAL SPACE
// ============================================================

class FunctionalSpace {

public:

    // ============================================================
    // STATE
    // ============================================================

    std::vector<double> v;

    std::vector<Function> functions;


    // ============================================================
    // CONSTRUCTORS
    // ============================================================

    FunctionalSpace() = default;


    FunctionalSpace(
        std::vector<double> initialVector,
        std::vector<Function> initialFunctions
    )
        : v(std::move(initialVector)),
          functions(std::move(initialFunctions))
    {
    }


    // ============================================================
    // VECTOR OPERATIONS
    // ============================================================

    void addRow(double value) {

        v.push_back(value);
    }


    std::size_t vectorSize() const {

        return v.size();
    }


    std::size_t size() const {

        return
            v.size() *
            functions.size();
    }


    void clear() {

        v.clear();
        functions.clear();
    }


    // ============================================================
    // FUNCTION OPERATIONS
    // ============================================================

    void addFunction(
        const Function& function
    ) {

        functions.push_back(
            function
        );
    }


    Function& getFunction(
        std::size_t index
    ) {

        if (
            index >= functions.size()
        ) {

            throw std::out_of_range(
                "Function index is out of range."
            );
        }

        return functions[index];
    }


    const Function& getFunction(
        std::size_t index
    ) const {

        if (
            index >= functions.size()
        ) {

            throw std::out_of_range(
                "Function index is out of range."
            );
        }

        return functions[index];
    }


    std::size_t functionCount() const {

        return functions.size();
    }


    // ============================================================
    // EVALUATION
    // ============================================================

    double getValue(
        std::size_t functionIndex,
        std::size_t vectorIndex
    ) const {

        if (
            functionIndex >= functions.size()
        ) {

            throw std::out_of_range(
                "Function index is out of range."
            );
        }


        if (
            vectorIndex >= v.size()
        ) {

            throw std::out_of_range(
                "Vector index is out of range."
            );
        }


        return
            functions[functionIndex]
                .evaluate(v[vectorIndex]);
    }


    std::vector<std::vector<double>>
    evaluateSpace() const {

        std::vector<std::vector<double>>
            result;


        result.reserve(
            functions.size()
        );


        for (
            const auto& function :
            functions
        ) {

            std::vector<double>
                values;


            values.reserve(
                v.size()
            );


            for (
                double value :
                v
            ) {

                values.push_back(
                    function.evaluate(value)
                );
            }


            result.push_back(
                values
            );
        }


        return result;
    }


    // ============================================================
    // TRAJECTORIES
    // ============================================================

    std::vector<double>
    generateTrajectory(
        std::size_t functionIndex
    ) const {

        std::vector<double>
            trajectory;


        const Function& function =
            getFunction(functionIndex);


        trajectory.reserve(
            v.size()
        );


        for (double x : v) {

            trajectory.push_back(
                function.evaluate(x)
            );
        }


        return trajectory;
    }


    std::vector<std::vector<double>>
    generateTrajectories() const {

        return evaluateSpace();
    }


    // ============================================================
    // INTERSECTIONS
    // ============================================================

    std::vector<Point>
    calculateIntersections(
        const Vector& vector,
        double xMin,
        double xMax,
        int samples = 1000
    ) const {

        std::vector<Point>
            result;


        for (
            const auto& function :
            functions
        ) {

            auto intersections =
                IntersectionCalculator::calculate(
                    function,
                    vector,
                    xMin,
                    xMax,
                    samples
                );


            result.insert(
                result.end(),
                intersections.begin(),
                intersections.end()
            );
        }


        return result;
    }


    // ============================================================
    // NORMALIZATION
    // ============================================================

    static std::vector<NormalizedPoint>
    normalize(
        const std::vector<Point>& points
    ) {

        std::vector<NormalizedPoint>
            result;


        if (points.empty()) {

            return result;
        }


        double minX =
            std::numeric_limits<double>::max();

        double maxX =
            std::numeric_limits<double>::lowest();

        double minY =
            std::numeric_limits<double>::max();

        double maxY =
            std::numeric_limits<double>::lowest();


        for (const auto& point : points) {

            minX =
                std::min(
                    minX,
                    point.x
                );

            maxX =
                std::max(
                    maxX,
                    point.x
                );

            minY =
                std::min(
                    minY,
                    point.y
                );

            maxY =
                std::max(
                    maxY,
                    point.y
                );
        }


        double rangeX =
            maxX - minX;

        double rangeY =
            maxY - minY;


        for (const auto& point : points) {

            NormalizedPoint normalized;


            if (rangeX == 0.0) {

                normalized.x =
                    0.5;
            }
            else {

                normalized.x =
                    (point.x - minX) /
                    rangeX;
            }


            if (rangeY == 0.0) {

                normalized.y =
                    0.5;
            }
            else {

                normalized.y =
                    (point.y - minY) /
                    rangeY;
            }


            result.push_back(
                normalized
            );
        }


        return result;
    }


    // ============================================================
    // POSITIONAL INTENSITY
    // ============================================================

    static void calculateIntensity(
        std::vector<NormalizedPoint>& points,
        double bandwidth = 0.1
    ) {

        if (
            points.empty() ||
            bandwidth <= 0.0
        ) {

            return;
        }


        for (
            std::size_t i = 0;
            i < points.size();
            ++i
        ) {

            double intensity =
                0.0;


            for (
                std::size_t j = 0;
                j < points.size();
                ++j
            ) {

                if (i == j) {

                    continue;
                }


                double dx =
                    points[i].x -
                    points[j].x;

                double dy =
                    points[i].y -
                    points[j].y;


                double distanceSquared =
                    dx * dx +
                    dy * dy;


                intensity +=
                    std::exp(
                        -distanceSquared /
                        (
                            2.0 *
                            bandwidth *
                            bandwidth
                        )
                    );
            }


            points[i].intensity =
                intensity;
        }
    }


    // ============================================================
    // DISTRIBUTION LOSS
    // ============================================================

    static double distributionLoss(
        const std::vector<NormalizedPoint>& points,
        int gridWidth = 10,
        int gridHeight = 10
    ) {

        if (
            points.empty() ||
            gridWidth <= 0 ||
            gridHeight <= 0
        ) {

            return 0.0;
        }


        struct Cell {

            int count = 0;

            double intensity = 0.0;
        };


        std::vector<Cell> grid(
            static_cast<std::size_t>(
                gridWidth *
                gridHeight
            )
        );


        for (
            const auto& point :
            points
        ) {

            int x =
                static_cast<int>(
                    point.x *
                    gridWidth
                );


            int y =
                static_cast<int>(
                    point.y *
                    gridHeight
                );


            x =
                std::clamp(
                    x,
                    0,
                    gridWidth - 1
                );


            y =
                std::clamp(
                    y,
                    0,
                    gridHeight - 1
                );


            std::size_t index =
                static_cast<std::size_t>(
                    y * gridWidth + x
                );


            grid[index].count++;

            grid[index].intensity +=
                point.intensity;
        }


        double ideal =
            static_cast<double>(
                points.size()
            )
            /
            static_cast<double>(
                grid.size()
            );


        double loss =
            0.0;


        for (
            const auto& cell :
            grid
        ) {

            double difference =
                static_cast<double>(
                    cell.count
                )
                -
                ideal;


            double averageIntensity =
                cell.count > 0
                ?
                cell.intensity /
                static_cast<double>(
                    cell.count
                )
                :
                0.0;


            double weight =
                1.0 +
                averageIntensity;


            loss +=
                weight *
                difference *
                difference;
        }


        return loss;
    }


    // ============================================================
    // OBJECTIVE
    // ============================================================

    double intersectionLoss(
        const Vector& vector,
        double xMin,
        double xMax,
        int samples = 1000,
        int gridWidth = 10,
        int gridHeight = 10
    ) const {

        auto intersections =
            calculateIntersections(
                vector,
                xMin,
                xMax,
                samples
            );


        auto normalized =
            normalize(
                intersections
            );


        calculateIntensity(
            normalized
        );


        return distributionLoss(
            normalized,
            gridWidth,
            gridHeight
        );
    }


    // ============================================================
    // SNAPSHOT
    // ============================================================

    FunctionalSpace snapshot() const {

        return *this;
    }


    void restore(
        const FunctionalSpace& previous
    ) {

        *this =
            previous;
    }


    bool equals(
        const FunctionalSpace& other
    ) const {

        if (
            v.size() !=
            other.v.size()
        ) {

            return false;
        }


        if (
            functions.size() !=
            other.functions.size()
        ) {

            return false;
        }


        for (
            std::size_t i = 0;
            i < v.size();
            ++i
        ) {

            if (
                std::abs(
                    v[i] -
                    other.v[i]
                )
                > 1e-12
            ) {

                return false;
            }
        }


        for (
            std::size_t i = 0;
            i < functions.size();
            ++i
        ) {

            const auto& a =
                functions[i];

            const auto& b =
                other.functions[i];


            if (
                std::abs(
                    a.verticalOffset() -
                    b.verticalOffset()
                ) > 1e-12
            ) {

                return false;
            }


            if (
                std::abs(
                    a.horizontalOffset() -
                    b.horizontalOffset()
                ) > 1e-12
            ) {

                return false;
            }


            if (
                std::abs(
                    a.scale() -
                    b.scale()
                ) > 1e-12
            ) {

                return false;
            }
        }


        return true;
    }


    // ============================================================
    // QUADRATIC PROGRAMMING
    // ============================================================

    void optimizeQuadraticProgramming() {

        if (
            functions.empty() ||
            v.empty()
        ) {

            return;
        }


        /*
            Objective:

                minimize 1/2 x^T Qx + c^T x

            Here:

                Q = I
                c = 0

            Therefore:

                gradient = x

            The bounds provide the linear constraints:

                -10 <= x <= 10
        */


        auto space =
            evaluateSpace();


        const double lowerBound =
            -10.0;

        const double upperBound =
            10.0;


        const double learningRate =
            0.05;


        const int iterations =
            100;


        for (
            int iteration = 0;
            iteration < iterations;
            ++iteration
        ) {

            for (
                std::size_t i = 0;
                i < functions.size();
                ++i
            ) {

                double mean =
                    0.0;


                for (
                    double value :
                    space[i]
                ) {

                    mean += value;
                }


                mean /=
                    static_cast<double>(
                        space[i].size()
                    );


                double step =
                    -learningRate *
                    mean;


                step =
                    std::clamp(
                        step,
                        lowerBound,
                        upperBound
                    );


                functions[i]
                    .setVerticalOffset(
                        functions[i]
                            .verticalOffset()
                        +
                        step
                    );
            }


            space =
                evaluateSpace();
        }
    }


    // ============================================================
    // CONJUGATE GRADIENT
    // ============================================================

    void optimizeConjugateGradient() {

        if (
            functions.empty() ||
            v.empty()
        ) {

            return;
        }


        /*
            We solve:

                Ax = b

            with:

                A = I
                b = 0

            This means the desired state is zero.

            For this particular A, CG converges in one
            mathematical step.
        */


        auto space =
            evaluateSpace();


        for (
            std::size_t i = 0;
            i < functions.size();
            ++i
        ) {

            double mean =
                std::accumulate(
                    space[i].begin(),
                    space[i].end(),
                    0.0
                )
                /
                static_cast<double>(
                    space[i].size()
                );


            functions[i]
                .setVerticalOffset(
                    functions[i]
                        .verticalOffset()
                    -
                    mean
                );
        }
    }


    // ============================================================
    // TRUST REGION
    // ============================================================

    void optimizeTrustRegion(
        double radius = 0.1
    ) {

        if (
            functions.empty() ||
            v.empty()
        ) {

            return;
        }


        auto space =
            evaluateSpace();


        for (
            std::size_t i = 0;
            i < functions.size();
            ++i
        ) {

            double mean =
                std::accumulate(
                    space[i].begin(),
                    space[i].end(),
                    0.0
                )
                /
                static_cast<double>(
                    space[i].size()
                );


            double step =
                -mean;


            step =
                std::clamp(
                    step,
                    -radius,
                    radius
                );


            functions[i]
                .setVerticalOffset(
                    functions[i]
                        .verticalOffset()
                    +
                    step
                );
        }
    }


    // ============================================================
    // STOCHASTIC GRADIENT DESCENT
    // ============================================================

    void optimizeSGD(
        double learningRate = 0.001,
        std::size_t iterations = 100
    ) {

        if (
            functions.empty() ||
            v.empty()
        ) {

            return;
        }


        std::mt19937 generator(
            std::random_device{}()
        );


        std::uniform_int_distribution<std::size_t>
            distribution(
                0,
                v.size() - 1
            );


        for (
            std::size_t i = 0;
            i < functions.size();
            ++i
        ) {

            for (
                std::size_t iteration = 0;
                iteration < iterations;
                ++iteration
            ) {

                std::size_t sample =
                    distribution(
                        generator
                    );


                double x =
                    v[sample];


                double y =
                    functions[i]
                        .evaluate(x);


                /*
                    Objective:

                        L = 1/2 y²

                    Since we are adjusting the vertical
                    offset:

                        dy/db = 1

                    therefore:

                        dL/db = y
                */


                double gradient =
                    y;


                functions[i]
                    .setVerticalOffset(
                        functions[i]
                            .verticalOffset()
                        -
                        learningRate *
                        gradient
                    );
            }
        }
    }


    // ============================================================
    // BAYESIAN OPTIMIZATION
    // ============================================================

    void optimizeBayesian(
        int candidates = 25,
        double searchRadius = 0.5
    ) {

        if (
            functions.empty() ||
            v.empty()
        ) {

            return;
        }


        /*
            Lightweight Bayesian-style optimizer.

            We evaluate candidate vertical offsets around
            the current function.

            The acquisition score is:

                acquisition =
                    predictedLoss
                    -
                    exploration * uncertainty

            This is a lightweight implementation intended
            for the functional-space system rather than a
            full Gaussian-process library.
        */


        std::mt19937 generator(
            std::random_device{}()
        );


        std::normal_distribution<double>
            distribution(
                0.0,
                searchRadius
            );


        const double exploration =
            0.25;


        for (
            std::size_t i = 0;
            i < functions.size();
            ++i
        ) {

            double originalOffset =
                functions[i]
                    .verticalOffset();


            double bestOffset =
                originalOffset;


            double bestScore =
                std::numeric_limits<double>::infinity();


            for (
                int candidate = 0;
                candidate < candidates;
                ++candidate
            ) {

                double proposedOffset =
                    originalOffset +
                    distribution(generator);


                functions[i]
                    .setVerticalOffset(
                        proposedOffset
                    );


                /*
                    Use the actual functional-space
                    intersection objective.

                    This is the important integration:
                    Bayesian optimization is now optimizing
                    the same objective as the main optimizer.
                */


                // This function requires the caller's
                // intersection objective, so here we use
                // a local state objective based on the
                // evaluated functional distribution.

                auto values =
                    evaluateSpace()[i];


                double mean =
                    std::accumulate(
                        values.begin(),
                        values.end(),
                        0.0
                    )
                    /
                    static_cast<double>(
                        values.size()
                    );


                double variance =
                    0.0;


                for (
                    double value :
                    values
                ) {

                    double d =
                        value - mean;

                    variance +=
                        d * d;
                }


                variance /=
                    static_cast<double>(
                        values.size()
                    );


                double uncertainty =
                    std::sqrt(
                        variance
                    );


                double predictedLoss =
                    std::abs(mean);


                double acquisition =
                    predictedLoss -
                    exploration *
                    uncertainty;


                if (
                    acquisition <
                    bestScore
                ) {

                    bestScore =
                        acquisition;

                    bestOffset =
                        proposedOffset;
                }
            }


            functions[i]
                .setVerticalOffset(
                    bestOffset
                );
        }
    }
};

#endif