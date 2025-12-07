# Cache Simulator in C

A fully configurable cache simulator supporting:
- Direct-mapped & N-way set-associative
- LRU replacement policy
- Write-back + write-allocate
- Realistic memory access patterns with temporal/spatial locality

Achieved **85-95% hit rates** in simulations.

### Usage
```bash
gcc cache_sim.c -o cache_sim -lm
./cache_sim
