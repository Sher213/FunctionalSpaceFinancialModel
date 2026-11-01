#ifndef FINANCIAL_FUNCTIONAL_SPACE_H
#define FINANCIAL_FUNCTIONAL_SPACE_H

#include "FunctionalSpace.h"
#include "FunctionalSpaceOptimizer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>


// ============================================================
// FINANCIAL OBSERVATION
// ============================================================

struct FinancialObservation {

    long long timestamp = 0;

    double open = 0.0;

    double high = 0.0;

    double low = 0.0;

    double close = 0.0;

    double volume = 0.0;
};


// ============================================================
// FINANCIAL FUNCTIONAL SPACE
// ============================================================
//
// Financial Data
//      |
//      v
// FinancialFunctionalSpace
//      |
//      v
// FunctionalSpace
//      |
//      v
// FunctionSpaceOptimizer
//
// This class is an adapter between the financial-data layer
// and the mathematical FunctionalSpace.
//
// It does not contain the optimization mathematics itself.
//
// ============================================================

class FinancialFunctionalSpace {

public:

    // ========================================================
    // STATE
    // ========================================================

    std::vector<FinancialObservation> observations;

    FunctionalSpace space;

    // Optional because the Vector class has no default
    // constructor and the financial space may exist before
    // an intersection vector is configured.
    std::optional<Vector> intersectionVector;


    // ========================================================
    // CONSTRUCTORS
    // ========================================================

    FinancialFunctionalSpace() = default;


    explicit FinancialFunctionalSpace(
        std::vector<FinancialObservation> data
    )
        : observations(std::move(data))
    {
        prepare();
    }


    FinancialFunctionalSpace(
        std::vector<FinancialObservation> data,
        Vector vector
    )
        : observations(std::move(data)),
          intersectionVector(std::move(vector))
    {
        prepare();
    }


    // ========================================================
    // DATA MANAGEMENT
    // ========================================================

    void setObservations(
        std::vector<FinancialObservation> data
    ) {

        observations = std::move(data);

        prepare();
    }


    void addObservation(
        const FinancialObservation& observation
    ) {

        observations.push_back(observation);

        sortObservations();

        buildFunctionalSpace();
    }


    std::size_t observationCount() const {

        return observations.size();
    }


    const FinancialObservation& getObservation(
        std::size_t index
    ) const {

        if (index >= observations.size()) {

            throw std::out_of_range(
                "Financial observation index is out of range."
            );
        }

        return observations[index];
    }


    // ========================================================
    // VECTOR / INTERSECTION CONFIGURATION
    // ========================================================

    void setIntersectionVector(
        const Vector& vector
    ) {

        intersectionVector = vector;
    }


    void setIntersectionVector(
        Vector&& vector
    ) {

        intersectionVector = std::move(vector);
    }


    bool hasIntersectionVector() const {

        return intersectionVector.has_value();
    }


    const Vector& getIntersectionVector() const {

        if (!intersectionVector.has_value()) {

            throw std::runtime_error(
                "Intersection vector has not been configured."
            );
        }

        return intersectionVector.value();
    }


    // ========================================================
    // PREPARATION
    // ========================================================

    void prepare() {

        sortObservations();

        buildFunctionalSpace();
    }


private:

    // ========================================================
    // SORT
    // ========================================================

    void sortObservations() {

        std::sort(
            observations.begin(),
            observations.end(),
            [](
                const FinancialObservation& a,
                const FinancialObservation& b
            ) {

                return a.timestamp < b.timestamp;
            }
        );
    }


    // ========================================================
    // NORMALIZED TIME
    // ========================================================

    std::vector<double>
    normalizedTime() const {

        std::vector<double> result;

        if (observations.empty()) {

            return result;
        }

        result.reserve(observations.size());

        long long minimum =
            observations.front().timestamp;

        long long maximum =
            observations.back().timestamp;

        long long range =
            maximum - minimum;

        if (range == 0) {

            result.assign(
                observations.size(),
                0.0
            );

            return result;
        }

        for (const auto& observation : observations) {

            double x =
                static_cast<double>(
                    observation.timestamp - minimum
                )
                /
                static_cast<double>(
                    range
                );

            result.push_back(x);
        }

        return result;
    }


    // ========================================================
    // LINEAR INTERPOLATION
    // ========================================================

    static double interpolate(
        double x,
        const std::vector<double>& xs,
        const std::vector<double>& ys
    ) {

        if (xs.empty() || ys.empty()) {

            return 0.0;
        }

        if (xs.size() != ys.size()) {

            throw std::runtime_error(
                "Interpolation vectors have different sizes."
            );
        }

        if (xs.size() == 1) {

            return ys.front();
        }

        if (x <= xs.front()) {

            return ys.front();
        }

        if (x >= xs.back()) {

            return ys.back();
        }

        auto upper =
            std::upper_bound(
                xs.begin(),
                xs.end(),
                x
            );

        std::size_t right =
            static_cast<std::size_t>(
                std::distance(
                    xs.begin(),
                    upper
                )
            );

        std::size_t left =
            right - 1;

        double x1 = xs[left];

        double x2 = xs[right];

        double y1 = ys[left];

        double y2 = ys[right];

        double denominator =
            x2 - x1;

        if (
            std::abs(denominator)
            <
            std::numeric_limits<double>::epsilon()
        ) {

            return y1;
        }

        double t =
            (x - x1)
            /
            denominator;

        return
            y1 +
            t * (y2 - y1);
    }


