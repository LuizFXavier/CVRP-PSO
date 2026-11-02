#!/usr/bin/env python3
"""Lê os CSVs do RunLogger (_init, _iter, _hist) e gera gráficos + summary.csv.

Uso:
  python analyze_runs.py DIR_COM_CSVS [--out figs] [--bks X-n303-k21=21736 ...] [--at 250]

Nome esperado (base):
  <inst>_c1<c1>_c2<c2>_sw<swarm>_el<elite>_it<iters>_init<modo>_run<r>
"""
import argparse, re, sys
from pathlib import Path
import numpy as np
import pandas as pd
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

# CONFIRMAR no CVRPLIB antes de usar na monografia
BKS = {
    "X-n101-k25": 27591.0,
    "X-n190-k8": 16980.0,
    "X-n303-k21": 21736.0,
    "X-n573-k30": 50673.0,
    "X-n513-k21": 24_201.0,
    "X-n524-k153": 154_593.0
}

PAT = re.compile(r"^(?P<inst>.+?)_c1(?P<c1>[\d.]+)_c2(?P<c2>[\d.]+)_sw(?P<sw>\d+)"
                 r"_el(?P<el>\d+)_it(?P<it>\d+)_init(?P<init>.+)_run(?P<run>\d+)$")
CFG = ["c1", "c2", "sw", "el", "it", "init"]


def load(d):
    iters, inits, hists = [], [], []
    for f in sorted(Path(d).glob("*_iter.csv")):
        m = PAT.match(f.name[:-len("_iter.csv")])
        if not m:
            print(f"[aviso] nome fora do padrão, ignorado: {f.name}", file=sys.stderr)
            continue
        meta = m.groupdict()
        meta = {k: (float(v) if k in ("c1", "c2") else int(v) if k in ("sw", "el", "it", "run") else v)
                for k, v in meta.items()}
        base = str(f)[:-len("_iter.csv")]
        df = pd.read_csv(f)
        for k, v in meta.items(): df[k] = v
        iters.append(df)
        p = Path(base + "_init.csv")
        if p.exists():
            di = pd.read_csv(p)
            for k, v in meta.items(): di[k] = v
            inits.append(di)
        p = Path(base + "_hist.csv")
        if p.exists():
            dh = pd.read_csv(p)
            for k, v in meta.items(): dh[k] = v
            hists.append(dh)
    if not iters:
        sys.exit("nenhum *_iter.csv válido encontrado")
    cat = lambda l: pd.concat(l, ignore_index=True) if l else pd.DataFrame()
    return cat(iters), cat(inits), cat(hists)


