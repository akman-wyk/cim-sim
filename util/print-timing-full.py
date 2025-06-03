#!/usr/bin/env python3
"""
Visualise timing profiles for *all* instruction-profiling modules in one figure,
giving each module a distinct colour and adding a legend.

Original single-module routine `print_timing()` is preserved in case you still
need per-module plots.
"""
import itertools
import json
import matplotlib
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import plotly.io as pio
import argparse
import pathlib

import plotly.graph_objects as go
import plotly.express as px
import plotly.io as pio

# ---------- user-configurable paths & settings ----------
_profiling_json_file_path = (
    # Replace with your actual path if different or make it a CLI argument
    "/home/yingjie/Documents/Projects/CIMFlow/cim-sim/report/profiling-resnet-inst-c64-inst100.json"
)
_out_dir = "/home/yingjie/Documents/Projects/CIMFlow/cim-sim/report/profile" # Simpler default for general use
_time_line_space = 4      # vertical gap between bars
_time_line_width = 2      # thickness of each bar
_colour_cycle = plt.cm.tab20.colors  # 20 visually distinct colours to rotate through
# --------------------------------------------------------

matplotlib.rcParams["axes.unicode_minus"] = False  # keep minus signs visible


# --------------------------------------------------------------------------- #
#                                   Helpers                                   #
# --------------------------------------------------------------------------- #
def _load_json(path: str):
    with open(path, "r") as f:
        return json.load(f)


def _segments(module_entry):
    """Return (start, duration) tuples compatible with `plt.broken_barh`."""
    return [
        (seg["start"], seg["end"] - seg["start"])
        for seg in module_entry["time_segment_list"]
    ]

def _leaf_has_data(leaf):
    """True iff leaf looks like {'time_segment_list': [...] } with >0 width."""
    if not isinstance(leaf, dict):
        return False
    segs = leaf.get("time_segment_list", [])
    return any(seg["end"] > seg["start"] for seg in segs)


def _collect_leaves(prefix, node):
    """
    Recursively yield (full_label, leaf_dict) where leaf_dict has data.
    'prefix' accumulates the path so labels stay readable.
    """
    if isinstance(node, dict) and "time_segment_list" not in node:
        for k, v in node.items():
            new_prefix = f"{prefix}: {k}" if prefix else k
            yield from _collect_leaves(new_prefix, v)
    else:
        if _leaf_has_data(node):
            yield (prefix, node)


# --------------------------------------------------------------------------- #
#                           Interactive Plotly timeline                       #
# --------------------------------------------------------------------------- #
def plot_interactive(profile, out_html=None, label_filters=None, time_fraction=1.0):
    """
    Generate an interactive HTML timeline (zoomable and pannable) using Plotly.
    The output file can be opened in any modern browser. A range-slider under
    the x-axis allows quick zooming to any time window.

    Parameters
    ----------
    profile : dict
        JSON-decoded profiling dictionary.
    out_html : str | None
        Destination HTML file. Defaults to `<_out_dir>/timeline.html`.
    label_filter : str | None
        Only plot leaves whose label contains this substring.
    time_fraction : float, optional
        Fraction of the total time duration to display initially (0 < fraction <= 1.0).
        Defaults to 1.0 (full duration). The rangeslider still shows the full range.
    """
    if out_html is None:
        out_html = f"{_out_dir}/timeline.html"

    colour_cycle = itertools.cycle(px.colors.qualitative.Alphabet)
    fig = go.Figure()
    max_end_time = 0.0

    for top_name, top_block in profile["instruction_profiling"].items():
        leaves = list(_collect_leaves(top_name, top_block))
        if not leaves:
            continue

        colour = next(colour_cycle)

        for full_label, leaf in leaves:
            # --- Apply label_filters ---
            if label_filters: # If a list of filters is provided and is not empty
                if not any(f_str in full_label for f_str in label_filters):
                    continue # Skip if no filter string matches
            # --- End label_filters ---
            for seg in leaf["time_segment_list"]:
                start, end = seg["start"], seg["end"]
                if start >= end:
                    continue
                max_end_time = max(max_end_time, end)
                fig.add_trace(
                    go.Scatter(
                        x=[start, end],
                        y=[full_label, full_label],
                        mode="lines",
                        line=dict(width=10, color=colour),
                        hovertemplate=(
                            f"{full_label}<br>"
                            "start = %{x[0]}<br>"
                            "end   = %{x[1]}<extra></extra>"
                        ),
                        showlegend=False,
                    )
                )

    if not fig.data:
        print("Nothing to plot for interactive timeline - all modules empty or filtered out.")
        return

    xaxis_config = dict(
        rangeslider=dict(visible=True),
        type="linear",
    )
    if max_end_time > 0 and time_fraction < 1.0:
        x_limit = max_end_time * time_fraction
        xaxis_config["range"] = [0, x_limit]
        print(f"Plotly plot initially showing first {time_fraction*100:.1f}% of time (0 to {x_limit:.2f} ns).")

    unique_labels = {trace.y[0] for trace in fig.data}
    fig_height = max(400, 20 * len(unique_labels))

    fig.update_layout(
        height=fig_height,
        xaxis_title="Time (ns)",
        yaxis=dict(autorange="reversed"),
        xaxis=xaxis_config,
        margin=dict(l=220, r=40, t=20, b=40),
    )

    pio.write_html(fig, file=out_html, auto_open=False)
    print(f"Interactive timeline written to {out_html}")


