# This script generates a topology for RBlocksim
import random
import argparse

"""
Inputs:
    n_nodes: Number of nodes
    max_peers: maximum numer of peers
    min_peers: minimum number of peers
"""


def generate_symmetric_topology(n_nodes, min_peers, max_peers, seed=None):
    """
    Generates a symmetric peer-to-peer network topology with random connections.

    Args:
        seed: The seed value for the random number generator.
        n_nodes: The number of nodes in the network.
        min_peers: The minimum number of connections per node (inclusive).
        max_peers: The maximum number of connections per node (inclusive).

    Returns:
        A list of lists, where each sublist represents the peers of a node.
    """
    if n_nodes <= 0:
        raise ValueError("Number of nodes must be positive")
    if min_peers < 1:
        raise ValueError("Minimum peers must be at least 1")
    if max_peers < min_peers:
        raise ValueError("Maximum peers must be greater than or equal to minimum peers")
    if max_peers >= n_nodes - 1:
        raise ValueError("Maximum peers cannot be greater than or equal to the number of nodes minus 1")

    # Seed the RNG if a seed is provided
    if seed is not None:
        random.seed(seed)

    # Initialize empty network
    topology = [[] for _ in range(n_nodes)]

    # Random connection attempts
    for i in range(n_nodes):
        connected_nodes = set(topology[i])  # Track connected nodes to avoid duplicates
        # Randomly choose the number of connections within the valid range
        if min_peers > max_peers * 0.9:
            raise ValueError("Minimum peers must be less than 90% of the maximum peers")
        num_connections = random.randint(min_peers, int(max_peers * 0.9))
        while len(connected_nodes) < num_connections:
            # Randomly pick a potential peer (excluding itself)
            potential_peer = random.randint(0, n_nodes - 1)
            if potential_peer not in connected_nodes and potential_peer != i and len(
                    topology[potential_peer]) < max_peers:
                connected_nodes.add(potential_peer)
                topology[i].append(potential_peer)
                topology[potential_peer].append(i)

    return topology


def get_pij(n_nodes, dens, prop, max_dis, i, j):
    """
    Calculates the connection probability between two nodes.

    Args:
        n_nodes: The number of nodes in the network.
        dens: The density parameter (p0 in the paper).
        prop: The propagation parameter (Beta in the paper).
        max_dis: The maximum distance between any two nodes (Dmax in the paper).
        i: The ID of the first node.
        j: The ID of the second node.

    Returns:
        The connection probability between the two nodes.
    """
    dis = i - j
    if dis < 0:
        dis = -dis
    # if dis > n_nodes / 2:
    if 2 * dis > n_nodes:
        dis = n_nodes - dis

    probability = prop * dens
    # if dens >= float(dis) / max_dis:
    if dens * max_dis >= dis:
        probability += 1 - prop

    return probability


def generate_small_world_topology(n_nodes, dens, prop, max_dis, seed=None):
    """
    Generates a small-world network topology with random connections.

    Args:
        n_nodes: The number of nodes in the network.
        dens: The density parameter (p0 in the paper).
        prop: The propagation parameter (Beta in the paper).
        max_dis: The maximum distance between any two nodes (Dmax in the paper).
        seed: The seed value for the random number generator.

    Returns:
        A list of lists, where each sublist represents the peers of a node.
    """
    if n_nodes <= 0:
        raise ValueError("Number of nodes must be positive")
    if dens < 0 or dens > 1:
        raise ValueError("Density parameter must be between 0 and 1")
    if prop < 0 or prop > 1:
        raise ValueError("Propagation parameter must be between 0 and 1")
    if max_dis <= 0:
        raise ValueError("Maximum distance must be positive")

    # Seed the RNG if a seed is provided
    if seed is not None:
        random.seed(seed)

    # Initialize empty network # TODO: Can be done more efficiently
    topology = [[] for _ in range(n_nodes)]

    # Random connection attempts
    for i in range(n_nodes):
        connected_nodes = set(topology[i])  # Track connected nodes to avoid duplicates
        for j in range(n_nodes):
            if i == j:
                continue
            if j in connected_nodes:
                continue
            if random.random() < get_pij(n_nodes, dens, prop, max_dis, i, j):
                connected_nodes.add(j)
                topology[i].append(j)
                topology[j].append(i)

    return topology


def main():
    # Parse command-line arguments
    # Parse command-line arguments
    parser = argparse.ArgumentParser(description="Generate random symmetric peer-to-peer network topology")
    parser.add_argument("--n_nodes", type=int, required=True, help="Number of nodes in the network")
    parser.add_argument("--min_peers", type=int, required=True,
                        help="Minimum number of connections per node (inclusive)")
    parser.add_argument("--max_peers", type=int, required=True,
                        help="Maximum number of connections per node (inclusive)")
    parser.add_argument("--seed", type=int, help="Seed value for the RNG (default: None)")
    args = parser.parse_args()

    # Generate and print the network topology
    topology = generate_symmetric_topology(args.n_nodes, args.min_peers, args.max_peers, args.seed)
    # sort the topology
    for i in range(args.n_nodes):
        topology[i].sort()
    topology_to_c(args, topology)


def topology_to_c(args, topology):
    # Output the topology to a file, for use in RBlockSim. The file has to be a .c file
    with open("Topology.c", "w") as f:
        f.write(f'#include "Topology.h"\n\n')
        f.write("const size_t peer_list_sizes[N_NODES] = {")
        for i, node_peers in enumerate(topology):
            f.write(f"{len(node_peers)}")
            if i < args.n_nodes - 1:
                f.write(", ")
        f.write("};\n")
        f.write("const node_id_t peer_lists[N_NODES][MAX_PEERS] = {\n")
        for i, node_peers in enumerate(topology):
            f.write("{")
            for j, peer in enumerate(node_peers):
                f.write(f"{peer}")
                if j < len(node_peers) - 1:
                    f.write(", ")
            if i < args.n_nodes - 1:
                f.write("},\n")
            else:
                f.write("}\n")
        f.write("};")

    with open("Topology.h", "w") as f:
        f.write(f'#pragma once\n')
        f.write(f'#include "RBlockSim.h"\n\n')
        f.write(f"#define N_NODES {args.n_nodes}\n")
        f.write(f"#define MIN_PEERS {args.min_peers}\n")
        f.write(f"#define MAX_PEERS {args.max_peers}\n\n")
        f.write("extern const size_t peer_list_sizes[N_NODES]; /// Holds the size of the peer list for each node\n")
        f.write("extern const node_id_t peer_lists[N_NODES][MAX_PEERS]; /// Holds the list of peers for each node\n")


if __name__ == "__main__":
    main()
