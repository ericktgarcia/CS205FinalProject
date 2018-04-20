#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string>
//#include <mpi.h>
//#include <omp.h>
#include <sstream>
#include <fstream>
#include <vector>
#include <random>

using namespace std;

string modelCmd;
string method;
string outFilepath;
int numParams;
int numNodes;
int maxIterations;

//compare current loss score across nodes
int minIndex;
double localMinScore;

double origTemp, curTemp; //temperature
double tempDelta=0.995; //cooling schedule
bool shrinkBounds;

double minLoss=1.7976931348623158e+308; //set to big number
string* paramNames;
double** paramBounds;
bool** hardBounds;
double** nodeParams; //[nodeIndex][parameter]
double* curParams;
double* bestParams;
mt19937 generator; //Mersenne Twister RNG
uniform_real_distribution<double> realDist(0,1);

void initializeParams(){
	for(int p=0; p<numParams; p++){
		double lb=paramBounds[p][0];
		double ub=paramBounds[p][1];
		bestParams[p]=(lb+ub)/2.0;
	}
}

void setCurToBest(int nodeIndex){
	for(int p=0; p<numParams; p++){
		bestParams[p]=nodeParams[nodeIndex][p];
	}
}

vector<string> split(string line){
	vector<string> tokens;
	while(line.find(",")!=-1){
		int index=line.find(",");
		tokens.push_back(line.substr(0,index));
		line=line.substr(index+1);
	}
	tokens.push_back(line);
	return(tokens);
}

double runModel(){
	double score=-1;
	FILE *output;
	ostringstream oss;
	oss<<modelCmd;
	for(int p=0; p<numParams; p++){
		oss<<" "<<curParams[p];
	}
	string command=oss.str();
	cout<<"Running model: "<<command<<" ";
	output = popen(command.c_str(), "r");
	if (!output){
		fprintf (stderr, "Error with popen.\n");
	}
	else{
		ostringstream stream;
		char line[1024];
		while(fgets(line,1024,output)){
			stream<<line<<'\n';
		}
		if (pclose (output) != 0){
			fprintf (stderr, "Error with pclose.\n");
		}
		string strScore=stream.str();
		score=atof(strScore.c_str());
		cout<<"Score: "<<score<<"\n";
	}
	return(score);
}

//sample new parameter sets
void sampleParams(){
	for(int p=0; p<numParams; p++){
		double width=paramBounds[p][1]-paramBounds[p][0]; //orig width
		if(shrinkBounds){
			double ratio=curTemp/origTemp;
			width*=ratio;
		}
		double lb=bestParams[p]-width/2.0;
		double ub=bestParams[p]+width/2.0;
		if(hardBounds[p][0]==true){
			lb=max(lb,paramBounds[p][0]);
		}
		if(hardBounds[p][1]==true){
			ub=min(ub,paramBounds[p][1]);
		}
		for(int n=0; n<numNodes; n++){
			double rand=realDist(generator);
			nodeParams[n][p]=lb+rand*(ub-lb);
		}
	}
}

void updateSearch(string method, double* lossScores){
	if(method.compare("SA")==0){
		//get current min loss score across nodes
		minIndex=0;
		localMinScore=lossScores[0];
		for(int n=1; n<numNodes; n++){
			if(lossScores[n]<localMinScore){
				minIndex=n;
				localMinScore=lossScores[n];
			}
		}
		//compare to global min loss
		if(localMinScore<minLoss){ //always move
			minLoss=localMinScore;
			setCurToBest(minIndex);
		}
		else{ //sometimes move
			double scoreDiff=localMinScore-minLoss;
			double prob=exp(-scoreDiff/curTemp);
			double rand=realDist(generator);
			if(rand<prob){ //accept move
				minLoss=localMinScore;
				setCurToBest(minIndex);
			}
		}
	}
	else if(method.compare("GD")==0){
		//calculate overall gradient and take step
	}
	else if(method.compare("SGD")==0){
		//calculate average gradient and take step
	}
}