    // ========================================================
    // BUILD FUNCTIONAL SPACE
    // ========================================================

    void buildFunctionalSpace() {

        space.clear();

        if (observations.empty()) {

            return;
        }

        std::vector<double> xs =
            normalizedTime();

        std::vector<double> opens;
        std::vector<double> highs;
        std::vector<double> lows;
        std::vector<double> closes;
        std::vector<double> volumes;

        opens.reserve(observations.size());
        highs.reserve(observations.size());
        lows.reserve(observations.size());
        closes.reserve(observations.size());
        volumes.reserve(observations.size());

        for (const auto& observation : observations) {

            opens.push_back(observation.open);

            highs.push_back(observation.high);

            lows.push_back(observation.low);

            closes.push_back(observation.close);

            volumes.push_back(observation.volume);
        }


        // ====================================================
        // INDEPENDENT VARIABLE
        // ====================================================

        space.v = xs;


        // ====================================================
        // OPEN
        // ====================================================

        space.addFunction(
            Function(
                [xs, opens](double x) {

                    return interpolate(
                        x,
                        xs,
                        opens
                    );
                },
                "Open(t)"
            )
        );


        // ====================================================
        // HIGH
        // ====================================================

        space.addFunction(
            Function(
                [xs, highs](double x) {

                    return interpolate(
                        x,
                        xs,
                        highs
                    );
                },
                "High(t)"
            )
        );


        // ====================================================
        // LOW
        // ====================================================

        space.addFunction(
            Function(
                [xs, lows](double x) {

                    return interpolate(
                        x,
                        xs,
                        lows
                    );
                },
                "Low(t)"
            )
        );


        // ====================================================
        // CLOSE
        // ====================================================

        space.addFunction(
            Function(
                [xs, closes](double x) {

                    return interpolate(
                        x,
                        xs,
                        closes
                    );
                },
                "Close(t)"
            )
        );


        // ====================================================
        // VOLUME
        // ====================================================

        space.addFunction(
            Function(
                [xs, volumes](double x) {

                    return interpolate(
                        x,
                        xs,
                        volumes
                    );
                },
                "Volume(t)"
            )
        );
    }


public:

    // ========================================================
    // FUNCTIONAL SPACE ACCESS
    // ========================================================

    FunctionalSpace& getSpace() {

        return space;
    }


    const FunctionalSpace& getSpace() const {

        return space;
    }


    // ========================================================
    // REBUILD
    // ========================================================

    void rebuild() {

        prepare();
    }


    // ========================================================
    // FUNCTION ACCESS
    // ========================================================

    Function& openFunction() {

        return space.getFunction(0);
    }


    Function& highFunction() {

        return space.getFunction(1);
    }


    Function& lowFunction() {

        return space.getFunction(2);
    }


    Function& closeFunction() {

        return space.getFunction(3);
    }


    Function& volumeFunction() {

        return space.getFunction(4);
    }


    // ========================================================
    // INTERSECTION LOSS
    // ========================================================

    double intersectionLoss(
        double xMin,
        double xMax,
        int samples = 1000,
        int gridWidth = 10,
        int gridHeight = 10
    ) const {

        if (!intersectionVector.has_value()) {

            throw std::runtime_error(
                "Cannot calculate intersection loss: "
                "intersection vector has not been configured."
            );
        }

        return
            space.intersectionLoss(
                intersectionVector.value(),
                xMin,
                xMax,
                samples,
                gridWidth,
                gridHeight
            );
    }


    // ========================================================
    // OPTIMIZATION
    // ========================================================

    OptimizationResult optimize(
        double xMin,
        double xMax,
        int samples = 1000,
        int gridWidth = 10,
        int gridHeight = 10,
        double perturbation = 0.05,
        int candidatesPerIteration = 25,
        int maxIterations = 200,
        double convergenceThreshold = 1e-6
    ) {

        if (!intersectionVector.has_value()) {

            throw std::runtime_error(
                "Cannot optimize financial functional space: "
                "intersection vector has not been configured."
            );
        }

        FunctionalOptimizer optimizer(
            intersectionVector.value(),
            xMin,
            xMax,
            samples,
            gridWidth,
            gridHeight,
            perturbation,
            candidatesPerIteration,
            maxIterations,
            convergenceThreshold
        );

        OptimizationResult result =
            optimizer.optimize(space);

        space = result.space;

        return result;
    }


    // ========================================================
    // CLOSE SERIES
    // ========================================================

    std::vector<double>
    closeSeries() const {

        std::vector<double> result;

        result.reserve(
            observations.size()
        );

        for (const auto& observation : observations) {

            result.push_back(
                observation.close
            );
        }

        return result;
    }


    // ========================================================
    // RETURN SERIES
    // ========================================================

    std::vector<double>
    returns() const {

        std::vector<double> result;

        if (observations.size() < 2) {

            return result;
        }

        result.reserve(
            observations.size() - 1
        );

        for (
            std::size_t i = 1;
            i < observations.size();
            ++i
        ) {

            double previous =
                observations[i - 1].close;

            double current =
                observations[i].close;

            if (
                std::abs(previous)
                <
                std::numeric_limits<double>::epsilon()
            ) {

                result.push_back(0.0);

                continue;
            }

            result.push_back(
                (current - previous)
                /
                previous
            );
        }

        return result;
    }
};


#endif