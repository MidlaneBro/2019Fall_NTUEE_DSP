#include <stdio.h>
#include <stdlib.h>
#include <utility>
#include <vector>
#include "File.h"
#include "Ngram.h"
#include "Vocab.h"

using namespace std;

int zhuyin_to_index(int second_bit_of_zhuyin){
    if(second_bit_of_zhuyin>=116&&second_bit_of_zhuyin<=126){
        return second_bit_of_zhuyin-116;
    }
    else if(second_bit_of_zhuyin>=-95&&second_bit_of_zhuyin<=-70){
        return second_bit_of_zhuyin+106;
    }
    else{
        return -1;
    }
}

int main(int argc, char *argv[]){
    if(argc!=5){
        cout<<"Usage: ./mydisambig <segmented file to be decoded> <ZhuYin-Big5 mapping> <language model> <output file>"<<endl;
	    return 0;
    }
    File text_file(argv[1],"r");
    if(!text_file){
        cout<<"Error: Cannot open the segmented file to be decoded."<<endl;
        return 0;
    }
    File map_file(argv[2],"r");
    if(!map_file){
        cout<<"Error: Cannot open ZhuYin-Big5 mapping file."<<endl;
        return 0;
    }
    File lm_file(argv[3],"r");
    if(!lm_file){
        cout<<"Error: Cannot open language model."<<endl;
        return 0;
    }

    char* buf;
    vector<vector<pair<char,char>> > index;
    index.resize(37);
    while(buf = map_file.getline()){
        if(int(buf[0])==-93){
            vector<char> bit;
            vector<pair<char,char>> chinese;
            for(int i=2;i<strlen(buf)-1;i++){
                if(int(buf[i])!=32){
                    bit.push_back(buf[i]);
                }
            }
            for(int i=0;i<bit.size();i+=2){
                chinese.push_back(pair<char,char>(bit[i],bit[i+1]));
            }
            index[zhuyin_to_index(buf[1])]=chinese;
        }
    }
    map_file.close();

    int ngram_order = 2;
    Vocab voc;
    Ngram lm(voc,ngram_order);
    lm.read(lm_file);
    lm_file.close();

    File output_file(argv[4],"w");
    while(buf = text_file.getline()){
        vector<char> bit;
        int count=0;
        for(int i=0;i<strlen(buf);i++){
            if(int(buf[i])!=32&&int(buf[i])!=10){
                 bit.push_back(buf[i]);
                 count++;
            }
            if(count==2){
                bit.push_back(' ');
                count=0;
            }
        }

        //viterbi start
        vector<vector<float> > opt_prob;
        vector<vector<int> > opt_sol;
        opt_prob.resize(bit.size()/3+1);//t store the maximun probability until t
        opt_sol.resize(bit.size()/3+1);//t store the solution of t-1
        for(int i=0;i<bit.size();i+=3){
            if(int(bit[i])!=-93){//not zhuyin, don't need to modify
                opt_prob[i/3].resize(1);
                opt_sol[i/3].resize(1);
                if(i==0){//what in front of it is <s>
                    opt_prob[i/3][0]=0;
                    opt_sol[i/3][0]=0;
                }
                else if(int(bit[i-3])!=-93){//what in front of it is a chinese
                    opt_prob[i/3][0]=opt_prob[i/3-1][0];
                    opt_sol[i/3][0]=0;
                }
                else{//what in front of it is a zhuyin
                    char this_word[3];
                    this_word[0] = bit[i];
                    this_word[1] = bit[i+1];
                    this_word[2] = '\0';
                    VocabIndex wid = voc.getIndex(this_word);
                    if(wid == Vocab_None){
                        wid = voc.getIndex(Vocab_Unknown);
                    }
                    char last_word[3];
                    VocabIndex wid2;
                    int num = zhuyin_to_index(int(bit[i-2]));
                    for(int q=0;q<index[num].size();q++){
                        last_word[0] = index[num][q].first;
                        last_word[1] = index[num][q].second;
                        last_word[2] = '\0';
                        wid2 = voc.getIndex(last_word);
                        if(wid2 == Vocab_None){
                            wid2 = voc.getIndex(Vocab_Unknown);
                        }
                        VocabIndex context[] = {wid2,Vocab_None};
                        float prob = lm.wordProb(wid,context)+opt_prob[i/3-1][q];
                        if(q==0){
                            opt_prob[i/3][0] = prob;
                            opt_sol[i/3][0] = q;
                        }
                        if(prob > opt_prob[i/3][0]){
                            opt_prob[i/3][0] = prob;
                            opt_sol[i/3][0] = q;
                        }
                    }
                }
            }
            else{//zhuyin, need modify
                int num = zhuyin_to_index(int(bit[i+1]));//which slot of index
                int choice = index[num].size();//how many elements in that slot
                opt_prob[i/3].resize(choice);
                opt_sol[i/3].resize(choice);
                if(i==0){//what in front of it is <s>
                    VocabIndex context[] = {voc.getIndex(Vocab_SentStart),Vocab_None};
                    char this_word[3];
                    VocabIndex wid;
                    for(int j=0;j<choice;j++){
                        opt_sol[i/3][j]=0;
                        this_word[0] = index[num][j].first;
                        this_word[1] = index[num][j].second;
                        this_word[2] = '\0';
                        wid = voc.getIndex(this_word);
                        if(wid == Vocab_None){
                            wid = voc.getIndex(Vocab_Unknown);
                        }
                        opt_prob[i/3][j] = lm.wordProb(wid,context);
                    }
                }
                else if(int(bit[i-3])!=-93){//what in front of it is a chinese
                    char last_word[3];
                    last_word[0] = bit[i-3];
                    last_word[1] = bit[i-2];
                    last_word[2] = '\0';
                    VocabIndex wid2 = voc.getIndex(last_word);
                    if(wid2 == Vocab_None){
                        wid2 = voc.getIndex(Vocab_Unknown);
                    }
                    VocabIndex context[] = {wid2,Vocab_None};
                    char this_word[3];
                    VocabIndex wid;
                    for(int j=0;j<choice;j++){
                        opt_sol[i/3][j]=0;
                        this_word[0] = index[num][j].first;
                        this_word[1] = index[num][j].second;
                        this_word[2] = '\0';
                        wid = voc.getIndex(this_word);
                        if(wid == Vocab_None){
                            wid = voc.getIndex(Vocab_Unknown);
                        }
                        opt_prob[i/3][j] = lm.wordProb(wid,context)+opt_prob[i/3-1][0];
                    }
                }
                else{//what in front of it is a zhuyin
                    char this_word[3];
                    VocabIndex wid;
                    char last_word[3];
                    VocabIndex wid2;
                    int last_num = zhuyin_to_index(int(bit[i-2]));
                    int last_choice = index[last_num].size();
                    for(int j=0;j<choice;j++){
                        this_word[0] = index[num][j].first;
                        this_word[1] = index[num][j].second;
                        this_word[2] = '\0';
                        wid = voc.getIndex(this_word);
                        if(wid == Vocab_None){
                            wid = voc.getIndex(Vocab_Unknown);
                        }
                        for(int q=0;q<last_choice;q++){
                            last_word[0] = index[last_num][q].first;
                            last_word[1] = index[last_num][q].second;
                            last_word[2] = '\0';
                            wid2 = voc.getIndex(last_word);
                            if(wid2 == Vocab_None){
                                wid2 = voc.getIndex(Vocab_Unknown);
                            }
                            VocabIndex context[] = {wid2,Vocab_None};
                            float prob = lm.wordProb(wid,context)+opt_prob[i/3-1][q];
                            if(q==0){
                                 opt_prob[i/3][j] = prob;
                                 opt_sol[i/3][j] = q;
                            }
                            if(prob > opt_prob[i/3][j]){
                                opt_prob[i/3][j] = prob;
                                opt_sol[i/3][j] = q;
                            }
                        }
                    }
                }
            }
        }
        //still need to handle </s>
        opt_prob[bit.size()/3].resize(1);
        opt_sol[bit.size()/3].resize(1);
        if(int(bit[bit.size()-3])!=-93){//the last word is chinese
            opt_prob[bit.size()/3][0] = opt_prob[bit.size()/3-1][0];
            opt_sol[bit.size()/3][0] = 0;
        }
        else{//the last word is zhuyin
             VocabIndex wid = voc.getIndex(Vocab_SentEnd);
            char last_word[3];
            VocabIndex wid2;
            int num = zhuyin_to_index(int(bit[bit.size()-2]));
            for(int q=0;q<index[num].size();q++){
                last_word[0] = index[num][q].first;
                last_word[1] = index[num][q].second;
                last_word[2] = '\0';
                wid2 = voc.getIndex(last_word);
                if(wid2 == Vocab_None){
                    wid2 = voc.getIndex(Vocab_Unknown);
                }
                VocabIndex context[] = {wid2,Vocab_None};
                float prob = lm.wordProb(wid,context)+opt_prob[bit.size()/3-1][q];
                if(q==0){
                    opt_prob[bit.size()/3][0] = prob;
                    opt_sol[bit.size()/3][0] = q;
                }
                if(prob > opt_prob[bit.size()/3][0]){
                    opt_prob[bit.size()/3][0] = prob;
                    opt_sol[bit.size()/3][0] = q;
                }
            }
        }
        //viterbi end

        vector<int> final_solution;
        final_solution.resize(opt_sol.size()-1);
        int x=0;
        for(int i=final_solution.size()-1;i>=0;i--){
            final_solution[i] = opt_sol[i+1][x];
            x = opt_sol[i+1][x];
        }
        for(int i=0;i<bit.size();i+=3){
            if(int(bit[i])==-93){
                int num = zhuyin_to_index(int(bit[i+1]));
                bit[i] = index[num][final_solution[i/3]].first;
                bit[i+1] =  index[num][final_solution[i/3]].second;
            }
        }
        fprintf(output_file,"%s","<s> ");
        for(int i=0;i<bit.size();i++){
            fprintf(output_file,"%c",bit[i]);
        }
        fprintf(output_file,"%s\n","</s>");
    }

    text_file.close();
    output_file.close();
    return 0;
}
