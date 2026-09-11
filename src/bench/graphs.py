import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.ticker as ticker
import numpy as np
import os

out_dir = "../benchmarks"
os.makedirs(out_dir, exist_ok=True)

plt.rcParams.update({
    "font.family":   "monospace",
    "font.size":     11,
    "axes.spines.top":   False,
    "axes.spines.right": False,
    "axes.grid":     True,
    "grid.alpha":    0.3,
    "grid.linestyle": "--",
})

BLUE   = "#2f7fc1"
GREEN  = "#2e9e5b"
ORANGE = "#d97706"
RED    = "#c0392b"


# --- graph 1: query latency cdf ---

df = pd.read_csv("latency.csv")
latencies = np.sort(df["latency_us"].values)
cdf = np.linspace(0, 100, len(latencies))

p50  = np.percentile(latencies, 50)
p95  = np.percentile(latencies, 95)
p99  = np.percentile(latencies, 99)
p999 = np.percentile(latencies, 99.9)

fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(latencies, cdf, color=BLUE, linewidth=1.8, label="latency cdf")

for val, color, label in [
    (p50,  GREEN,  f"p50   {p50:.1f}µs"),
    (p95,  ORANGE, f"p95   {p95:.1f}µs"),
    (p99,  RED,    f"p99   {p99:.1f}µs"),
]:
    ax.axvline(val, color=color, linestyle="--", linewidth=1.2, label=label)

ax.set_xlabel("latency (µs)")
ax.set_ylabel("percentile")
ax.set_title("query latency cdf - mini-search-engine")
ax.legend(framealpha=0.6)
fig.tight_layout()
fig.savefig(f"{out_dir}/latency_cdf.png", dpi=150)
plt.close()
print("saved latency_cdf.png")


# --- graph 2: index build time vs corpus size ---

df = pd.read_csv("build.csv")

fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(df["num_docs"], df["build_time_ms"],
        color=BLUE, linewidth=1.8, marker="o", markersize=5)

ax.set_xlabel("documents indexed")
ax.set_ylabel("build time (ms)")
ax.set_title("index build time vs corpus size - mini-search-engine")
ax.xaxis.set_major_formatter(ticker.FuncFormatter(lambda x, _: f"{int(x):,}"))
fig.tight_layout()
fig.savefig(f"{out_dir}/build_time.png", dpi=150)
plt.close()
print("saved build_time.png")


# --- graph 3: throughput vs thread count ---

df = pd.read_csv("qps.csv")

fig, ax = plt.subplots(figsize=(8, 5))
ax.plot(df["threads"], df["qps"],
        color=BLUE, linewidth=1.8, marker="o", markersize=5, label="measured qps")

# linear scaling reference line - shows ideal parallelism
ideal_qps = df["qps"].iloc[0] * df["threads"]
ax.plot(df["threads"], ideal_qps,
        color="gray", linewidth=1, linestyle="--", label="linear scaling (ideal)")

ax.set_xlabel("thread count")
ax.set_ylabel("queries per second")
ax.set_title("throughput vs thread count - mini-search-engine")
ax.legend(framealpha=0.6)
fig.tight_layout()
fig.savefig(f"{out_dir}/qps_scaling.png", dpi=150)
plt.close()
print("saved qps_scaling.png")
