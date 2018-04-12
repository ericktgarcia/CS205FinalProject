#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>
#include <omp.h>

int main(int argc, char** argv){
	/* Initialize MPI and get rank and size */
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

	//Get arguments
	String program=argv[0]; //program name to run
	String method=argv[1]; //'SA'=Simulated Annealing; 'GD'=Gradient Descent
	int nCores=atoi(argv[2]);
	String programArgs[]; //get other arguments to pass to the program
	double params[][]; //some type of search parameters

	//Perform search
	if(rank==0){ //master
		initialize(); //initialize search parameters
		bool stop=false;
		while(stop==false){//check termination condition
			if(method='GD' or method='SGD'){
				//split up data for programs to process in parallel
			}
			MPI_Send(params); //send directions
			//do some work too
			#pragma omp parallel for
			for(int i=0; i<nCores; i++){ //if nCores=1 just serial, but program may run multi-thread
				loss=runProgram(program,params,programArgs);
			}
			MPI_Receive(loss); //get loss scores back
			updateSearch(method,loss); //determine next search space/check for convergence
		}
		writeResults();
	}
	else{ //worker
		bool stop=false;
		while(stop==false){ //check termination condition
			MPI_Receive(params)//get directions
			if(stop){break}
			else{ //do work
				#pragma omp parallel for
				for(int i=0; i<nCores; i++){
					loss=runProgram(program,params,programArgs);
				}
				MPI_Send(loss) //send loss score
			}
		}
	}

    /* Clean everything up */
    free(...);
 
    MPI_Finalize();
    return 0;
}

void updateSearch(String method, double loss){
	if(method='SA'){
		//find min loss and set those parameters to 'best'
	}
	else if(method='GD'){
		//calculate overall gradient and take step
	}
	else if(method='SGD'){
		//calculate average gradient and take step
	}
}