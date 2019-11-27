#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <stdlib.h>
#include <string>
#include <stack>
#include <algorithm>
// #include <boost/circular_buffer.hpp>
using namespace std;
#define SYSERR 1
class FunctionalUnit
{
    #define REGISTER_FILE_LENGTH 32
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
        {2,5}
    };
    vector<int> register_file;
    map<int,InstructionMeta*> fake_rob;
    vector<int> dispatch_list;
    vector<int> issue_list;
    vector<pair<int,int>> execute_list;

    //Extra stuff
    vector<InstructionMeta*> retired_instructions;

    float getIpc();
    bool Advance_Cycle();
    public:
    Scheduler(int s_arg, int n_arg, int block_size, int L1_size, int L1_assoc, int L2_size, int L2_assoc, string trace_file_arg);
    void printConfig();
    void printResults();
    void start();
    void FakeRetire();
    void Execute();
    void Issue();
    void Dispatch();
    void Fetch();

};


Scheduler::Scheduler(int s_arg, int n_arg, int block_size, int L1_size, int L1_assoc, int L2_size, int L2_assoc, string trace_file_arg)
{
    s=s_arg;
    n=n_arg;
    dispatch_q_size=2*n;
    
    
    register_file.reserve(REGISTER_FILE_LENGTH);
    for(int i=0;i<REGISTER_FILE_LENGTH;i++)
    {
        register_file[i]=-1;
    }
    trace_file=trace_file_arg;


}
bool Scheduler::Advance_Cycle()
{
    bool continue_execution=true;
    if(eof_reached && fake_rob.empty()) continue_execution=false;
    num_cycles++;    
    for(auto it=retired_instructions.begin();it!=retired_instructions.end();++it)
    {
        cout<<(*it)->tag <<" fu{"<<(*it)->operation_type<<"} src{"<<(*it)->src1<<","<<(*it)->src2<<"} dst{"<<(*it)->dst
                            <<"} IF{"<<(*it)->if_beg<<","<<(*it)->if_dur
                            <<"} ID{"<<(*it)->id_beg<<","<<(*it)->id_dur
                            <<"} IS{"<<(*it)->is_beg<<","<<(*it)->is_dur
                            <<"} EX{"<<(*it)->ex_beg<<","<<(*it)->ex_dur
                            <<"} WB{"<<(*it)->wb_beg<<","<<(*it)->wb_dur<<"}\n";
        delete *it;
    }
    return continue_execution;
}

void Scheduler::FakeRetire()
{
    retired_instructions.clear();
    for(auto it=fake_rob.begin();it->second->current_stage==WB;it++)
    {
        retired_instructions.push_back(it->second);
        fake_rob.erase(it);
    }
}

void Scheduler::Execute()
{
    for(int i=0;i<execute_list.size();i++)
    {
        if(execute_list[i].second!=0)
        {
            execute_list[i].second--;
        }
    }
}

void Scheduler::Issue(){
    int free_slots_el= n-execute_list.size();
    stack<int> ready_list;
    for(int i=0;i<issue_list.size();i++)
    {
        if(fake_rob[issue_list[i]]->src1tag==-1 && fake_rob[issue_list[i]]->src2tag==-1)
        {
            if(ready_list.size()<=free_slots_el)
            {
                ready_list.push(i);
            }
            else
            {
                break;
            }            
        }
    }
    while (!ready_list.empty())
    {
        int index=ready_list.top();
        fake_rob[issue_list[index]]->current_stage=EX;
        fake_rob[issue_list[index]]->ex_beg=num_cycles;
        fake_rob[issue_list[index]]->is_dur=num_cycles-fake_rob[issue_list[index]]->is_beg;
        int latency=instruction_latency[fake_rob[issue_list[index]]->operation_type];
        execute_list.push_back(pair<int,int>(issue_list[index],latency));

        issue_list.erase(issue_list.begin()+index);
        ready_list.pop();
        
    }
    

}

