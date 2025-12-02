import os
import sys
import subprocess
import time
import random
import networkx as nx
import pandas as pd
import matplotlib.pyplot as plt
import shutil
from datetime import datetime
import argparse

# Configuration
IMPLEMENTATION_BIN = "../implementation/bin/main.o"
EXPERIMENTS_DIR = "experiments"

def ensure_dir(path):
    if not os.path.exists(path):
        os.makedirs(path)

def generate_random_multigraph(n, density, max_multiplicity, seed=None):
    if seed is not None:
        random.seed(seed)
    
    G = nx.MultiGraph()
    G.add_nodes_from(range(n))
    
    for i in range(n):
        for j in range(i, n):
            if random.random() < density:
                w = random.randint(1, max_multiplicity)
                for _ in range(w):
                    G.add_edge(i, j)
    return G

def save_graph_to_file(G, H, filename):
    with open(filename, 'w') as f:
        # Write G
        n = G.number_of_nodes()
        f.write(f"{n}\n")
        adj = [[0] * n for _ in range(n)]
        for u, v in G.edges():
            adj[u][v] += 1
            if u != v:
                adj[v][u] += 1
        
        for row in adj:
            f.write(" ".join(map(str, row)) + "\n")
            
        # Write H
        n = H.number_of_nodes()
        f.write(f"{n}\n")
        adj = [[0] * n for _ in range(n)]
        for u, v in H.edges():
            adj[u][v] += 1
            if u != v:
                adj[v][u] += 1
        
        for row in adj:
            f.write(" ".join(map(str, row)) + "\n")

def visualize_graph(G, title, filename):
    plt.figure(figsize=(6, 6))
    pos = nx.circular_layout(G)
    
    # Draw nodes
    nx.draw_networkx_nodes(G, pos, node_color='lightblue', node_size=500)
    nx.draw_networkx_labels(G, pos)
    
    # Draw edges with curvature for multigraphs
    ax = plt.gca()
    for u, v, key, data in G.edges(keys=True, data=True):
        rad = 0.1 * (key + 1)
        if key % 2 == 1: rad = -rad
        
        nx.draw_networkx_edges(G, pos, edgelist=[(u, v)], connectionstyle=f'arc3, rad={rad}', ax=ax)
        
    plt.title(title)
    plt.axis('off')
    plt.savefig(filename)
    plt.close()

def parse_extension_matrix(output_str, n):
    matrix = []
    lines = output_str.split('\n')
    reading = False
    for line in lines:
        if "Extension Matrix:" in line:
            reading = True
            continue
        if reading and line.strip():
            try:
                row = list(map(int, line.strip().split()))
                if len(row) == n:
                    matrix.append(row)
            except:
                pass
    return matrix

def visualize_extension(H, extension_matrix, title, filename):
    # Create a copy of H to add extension edges
    H_ext = H.copy()
    n = H.number_of_nodes()
    
    # Add edges from extension matrix
    # Matrix indices correspond to H nodes
    added_edges = []
    if len(extension_matrix) == n:
        for i in range(n):
            for j in range(i, n): # Undirected/Symmetric assumption for visualization
                count = extension_matrix[i][j]
                if count > 0:
                    for _ in range(count):
                        H_ext.add_edge(i, j)
                        added_edges.append((i, j))
    
    plt.figure(figsize=(6, 6))
    pos = nx.circular_layout(H_ext)
    
    # Draw original edges
    nx.draw_networkx_nodes(H_ext, pos, node_color='lightgreen', node_size=500)
    nx.draw_networkx_labels(H_ext, pos)
    
    # Draw original edges (black)
    original_edges = [(u, v) for u, v, k in H.edges(keys=True)]
    ax = plt.gca()
    for u, v in original_edges:
        nx.draw_networkx_edges(H_ext, pos, edgelist=[(u, v)], edge_color='black', alpha=0.3, ax=ax)

    # Draw new edges (red, dashed)
    # Note: This simple visualization might overlap multiple added edges
    # For a cleaner look, we just draw one red line per pair if edges were added
    added_pairs = list(set(added_edges))
    nx.draw_networkx_edges(H_ext, pos, edgelist=added_pairs, edge_color='red', style='dashed', width=2, ax=ax)
        
    plt.title(title)
    plt.axis('off')
    plt.savefig(filename)
    plt.close()

