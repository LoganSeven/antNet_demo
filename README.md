# antNet_demo

## Overview

**AntNet_demo** is a dynamic network simulation inspired by the Traveling Salesman Problem (TSP) and optimized using a parallel, asynchronous Ant Colony Optimization algorithm (ACA). It visually models data packet movement (ants) from a client to a server over a graph where nodes and edges evolve in real-time.

---

## Architecture

### Frontend (Python with PyQt5 or QtPy)

- Built using Python and the Qt framework
- Thread-safe and responsive GUI using Qt signals/slots
- Core components:
  - `MainWindow`: main application window
  - `Canvas`: interactive network visualization
  - `CoreManager`: handles application control logic, C interop
  - `Worker` (in a `QThread`): executes backend computations

### Backend (C via CFFI)

- Pure C implementation (no C++)
- Interfaced via CFFI for low-latency calls
- Handles all ACA-related optimization logic
- Fully thread-safe, capable of executing parallel agents (ants)

---

## Application Behavior

- Each simulated packet is an **ant** carrying two integers.
- When it reaches its destination:
  - The node displays the **sum** of the integers
  - The **Round Trip Time (RTT)** is measured and shown
- Ants choose paths based on dynamically updated probabilities and past network conditions.

---

## User Interaction

- Dynamically destroy nodes
- Add latency to nodes (simulated delay, in ms)
- All network changes are instantly reflected in the simulation

---

## Threading and Safety

- C logic is executed in a dedicated `QThread`
- The `Worker` acts as a bridge between C and Python layers
- All communication is done using `Qt.QueuedConnection` to avoid race conditions and GUI freezes

---

## Configurable Parameters

/!\ this is work in progress it will change over time:
this part will be updated time to time.

- Number of graph nodes
- (x, y) positions for each node
- Explicit node-to-node connections
- Per-node latency in milliseconds
- Min/max hops each ant must traverse

---

## Algorithm: Asynchronous Ant Colony Optimization (ACA)

Ant agents asynchronously explore the graph, choosing paths based on a probabilistic policy shaped by past network delays and pheromone levels. Over time, the network converges toward optimal or near-optimal routing paths, adjusting live to topological or delay changes.

For deeper insights into the ACA algorithm, refer to:

 [Sources and Resources (PDF)](/documentation/sourcesAndResources.pdf)
 
For an overview of antNet_demo internal organization:
  
 [System Overview Diagram (PNG)](/documentation/antnet_demo_overview.png)

---

## Future Directions

- Multi-worker C threading model
- ACA island-model extensions
- Dynamic graph reconfiguration and scaling

---

## License

This project is released under a [modified MIT License](/LICENCE).

---

## Credits

Humbly presented by **Logan7**  
Powered by **Python**, **QtPy**, and **pure C**

_“Please be kind with artificial life — one day, it could well be a good time investment.”_

## Version history
0.0.1-alpha.1 MVP with mockup ant colony algorithm  
0.0.2-alpha.1 MVP minimal gui interface and signal manager. Communication with c backend tested.  
0.0.3-alpha.1 MVP topology updates between frontend and backend integrated. Tested with success.
0.0.4-alpha.1 MVP The 3 main algo (stochastic ,brute force, ant colony optimization) are implemented and tested. 
