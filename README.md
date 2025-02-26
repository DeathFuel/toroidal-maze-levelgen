# toroidal-maze level generator
This repository contains a level generator program that was designed for [a game I made](https://github.com/DeathFuel/toroidal-maze).

## How it works
The generator first determines, for each tile, the probability of a wall being present there (referred to as a tile 'density'). This is accomplished by filling an array of floating-point numbers with values centered around a previously selected average density, then optionally adding significant noise and triggering a run of [simulated annealing](https://en.wikipedia.org/wiki/Simulated_annealing) to rearrange the density values, producing some pretty-looking crystal patterns in the process.

The next steps rely on iteratively improving a solution (level) such that its score, determined by the complexity of the longest path in the level, does not decrease. Each iteration then rerolls a portion of the tiles based on the previously determined densities.
Significant efforts are made to generate a level that is both beatable and has no spots where a player can get irreversibly stuck.

## Known issues
Level generation may fail sporadically for low iteration counts. The likelihood of this happening appears to decrease rapidly as iterations increase past ~1024.
