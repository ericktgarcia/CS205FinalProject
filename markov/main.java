public class main {
	
	public static void main(String[] args) {
		try{
			
			//args=new String[]{"0.3","0.2","0.8","0.2","0.3","0.4","0.25","0.8","0.3"};
						
			//Get parameters
			int targetSeed=Integer.parseInt(args[0]);
			int numParams=args.length-1;
			double curParams[]=new double[numParams];
			for(int i=0; i<numParams; i++){
				curParams[i]=Double.parseDouble(args[i+1]);
			}
			
			int numStates=(int) Math.sqrt(numParams);
			
			//Generate targets
			int numCycles=50;
			double prev[]=new double[numStates];
			MersenneTwisterFast targetGenerator=new MersenneTwisterFast(targetSeed); //seed generator
			double total=0;
			for(int i=0; i<numStates; i++){
				prev[i]=targetGenerator.nextDouble();
				total+=prev[i];
			}
			//Re-normalize
			for(int i=0; i<numStates; i++){
				prev[i]/=total;
			}
			
			//Construct transition matrix
			double probs[][]=new double[numStates][numStates];
			int index=0;
			for(int i=0; i<numStates; i++){ //row
				double rowTotal=0;
				for(int j=0; j<numStates; j++){ //column
					probs[i][j]=curParams[index];
					rowTotal+=probs[i][j];
					index++;
				}
				//Renormalize row transition probabilities
				for(int j=0; j<numStates; j++){
					probs[i][j]/=rowTotal;
				}
				//Make cumulative
				for(int j=1; j<numStates; j++){
					probs[i][j]=probs[i][j-1]+probs[i][j];
				}
			}
			
			
			//Run microsimulation model on multiple threads
			int numPeople=100000;
			int numThreads=2;
			final MersenneTwisterFast generators[]=new MersenneTwisterFast[numThreads];
			for(int n=0; n<numThreads; n++){
				generators[n]=new MersenneTwisterFast(n);
			}
			
			int counts[][]=new int[numThreads][numStates];
			
			final int blockSize= numPeople/numThreads;
			Thread[] threads = new Thread[numThreads];
			for(int n=0; n<numThreads; n++){
				final int finalN = n;
				threads[n] = new Thread() {
					public void run() {
						final int beginIndex = finalN * blockSize;
						final int endIndex = (finalN==numThreads-1) ? numPeople :(finalN+1)*blockSize;
						for(int p=beginIndex; p<endIndex; p++){
							int curState=0; //start in state 0
							double rand=0;
							for(int i=0; i<numCycles; i++){ //sim next state
								rand=generators[finalN].nextDouble();
								int k=0;
								while(rand>probs[curState][k]){k++;}
								curState=k; //update state
							} //End cycle loop
							counts[finalN][curState]++; //record end state
						} //End person loop
					}
				};
				threads[n].start();
			}
			//Wait for threads to finish
			for(int n=0; n<numThreads; n++){
				try{
					threads[n].join();
				} catch (InterruptedException e){
					System.exit(-1);
				}
			}	
			
			//Score model
			double score=0;
			for(int i=0; i<numStates; i++){
				double curPrev=0;
				for(int n=0; n<numThreads; n++){
					curPrev+=counts[n][i];
				}
				curPrev/=(numPeople*1.0);
				score+=(curPrev-prev[i])*(curPrev-prev[i]); //distance squared
			}
			score*=1000; //re-scale
			System.out.println(score);
			
		} catch (Exception e){
			System.out.println(e.getMessage());
		}
	}
}