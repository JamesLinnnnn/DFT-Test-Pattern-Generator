# ATPG and Fault Simulation Project

This team work project implements and evaluates several ATPG (Automatic Test Pattern Generation) and Fault Simulation techniques for digital circuits. The goal is to generate high-quality test patterns to detect stuck-at faults with optimized runtime, fault coverage, and test volume.

## Supported Features

- **Logic Simulation (Logicsim)**
- **Random Test Pattern Generation (RTPG)**
- **Deterministic ATPG**
  - DALG
  - PODEM
- **Fault Simulation**
  - Parallel Fault Simulation (PFS)
  - Deductive Fault Simulation (DFS)
- **SCOAP**
  - Controllability
  - Observability
- **Heuristic-Guided ATPG Improvements**

## Key Modules

### ATPG Algorithms

- **DALG**: Implements D-algorithm with recursive implication and frontier handling.
- **PODEM**: Recursive backtracking algorithm for fault activation and propagation.

### TPG Heuristics

- **Fault Order** (`-fo`)
- **D-Frontier Selection** (`-df`): by node ID or level
- **J-Frontier Selection** (`-jf`): based on SCOAP
- **RTPG Variants**: `v0`, `v1`, `v2` with different stopping conditions

### Simulation & Evaluation

- **Logic Simulator**
- **DFS-Based Fault Simulator**
- **DTPFC**: Evaluates external test patterns and reports fault coverage

