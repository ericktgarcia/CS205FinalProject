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

Finally, test the result:

`mpirun -np 2 sampler InputFile.csv 1234`