# --------------------------------------------------------------------------- #
#                                   Plotting                                  #
# --------------------------------------------------------------------------- #
def plot_all_modules(profile, time_fraction=1.0, figure_width=20, label_filters=None):
    """
    Draw every module (any depth) on one chart, skipping empty entries,
    with dynamic figure size and a right-hand legend.

    Parameters
    ----------
    profile : dict
        JSON-decoded profiling dictionary.
    time_fraction : float, optional
        Fraction of the total time duration to plot (0 < fraction <= 1.0).
        Defaults to 1.0 (full duration).
    figure_width : float, optional
        The total width of the output PNG figure in inches. Defaults to 20.
    label_filter : str | None, optional
        Only plot leaves whose label contains this substring. Defaults to None.
    """
    instr_prof   = profile["instruction_profiling"]
    colour_cycle = itertools.cycle(_colour_cycle)
    fig, ax = plt.subplots(figsize=(figure_width, 4))

    y_tick, y_pos, y_labels, legend_handles = _time_line_space, [], [], []
    max_end_time = 0.0
    plot_data_exists = False

    for top_name, top_block in instr_prof.items():
        leaves = list(_collect_leaves(top_name, top_block))
        if not leaves:
            continue

        colour = next(colour_cycle)
        current_legend_handle = mpatches.Patch(color=colour, label=top_name)
        added_legend_for_top_name = False

        for full_label, leaf in leaves:
            # --- Apply label_filters ---
            if label_filters: # If a list of filters is provided and is not empty
                if not any(f_str in full_label for f_str in label_filters):
                    continue # Skip if no filter string matches
            # --- End label_filters ---
            
            segs = _segments(leaf)
            if not segs: continue

            plot_data_exists = True
            if not added_legend_for_top_name:
                legend_handles.append(current_legend_handle)
                added_legend_for_top_name = True

            for start, duration in segs:
                max_end_time = max(max_end_time, start + duration)

            ax.broken_barh(
                segs,
                (y_tick - _time_line_width / 2, _time_line_width),
                facecolors=colour,
            )
            y_pos.append(y_tick)
            y_labels.append(full_label)
            y_tick += _time_line_space

        if added_legend_for_top_name:
            y_tick += _time_line_space / 2

    if not plot_data_exists:
        print("Nothing to plot for PNG - all modules empty or filtered out.")
        plt.close(fig)
        return

    bars   = len(y_pos)
    fig_h = max(4, 0.35 * bars)
    fig.set_size_inches(figure_width, fig_h)

    ax.set_ylim(0, y_tick)
    ax.set_yticks(y_pos)
    ax.set_yticklabels(y_labels, fontsize=8)
    ax.set_xlabel("Time (ns)", fontsize=10)

    if max_end_time > 0 and time_fraction < 1.0:
        x_limit = max_end_time * time_fraction
        ax.set_xlim(0, x_limit)
        print(f"Matplotlib plot limited to first {time_fraction*100:.1f}% of time (0 to {x_limit:.2f} ns).")
    elif max_end_time > 0 :
        ax.set_xlim(0, max_end_time)
    else:
        ax.set_xlim(0, 1)

    if legend_handles:
        ax.legend(
            handles=legend_handles,
            loc="upper left",
            bbox_to_anchor=(1.02, 1.0),
            borderaxespad=0.0,
            frameon=False,
        )
    
    fig.subplots_adjust(left=0.10, right=0.95, bottom=0.1, top=0.95)
    fig.tight_layout(rect=[0, 0, 1, 1])


    output_png_path = f"{_out_dir}/all_modules.png"
    fig.savefig(output_png_path, dpi=300)
    print(f"PNG written to {output_png_path}")
    plt.close(fig)


# --------------------------------------------------------------------------- #
#                   (Optional) original single-module routine                 #
# --------------------------------------------------------------------------- #
def print_timing(profile, name):
    """Legacy helper: keep old single-module plot for convenience."""
    module = profile["instruction_profiling"][name]
    y_pos, y_labels = [], []
    y_tick = _time_line_space

    fig, ax = plt.subplots() # Create a figure and axes for this plot

    for sub, entry in module.items():
        segs = _segments(entry)
        ax.broken_barh( # Use ax.broken_barh
            segs,
            (y_tick - _time_line_width / 2, _time_line_width),
            facecolors="#9AD6D2",
        )
        y_pos.append(y_tick)
        y_labels.append(sub)
        y_tick += _time_line_space

    ax.set_ylim(0, y_tick) # Use ax.set_ylim
    ax.set_yticks(y_pos) # Use ax.set_yticks
    ax.set_yticklabels(y_labels, fontsize=10.5) # Use ax.set_yticklabels
    fig.tight_layout() # Use fig.tight_layout
    fig.savefig(f"{_out_dir}/{name}.png", bbox_inches="tight") # Use fig.savefig
    plt.close(fig)


