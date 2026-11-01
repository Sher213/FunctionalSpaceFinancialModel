#ifndef FUNCTIONAL_H
#define FUNCTIONAL_H

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cstddef>

#include "Function.h"

class Functional {
public:

    // ============================================================
    // DATA
    // ============================================================

    // Column of observed data
    std::vector<double> data;

    // Mathematical function associated with this functional
    Function function;

    // Label identifying the functional
    std::string label;


    // ============================================================
    // CONSTRUCTORS
    // ============================================================

    Functional();

    Functional(
        std::string label,
        Function function,
        std::vector<double> data
    ) :
        label(label),
        function(function),
        data(data)
    {
    };


    // ============================================================
    // OBSERVATION / DATA
    // ============================================================

    // Add an observation to the column
    void addObservation(double value) {
        data.push_back(value);
    };

    // Remove an observation (NOT IMPLEMENTED ASSUMING FUTURE OPTIMIZATION IMP. SO REMOVE WOULD BREAK)
    void removeObservation(std::size_t index) {};

    // Change an observation (REQUIRES HIGH OVERHEAD, SO NOT IMPLEMENTED)
    void setObservation(
        std::size_t index,
        double value
    );

    // Retrieve an observation
    double getObservation(
        std::size_t index
    ) const {
        return data[index];
    };

    // Number of observations
    std::size_t size() const {
        return data.size();
    };

    // Clear observations
    void clear() {
        data.clear();
    };


    // ============================================================
    // FUNCTION
    // ============================================================

    // Evaluate the associated function at one value
    double evaluate(double x) const {
        return function.evaluate(x);
    };

    // Evaluate the function across all observations (HIGH OVERHEAD, SO NOT IMPLEMENTED)
    std::vector<double> evaluateAll() const {};


    // ============================================================
    // LABEL
    // ============================================================

    // Get label
    std::string getLabel() const {
        return label;
    };

    // Change label
    void setLabel(
        std::string newLabel
    ) {
        label = newLabel;
    };

    // ============================================================
    // FUNCTION TRANSFORMATIONS
    // ============================================================

    // Transform the associated function
    void transform();

    // Compose this functional's function with another function
    void compose(
        const Function& other
    );

    // Derivative of associated function
    Function derivative() const;

    // Integral of associated function
    Function integrate() const;


    // ============================================================
    // FUNCTIONAL PROPERTIES
    // ============================================================

    // Minimum observed value
    double minimum() const {
        return *std::min_element(data.begin(), data.end());
    };

    // Maximum observed value
    double maximum() const {
        return *std::max_element(data.begin(), data.end());
    };

    // Mean of observations
    double mean() const {
        double sum = 0.0;
        for (const auto& val : data) {
            sum += val;
        }
        return sum / data.size();
    };

    // Variance
    double variance() const;

    // Standard deviation
    double standardDeviation() const;


    // ============================================================
    // RELATIONSHIPS
    // ============================================================

    // Distance between this Functional and another
    double distance(
        const Functional& other
    ) const;

    // Similarity between this Functional and another
    double similarity(
        const Functional& other
    ) const;

    // Compare functional trajectories
    double trajectoryDistance(
        const Functional& other
    ) const;


    // ============================================================
    // TRAJECTORY
    // ============================================================

    // Generate trajectory produced by the associated function
    std::vector<double> trajectory() const;

    // Difference between consecutive observations
    std::vector<double> change() const;

    // Determine whether the functional is increasing
    bool isIncreasing() const;

    // Determine whether the functional is decreasing
    bool isDecreasing() const;


    // ============================================================
    // ENTROPY
    // ============================================================

    // Entropy of the observed data
    double entropy() const;

    // Entropy after applying the associated function
    double functionalEntropy() const;

    // Change in entropy
    double entropyChange(
        const Functional& previous
    ) const;


    // ============================================================
    // STATE
    // ============================================================

    // Return a copy of the current functional
    Functional snapshot() const;

    // Compare two functional states
    bool equals(
        const Functional& other
    ) const;


    // ============================================================
    // DEBUG / REPRESENTATION
    // ============================================================

    // Print functional information
    void print() const {
        std::cout << "Functional Label: " << label << std::endl;
        std::cout << "Function Name: " << function.getName() << std::endl;
        std::cout << "Observations: ";
        for (const auto& val : data) {
            std::cout << val << " ";
        }
        std::cout << std::endl;
    };
};

#endif // FUNCTIONAL_H
