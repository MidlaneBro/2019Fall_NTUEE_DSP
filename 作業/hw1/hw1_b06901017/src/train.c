#include <stdio.h>
#include <stdlib.h>
#include "hmm.h"
#include <math.h>

int main(int argc, char *argv[])
{
	if(argc != 5){
		printf("Usage: ./train <iter> <model_init_path> <seq_path> <output_model_path>\n");
		return 0;
	}
	int iter = atoi(argv[1]);
	HMM hmm_initial;
	loadHMM( &hmm_initial, argv[2] );
	int N=hmm_initial.state_num;
	for(int t=0;t<iter;t++){
		FILE *train_seq = open_or_die(argv[3],"r");
		char seq[MAX_SEQ];
		double re_estimate_pi_numerator[MAX_STATE]={0};
		double sample_num=0;//re_estimate_pi_denominator
		double re_estimate_a_numerator[MAX_STATE][MAX_STATE]={{0}};
		double re_estimate_a_denominator[MAX_STATE]={0};
		double re_estimate_b_numerator[MAX_OBSERV][MAX_STATE]={{0}};
		double re_estimate_b_denominator[MAX_STATE]={0};
		while(fscanf(train_seq,"%s",seq)>0){
			sample_num++;
			int T=strlen(seq);
			double alpha[MAX_STATE][MAX_SEQ]={{0}};
			double beta[MAX_STATE][MAX_SEQ]={{0}};
			double gamma[MAX_STATE][MAX_SEQ]={{0}};
			double epsilon[MAX_SEQ-1][MAX_STATE][MAX_STATE]={{{0}}};
			//initialize alpha and beta
			for(int i=0;i<N;i++){
				alpha[i][0]=(hmm_initial.initial[i])*(hmm_initial.observation[seq[0]-'A'][i]);
				beta[i][T-1]=1;
			}
			//calculate alpha
			for(int i=1;i<T;i++){
				for(int j=0;j<N;j++){
					double sum=0;
					for(int k=0;k<N;k++){
						sum+=(alpha[k][i-1])*(hmm_initial.transition[k][j]);
					}
					alpha[j][i]=sum*(hmm_initial.observation[seq[i]-'A'][j]);
				}
			}
			//calculate beta
			for(int i=T-2;i>=0;i--){
				for(int j=0;j<N;j++){
					for(int k=0;k<N;k++){
						beta[j][i]+=(hmm_initial.transition[j][k])*(hmm_initial.observation[seq[i+1]-'A'][k])*(beta[k][i+1]);
					}
				}
			}
			//calculate gamma
			for(int i=0;i<T;i++){
				double denominator=0;
				for(int j=0;j<N;j++){
					gamma[j][i]=alpha[j][i]*beta[j][i];
					denominator+=gamma[j][i];
				}
				for(int j=0;j<N;j++){
					gamma[j][i]/=denominator;
				}
			}
			//calculate epsilon
			for(int i=0;i<T-1;i++){
				double denominator=0;
				for(int j=0;j<N;j++){
					for(int k=0;k<N;k++){
						epsilon[i][j][k]=alpha[j][i]*(hmm_initial.transition[j][k])*(hmm_initial.observation[seq[i+1]-'A'][k])*beta[k][i+1];
						denominator+=epsilon[i][j][k];
					}
				}
				for(int j=0;j<N;j++){
					for(int k=0;k<N;k++){
						epsilon[i][j][k]/=denominator;
					}
				}
			}
			//re_estimation
			for(int i=0;i<N;i++){
				re_estimate_pi_numerator[i]+=gamma[i][0];
				for(int j=0;j<N;j++){
					for(int k=0;k<T-1;k++){
						re_estimate_a_numerator[i][j]+=epsilon[k][i][j];
					}
				}
				for(int j=0;j<T;j++){
					if(j<T-1){
						re_estimate_a_denominator[i]+=gamma[i][j];
					}
					re_estimate_b_numerator[seq[j]-'A'][i]+=gamma[i][j];
					re_estimate_b_denominator[i]+=gamma[i][j];
				}
			}
		}
		fclose(train_seq);
		//update pi and a
		for(int i=0;i<N;i++){
			hmm_initial.initial[i]=re_estimate_pi_numerator[i]/sample_num;
			for(int j=0;j<N;j++){
				hmm_initial.transition[i][j]=re_estimate_a_numerator[i][j]/re_estimate_a_denominator[i];
			}
		}
		//update b
		for(int i=0;i<hmm_initial.observ_num;i++){
			for(int j=0;j<N;j++){
				hmm_initial.observation[i][j]=re_estimate_b_numerator[i][j]/re_estimate_b_denominator[j];
			}
		}
	}
	FILE *output=open_or_die(argv[4],"w");
	dumpHMM(output,&hmm_initial);
	fclose(output);
	return 0;
}
