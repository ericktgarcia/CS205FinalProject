# Coupled Simulated Annealing
We developed a coupled simulated annealing implementation that runs stochastic optimizations in parallel using MPI.  We describe here how to use the sampler and explore some of its properties and potential efficiency gains on a series of increasingly complex random Markov Chain microsimulations.

## Run the Sampler
After cloning the repo on Odyssey or Orchestra, we must compile the sampler. The easiest way to do this is to get an interactive job. We request two nodes, so we can test the compiled sampler when we finish.
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
The sampler takes in a configuration file in the form of a .csv to specify the model, output file, optimization method, and other parameters to test.

`InputFile.csv` contains the settings used for our results.

### Compute Platforms
We tested the implementation on O2 (https://wiki.rc.hms.harvard.edu/display/O2) and Odyssey (https://www.rc.fas.harvard.edu/odyssey/), both of which use a SLURM scheduler.
Odyssey is Harvard's largest cluster with over 78,000 cores and 2,000+ nodes.
O2 is a cluster computer at Harvard Medical School that currently includes 268 computing nodes for a total of 8064 cores and ~68TB of memory.

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
While simulated annealing is typically used in an embarrassingly parallel workflow (High Throughput Computing), the algorithm is amenable to a more tightly coupled parallelization.  We developed a parallel MPI implementation of the algorithm, allowing it to be used to optimize models running on a cluster (i.e. distributed memory).  Each worker node runs the model independently and then communicates the loss score back to the master, which decides on the next parameter set for each worker to evaluate.
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
To evaluate the speed-up of the parallel algorithm we optimized a set of random Markov Chain microsimulation models written in Java. We defined Markov Chains of different sizes *N*=3, 10, 32  (i.e. # parameters = 9, 100, 1024), and ran each chain with 100,000 individuals.  We used Java multi-threading (2 cores) to run the individuals in parallel within each model for faster performance.

We used an increasing number of Markov states to benchmark the parallel performance with respect to increasing model complexity.  As the state space grows the number of parameters grows O(n<sup>2</sup>), yielding a highly dimensional parameter space to optimize.
For each size *N* we generated 1,000 random target state distributions (i.e. 1,000 trials) and optimized the parameters via coupled simulated annealing with *k*=1, 2, 4, 8 chains, where *k*=1 corresponds to the sequential algorithm.
We calculated the average and minimum score across all runs for each search iteration. We also estimated the expected speed-up by ‘terminating’ each recorded chain once a score below a specified threshold was reached.

### Random Markov Chain
A random Markov chain is defined by a random (square) transition matrix.
![Markov Chain](/images/markov.png)
For each chain we randomly generated a target state distribution at cycle 50:

<a href="https://www.codecogs.com/eqnedit.php?latex=S^*&space;=&space;[s_0^*^{50},&space;s_1^*^{50},&space;...&space;,s_{N-1}^*^{50}]&space;s.t.&space;\sum_{i=0}^{N-1}&space;s_i^*=1" target="_blank"><img src="https://latex.codecogs.com/gif.latex?S^*&space;=&space;[s_0^*^{50},&space;s_1^*^{50},&space;...&space;,s_{N-1}^*^{50}]&space;s.t.&space;\sum_{i=0}^{N-1}&space;s_i^*=1" title="S^* = [s_0^*^{50}, s_1^*^{50}, ... ,s_{N-1}^*^{50}] s.t. \sum_{i=0}^{N-1} s_i^*=1" /></a>

With the current parameter set (i.e. transition matrix T), we ran 100,000 individuals through the Markov chain to estimate:
<a href="https://www.codecogs.com/eqnedit.php?latex=\hat{S}=[\hat{s}_0^{50},\hat{s}_1^{50},...,\hat{s}_{N-1}^{50}]" target="_blank"><img src="https://latex.codecogs.com/gif.latex?\hat{S}=[\hat{s}_0^{50},\hat{s}_1^{50},...,\hat{s}_{N-1}^{50}]" title="\hat{S}=[\hat{s}_0^{50},\hat{s}_1^{50},...,\hat{s}_{N-1}^{50}]" /></a>

We then calculated the distance (loss) score:

<a href="https://www.codecogs.com/eqnedit.php?latex=\hat{D}=\left&space;\|&space;S^*-\hat{S}&space;\right&space;\|_2" target="_blank"><img src="https://latex.codecogs.com/gif.latex?\hat{D}=\left&space;\|&space;S^*-\hat{S}&space;\right&space;\|_2" title="\hat{D}=\left \| S^*-\hat{S} \right \|_2" /></a>

## Results
Here we plot the mean and minimum scores by search iteration (across the 1,000 trials) for each size *N* of Markov Chain and coupling size *k*.  We also plot the expected speed-up (i.e. number of iterations to reach a certain score) at different score thresholds.

![3 states](/images/S3.png)
![10 states](/images/S10.png)
![32 states](/images/S32.png)

We see above that due to the exponential cooling schedule we used in simulated annealing we could achieve super-linear speed-up by coupling chains together!  That is, the expected number of iterations to achieve a score below a certain threshold decreased by a factor larger than the increase in additional chains (i.e. MPI processes) used.

We also see that potential speed-up is greater for 'simple' models (e.g. 9 parameters) and 'complex' models (e.g. 1024 parameters), with smaller (but still substantial and potentially super-linear) speed-up for 'medium' models (e.g. 100 parameters).  

For simple models coupled SA offers a speed-up in finding good scores near the beginning of the search, but since it is not too difficult for the sequential (*k*=1) algorithm to approximate the global optimum in this low-dimensional space the results from the coupled and sequential algorithms quickly converge.  Thus the adantage of coupling diminishes at lower thresholds for simple models.

For medium models coupled SA still offers substantial speed-up, but is less likely to be super-linear.  Again we see faster speed-up for higher scores (i.e. near the beginning of the search), but the parameter space can still be optimized efficiently by a single chain, so the sequential and parallel results converge about half-way through the search.  Coupled SA still offers expected speed-up, but it diminishes as the score threshold decreases.

For complex models we see that coupled SA offers large speed-up again, as this highly-dimensional parameter space is more difficult for a single chain to explore.  More importantly, we see that with increasing *k* we are able to achieve scores not possible with smaller coupling (or sequential) implementations - the scores do not converge.  Thus for complex models coupled SA provides an efficient approach to optimizing the parameter space that is not feasible with independent (sequential) SA chains.  We see that the minimum score achieved across all sequential trials never goes below 3, while using *k*=8 we can approach 0.  Thus the speed-up for scores below 3 cannot be calculated (i.e. is infinite) since the sequential algorithm cannot achieve these thresholds.


## Conclusions
We find that coupled simulated annealing offers large potential efficiency gains over sequential SA, with super-linear speed-up possible for various combinations of score threshold and model complexity.  Moreover, we find that coupled SA offers the possibility of achieving loss scores not possible with sequential SA.
