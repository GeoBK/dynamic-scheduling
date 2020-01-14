#include "scheduler.h"
#define SYSERR 1
#define REGISTER_FILE_LENGTH 128

Scheduler::Scheduler(int s_arg, int n_arg, int block_size, int L1_size, int L1_assoc, int L2_size, int L2_assoc, string trace_file_arg)
{
    s=s_arg;
    n=n_arg;
    dispatch_q_size=2*n;    
    
    for(int i=0;i<REGISTER_FILE_LENGTH;i++)
    {
        register_file[i]=-1;
    }
    trace_file=trace_file_arg;
    if(L1_size!=0)
    {
        L1_cache = new Cache(block_size,L1_size,L1_assoc,1,1,1);
        if(L2_size!=0)
        {
            L2_cache = new Cache(block_size,L2_size,L2_assoc,1,1,2);
            L1_cache->AttachNextLevelCache(L2_cache);
        }
    }
}

Scheduler::~Scheduler()
{
    if(L1_cache!=NULL)
    {
        delete L1_cache;
    }
    if(L2_cache!=NULL)
    {
        delete L2_cache;
    }
}

bool Scheduler::Advance_Cycle()
{
    //cout<<"AdvanceCycle\n";
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
    //cout<<"FakeRetire\n";
    retired_instructions.clear();
    stack<map<int,InstructionMeta*>::iterator> retire_list; 
    for(auto it=fake_rob.begin();fake_rob.size()!=0 && it->second->current_stage==WB;it++)
    {
        retired_instructions.push_back(it->second);
        it->second->wb_dur=1;
        retire_list.push(it);
    }
    while(!retire_list.empty())
    {
        fake_rob.erase(retire_list.top());
        retire_list.pop();
    }
}

void Scheduler::Execute()
{
    stack<int> complete_list;
    for(unsigned int i=0;i<execute_list.size();i++)
    {      
        execute_list[i].second--;  
        if(execute_list[i].second==0)
        {
            complete_list.push(i);
        }             
    }
    while (!complete_list.empty())
    {
        int index = complete_list.top();
        fake_rob[execute_list[index].first]->current_stage=WB;
        fake_rob[execute_list[index].first]->wb_beg=num_cycles;
        fake_rob[execute_list[index].first]->ex_dur=num_cycles-fake_rob[execute_list[index].first]->ex_beg;
        int tag = fake_rob[execute_list[index].first]->tag;
        if(register_file[fake_rob[execute_list[index].first]->dst]==tag)
        {
            register_file[fake_rob[execute_list[index].first]->dst]=-1;
        }        
        for(auto it=fake_rob.begin();it!=fake_rob.end();it++)
        {
            if(it->second->src1tag==tag)it->second->src1tag=-1;
            if(it->second->src2tag==tag)it->second->src2tag=-1;
        }
        execute_list.erase(execute_list.begin()+index);
        complete_list.pop();
    }
    
}

void Scheduler::Issue(){
    unsigned int free_slots_el= n;
    stack<int> ready_list;
    for(unsigned int i=0;i<issue_list.size();i++)
    {
        if((fake_rob[issue_list[i]]->src1tag==-1 || fake_rob[issue_list[i]]->src1==-1) && (fake_rob[issue_list[i]]->src2tag==-1 || fake_rob[issue_list[i]]->src2tag==-1))
        {
            if(ready_list.size()<free_slots_el)
            {
                ready_list.push(i);
            }
            else
            {
                break;
            }            
        }
    }
    stack<int> ready_list_ordered;
    while (!ready_list.empty())
    {
        int index=ready_list.top();
        ready_list_ordered.push(issue_list[index]);        
        issue_list.erase(issue_list.begin()+index);
        ready_list.pop();
        
    }
    while (!ready_list_ordered.empty())
    {
        int tag=ready_list_ordered.top();
        ready_list_ordered.pop();
        fake_rob[tag]->current_stage=EX;
        fake_rob[tag]->ex_beg=num_cycles;
        fake_rob[tag]->is_dur=num_cycles-fake_rob[tag]->is_beg;
        int latency;
        if(L1_cache!=NULL && fake_rob[tag]->operation_type==2)
        {
            latency=instruction_latency[L1_cache->ReadFromAddress(fake_rob[tag]->mem_address)];
        }
        else
        {
            latency=instruction_latency[fake_rob[tag]->operation_type];
        }       
        
        execute_list.push_back(pair<int,int>(tag,latency));

    }
    
    

}

void Scheduler::Dispatch(){
    
    unsigned int free_slots_sl= s-issue_list.size();
    int recs_to_remove=0;
    for(unsigned int i=0;i<free_slots_sl && i<dispatch_list.size();i++)
    {
        if(fake_rob[dispatch_list[i]]->current_stage==ID)
        {
            recs_to_remove++;
            fake_rob[dispatch_list[i]]->current_stage=IS;

            if(fake_rob[dispatch_list[i]]->src1!=-1)
            {
                fake_rob[dispatch_list[i]]->src1tag=register_file[fake_rob[dispatch_list[i]]->src1];
            }
            if(fake_rob[dispatch_list[i]]->src2!=-1)
            {
                fake_rob[dispatch_list[i]]->src2tag=register_file[fake_rob[dispatch_list[i]]->src2];
            }
            if(fake_rob[dispatch_list[i]]->dst!=-1)
            {
                register_file[fake_rob[dispatch_list[i]]->dst]=fake_rob[dispatch_list[i]]->tag;
            }            
            

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
    

    for(unsigned int i=0;i<dispatch_list.size();++i)
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
    string instruction="";
    
    for(int i=0;i<records_to_fetch;i++)
    {
        getline(infile,instruction); 
        if(instruction!="")
        {
            num_instructions++;
            //2b6424 2 14 29 -1 400341a0
            vector<string> split;
            string seg="";
            for(unsigned int i=0;i<instruction.length();i++)
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
            current->pc                     = stoi(split[0],nullptr,16);
            current->operation_type         = stoi(split[1]);
            current->dst                    = stoi(split[2]);
            current->src1                   = stoi(split[3]);
            current->src2                   = stoi(split[4]);
            
            current->mem_address            = stoul(split[5],nullptr,16);        

            current->current_stage          = IF;
            current->if_beg                 = num_cycles;
            fake_rob.insert(pair<int,InstructionMeta*>(current->tag,current));
            //cout<<current->tag<<"\n";
            dispatch_list.push_back(current->tag);

            if(infile.eof())
            {
                eof_reached=true;
                break;
            }
        }
        else
        {
            eof_reached=true;
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
    return float(num_instructions)/float(num_cycles-1);
}
void Scheduler::printConfig()
{
    cout<<"CONFIGURATION\n";
    cout<<"superscalar bandwidth (N) = "<<n <<"\n";
    cout<<"dispatch queue size (2*N) = "<<dispatch_q_size <<"\n";
    cout<<"schedule queue size (S)   = "<<s<<"\n";
}

void Scheduler::printResults()
{
    cout<<"RESULTS\n";
    cout<<"number of instructions = "<<num_instructions<<"\n";
    cout<<"number of cycles       = "<<num_cycles-1<<"\n";
    printf("IPC                    = %0.2f\n",getIpc());
    // printf("%0.2f\n",getIpc());
}