# Functional Space Predictor (FSP)

A high-performance C++ financial forecasting and optimization pipeline. The **Functional Space Predictor** treats time-series market data (such as daily open, high, low, close, and volume values) as evolving mathematical functions within a multi-dimensional "Functional Space," optimizing and evaluating predictive accuracy across various temporal horizons.

---

## 🚀 Key Features

* **Functional Representation:** Converts flat CSV rows into continuous mathematical functions for feature-wise modeling (`Open`, `High`, `Low`, `Close`, `Volume`).
* **Automated Optimization:** Calibrates baseline functional spaces using a robust optimization engine (`FunctionalOptimizer`).
* **Hybrid Forecasting Simulation:** Performs split-sample testing by blending actual historical source data with solved/forecasted data windows.
* **Multi-Scope Evaluation:** Automatically benchmarks predictions across multiple horizon scopes (`scope_10`, `scope_20`, `scope_30`, `scope_50`).
* **Comprehensive Metrics:** Computes error statistics including **MAE** (Mean Absolute Error), **MSE** (Mean Squared Error), **RMSE** (Root Mean Squared Error), and **Max Error**.
* **Audit & Export:** Generates clean, separated data checkpoints and comparison reports for every execution run.

---

## 🏗️ Pipeline Architecture

```text
[Load & Validate CSV] ──► [Extract Functional Space] ──► [Optimize Source Space]
                                                                    │
                                                                    ▼
                                                         [Run Prediction Tests]
                                                                    │
                                         ┌──────────────────────────┴──────────────────────────┐
                                         ▼                                                     ▼
                             [Build Hybrid Vector]                               [Optimize Hybrid Space]
                             (Source + Forecasted)                                         │
                                         │                                                     ▼
                                         └────────────────────────► [Compare & Generate Report]
```

---

## 📁 Project Directory Structure

```text
project_root/
├── data/
│   ├── raw/
│   │   └── daily_IBM.csv          # Input raw time-series data
│   └── processed/
│       ├── source.csv             # Processed source functional space export
│       ├── solved.csv             # Fully optimized functional space export
│       ├── scope_10_hybrid.csv    # 10-period prediction test hybrid output
│       ├── scope_10_comparison.txt# Error metrics report for 10-period test
│       └── ...                    # Additional scopes (20, 30, 50)
├── include/                       # Header files (FunctionalSpace, Optimizer, etc.)
├── src/                           # Source implementation files (main.cpp, loaders, etc.)
└── README.md
```

---

## 🛠️ Prerequisites & Compilation

* **C++ Compiler:** Supporting **C++17** or higher (e.g., `g++`, `clang++`, or MSVC) with `std::filesystem` support.
* **Build System:** CMake (recommended) or direct command-line compilation.

### Compiling via g++ (Example)
```bash
g++ -std=c++17 src/main.cpp -o functional_predictor -O3
```

---

## ⚙️ Usage

1. Place your input CSV file (formatted with `timestamp,open,high,low,close,volume`) into the `../data/raw/` directory.
2. Run the executable:
   ```bash
   ./functional_predictor
   ```
3. Check the `../data/processed/` directory for generated outputs, hybrid CSV predictions, and detailed evaluation text reports.

---

## 📊 Output Metrics Explained

Each completed test scope generates a comparison report detailing:
* **MAE (Mean Absolute Error):** Average magnitude of absolute errors.
* **MSE (Mean Squared Error):** Average squared difference between estimated values and actual value.
* **RMSE (Root Mean Squared Error):** Square root of the variance of the residuals, providing error metrics in the original scale of the data.
* **Max Error:** The single worst-case deviation observed in the test window.