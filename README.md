# Coupled Simulated Annealing
We developed a coupled simulated annealing implementation that runs stochastic optimizations in parallel using MPI.  We describe here how to use the sampler and explore some of its properties and potential efficiency gains on a series of increasingly complex random Markov Chain microsimulations.

## Run the Sampler
First, we must compile the sampler. The easiest way to do this is to get an interactive job. We request two nodes, so we can test the compiled sampler when we finish.
`srun --pty -p test -n 2 --mem 500M -t 0-06:00 /bin/bash`

From here, load the necessary modules. 

For Odyssey:
`module load gcc/6.3.0-fasrc01 openmpi/2.1.0-fasrc01 java/1.8.0_45-fasrc01`

For Orchestra:
`module load gcc openmpi java`

Now we can compile the sampler:
`mpic++ sampler_MPI.cpp -o sampler -lm`

Finally, let's test the result. The sampler takes two parameters: the config file, which specifies the model and optimization technique, as well as a task number, for use with batch deployment:
`mpirun -np 2 sampler InputFile.csv 1`

With the sampler compiled, you can exit your interactive session and launch further experiments with sbatch. `mpiScript-Odyssey.sh` and `mpiScript-Orchestra.sh` contain a minimal example experiment.

To run with sbatch, from the repo, copy the compiled sampler, model, config and .sh file to a fresh directory:
```
mkdir ../example
cp sampler InputFile.csv testModel.jar mpiScript-odyssey.sh ../example
cd ../example
sbatch mpiScript.sh
```
You will need to adjust the output and error options in `mpiScript-odyssey.sh` to accomodate your directory location.

## Configuration File
The sampler takes in a configuration file in the form of a .csv to specific the model, output file, optimization method, and other parameters to test.

`InputFile.csv` contains the settings used for our results.

