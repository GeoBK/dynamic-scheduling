#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;
class Block {
    public:    
    bool is_dirty=false;
    bool is_valid=false;
    unsigned int tag_selector=0;     
};

class Tag {
    public:
    unsigned int tag_id;
    bool is_valid = false;
};

class Sector{
    public:
    Sector(int data_blocks,int addr_tags){
        for(int i=0;i<data_blocks;i++){
            blocks.push_back(Block());
        }
        for(int i=0;i<addr_tags;i++){
            tags.push_back(Tag());
        }
    }
    vector<Block> blocks;
    vector<Tag> tags;
    bool is_valid=false;
    unsigned int timestamp=0;
};

class Set{
    public:
    Set(int associativity, int data_blocks, int addr_tags){
        for(int i=0;i<associativity;i++){
            sectors.push_back(Sector(data_blocks,addr_tags));
        }        
    }   
    vector<Sector> sectors;
};

class Cache {
    private:
    unsigned int block_size;
    unsigned int cache_size;        
    unsigned int assoc;
    
    Cache* next_level       =   nullptr;
    vector<Set> sets;

    void evictSector(vector<Sector>::reverse_iterator sector_to_replace,unsigned int c0,unsigned int c1,unsigned int c2, unsigned int c3, char mode);

    public:
    unsigned int data_blocks;
    unsigned int addr_tags;
    int num_reads                   =   0;
    int num_writes                  =   0;
    int num_read_miss               =   0;
    int num_write_miss              =   0;
    int num_write_backs             =   0;
    int num_memory_access           =   0;

    int sector_miss                 =   0;
    int cache_block_miss            =   0;

    unsigned int number_of_sets              =   0;    
    unsigned int total_sectors_in_cache      =   0;   

    int cache_level                 =   0;

    
    
    Cache(int block_size, int cache_size, int associativity, int data_blocks, int addr_tags, int level): 
        block_size(block_size), cache_size(cache_size), assoc(associativity), data_blocks(data_blocks), addr_tags(addr_tags), cache_level(level){
            total_sectors_in_cache = cache_size / (block_size*data_blocks);            
            number_of_sets = total_sectors_in_cache/associativity;
            for(unsigned int i=0;i<number_of_sets;i++){
                sets.push_back(Set(associativity,data_blocks,addr_tags));
            }
        }

    void AttachNextLevelCache(Cache *c);

    int FetchBlock(unsigned int address,char mode);


    int ReadFromAddress(unsigned int address);

    bool WriteToAddress(unsigned int address);

    float GetMissRate();

    int GetCacheBlockMiss();
    
    void PrintDataCacheContent();

    void PrintAddressCacheContent();

    void PrintCacheContent();

    void DisplayStats();

};