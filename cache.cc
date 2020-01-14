#include "cache.h"
unsigned int call_count;
// unsigned int line=0;

bool CompareTimestamps(Sector a, Sector b){
    return a.timestamp>b.timestamp;
}

void Cache::evictSector(vector<Sector>::reverse_iterator sector_to_replace,unsigned int c0,unsigned int c1,unsigned int c2, unsigned int c3, char mode){
    // blocks associated with the old address tag need to be invalidated
    unsigned int ind_addr_tag_to_be_evicted = c2;            
    unsigned int i=0;
    for(vector<Block>::iterator bl_it=sector_to_replace->blocks.begin();bl_it!=sector_to_replace->blocks.end();bl_it++){
        if((bl_it->tag_selector==ind_addr_tag_to_be_evicted && c3!=sector_to_replace->tags[bl_it->tag_selector].tag_id) || (i == c0)){    
            //                    
            if(bl_it->is_valid)
            {
                if(bl_it->is_dirty)
                {
                    num_write_backs++;
                    num_memory_access++;
                    unsigned int old_addr = (((sector_to_replace->tags[ind_addr_tag_to_be_evicted].tag_id*addr_tags+ind_addr_tag_to_be_evicted)*number_of_sets+c1)*data_blocks+i)*block_size;
                    if(next_level != NULL){                        
                        next_level->WriteToAddress(old_addr);
                    }
                }
                bl_it->tag_selector=0;;
                bl_it->is_valid=false;
                bl_it->is_dirty=false;
            }                
        }
        i++;
    }        
    sector_to_replace->blocks[c0].is_valid=true;
    sector_to_replace->tags[c2].is_valid=true;
    sector_to_replace->blocks[c0].is_dirty=false;
    if(mode=='w'){
        sector_to_replace->blocks[c0].is_dirty= true;
    }
    sector_to_replace->blocks[c0].tag_selector=c2;
    sector_to_replace->tags[c2].tag_id=c3;
    sector_to_replace->timestamp=call_count++;        
}


void Cache::AttachNextLevelCache(Cache *c){
    next_level=c;
}    

int Cache::FetchBlock(unsigned int address,char mode){
    if(mode=='r'){
        num_reads++;
    }else if(mode == 'w'){
        num_writes++;
    }

    unsigned int c = address/block_size;  
    unsigned int c0 =   c%data_blocks;  
    unsigned int c1 =   (c/data_blocks)%number_of_sets;        
    unsigned int c2 =   (c/(data_blocks*number_of_sets))%addr_tags;
    unsigned int c3 =   c/(data_blocks*number_of_sets*addr_tags);
    bool is_found=false;
    bool is_tag_found = false;
    bool is_block_found = false;
    int next_level_status = 0;

    bool invalid_block_found=false;
    bool invalid_tag_block_found=false;
    vector<Sector>::reverse_iterator lru_block=sets[c1].sectors.rbegin();
    vector<Sector>::reverse_iterator invalid_block;
    vector<Sector>::reverse_iterator invalid_tag_block;
    
    for(vector<Sector>::reverse_iterator it=sets[c1].sectors.rbegin();it != sets[c1].sectors.rend();it++)
    {
        for(vector<Block>::iterator bl_it=it->blocks.begin(); bl_it != it->blocks.end(); bl_it++){
            if(bl_it->is_valid){
                is_tag_found=true;
            }
        }
        if((it->tags[c2].tag_id == c3 && it->tags[c2].is_valid)){
            
            if(it->blocks[c0].is_valid && it->blocks[c0].tag_selector == c2 ){
                is_found=true;
                it->timestamp=call_count++;
                if(mode=='w'){
                    it->blocks[c0].is_dirty= true;
                }
                                
            }                                
        }
        if(it->blocks[c0].is_valid && it->blocks[c0].tag_selector == c2){
            is_block_found=true;
        }            

        //Third would be lru
        if(it->timestamp < lru_block->timestamp){
            lru_block=it;
        }
        //If block is not found first priority to replace would be for matching tag but invalid block
        if(it->tags[c2].tag_id==c3 && it->tags[c2].is_valid && !it->blocks[c0].is_valid){
            invalid_block=it;
            invalid_block_found=true;
        }
        //Second best option would be invalid tag
        if(!it->tags[c2].is_valid  && !invalid_tag_block_found){
            invalid_tag_block=it;
            invalid_tag_block_found=true;
        }
    }
    // if(cache_level==1){
    //     printf("------------------------------------------------\n");
    //     if(mode=='r'){
    //         printf("# %d: read %0x\n",line,address);
    //         printf("L1 read: %0x (tag %0x, index %d)\n", address, c3, c1);
    //     }else if (mode=='w'){
    //         printf("# %d: write %0x\n",line,address);
    //         printf("L1 write: %0x (tag %0x, index %d)\n", address, c3, c1);
    //     }            
    //     if(is_found){
    //         printf("L1 hit \n");
    //     }else{
    //         printf("L1 miss \n");
    //     } 
    // }else if(cache_level==2){
    //     if(mode=='r'){
    //         printf("L2 read: %0x (C0 %0x, C1 %0x, C2 %0x, C3 %0x) \n",address,c0,c1,c2,c3);
    //     }else if(mode == 'w'){
    //         printf("L2 write:  (C0 %0x, C1 %0x, C2 %0x, C3 %0x) \n",c0,c1,c2,c3);
    //     }
        
    //     if(is_found){
    //         printf("L2 hit \n");
    //     }else{
    //         printf("L2 miss \n");
    //     } 
    // }
    if(!is_block_found){
        cache_block_miss++;
    }
    if(!is_tag_found){
            sector_miss++;                       
    }
    if(!is_found){  
        num_memory_access++;
        if(mode=='r'){
            num_read_miss++;
        }else if(mode == 'w'){
            num_write_miss++;
        }

        if(invalid_block_found){
            evictSector(invalid_block,c0,c1,c2,c3,mode);

        }else if(invalid_tag_block_found){
            evictSector(invalid_tag_block,c0,c1,c2,c3,mode);
            
        }else{
            evictSector(lru_block,c0,c1,c2,c3,mode);
        }
        if(next_level != NULL){
            next_level_status = next_level->ReadFromAddress(address);
        }            
    }
    // if(cache_level==1){
    //     printf("L1 update LRU\n");
    // }else
    // {
    //     printf("L2 update LRU\n");
    // } 
    if(is_found)
    {
        return 3+next_level_status;
    }       
    else
    {
        return 5+next_level_status;
    }
    
}