def label(cfg, varying):
    d = dict(zip(CFG, cfg))
    return ", ".join(f"{k}={d[k]:g}" if isinstance(d[k], float) else f"{k}={d[k]}" for k in varying) or "única"


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("dir")
    ap.add_argument("--out", default=None)
    ap.add_argument("--bks", nargs="*", default=[], help="inst=valor")
    ap.add_argument("--at", type=int, default=250, help="iteração de referência para gap_at")
    a = ap.parse_args()
    out = Path(a.out) if a.out else Path(a.dir) / "figs"
    out.mkdir(parents=True, exist_ok=True)
    for kv in a.bks:
        k, v = kv.split("="); BKS[k] = float(v)

    it, ini, hist = load(a.dir)
    miss = sorted(set(it.inst) - set(BKS))
    if miss: sys.exit(f"faltam BKS para {miss}; passe --bks inst=valor")
    it["gap"] = (it.g_best / it.inst.map(BKS) - 1) * 100
    if len(ini):
        ini["init_gap"] = (ini.init_best_cost / ini.inst.map(BKS) - 1) * 100
        ini["init_mean_gap"] = (ini.init_mean_cost / ini.inst.map(BKS) - 1) * 100

    varying = [c for c in CFG if it[c].nunique() > 1]
    print("parâmetros que variam:", varying or "nenhum")
    insts = sorted(it.inst.unique())
    cfgs = sorted(it.groupby(CFG).groups.keys())
    cmap = plt.get_cmap("tab10")
    col = {c: cmap(i % 10) for i, c in enumerate(cfgs)}

    # ---------- resumo por (inst, cfg, run)
    rows = []
    for (inst, *c, run), g in it.groupby(["inst"] + CFG + ["run"]):
        g = g.sort_values("iter")
        r = dict(zip(["inst"] + CFG, [inst] + c)); r["run"] = run
        r["gap_final"] = g.gap.iloc[-1]
        ga = g[g.iter == a.at - 1].gap
        r[f"gap_at{a.at}"] = ga.iloc[0] if len(ga) else np.nan
        r["time_s"] = g.cum_ms.iloc[-1] / 1000
        r["swap_s"] = g.swap_star_ms.sum() / 1000
        r["two_opt_s"] = g.two_opt_ms.sum() / 1000
        r["import_s"] = g.import_ms.sum() / 1000
        r["frac_hit_cap"] = g.hit_cap.mean()
        rows.append(r)
    S = pd.DataFrame(rows)
    if len(ini):
        S = S.merge(ini[["inst"] + CFG + ["run", "init_gap", "init_mean_gap", "distinct_init"]],
                    on=["inst"] + CFG + ["run"], how="left")
    S.to_csv(out / "summary.csv", index=False)

    # tabela agregada: média sobre runs, depois sobre instâncias
    per_inst = S.groupby(["inst"] + CFG).mean(numeric_only=True).reset_index()
    agg = per_inst.groupby(CFG).agg(gap_final=("gap_final", "mean"), time_s=("time_s", "mean"),
                                    gap_at=(f"gap_at{a.at}", "mean")).reset_index()
    noise = S.groupby(["inst"] + CFG).gap_final.std().mean()
    print("\n=== média sobre instâncias (e runs) ===")
    print(agg.sort_values("gap_final").to_string(index=False, float_format="%.3f"))
    print(f"\nruído médio run-a-run (desvio do gap final): {noise:.3f} pontos")
    agg.to_csv(out / "agregado.csv", index=False)

    # ---------- Fig 1: convergência por instância
    fig, axs = plt.subplots(1, len(insts), figsize=(5 * len(insts), 4), squeeze=False)
    for ax, inst in zip(axs[0], insts):
        for c in cfgs:
            g = it[(it.inst == inst) & (it[CFG].apply(tuple, axis=1) == c)]
            if g.empty: continue
            m = g.groupby("iter").gap.agg(["mean", "std"]).fillna(0)
            ax.plot(m.index, m["mean"], color=col[c], label=label(c, varying))
            ax.fill_between(m.index, m["mean"] - m["std"], m["mean"] + m["std"], color=col[c], alpha=.15)
        ax.set_title(inst); ax.set_xlabel("iteração"); ax.set_ylabel("gap (%)"); ax.set_yscale("log")
        ax.grid(alpha=.3)
    axs[0][0].legend(fontsize=7)
    fig.suptitle("Convergência (média ± desvio entre runs)"); fig.tight_layout()
    fig.savefig(out / "1_convergencia.png", dpi=130); plt.close(fig)

    # ---------- Fig 2: init
    if len(ini):
        fig, axs = plt.subplots(1, 3, figsize=(15, 4))
        modes = sorted(ini.init.unique())
        w = .8 / max(len(modes), 1)
        for j, (colname, tt) in enumerate([("init_gap", "gap do melhor inicial (%)"),
                                           ("init_mean_gap", "gap médio inicial (%)"),
                                           ("distinct_init", "soluções iniciais distintas")]):
            for k, mo in enumerate(modes):
                vals = [ini[(ini.inst == i) & (ini.init == mo)][colname].mean() for i in insts]
                errs = [ini[(ini.inst == i) & (ini.init == mo)][colname].std() for i in insts]
                axs[j].bar(np.arange(len(insts)) + k * w, vals, w, yerr=np.nan_to_num(errs), label=mo, capsize=2)
            axs[j].set_xticks(np.arange(len(insts)) + w * (len(modes) - 1) / 2)
            axs[j].set_xticklabels(insts, rotation=20, fontsize=8); axs[j].set_title(tt)
            axs[j].grid(alpha=.3, axis="y")
        axs[0].legend(title="init")
        fig.tight_layout(); fig.savefig(out / "2_init.png", dpi=130); plt.close(fig)

    # ---------- Fig 3: diversidade do enxame
    d = it.dropna(subset=["distinct_swarm"])
    if len(d):
        fig, axs = plt.subplots(2, len(insts), figsize=(5 * len(insts), 7), squeeze=False)
        for j, inst in enumerate(insts):
            for c in cfgs:
                g = d[(d.inst == inst) & (d[CFG].apply(tuple, axis=1) == c)]
                if g.empty: continue
                m = g.groupby("iter")[["distinct_swarm", "frac_eq_gbest"]].mean()
                sw = c[CFG.index("sw")]
                axs[0][j].plot(m.index, m.distinct_swarm / sw, color=col[c], label=label(c, varying))
                axs[1][j].plot(m.index, m.frac_eq_gbest, color=col[c])
            axs[0][j].set_title(inst); axs[0][j].set_ylabel("distintas / swarm")
            axs[1][j].set_ylabel("fração == g_best"); axs[1][j].set_xlabel("iteração")
            for r in (0, 1): axs[r][j].grid(alpha=.3); axs[r][j].set_ylim(-.02, 1.02)
        axs[0][0].legend(fontsize=7)
        fig.suptitle("Diversidade do enxame (giant tours distintos)"); fig.tight_layout()
        fig.savefig(out / "3_diversidade.png", dpi=130); plt.close(fig)

    # ---------- Fig 4: heatmap c1 x c2
    if S.c1.nunique() > 1 and S.c2.nunique() > 1:
        sub = S.copy()
        for c in ("sw", "el", "it", "init"):
            if sub[c].nunique() > 1:  # fixa no valor mais frequente
                sub = sub[sub[c] == sub[c].mode()[0]]
        P = sub.groupby(["inst", "c1", "c2"]).gap_final.mean().reset_index()
        H = P.groupby(["c1", "c2"]).gap_final.mean().unstack()
        fig, ax = plt.subplots(figsize=(1.2 * len(H.columns) + 3, 1.0 * len(H) + 2))
        im = ax.imshow(H.values, cmap="viridis_r")
        ax.set_xticks(range(len(H.columns))); ax.set_xticklabels(H.columns)
        ax.set_yticks(range(len(H))); ax.set_yticklabels(H.index)
        ax.set_xlabel("c2"); ax.set_ylabel("c1")
        for i in range(H.shape[0]):
            for j in range(H.shape[1]):
                ax.text(j, i, f"{H.values[i, j]:.2f}", ha="center", va="center", color="w", fontsize=9)
        fig.colorbar(im, label="gap final médio (%)")
        ax.set_title(f"c1 × c2 (ruído ≈ {noise:.2f} pt)"); fig.tight_layout()
        fig.savefig(out / "4_heatmap_c1c2.png", dpi=130); plt.close(fig)

    # ---------- Fig 5: Pareto gap x tempo
    fig, ax = plt.subplots(figsize=(7, 5))
    for c in cfgs:
        r = agg[(agg[CFG] == pd.Series(c, index=CFG)).all(axis=1)].iloc[0]
        ax.scatter(r.time_s, r.gap_final, color=col[c], s=50)
        ax.annotate(label(c, varying), (r.time_s, r.gap_final), fontsize=7, xytext=(4, 3),
                    textcoords="offset points")
    ax.set_xlabel("tempo médio por run (s)"); ax.set_ylabel("gap final médio (%)")
    ax.set_title("Gap × tempo (cada ponto = uma configuração)"); ax.grid(alpha=.3)
    fig.tight_layout(); fig.savefig(out / "5_pareto.png", dpi=130); plt.close(fig)

    # ---------- Fig 6: paciência (parar cedo)
    ks = [50, 100, 150, 200, 250, 300, 400, 500, 750, 1000]
    fig, axs = plt.subplots(1, len(insts), figsize=(5 * len(insts), 4), squeeze=False)
    for ax, inst in zip(axs[0], insts):
        for c in cfgs:
            g = it[(it.inst == inst) & (it[CFG].apply(tuple, axis=1) == c)]
            if g.empty: continue
            piv = g.pivot_table(index="run", columns="iter", values="gap")
            last = piv.iloc[:, -1]
            kk = [k for k in ks if k - 1 in piv.columns]
            ax.plot(kk, [(piv[k - 1] - last).mean() for k in kk], marker="o", color=col[c],
                    label=label(c, varying))
        ax.axhline(noise, ls="--", color="gray", lw=1)
        ax.set_title(inst); ax.set_xlabel("parar na iteração k"); ax.set_ylabel("gap(k) − gap final (pt)")
        ax.grid(alpha=.3)
    axs[0][0].legend(fontsize=7)
    fig.suptitle("Quanto se perde parando cedo (linha tracejada = ruído)"); fig.tight_layout()
    fig.savefig(out / "6_paciencia.png", dpi=130); plt.close(fig)

    # ---------- Fig 7: tempo por fase
    T = S.groupby(["inst"] + CFG)[["swap_s", "two_opt_s", "import_s", "time_s"]].mean().reset_index()
    T["outros_s"] = (T.time_s - T.swap_s - T.two_opt_s - T.import_s).clip(lower=0)
    T["cfg"] = T[CFG].apply(lambda r: label(tuple(r), varying), axis=1)
    G = T.groupby("cfg")[["swap_s", "two_opt_s", "import_s", "outros_s"]].mean()
    fig, ax = plt.subplots(figsize=(1.0 * len(G) + 5, 4.5))
    bot = np.zeros(len(G))
    for cn, nm in [("swap_s", "swap* (GPU)"), ("two_opt_s", "2-opt (CPU)"),
                   ("import_s", "import/cópia"), ("outros_s", "outros")]:
        ax.bar(range(len(G)), G[cn], bottom=bot, label=nm); bot += G[cn].values
    ax.set_xticks(range(len(G))); ax.set_xticklabels(G.index, rotation=30, ha="right", fontsize=7)
    ax.set_ylabel("s por run (média)"); ax.legend(); ax.grid(alpha=.3, axis="y")
    ax.set_title("Tempo por fase"); fig.tight_layout(); fig.savefig(out / "7_tempo_fases.png", dpi=130)
    plt.close(fig)

    # ---------- Fig 8: chamadas do swap* e teto
    fig, axs = plt.subplots(1, 2, figsize=(11, 4))
    for c in cfgs:
        g = it[it[CFG].apply(tuple, axis=1) == c]
        m = g.groupby("iter")[["calls_used", "hit_cap"]].mean()
        axs[0].plot(m.index, m.calls_used, color=col[c], label=label(c, varying))
        axs[1].plot(m.index, m.hit_cap, color=col[c])
    axs[0].set_ylabel("chamadas do swap* por iteração"); axs[1].set_ylabel("fração de iterações no teto")
    for x in axs: x.set_xlabel("iteração"); x.grid(alpha=.3)
    axs[0].legend(fontsize=7); fig.tight_layout(); fig.savefig(out / "8_chamadas_teto.png", dpi=130)
    plt.close(fig)

    # ---------- Fig 9: histograma de melhorias por chamada (se houver)
    if len(hist):
        fig, ax = plt.subplots(figsize=(7, 4))
        for itn, g in hist.groupby("iter"):
            m = g.groupby("call").n_improved.mean()
            ax.plot(m.index, m.values, marker=".", label=f"iter {itn}")
        ax.set_xlabel("chamada"); ax.set_ylabel("partículas melhoradas (média)")
        ax.legend(); ax.grid(alpha=.3); ax.set_title("Melhorias por chamada do swap*")
        fig.tight_layout(); fig.savefig(out / "9_melhorias_por_chamada.png", dpi=130); plt.close(fig)

    print(f"\nfiguras e CSVs em {out}")


if __name__ == "__main__":
    main()
