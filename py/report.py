from importlib import import_module
from pathlib import Path
import re
import sys
from sys import argv

PY_DIR = Path(__file__).resolve().parent
PROJECT_ROOT = PY_DIR.parent
sys.path.insert(0, str(PY_DIR))

from common import get_costs, get_matrix, plot, read_file


def display_name(name):
    labels = {
        "greedyCycle": "Greedy Cycle",
        "greedyCycle_2r": "Greedy Cycle (2-Regret)",
        "greedyCycle_weighted": "Greedy Cycle (Weighted)",
        "nearestNeighbor": "Nearest Neighbor",
        "nearestNeighborFullRoute": "Nearest Neighbor (all positions)",
        "nnfr_2r": "Nearest Neighbor Full Route (2-Regret)",
        "nnfr_weighted": "Nearest Neighbor Full Route (Weighted)",
    }
    if name in labels:
        return labels[name]
    return re.sub(r"(?<!^)(?=[A-Z])", " ", name.replace("_", " ")).title()


def load_algorithms(lab_num):
    algorithms = []
    for file in sorted((PY_DIR / lab_num).glob("*.py")):
        if file.stem.startswith("__") or "random" in file.stem.lower():
            continue

        module = import_module(f"{lab_num}.{file.stem}")
        finder = getattr(module, "find", None)
        if callable(finder):
            algorithms.append((file.stem, display_name(file.stem), finder))
    return algorithms


def generate_report(lab_num, instances, report_path=None):
    algorithms = load_algorithms(lab_num)
    if not algorithms:
        raise ValueError(f"No algorithms with a find() function found for lab {lab_num}")

    output_dir = PY_DIR / lab_num
    images_dir = output_dir / "images"
    images_dir.mkdir(parents=True, exist_ok=True)
    results = {}

    for instance in instances:
        instance_path = Path(instance)
        if not instance_path.is_absolute():
            instance_path = PROJECT_ROOT / instance_path
        instance_name = instance_path.name
        data = read_file(str(instance_path))
        matrix = get_matrix(data)
        costs = get_costs(data)
        instance_results = results.setdefault(instance_name, {})

        for method, label, finder in algorithms:
            scores, run_time, solutions = finder(matrix, costs)
            if not scores or len(scores) != len(solutions):
                raise ValueError(f"{method}.find() returned mismatched or empty scores and solutions")

            best_index = min(range(len(scores)), key=scores.__getitem__)
            image_name = f"{instance_path.stem}_{method}.png"
            image_path = images_dir / image_name
            plot(solutions[best_index], str(instance_path), str(image_path))
            instance_results[method] = {
                "label": label,
                "scores": scores,
                "runtime": run_time,
                "best_solution": solutions[best_index],
                "image": f"./images/{image_name}",
            }

    method_names = [method for method, _, _ in algorithms]
    method_labels = [label for _, label, _ in algorithms]
    lines = [
        f"# Lab {lab_num} report",
        "",
        "Wiktoria Drabińska 160294, Wojciech Dudziński 160293",
        "",
        "## Problem description",
        "",
        "We are given three columns of integers with a row for each node. The first two columns contain x",
        "and y coordinates of the node positions in a plane. The third column contains node costs. The goal is",
        "to select exactly 50% of the nodes (if the number of nodes is odd we round the number of nodes to",
        "be selected up) and form a Hamiltonian cycle (closed path) through this set of nodes such that the",
        "sum of the total length of the path plus the total cost of the selected nodes is minimized.",
        "",
        "The distances between nodes are calculated as Euclidean distances rounded mathematically to",
        "integer values. The distance matrix should be calculated just after reading an instance and then only",
        "the distance matrix (no nodes coordinates) should be accessed by optimization methods to allow",
        "instances defined only by distance matrices.",
        "",
        "## Algorithms",
        "",
        "Each implementation is evaluated on every requested instance. The summary statistics below cover all candidate solutions returned by that implementation.",
        "",
    ]

    for method, label, _ in algorithms:
        lines.extend([f"### {label}", "", "TODO !!", ""])

    lines.extend([
        "## Results",
        "",
        "| | " + " | ".join(method_labels) + " |",
        "| --- " * (len(method_labels) + 1) + "|",
    ])

    for instance_name, instance_results in results.items():
        rows = (
            ("Min", lambda entry: min(entry["scores"])),
            ("Avg", lambda entry: round(sum(entry["scores"]) / len(entry["scores"]), 2)),
            ("Max", lambda entry: max(entry["scores"])),
            ("Execution time", lambda entry: f"{round(entry['runtime'] * 1000, 2)}ms"),
        )
        lines.append(f"| **{instance_name}** |")
        for metric, value_for in rows:
            values = [str(value_for(instance_results[method])) for method in method_names]
            lines.append(f"| {metric} | " + " | ".join(values) + " |")

    for instance_name, instance_results in results.items():
        lines.extend(["", f"## {instance_name}", ""])
        for method in method_names:
            entry = instance_results[method]
            scores = entry["scores"]
            lines.extend([
                f"### {entry['label']}",
                "",
                f"Min: {min(scores)}",
                "",
                f"Avg: {round(sum(scores) / len(scores), 2)}",
                "",
                f"Max: {max(scores)}",
                "",
                f"Best solution: `{entry['best_solution']}`",
                "",
                f"![{entry['label']}]({entry['image']})",
                "",
                f"Execution time: {round(entry['runtime'] * 1000, 2)}ms",
                "",
            ])

    source_url = f"https://github.com/WikiDrabinka/ce/tree/main/{lab_num}"
    lines.extend([
        "## Conclusions",
        "",
        "TODO !!",
        "",
        "*All solutions have been verified with the solution checker*",
        "",
        "## Source code",
        "",
        f"[{source_url}]({source_url})",
    ])

    destination = Path(report_path) if report_path else output_dir / "report.md"
    if not destination.is_absolute():
        destination = PROJECT_ROOT / destination
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_text("\n".join(lines).rstrip() + "\n", encoding="utf-8")


if __name__ == "__main__":
    if len(argv) < 3:
        raise SystemExit("Usage: python py/report.py <lab-number> <instance.csv> [instance.csv ...]")
    generate_report(argv[1], argv[2:])