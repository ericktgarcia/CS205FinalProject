# CS205FinalProject

## Run the Sampler
First, we must compile the model. The easiest way to do this is to get an interactive job. We request two nodes to test the compiled model.

`srun --pty -p test -n 2 --mem 500M -t 0-06:00 /bin/bash`

From here, load the necessary modules. For Odyssey:

`module load gcc/6.3.0-fasrc01 openmpi/2.1.0-fasrc01 java/1.8.0_45-fasrc01`

For Orchestra:

`module load gcc openmpi java`

Now we can compile the model:

`mpic++ sampler_MPI.cpp -o sampler -lm`

Finally, test the result. The sampler takes two parameters: the config file (here, InputFile.csv) and a task number.

`mpirun -np 2 sampler InputFile.csv 1`

With the sampler compiled, you can exit your interactive session and launch further experiments with sbatch. `mpiScript.sh` contains a minimal example experiment and can be launched with:

'sbatch mpiScript.sh'

## Configuration File
The sampler takes in a configuration file in the form of a .csv to specific the model, output file, optimization method, and other parameters to test.

`InputFile.csv` contains the settings used for our results.
