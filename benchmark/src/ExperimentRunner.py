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
    """Generate a random DIRECTED multigraph.
    
    For each ordered pair (i, j) where i != j, with probability 'density',
    add between 1 and max_multiplicity directed edges from i to j.
    Self-loops (i -> i) are also possible.
    """
    if seed is not None:
        random.seed(seed)
    
    G = nx.MultiDiGraph()  # Directed multigraph
    G.add_nodes_from(range(n))
    
    for i in range(n):
        for j in range(n):  # All pairs including i==j for self-loops
            if random.random() < density:
                w = random.randint(1, max_multiplicity)
                for _ in range(w):
                    G.add_edge(i, j)
    return G

def save_graph_to_file(G, H, filename):
    """Save directed multigraphs G and H to file.
    
    Format: adjacency matrix where M[i][j] = number of directed edges from i to j.
    """
    with open(filename, 'w') as f:
        # Write G
        n = G.number_of_nodes()
        f.write(f"{n}\n")
        adj = [[0] * n for _ in range(n)]
        for u, v in G.edges():
            adj[u][v] += 1  # Directed: only u -> v
        
        for row in adj:
            f.write(" ".join(map(str, row)) + "\n")
            
        # Write H
        n = H.number_of_nodes()
        f.write(f"{n}\n")
        adj = [[0] * n for _ in range(n)]
        for u, v in H.edges():
            adj[u][v] += 1  # Directed: only u -> v
        
        for row in adj:
            f.write(" ".join(map(str, row)) + "\n")