void Scheduler::Dispatch(){
    
    int free_slots_sl= s-issue_list.size();
    int recs_to_remove=0;
    for(int i=0;i<free_slots_sl && i<dispatch_list.size();i++)
    {
        if(fake_rob[dispatch_list[i]]->current_stage==ID)
        {
            recs_to_remove++;
            fake_rob[dispatch_list[i]]->current_stage=IS;

            fake_rob[dispatch_list[i]]->src1tag=register_file[fake_rob[dispatch_list[i]]->src1];
            fake_rob[dispatch_list[i]]->src2tag=register_file[fake_rob[dispatch_list[i]]->src2];
            register_file[fake_rob[dispatch_list[i]]->dst]=fake_rob[dispatch_list[i]]->tag;

            fake_rob[dispatch_list[i]]->is_beg=num_cycles;
            fake_rob[dispatch_list[i]]->id_dur=num_cycles-fake_rob[dispatch_list[i]]->id_beg;
            issue_list.push_back(dispatch_list[i]);
        }
    }
    while (recs_to_remove!=0)
    {
        dispatch_list.erase(dispatch_list.begin());
        recs_to_remove--;
    }   
    

    for(int i=0;i<dispatch_list.size();++i)
    {        
        if(fake_rob[dispatch_list[i]]->current_stage==IF)
        {
            fake_rob[dispatch_list[i]]->current_stage=ID;
            fake_rob[dispatch_list[i]]->id_beg=num_cycles;
            fake_rob[dispatch_list[i]]->if_dur=num_cycles-fake_rob[dispatch_list[i]]->if_beg;
        }
        
    }
}

void Scheduler::Fetch(){
    int records_to_fetch=min(n,(int)(dispatch_q_size-dispatch_list.size()));
    string instruction;
    
    for(int i=0;i<records_to_fetch;i++)
    {
        getline(infile,instruction); 
        if(instruction!="")
        {
            //2b6424 2 14 29 -1 400341a0
            vector<string> split;
            string seg="";
            for(int i=0;instruction[i]!='\n';i++)
            {
                if(instruction[i]==' ')
                {
                    split.push_back(seg);
                    seg="";
                }
                else
                {
                    seg.push_back(instruction[i]);
                }            
            }
            split.push_back(seg);

            InstructionMeta* current=new InstructionMeta();
            current->tag                    = line_num++;
            current->pc                     = stoi(split[0]);
            current->operation_type         = stoi(split[1]);
            current->src1                   = stoi(split[2]);
            current->src2                   = stoi(split[3]);
            current->dst                    = stoi(split[4]);
            current->mem_address            = stoi(split[5]);        

            current->current_stage          = IF;
            current->if_beg                 = num_cycles;
            fake_rob.insert(pair<int,InstructionMeta*>(current->tag,current));

            dispatch_list.push_back(current->tag);

            if(infile.eof())
            {
                eof_reached=true;
                break;
            }
        }
        
    }
}

void Scheduler::start()
{   
    infile.open(trace_file);
    if(!infile.is_open()){
        cerr<<"Unable to open "<<trace_file<<"!!!";
        return;
    }
    
    string instruction;
    do
    {
        FakeRetire();
        Execute();
        Issue();
        Dispatch();
        Fetch();
    }while(Advance_Cycle());
}




float Scheduler::getIpc()
{
    return float(num_instructions)/float(num_cycles);
}
void Scheduler::printConfig()
{
    cout<<"CONFIGURATION\n";
    cout<<"superscalar bandwidth (N) = "<<n <<"\n";
    cout<<"dispatch queue size (2*N) = "<<2*n <<"\n";
    cout<<"schedule queue size (S)   = "<<s<<"\n";
}

void Scheduler::printResults()
{
    cout<<"RESULTS\n";
    cout<<"number of instructions = "<<num_instructions<<"\n";
    cout<<"number of cycles       = "<<num_cycles<<"\n";
    cout<<"IPC                    = "<<getIpc()<<"\n";
}

