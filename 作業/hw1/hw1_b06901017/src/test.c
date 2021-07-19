#include <stdio.h>
#include <stdlib.h>
#include "hmm.h"
#include <math.h>

#define MAX_MODEL_NUM 10

int main(int argc, char *argv[])
{
  if(argc != 4){
		printf("Usage: ./test <models_list_path> <seq_path> <output_result_path>\n");
		return 0;
	}
	HMM hmms[MAX_MODEL_NUM];
	int model_num;
	model_num=load_models(argv[1],hmms,MAX_MODEL_NUM);
	FILE *test_seq = open_or_die(argv[2],"r");
	FILE *output = open_or_die(argv[3],"w");
	char seq[MAX_SEQ];
	while(fscanf(test_seq,"%s",seq)>0){
		int T=strlen(seq);
		double max_p=0;
		int best_model;
		for(int number=0;number<model_num;number++){
			int N=hmms[number].state_num;
			double delta[MAX_STATE][MAX_SEQ]={{0}};
			for(int i=0;i<N;i++){
				delta[i][0]=(hmms[number].initial[i])*(hmms[number].observation[seq[0]-'A'][i]);
			}
			for(int t=1;t<T;t++){
				for(int j=0;j<N;j++){
					double max=0;
					for(int i=0;i<N;i++){
						if(delta[i][t-1]*hmms[number].transition[i][j]>max){
							max=delta[i][t-1]*hmms[number].transition[i][j];
						}
					}
					delta[j][t]=max*hmms[number].observation[seq[t]-'A'][j];
				}
			}
			double p=0;
			for(int i=0;i<N;i++){
				if(delta[i][T-1]>p){
					p=delta[i][T-1];
				}
			}
			if(p>max_p){
				max_p=p;
				best_model=number;
			}
		}
		fprintf(output,"%s %.6e\n",hmms[best_model].model_name,max_p);
	}
	fclose(test_seq);
	fclose(output);
	return 0;
}
