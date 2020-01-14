#include "scheduler.h"

// using namespace std;

int main(int argc, char* argv[])
{
    //check for correct argument length
    if(argc!=9)
    {
        cout<<"Improper arguments!!!\n";
        cout<<"Example usage:   sim <S> <N> <BLOCKSIZE> <L1_size> <L1_ASSOC> <L2_SIZE> <L2_ASSOC> <tracefile> \n";
        return 1;
    }
    int s               = atoi(argv[1]);
    int n               = atoi(argv[2]);    
    int block_size      = atoi(argv[3]);
    int L1_size         = atoi(argv[4]);
    int L1_assoc        = atoi(argv[5]);
    int L2_size         = atoi(argv[6]);
    int L2_assoc        = atoi(argv[7]);    
    string trace_file   = argv[8];

    Scheduler sched(s,n,block_size,L1_size,L1_assoc,L2_size,L2_assoc,trace_file);
    sched.start();
    if(L1_size!=0)
    {
        cout<<"L1 CACHE CONTENTS\n";
        sched.L1_cache->DisplayStats();
        sched.L1_cache->PrintCacheContent();
        cout<<"\n";
    }
    if(L2_size!=0)
    {
        cout<<"L2 CACHE CONTENTS\n";
        sched.L2_cache->DisplayStats();
        sched.L2_cache->PrintCacheContent();
        cout<<"\n";
    }
    sched.printConfig();
    sched.printResults(); 


}