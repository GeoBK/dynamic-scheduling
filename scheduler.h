#include <fstream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <string>
#include <stack>
#include <iostream>
#include <algorithm>
#include "cache.h"
using namespace std;
class FunctionalUnit
{
    
    public:
    int remaining_cycles;
    int tag;
    FunctionalUnit(int t,int c):remaining_cycles(c),tag(t){}
};
enum iStages {IF,ID,IS,EX,WB};
class InstructionMeta
{
    public:
    int tag;
    int pc;
    iStages current_stage;
    int operation_type;
    int src1,src2,dst;
    int src1tag=-1,src2tag=-1;
    int if_beg=0,if_dur=0;
    int id_beg=0,id_dur=0;
    int is_beg=0,is_dur=0;
    int ex_beg=0,ex_dur=0;
    int wb_beg=0,wb_dur=0;
    unsigned int mem_address;

};
class Scheduler
{
    int n;  //Superscalar bandwidth
    int s;  //Schedule queue size
    string trace_file;
    int line_num=0;
    int num_cycles=0;         //Number of cycles executed
    int num_instructions=0;   //Number of instructions processed

    bool eof_reached=false;

    int dispatch_q_size;
    
    ifstream infile;
    map<int,int> instruction_latency={
        {0,1},
        {1,2},
        {2,5},
        {3,5},  //L1 hit
        {8,10}, //L2 hit
        {5,20}, //no L2 + L1 miss
        {10,20}  ////L2 miss
    };

    

    int register_file[128];
    map<int,InstructionMeta*> fake_rob;
    vector<int> dispatch_list;
    vector<int> issue_list;
    vector<pair<int,int>> execute_list;

    //Extra stuff
    vector<InstructionMeta*> retired_instructions;

    float getIpc();
    bool Advance_Cycle();
    public:
    Cache *L1_cache=NULL;
    Cache *L2_cache=NULL;
    Scheduler(int s_arg, int n_arg, int block_size, int L1_size, int L1_assoc, int L2_size, int L2_assoc, string trace_file_arg);
    ~Scheduler();
    void printConfig();
    void printResults();
    void start();
    void FakeRetire();
    void Execute();
    void Issue();
    void Dispatch();
    void Fetch();

};