# --------------------------------------------------------------------------- #
#                                     CLI                                     #
# --------------------------------------------------------------------------- #
if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Visualise CIMFlow timing profiles."
    )
    parser.add_argument(
        "--json-file", # Added argument for specifying JSON file
        type=str,
        default=_profiling_json_file_path,
        help=f"Path to the profiling JSON file. Defaults to '{_profiling_json_file_path}'",
    )
    parser.add_argument(
        "--out-dir", # Added argument for specifying output directory
        type=str,
        default=_out_dir,
        help=f"Directory to save output figures. Defaults to '{_out_dir}'",
    )
    parser.add_argument(
        "--html",
        action="store_true",
        help="Generate an interactive HTML timeline.", # Kept this help text
    )
    parser.add_argument(
        "--html-out",
        type=str,
        metavar="FILE",
        default=None,
        help="Custom path for the HTML output (implies --html). "
             "Defaults to <out-dir>/timeline.html",
    )
    parser.add_argument(
        "--label",
        type=str,
        metavar="STR",
        nargs='+', # Accept one or more arguments
        default=None,
        help="Filter: only plot items whose full label contains ANY of these substrings (space-separated). "
            "Applies to PNG and, if generated, HTML outputs.",
    )
    
    parser.add_argument(
        "--time-fraction",
        type=float,
        metavar="FRAC",
        default=1.0,
        help="Plot only the first FRAC portion of the total time duration "
             "(e.g., 0.2 for the first fifth). Default is 1.0 (full duration).",
    )

    parser.add_argument(
        "--figure-width",
        type=float,
        metavar="INCHES",
        default=20.0,
        help="Width of the generated PNG image in inches. Default is 20.0.",
    )
    
    # Add an argument to call the legacy print_timing function
    parser.add_argument(
        "--legacy-plot",
        type=str,
        metavar="MODULE_NAME",
        default=None,
        help="Generate a legacy single-module plot for the specified top-level module name."
    )


    args = parser.parse_args()

    if not 0.0 < args.time_fraction <= 1.0:
        parser.error("--time-fraction must be between 0 (exclusive) and 1 (inclusive).")

    # Update global _out_dir and _profiling_json_file_path from args
    _out_dir = args.out_dir
    _profiling_json_file_path = args.json_file

    pathlib.Path(_out_dir).mkdir(parents=True, exist_ok=True)

    try:
        profile = _load_json(_profiling_json_file_path)
    except FileNotFoundError:
        print(f"Error: Profiling JSON file not found at '{_profiling_json_file_path}'")
        exit(1)
    except json.JSONDecodeError:
        print(f"Error: Could not decode JSON from '{_profiling_json_file_path}'")
        exit(1)
        
    # Check for instruction_profiling key
    if "instruction_profiling" not in profile:
        print(f"Error: 'instruction_profiling' key not found in JSON file '{_profiling_json_file_path}'.")
        print("Please ensure the JSON file is a valid CIMFlow profiling report.")
        exit(1)


    # Always generate the PNG plot (it's the primary output of plot_all_modules)
    # Pass the label_filter to plot_all_modules
    print("Generating PNG timeline...")
    plot_all_modules(profile,
                     time_fraction=args.time_fraction,
                     figure_width=args.figure_width,
                     label_filters=args.label)

    # Generate HTML plot only if --html or --html-out is specified
    if args.html or args.html_out:
        html_output_file = args.html_out
        if html_output_file is None: # If --html is true but --html-out is not set
            html_output_file = f"{_out_dir}/timeline.html"
        
        # Ensure the directory for html_output_file exists if it's custom
        pathlib.Path(html_output_file).parent.mkdir(parents=True, exist_ok=True)

        print("Generating interactive HTML timeline...")
        plot_interactive(profile,
                         out_html=html_output_file,
                         label_filters=args.label, # Pass label filter here too
                         time_fraction=args.time_fraction)
    
    # Handle legacy plot generation
    if args.legacy_plot:
        if args.legacy_plot in profile["instruction_profiling"]:
            print(f"Generating legacy plot for module: {args.legacy_plot}...")
            print_timing(profile, args.legacy_plot)
        else:
            print(f"Error: Module '{args.legacy_plot}' not found for legacy plot.")
            print(f"Available top-level modules: {list(profile['instruction_profiling'].keys())}")


    print("Script finished.")