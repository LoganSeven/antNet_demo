AntNet Backend

This directory hosts the complete C implementation for the AntNet routing optimization demo.
It manages the stateful contexts, maintains the network topology, handles solvers (ACO, random, brute-force), and supports real-time visualization. Below is an overview of each major component:

    Context Management

        Global Context Pool: A fixed number of possible contexts (MAX_CONTEXTS) allow multiple simulations to run in isolation.

        Initialization/Shutdown: Each context is created via pub_initialize(...), storing node/edge data, solver states, and config info.

        Thread Safety: All contexts embed a pthread_mutex_t to guard memory shared across threads (especially for solver updates).

    Topology & Parameters

        Topology Updates: Functions like pub_update_topology(...) replace the node/edge data, reset solver states, and prevent stale data issues.

        Parameter Handlers: Routines in backend_params.c and related headers let you query and modify ACO parameters, SASA coefficients, or general simulation settings in a consistent, thread-safe fashion.

    Solvers

        ACO (Ant Colony Optimization): A pheromone-based solver that can run in single- or multi-ant modes. Multi-threading support improves performance, with each ant computing local deltas and merging them into a global pheromone matrix.

        Random: A straightforward pathfinder selecting intermediate nodes randomly, useful as a baseline or performance reference.

        Brute Force: Exhaustively enumerates possible paths, gradually increasing the number of intermediate hops. It yields an exact solution, serving as a correctness or comparison measure.

        SASA Ranking: Each solver tracks improvements over iterations; a SASA-based score is computed to rank algorithms by their performance gains.

    Configuration & Managers

        Config Manager: Loads, saves, and initializes default settings from .ini files.

        Hop Map Manager: (Optional) Generates or updates a node map with random or configured latency ranges, providing a quick way to build test networks.

        Ranking Manager: Implements the internal SASA scoring update and ranking logic.

    Rendering

        Heatmap Renderer: Responsible for GPU-accelerated visualization of pheromone intensity or other node-based metrics.

        Asynchronous Mode: An offscreen, EGL-based thread processes render jobs independently. This helps avoid blocking the main simulation thread during potentially heavy graphics operations.

    Public API & CFFI

        Header Files: Stored under include/, these define the AntNetContext, solver function signatures, and parameter sets.

        CFFI Interface: Python integration is handled via CFFI. The aggregator header (cffi_entrypoint.h) gathers all declarations needed for parsing in Python.

    Build & Integration

        Compiled into a shared library (.so on Linux, .dll on Windows).

        Can be invoked directly from C, Python (via CFFI), or other environments that support native library calls.

        Offers a clean set of functions (pub_initialize, pub_run_iteration, pub_shutdown, etc.) for end-to-end usage, from initial config loading to final graph tear-down.

Collectively, these modules form a cohesive backend for experimenting with routing algorithms in real-time, blending concurrency, performance, and flexibility. By isolating each solver and maintaining clear data structures, the AntNet backend can be extended or integrated seamlessly into diverse simulation or research contexts.