def run_single_experiment(algo, G, H, temp_file):
    save_graph_to_file(G, H, temp_file)
    
    start_time = time.time()
    try:
        result = subprocess.run(
            [IMPLEMENTATION_BIN, algo, temp_file, "1"],
            capture_output=True,
            text=True,
            timeout=60 # 1 minute timeout
        )
        end_time = time.time()
        duration_ms = (end_time - start_time) * 1000
        
        output = result.stdout
        cost = -1
        found = False
        
        for line in output.split('\n'):
            if "Best cost:" in line:
                try:
                    cost = int(line.split(":")[1].strip())
                except:
                    pass
            if "EXACT: YES" in line or "APPROXIMATION: YES" in line:
                found = True
                cost = 0
        
        return {
            "success": True,
            "time_ms": duration_ms,
            "cost": cost,
            "found_subgraph": found,
            "output": output
        }
        
    except subprocess.TimeoutExpired:
        return {"success": False, "error": "Timeout"}
    except Exception as e:
        return {"success": False, "error": str(e)}

def run_suite(experiments=None):
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    base_dir = os.path.join(EXPERIMENTS_DIR, f"exp_{timestamp}")
    viz_dir = os.path.join(base_dir, "visualizations")
    ensure_dir(viz_dir)
    
    results = []
    temp_file = "temp_exp_graph.txt"
    
    print(f"Starting experiments... Output: {base_dir}")
    
    # --- Experiment 1: Accuracy (Exact vs Approx) on Small Graphs ---
    if experiments is None or "accuracy" in experiments:
        print("Running Accuracy Tests...")
        # Increased iterations to 30 for statistical significance
        # Reduced max size to 7 to keep runtime manageable with high iterations
        for size in range(3, 8): # 3 to 7
            print(f"  Testing Size {size} (30 iterations)...")
            for i in range(30): 
                # Simple progress indicator
                if i % 5 == 0: print(f"    Iter {i}/30", end="\r")
                
                G = generate_random_multigraph(size, 0.5, 2)
                H = generate_random_multigraph(size + 2, 0.5, 2)
                
                # Visualize first iteration of each size
                if i == 0:
                    visualize_graph(G, f"G (Size {size})", os.path.join(viz_dir, f"acc_s{size}_G.png"))
                    visualize_graph(H, f"H (Size {size+2})", os.path.join(viz_dir, f"acc_s{size}_H.png"))
                
                # Run Exact
                res_exact = run_single_experiment("exact", G, H, temp_file)
                
                # Run Approx
                res_approx = run_single_experiment("approx", G, H, temp_file)
                
                if res_exact["success"] and res_approx["success"]:
                    results.append({
                        "experiment": "accuracy",
                        "size_G": size,
                        "size_H": size + 2,
                        "density": 0.5,
                        "exact_time": res_exact["time_ms"],
                        "approx_time": res_approx["time_ms"],
                        "exact_cost": res_exact["cost"],
                        "approx_cost": res_approx["cost"],
                        "exact_found": res_exact["found_subgraph"],
                        "approx_found": res_approx["found_subgraph"]
                    })
            print(f"    Completed Size {size}        ")

    # --- Experiment 2: Scalability (Exact vs Approx) ---
    if experiments is None or "scalability" in experiments:
        print("Running Scalability Tests...")
        
        # 2a. Exact Scalability (Small Sizes)
        print("  Testing Exact Scalability (Sizes 3-9)...")
        for size in range(3, 10): # 3 to 9
            for i in range(5): # 5 iterations
                G = generate_random_multigraph(size, 0.4, 2)
                H = generate_random_multigraph(size + 2, 0.4, 2)
                
                res = run_single_experiment("exact", G, H, temp_file)
                if res["success"]:
                    results.append({
                        "experiment": "scalability",
                        "algorithm": "exact",
                        "size_G": size,
                        "size_H": size + 2,
                        "density": 0.4,
                        "time": res["time_ms"],
                        "cost": res["cost"]
                    })
                    print(f"    Size {size}: Time={res['time_ms']:.2f}ms")
                else:
                    print(f"    Size {size}: Failed ({res.get('error', 'unknown')})")

        # 2b. Approx Scalability (Large Sizes)
        print("  Testing Approx Scalability (Sizes 5-50)...")
        sizes = [5, 6, 7, 8, 10, 15, 20, 30, 40, 50]
        for size in sizes:
            for i in range(5): # 5 iterations
                G = generate_random_multigraph(size, 0.4, 2)
                H = generate_random_multigraph(size + 5, 0.4, 2)
                
                res = run_single_experiment("approx", G, H, temp_file)
                
                if res["success"]:
                    results.append({
                        "experiment": "scalability",
                        "algorithm": "approx",
                        "size_G": size,
                        "size_H": size + 5,
                        "density": 0.4,
                        "time": res["time_ms"],
                        "cost": res["cost"]
                    })
                    print(f"    Size {size}: Time={res['time_ms']:.2f}ms")
                else:
                    print(f"    Size {size}: Failed ({res.get('error', 'unknown')})")

    # --- Experiment 3: Density Impact ---
    if experiments is None or "density" in experiments:
        print("Running Density Tests...")
        densities = [0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9]
        
        # 3a. Density Impact on Approx (Medium & Large)
        for size in [20, 25]:
            print(f"  Testing Approx Density for Size {size}...")
            for d in densities:
                for i in range(5):
                    G = generate_random_multigraph(size, d, 2)
                    H = generate_random_multigraph(size + 3, d, 2)
                    
                    res = run_single_experiment("approx", G, H, temp_file)
                    
                    if res["success"]:
                        results.append({
                            "experiment": "density",
                            "algorithm": "approx",
                            "size_G": size,
                            "size_H": size + 3,
                            "density": d,
                            "time": res["time_ms"],
                            "cost": res["cost"]
                        })
                        print(f"    Density {d}: Time={res['time_ms']:.2f}ms")
                    else:
                        print(f"    Density {d}: Failed ({res.get('error', 'unknown')})")

        # 3b. Density Impact on Exact (Small)
        print(f"  Testing Exact Density for Size 6...")
        size_exact = 6
        for d in densities:
            for i in range(30):
                G = generate_random_multigraph(size_exact, d, 2)
                H = generate_random_multigraph(size_exact + 2, d, 2)
                
                res = run_single_experiment("exact", G, H, temp_file)
                
                if res["success"]:
                    results.append({
                        "experiment": "density",
                        "algorithm": "exact",
                        "size_G": size_exact,
                        "size_H": size_exact + 2,
                        "density": d,
                        "time": res["time_ms"],
                        "cost": res["cost"]
                    })
                    print(f"    Density {d}: Time={res['time_ms']:.2f}ms")
                else:
                    print(f"    Density {d}: Failed ({res.get('error', 'unknown')})")

    # --- Experiment 4: Visual Comparison (Exact vs Approx) ---
    if experiments is None or "visual" in experiments:
        print("Running Visual Comparison...")
        # Generate a single interesting case
        size_vis = 7
        G_vis = generate_random_multigraph(size_vis, 0.6, 3, seed=42)
        H_vis = generate_random_multigraph(size_vis + 2, 0.6, 3, seed=123)
        
        visualize_graph(G_vis, "Pattern Graph G", os.path.join(viz_dir, "comparison_G.png"))
        visualize_graph(H_vis, "Host Graph H", os.path.join(viz_dir, "comparison_H.png"))
        
        # Run Exact
        res_exact = run_single_experiment("exact", G_vis, H_vis, temp_file)
        if res_exact["success"]:
            ext_matrix = parse_extension_matrix(res_exact["output"], H_vis.number_of_nodes())
            visualize_extension(H_vis, ext_matrix, f"Exact Extension (Cost {res_exact['cost']})", os.path.join(viz_dir, "comparison_Exact.png"))

        # Run Approx
        res_approx = run_single_experiment("approx", G_vis, H_vis, temp_file)
        if res_approx["success"]:
            ext_matrix = parse_extension_matrix(res_approx["output"], H_vis.number_of_nodes())
            visualize_extension(H_vis, ext_matrix, f"Approx Extension (Cost {res_approx['cost']})", os.path.join(viz_dir, "comparison_Approx.png"))
        
        # We can't easily visualize the *result* graph without parsing the output matrix
        # But we can log the costs to compare
        results.append({
            "experiment": "visual_comparison",
            "size_G": size_vis,
            "exact_cost": res_exact["cost"],
            "approx_cost": res_approx["cost"]
        })

    # Save Results
    df = pd.DataFrame(results)
    df.to_csv(os.path.join(base_dir, "results.csv"), index=False)
    print("Experiments completed.")
    
    if os.path.exists(temp_file):
        os.remove(temp_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run benchmark experiments.")
    parser.add_argument("experiments", nargs="*", help="List of experiments to run (accuracy, scalability, density, visual). If empty, runs all.")
    args = parser.parse_args()
    
    run_suite(experiments=args.experiments if args.experiments else None)