def visualize_graph(G, title, filename):
    """Visualize a directed multigraph with arrows showing direction (paper-ready)."""
    plt.figure(figsize=(8, 8))
    pos = nx.circular_layout(G)
    ax = plt.gca()
    
    # Draw nodes - larger for clarity
    nx.draw_networkx_nodes(G, pos, node_color='#87CEEB', node_size=900, 
                          edgecolors='black', linewidths=2, ax=ax)
    nx.draw_networkx_labels(G, pos, font_size=14, font_weight='bold', ax=ax)
    
    # Draw directed edges with curvature for multigraphs
    for u, v, key in G.edges(keys=True):
        rad = 0.12 + 0.1 * key
        
        nx.draw_networkx_edges(G, pos, edgelist=[(u, v)], 
                               connectionstyle=f'arc3, rad={rad}',
                               arrows=True, arrowstyle='-|>', arrowsize=20,
                               width=2, edge_color='#333333',
                               min_source_margin=18, min_target_margin=18, ax=ax)
        
    plt.title(title, fontsize=18, fontweight='bold', pad=15)
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(filename, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()

def parse_extension_matrix(output_str, n):
    """Parse the extension matrix from algorithm output.
    
    The matrix appears after 'Best cost:' line. Format varies slightly:
    - Exact: matrix lines directly after 'Best cost: X'
    - Approx: matrix lines after 'Extension Matrix:' header
    
    The matrix is n x n where n is the size of H (host graph).
    """
    matrix = []
    lines = output_str.split('\n')
    
    # Find where the matrix starts
    start_idx = -1
    for i, line in enumerate(lines):
        if "Best cost:" in line:
            # Matrix starts on the next line (or after "Extension Matrix:" for approx)
            start_idx = i + 1
            break
    
    if start_idx == -1:
        return matrix
    
    # Skip "Extension Matrix:" header if present
    if start_idx < len(lines) and "Extension Matrix" in lines[start_idx]:
        start_idx += 1
    
    # Read matrix rows
    for i in range(start_idx, min(start_idx + n, len(lines))):
        line = lines[i].strip()
        if line:
            try:
                row = list(map(int, line.split()))
                if len(row) == n:
                    matrix.append(row)
                elif len(row) > 0:
                    # Might be end of matrix
                    break
            except ValueError:
                break
    
    return matrix

def visualize_extension(H, extension_matrix, title, filename):
    """Visualize the host graph H with extension edges highlighted as a DIRECTED multigraph.
    
    Paper-ready visualization with:
    - Original edges: gray arrows, semi-transparent
    - Added edges: red dashed arrows, with direction clearly shown
    - The extension matrix M[i][j] represents directed edges from i to j
    """
    n = H.number_of_nodes()
    
    plt.figure(figsize=(10, 10))
    pos = nx.circular_layout(H)
    ax = plt.gca()
    
    # Draw nodes - larger for paper clarity
    nx.draw_networkx_nodes(H, pos, node_color='#90EE90', node_size=1000, 
                          edgecolors='#2E8B57', linewidths=2.5, ax=ax)
    nx.draw_networkx_labels(H, pos, font_size=16, font_weight='bold', ax=ax)
    
    # Draw original edges (gray) as directed arrows with curvature for multigraphs
    for u, v, key in H.edges(keys=True):
        rad = 0.12 + 0.1 * key
        nx.draw_networkx_edges(H, pos, edgelist=[(u, v)], 
                               connectionstyle=f'arc3, rad={rad}',
                               edge_color='#666666', alpha=0.5, width=2, 
                               arrows=True, arrowstyle='-|>', arrowsize=18,
                               min_source_margin=20, min_target_margin=20, ax=ax)
    
    # Collect ALL directed edges from the extension matrix (not just upper triangle)
    # M[i][j] = number of directed edges to add FROM i TO j
    added_edges = {}  # (i, j) -> count (directed: i -> j)
    total_cost = 0
    
    if len(extension_matrix) == n:
        for i in range(n):
            for j in range(n):
                count = extension_matrix[i][j]
                if count > 0:
                    added_edges[(i, j)] = count
                    total_cost += count
    
    # Draw added directed edges (red, dashed arrows)
    edge_count_drawn = {}  # Track how many edges drawn between each pair for offset
    
    for (u, v), count in added_edges.items():
        # Get existing count for offset calculation
        pair_key = (min(u, v), max(u, v))
        existing = edge_count_drawn.get(pair_key, 0)
        
        for k in range(count):
            if u == v:
                # Self-loop
                loop_rad = 0.4 + 0.15 * k
                nx.draw_networkx_edges(H, pos, edgelist=[(u, v)],
                                       connectionstyle=f'arc3, rad={loop_rad}',
                                       edge_color='#DC143C', style='dashed', 
                                       width=3, alpha=0.9,
                                       arrows=True, arrowstyle='-|>', arrowsize=22,
                                       min_source_margin=20, min_target_margin=20, ax=ax)
            else:
                # Regular directed edge - use curvature to separate multiple edges
                base_rad = 0.18
                edge_idx = existing + k
                rad = base_rad * (1 + edge_idx * 0.5)
                # Alternate curvature direction for edges in opposite directions
                if u > v:
                    rad = -rad
                
                nx.draw_networkx_edges(H, pos, edgelist=[(u, v)],
                                       connectionstyle=f'arc3, rad={rad}',
                                       edge_color='#DC143C', style='dashed', 
                                       width=3, alpha=0.9,
                                       arrows=True, arrowstyle='-|>', arrowsize=22,
                                       min_source_margin=20, min_target_margin=20, ax=ax)
        
        edge_count_drawn[pair_key] = existing + count
    
    # Add edge labels showing multiplicity for added edges (only if > 1)
    for (u, v), count in added_edges.items():
        if count > 1 and u != v:
            # Calculate label position (midpoint with offset)
            x = (pos[u][0] + pos[v][0]) / 2
            y = (pos[u][1] + pos[v][1]) / 2
            # Offset perpendicular to edge
            dx = pos[v][0] - pos[u][0]
            dy = pos[v][1] - pos[u][1]
            length = (dx**2 + dy**2)**0.5
            if length > 0:
                offset = 0.1 if u < v else -0.1
                x += -dy/length * offset
                y += dx/length * offset
            ax.text(x, y, f"×{count}", fontsize=12, fontweight='bold', color='#8B0000',
                   ha='center', va='center',
                   bbox=dict(boxstyle='round,pad=0.3', facecolor='white', 
                            edgecolor='#DC143C', alpha=0.95, linewidth=1.5))
    
    # Paper-ready legend
    from matplotlib.lines import Line2D
    legend_elements = [
        Line2D([0], [0], color='#666666', alpha=0.6, linewidth=3, 
               marker='>', markersize=12, label='Original edges'),
        Line2D([0], [0], color='#DC143C', linewidth=3, linestyle='--',
               marker='>', markersize=12, label='Added edges')
    ]
    ax.legend(handles=legend_elements, loc='upper left', fontsize=14, 
             framealpha=0.95, edgecolor='black', fancybox=True)
    
    # Clean title with cost info
    plt.title(title, fontsize=20, fontweight='bold', pad=15)
    plt.axis('off')
    plt.tight_layout()
    plt.savefig(filename, dpi=300, bbox_inches='tight', facecolor='white')
    plt.close()

def run_single_experiment(algo, G, H, temp_file, n_mappings=1):
    save_graph_to_file(G, H, temp_file)
    
    start_time = time.time()
    try:
        result = subprocess.run(
            [IMPLEMENTATION_BIN, algo, temp_file, str(n_mappings)],
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
        # Use smaller, simpler graphs for clearer visualization of directed edges
        size_vis = 4  # Small pattern graph
        G_vis = generate_random_multigraph(size_vis, 0.4, 2, seed=42)  # Sparser, fewer multi-edges
        H_vis = generate_random_multigraph(size_vis + 2, 0.3, 2, seed=123)  # 6-node host graph
        
        print(f"  G: {G_vis.number_of_nodes()} nodes, {G_vis.number_of_edges()} directed edges")
        print(f"  H: {H_vis.number_of_nodes()} nodes, {H_vis.number_of_edges()} directed edges")
        
        visualize_graph(G_vis, f"Pattern Graph G ({G_vis.number_of_nodes()} nodes, {G_vis.number_of_edges()} edges)", 
                       os.path.join(viz_dir, "comparison_G.png"))
        visualize_graph(H_vis, f"Host Graph H ({H_vis.number_of_nodes()} nodes, {H_vis.number_of_edges()} edges)", 
                       os.path.join(viz_dir, "comparison_H.png"))
        
        # Run Exact
        res_exact = run_single_experiment("exact", G_vis, H_vis, temp_file)
        if res_exact["success"]:
            print(f"  Exact cost: {res_exact['cost']}")
            ext_matrix = parse_extension_matrix(res_exact["output"], H_vis.number_of_nodes())
            print(f"  Exact extension matrix ({len(ext_matrix)}x{len(ext_matrix[0]) if ext_matrix else 0}):")
            for row in ext_matrix:
                print(f"    {row}")
            visualize_extension(H_vis, ext_matrix, f"Exact Extension (Cost {res_exact['cost']})", os.path.join(viz_dir, "comparison_Exact.png"))
        else:
            print(f"  Exact failed: {res_exact.get('error', 'unknown')}")

        # Run Approx
        res_approx = run_single_experiment("approx", G_vis, H_vis, temp_file)
        if res_approx["success"]:
            print(f"  Approx cost: {res_approx['cost']}")
            ext_matrix = parse_extension_matrix(res_approx["output"], H_vis.number_of_nodes())
            print(f"  Approx extension matrix ({len(ext_matrix)}x{len(ext_matrix[0]) if ext_matrix else 0}):")
            for row in ext_matrix:
                print(f"    {row}")
            visualize_extension(H_vis, ext_matrix, f"Approx Extension (Cost {res_approx['cost']})", os.path.join(viz_dir, "comparison_Approx.png"))
        else:
            print(f"  Approx failed: {res_approx.get('error', 'unknown')}")
        
        # Log the costs to compare
        results.append({
            "experiment": "visual_comparison",
            "size_G": size_vis,
            "exact_cost": res_exact["cost"],
            "approx_cost": res_approx["cost"]
        })

    # --- Experiment 5: Cost vs N (Simultaneous Embeddings) ---
    # Tests how extension cost grows as we demand more simultaneous embeddings (N parameter)
    if experiments is None or "cost_vs_n" in experiments:
        print("Running Cost vs N (Simultaneous Embeddings) Tests...")
        
        # Fixed graph pair, varying N (number of required simultaneous mappings)
        # As N increases, the algorithm must find N vertex-disjoint embeddings,
        # requiring more edges to be added to H
        
        densities_to_test = [0.3, 0.5, 0.7]
        
        for density in densities_to_test:
            # G size and H size chosen so H can potentially accommodate multiple copies of G
            size_g = 5
            size_h = 25  # H has 5x the vertices of G, so up to 5 disjoint embeddings possible
            max_n = size_h // size_g  # Maximum possible disjoint embeddings
            
            print(f"  Testing density {density}, G={size_g}, H={size_h}, N=1..{max_n}...")
            
            for i in range(10):  # 10 random graph pairs per density
                G = generate_random_multigraph(size_g, density, 2, seed=3000 + i)
                H = generate_random_multigraph(size_h, density, 2, seed=4000 + i)
                
                # Test increasing values of N
                for n_mappings in range(1, max_n + 1):
                    res = run_single_experiment("approx", G, H, temp_file, n_mappings=n_mappings)
                    
                    if res["success"]:
                        results.append({
                            "experiment": "cost_vs_n",
                            "algorithm": "approx",
                            "size_G": size_g,
                            "size_H": size_h,
                            "n_mappings": n_mappings,
                            "density": density,
                            "time": res["time_ms"],
                            "cost": res["cost"]
                        })
                        print(f"    N={n_mappings}, density={density}: Cost={res['cost']}, Time={res['time_ms']:.2f}ms")
                    else:
                        print(f"    N={n_mappings}, density={density}: Failed ({res.get('error', 'unknown')})")
        
        # 5b. Exact algorithm version for small N values (ground truth)
        print("  Testing Exact algorithm for small N values...")
        size_g_exact = 4
        size_h_exact = 12  # Can fit up to 3 disjoint copies
        max_n_exact = 3
        density_exact = 0.5
        
        for i in range(10):
            G = generate_random_multigraph(size_g_exact, density_exact, 2, seed=5000 + i)
            H = generate_random_multigraph(size_h_exact, density_exact, 2, seed=6000 + i)
            
            for n_mappings in range(1, max_n_exact + 1):
                res = run_single_experiment("exact", G, H, temp_file, n_mappings=n_mappings)
                
                if res["success"]:
                    results.append({
                        "experiment": "cost_vs_n",
                        "algorithm": "exact",
                        "size_G": size_g_exact,
                        "size_H": size_h_exact,
                        "n_mappings": n_mappings,
                        "density": density_exact,
                        "time": res["time_ms"],
                        "cost": res["cost"]
                    })
                    print(f"    Exact N={n_mappings}: Cost={res['cost']}, Time={res['time_ms']:.2f}ms")
                else:
                    print(f"    Exact N={n_mappings}: Failed ({res.get('error', 'unknown')})")

    # Save Results
    df = pd.DataFrame(results)
    df.to_csv(os.path.join(base_dir, "results.csv"), index=False)
    print("Experiments completed.")
    
    if os.path.exists(temp_file):
        os.remove(temp_file)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Run benchmark experiments.")
    parser.add_argument("experiments", nargs="*", help="List of experiments to run (accuracy, scalability, density, visual, cost_vs_n). If empty, runs all.")
    args = parser.parse_args()
    
    run_suite(experiments=args.experiments if args.experiments else None)
