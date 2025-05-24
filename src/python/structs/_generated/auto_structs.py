from typing import TypedDict, List, Any

# This file is auto-generated. Do not edit.


# from include/types/antnet_network_types.h
class NodeData(TypedDict):
    node_id: int
    delay_ms: int
    x: float
    y: float
    radius: int

# from include/types/antnet_network_types.h
class EdgeData(TypedDict):
    from_id: int
    to_id: int

# from include/types/antnet_config_types.h
class AppConfig(TypedDict):
    nb_ants: int
    set_nb_nodes: int
    min_hops: int
    max_hops: int
    default_min_delay: int
    default_max_delay: int
    death_delay: int
    under_attack_id: int
    attack_started: bool
    simulate_ddos: bool
    show_random_performance: bool
    show_brute_performance: bool
    ranking_alpha: float
    ranking_beta: float
    ranking_gamma: float
    ant_alpha: float
    ant_beta: float
    ant_Q: float
    ant_evaporation: float

# from include/types/antnet_path_types.h
class AntNetPathInfo(TypedDict):
    nodes: List[int]
    node_count: int
    total_latency: int

# from include/types/antnet_brute_force_types.h
class BruteForceState(TypedDict):
    candidate_nodes: List[int]
    candidate_count: int
    current_L: int
    permutation: List[int]
    combination: List[int]
    at_first_permutation: int
    at_first_combination: int
    done: int

# from include/types/antnet_aco_v1_types.h
class AcoV1State(TypedDict):
    adjacency: List[int]
    adjacency_size: int
    pheromones: List[float]
    pheromone_size: int
    alpha: float
    beta: float
    evaporation: float
    Q: float
    num_ants: int
    is_initialized: int

# from include/types/antnet_sasa_types.h
class SasaCoeffs(TypedDict):
    alpha: float
    beta: float
    gamma: float

# from include/types/antnet_sasa_types.h
class SasaState(TypedDict):
    best_L: float
    last_improve_iter: int
    m: int
    sum_tau: float
    sum_r: float
    score: float

# from include/types/antnet_ranking_types.h
class RankingEntry(TypedDict):
    name: Any
    score: float
    latency_ms: int