int Cache::ReadFromAddress(unsigned int address){         
    return FetchBlock(address,'r');        
    
            
}
bool Cache::WriteToAddress(unsigned int address){        
    FetchBlock(address,'w');              
    return true;
}

float Cache::GetMissRate(){
    return ((float)num_read_miss+(float)num_write_miss)/((float)num_reads+(float)num_writes);

}

int Cache::GetCacheBlockMiss(){
    return num_read_miss + num_write_miss - sector_miss;
}

void Cache::PrintDataCacheContent(){               
    int i=0;
    for(vector<Set>::iterator it=sets.begin();it != sets.end();it++){
        cout<<"set\t"<<i<<":";
        sort(it->sectors.begin(),it->sectors.end(),CompareTimestamps);
        for(vector<Sector>::iterator sec_it=it->sectors.begin(); sec_it!=it->sectors.end(); sec_it++){ 
            for(vector<Block>::iterator bl_it= sec_it->blocks.begin(); bl_it!=sec_it->blocks.end();bl_it++){
                char dirty_char;
                char valid_char;
                if(bl_it->is_dirty){
                    dirty_char='D';
                }else{
                    dirty_char='N';
                }
                if(bl_it->is_valid){
                    valid_char='V';
                }else{
                    valid_char='I';
                }
                printf("\t%0x,%c,%c\t",bl_it->tag_selector,valid_char,dirty_char);
            } 
            printf("||");          
        }
        cout<<"\n";
        i++;
    }

}

void Cache::PrintAddressCacheContent(){
    int i=0;
    for(vector<Set>::iterator it=sets.begin();it != sets.end();it++){
        cout<<"set\t"<<i<<":";
        sort(it->sectors.begin(),it->sectors.end(),CompareTimestamps);
        for(vector<Sector>::iterator sec_it=it->sectors.begin(); sec_it!=it->sectors.end(); sec_it++){ 
            for(vector<Tag>::iterator addr_it= sec_it->tags.begin(); addr_it!=sec_it->tags.end();addr_it++){
                printf("\t%0x\t",addr_it->tag_id);
            } 
            printf("||");               
        }
        cout<<"\n";
        i++;
    }

}

void Cache::PrintCacheContent(){        
    int i=0;
    for(vector<Set>::iterator it=sets.begin();it != sets.end();it++){
        cout<<"set\t"<<i<<":";
        //sort(it->sectors.begin(),it->sectors.end(),CompareTimestamps);
        for(vector<Sector>::reverse_iterator sec_it=it->sectors.rbegin(); sec_it!=it->sectors.rend(); sec_it++){
            // char dirty_char;
             for(vector<Block>::reverse_iterator bl_it=sec_it->blocks.rbegin();bl_it!=sec_it->blocks.rend();bl_it++){
            //     if(bl_it->is_dirty){
            //         dirty_char='D';
            //     }else{
            //         dirty_char='N';
            //     }
                printf("%0x\t",sec_it->tags[bl_it->tag_selector].tag_id*addr_tags+bl_it->tag_selector);
            }
        }
        cout<<"\n";
        i++;
    }

}

void Cache::DisplayStats(){    
    cout<<"a. number of accesses:			"<<num_reads<<"\n";
    cout<<"b. number of misses:		"<<num_read_miss<<"\n";
    // cout<<"c. number of L1 writes:			"<<num_writes<<"\n";
    // cout<<"d. number of L1 write misses:		"<<num_write_miss<<"\n";
    // printf("e. L1 miss rate:			%0.4f\n",((float)num_read_miss+(float)num_write_miss)/((float)num_reads+(float)num_writes));        
    // cout<<"f. number of writebacks from L1 memory:	"<<num_write_backs<<"\n";
    // cout<<"g. total memory traffic:		"<<num_memory_access<<"\n";
}