### Compute Platforms
We tested the implementation on O2 (https://wiki.rc.hms.harvard.edu/display/O2) and Odyssey (https://www.rc.fas.harvard.edu/odyssey/).  Information on how to run on these SLURM environments are provided here…

O2 is a cluster computer at Harvard Medical School that currently includes 268 computing nodes for a total of 8064 cores and ~68TB of memory.  It uses a SLURM scheduler.


## Simulated Annealing
Simulated annealing is a stochastic optimization technique that approximates the global optimum by trying to minimize the energy of a system via a ‘cooling schedule’.  [https://en.wikipedia.org/wiki/Simulated_annealing]
The algorithm works by proposing new states that are ‘near’ the current state.  If a new state is ‘better’ (i.e. lower loss score) than the current state, the proposed state will always be accepted.  If the new state has a higher loss score it will be accepted with some probability – this helps the search path to not get trapped in local minima.  Higher temperatures correspond to higher acceptance probabilities, allowing the algorithm to better search the global parameter space.  As the temperature cools the acceptance probabilities decrease, allowing the algorithm to fine tune the local search space.
The algorithm is typically run sequentially:
### Sequential Algorithm
```
Let t = t_0 #initial temperature   
Let s = s_0 #initial state   
Let d = d_0 #initial loss/distance score   
For i = 0 to i_max:   
	Propose s*|s #propose nearby state   
	Calculate d* = D(s*) #calculate loss function   
	If d* < d: #always move   
		 s = s*, d = d*   
	Else If rand(0,1) < exp(-(d*-d)/t) #sometimes move   
		 s = s*, d = d*   
	t = t_i #cool temperature
```
### Coupled Simulated Annealing
While simulated annealing is typically used in an embarrassingly parallel workflow (High Throughput Computing), the algorithm is amenable to a more tightly coupled parallelization.  We developed a parallel MPI implementation of the sampling algorithm, allowing it to be used to optimize models running on a distributed computing cluster.  Each worker node runs the model independently and then communicates the loss score back to the master, who decides on the next parameter set for each worker to evaluate.
### Parallel Algorithm
```
Let t = t_0 #initial temperature
Let s = s_0 #initial state
Let d = d_0 #initial loss/distance score
Set k #define number of coupled chains
For i = 0 to i_max:
	For j = 0 to k: #for each chain
		Propose s*_j|s #master proposes nearby state
		MPI Send/Receive (s*_j) #worker receives proposed state from master
		Calculate dj* = D(s*_j) #worker calculates loss function
		MPI_Send/Receive (d*_j) #master receives all scores
	d* = min(d*_0, d*_1,…, d*_k-1)
	If d* < d: #always move
		 s = s*, d = d*
	Else If rand(0,1) < exp(-(d*-d)/t) #sometimes move
		 s = s*, d = d*
	t = t_i #cool temperature`
```
## Benchmarking
To evaluate the speed-up of the parallel algorithm we optimized a set of random Markov Chain microsimulation models. We defined Markov Chains of different sizes N=3, 10, 32  (i.e. # parameters = 9, 100, 1024), and ran each chain with 100,000 individuals.  We used an increasing number of Markov states to benchmark the parallel performance with respect to increasing model complexity.  As the state space grows the number of parameters grows O2, yielding a highly dimensional parameter space to optimize over.
For each size N we generated 1,000 random target state distributions and optimized the parameters via coupled simulated annealing with k=1, 2, 4, 8 chains
We calculated the average score by iteration
We also calculated speed-up by ‘terminating’ each recorded chain once a score below a specified threshold was reached
### Random Markov Chain
We defined Markov Chains of different sizes N=3, 10, 32  (i.e. # parameters =9, 100, 1024), and ran each chain with 100,000 individuals.  A random Markov chain is defined by a random (square) transition matrix.
![Markov Chain](/images/markov.png)
For each chain we randomly generated a target state distribution at cycle 50:

<a href="https://www.codecogs.com/eqnedit.php?latex=S^*&space;=&space;[s_0^*^{50},&space;s_1^*^{50},&space;...&space;,s_{N-1}^*^{50}]&space;s.t.&space;\sum_{i=0}^{N-1}&space;s_i^*=1" target="_blank"><img src="https://latex.codecogs.com/gif.latex?S^*&space;=&space;[s_0^*^{50},&space;s_1^*^{50},&space;...&space;,s_{N-1}^*^{50}]&space;s.t.&space;\sum_{i=0}^{N-1}&space;s_i^*=1" title="S^* = [s_0^*^{50}, s_1^*^{50}, ... ,s_{N-1}^*^{50}] s.t. \sum_{i=0}^{N-1} s_i^*=1" /></a>

With current parameter set (i.e. transition matrix T), run 100,000 individuals through the Markov chain to estimate:
<a href="https://www.codecogs.com/eqnedit.php?latex=\hat{S}=[\hat{s}_0^{50},\hat{s}_1^{50},...,\hat{s}_{N-1}^{50}]" target="_blank"><img src="https://latex.codecogs.com/gif.latex?\hat{S}=[\hat{s}_0^{50},\hat{s}_1^{50},...,\hat{s}_{N-1}^{50}]" title="\hat{S}=[\hat{s}_0^{50},\hat{s}_1^{50},...,\hat{s}_{N-1}^{50}]" /></a>

Calculate distance score:
<a href="https://www.codecogs.com/eqnedit.php?latex=\hat{D}=\left&space;\|&space;S^*-\hat{S}&space;\right&space;\|_2" target="_blank"><img src="https://latex.codecogs.com/gif.latex?\hat{D}=\left&space;\|&space;S^*-\hat{S}&space;\right&space;\|_2" title="\hat{D}=\left \| S^*-\hat{S} \right \|_2" /></a>

## Results
link images here

## Conclusions
As models get more complex, time to converge is pushed out (i.e. more iterations)
Peak speed-up is at higher thresholds for more complex models
Super-linear speed-up is possible for some combinations of threshold/model complexity
For complex models, coupled SA offers the possibility of achieving loss scores not possible with sequential SA