int main(int argc, char** argv){

	cout<<"***CoSampler***\n";

	/* Initialize MPI and get rank and size */
	int rank=0, size=1;
	numNodes=size;

	//MPI_Init(&argc, &argv);
	//MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//MPI_Comm_size(MPI_COMM_WORLD, &size);

	if(rank==0){ //master
		//generator(time(0)); //seed RNG with current time
		generator.seed(123); //seed RNG

		//Get input file
		int numArgs=argc;
		numArgs=1; //debug
		if(numArgs!=1){
			fprintf (stderr, "Missing input file.\n");
			return(-1);
		}
		else{
			string filepath="C:/Users/zward/Dropbox/PHD/Spring 2017-2018/CS 205/Project/InputFile.csv";
			//string filepath=&argv[0]; //input filepath
			cout<<"Reading inputs: "<<filepath<<"\n";
			ifstream inputFile;
			inputFile.open(filepath);
			if(inputFile.is_open()){
				string line;
				getline(inputFile,line); //Model command
				modelCmd=split(line)[1];
				getline(inputFile,line); //Out filepath
				outFilepath=split(line)[1];
				getline(inputFile,line); //Method
				method=split(line)[1];
				getline(inputFile,line); //Max iterations
				maxIterations=atoi(split(line)[1].c_str());
				getline(inputFile,line); //Start temperature
				origTemp=atof(split(line)[1].c_str());
				curTemp=origTemp;
				getline(inputFile,line); //Temperature delta
				tempDelta=atof(split(line)[1].c_str());
				getline(inputFile,line); //Shrink bounds
				shrinkBounds=true;
				if(split(line)[1].compare("F")==0){shrinkBounds=false;}
				getline(inputFile,line); //Number of parameters
				numParams=atoi(split(line)[1].c_str());
				//Get parameter bounds
				getline(inputFile,line); //headers
				paramNames=new string[numParams];
				paramBounds=new double*[numParams];
				hardBounds=new bool*[numParams];
				for(int p=0; p<numParams; p++){
					paramBounds[p]=new double[2];
					hardBounds[p]=new bool[2];
					getline(inputFile,line);
					vector<string> tokens=split(line);
					paramNames[p]=tokens[0];
					paramBounds[p][0]=atof(tokens[1].c_str());
					paramBounds[p][1]=atof(tokens[2].c_str());
					hardBounds[p][0]=false;
					if(tokens[3].compare("T")==0){hardBounds[p][0]=true;}
					hardBounds[p][1]=false;
					if(tokens[4].compare("T")==0){hardBounds[p][1]=true;}
				}
				bestParams=new double[numParams];
				curParams=new double[numParams];
				nodeParams=new double*[numNodes];
				for(int n=0; n<numNodes; n++){
					nodeParams[n]=new double[numParams];
				}

				inputFile.close();
			}
			else{
				cout<<"Unable to open inputs file";
				return(-1);
			}
		}
	}

	//Perform search
	if(rank==0){ //master
		initializeParams(); //initialize search parameters
		ofstream trace;
		trace.open(outFilepath);
		//trace headers
		trace<<"Iteration,Temp,Score,Node";
		for(int p=0; p<numParams; p++){
			trace<<","<<paramNames[p];
		}
		trace<<"\n";

		bool stop=false;
		int iteration=0;
		while(stop==false){//check termination condition
			if(method.compare("GD")==0 || method.compare("SGD")==0){
				//split up data for programs to process in parallel
			}
			sampleParams();

			//MPI_Send(nodeParams[node]); //send directions
			//do some work too
			for(int p=0; p<numParams; p++){curParams[p]=nodeParams[0][p];}
			double* loss=new double[numNodes];
			loss[0]=runModel();

			//MPI_Receive(loss); //get loss scores back
			updateSearch(method,loss); //determine next search space

			//update trace
			trace<<iteration<<","<<curTemp<<","<<localMinScore<<","<<minIndex;
			for(int p=0; p<numParams; p++){
				trace<<","<<curParams[p];
			}
			trace<<"\n";

			curTemp*=tempDelta; //cool temperature
			iteration++;
			if(iteration>maxIterations){stop=true;}
		}
		trace.close();
	}
	else{ //worker
		bool stop=false;
		while(stop==false){ //check termination condition
			//MPI_Receive(curParams);//get parameters
			//do work
			double loss=runModel();
			//MPI_Send(loss); //send loss score
			//MPI_Receive(stop)
		}
	}

	//MPI_Finalize();

	/* Clean everything up */
	if(rank==0){ //master
		delete[] paramNames;
		delete[] curParams;
		delete[] bestParams;
		for(int p=0; p<numParams; p++){
			delete[] paramBounds[p];
			delete[] hardBounds[p];
		}
		delete[] paramBounds;
		delete[] hardBounds;
		for(int n=0; n<numNodes; n++){
			delete[] nodeParams[n];
		}
		delete[] nodeParams;
	}

	return 0;
}
