from typing import TypedDict, Any

# This file is auto-generated. Do not edit.


# from include/antnet_network_types.h
class NodeData(TypedDict):
    node_id: int
    delay_ms: int
    x: float
    y: float
    radius: int

# from include/antnet_network_types.h
class EdgeData(TypedDict):
    from_id: int
    to_id: int

# from include/antnet_network_types.h
class NodeData(TypedDict):
    node_id: int
    delay_ms: int
    x: float
    y: float
    radius: int

# from include/antnet_network_types.h
class EdgeData(TypedDict):
    from_id: int
    to_id: int

# from include/antnet_config_types.h
class AppConfig(TypedDict):
    nb_swarms: int
    set_nb_nodes: int
    min_hops: int
    max_hops: int
    default_delay: int
    death_delay: int
    under_attack_id: int
    attack_started: bool
    simulate_ddos: bool
    show_random_performance: bool
    show_brute_performance: bool

# from include/antnet_brute_force_types.h
class BruteForceState(TypedDict):
    candidate_nodes: int
    candidate_count: int
    current_L: int
    permutation: int
    combination: int
    at_first_permutation: int
    at_first_combination: int
    done: int

# from include/backend.h
class AntNetContext(TypedDict):
    node_count: int
    min_hops: int
    max_hops: int
    num_nodes: int
    num_edges: int
    iteration: int
    lock: Any
    random_best_nodes: int
    random_best_length: int
    random_best_latency: int
    config: Any
    brute_best_nodes: int
    brute_best_length: int
    brute_best_latency: int
    brute_state: Any
    aco_best_nodes: int
    aco_best_length: int
    aco_best_latency: int
    aco_v1: Any
