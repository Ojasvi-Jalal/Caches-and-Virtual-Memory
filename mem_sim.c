/***************************************************************************
 * *    Inf2C-CS Coursework 2: TLB and Cache Simulation
 * *
 * *    Instructor: Boris Grot
 * *
 * *    TA: Priyank Faldu
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>
#include <math.h>
/* Do not add any more header files */

/*
 * Various structures
 */
typedef enum {tlb_only, cache_only, tlb_cache} hierarchy_t;
typedef enum {instruction, data} access_t;
const char* get_hierarchy_type(uint32_t t) {
    switch(t) {
        case tlb_only: return "tlb_only";
        case cache_only: return "cache-only";
        case tlb_cache: return "tlb+cache";
        default: assert(0); return "";
    };
    return "";
}

typedef struct {
    uint32_t address;
    access_t accesstype;
} mem_access_t;

// These are statistics for the cache and TLB and should be maintained by you.
typedef struct {
    uint32_t tlb_data_hits;
    uint32_t tlb_data_misses;
    uint32_t tlb_instruction_hits;
    uint32_t tlb_instruction_misses;
    uint32_t cache_data_hits;
    uint32_t cache_data_misses;
    uint32_t cache_instruction_hits;
    uint32_t cache_instruction_misses;
} result_t;


/*
 * Parameters for TLB and cache that will be populated by the provided code skeleton.
 */
hierarchy_t hierarchy_type = tlb_cache;
uint32_t number_of_tlb_entries = 0;
uint32_t page_size = 0;
uint32_t number_of_cache_blocks = 0;
uint32_t cache_block_size = 0;
uint32_t num_page_table_accesses = 0;


/*
 * Each of the variables (subject to hierarchy_type) below must be populated by you.
 */
uint32_t g_total_num_virtual_pages = 0;
uint32_t g_num_tlb_tag_bits = 0;
uint32_t g_tlb_offset_bits = 0;
uint32_t g_num_cache_tag_bits = 0;
uint32_t g_cache_offset_bits= 0;
result_t g_result;


/* Reads a memory access from the trace file and returns
 * 1) access type (instruction or data access)
 * 2) 32-bit virtual memory address
 */
mem_access_t read_transaction(FILE *ptr_file) {
    char buf[1002];
    char* token = NULL;
    char* string = buf;
    mem_access_t access;

    if (fgets(buf, 1000, ptr_file)!=NULL) {

        /* Get the access type */
        token = strsep(&string, " \n");
        if (strcmp(token,"I") == 0) {
            access.accesstype = instruction;
        } else if (strcmp(token,"D") == 0) {
            access.accesstype = data;
        } else {
            printf("Unkown access type\n");
            exit(-1);
        }

        /* Get the address */
        token = strsep(&string, " \n");
        access.address = (uint32_t)strtol(token, NULL, 16);

        return access;
    }

    /* If there are no more entries in the file return an address 0 */
    access.address = 0;
    return access;
}

/*
 * Call this function to get the physical page number for a given virtual number.
 * Note that this function takes virtual page number as an argument and not the whole virtual address.
 * Also note that this is just a dummy function for mimicing translation. Real systems maintains multi-level page tables.
 */
uint32_t dummy_translate_virtual_page_num(uint32_t virtual_page_num) {
    uint32_t physical_page_num = virtual_page_num ^ 0xFFFFFFFF;
    num_page_table_accesses++;
    if ( page_size == 256 ) {
        physical_page_num = physical_page_num & 0x00FFF0FF;
    } else {
        assert(page_size == 4096);
        physical_page_num = physical_page_num & 0x000FFF0F;
    }
    return physical_page_num;
}

void print_statistics(uint32_t num_virtual_pages, uint32_t num_tlb_tag_bits, uint32_t tlb_offset_bits, uint32_t num_cache_tag_bits, uint32_t cache_offset_bits, result_t* r) {
    /* Do Not Modify This Function */

    printf("NumPageTableAccesses:%u\n", num_page_table_accesses);
    printf("TotalVirtualPages:%u\n", num_virtual_pages);
    if ( hierarchy_type != cache_only ) {
        printf("TLBTagBits:%u\n", num_tlb_tag_bits);
        printf("TLBOffsetBits:%u\n", tlb_offset_bits);
        uint32_t tlb_total_hits = r->tlb_data_hits + r->tlb_instruction_hits;
        uint32_t tlb_total_misses = r->tlb_data_misses + r->tlb_instruction_misses;
        printf("TLB:Accesses:%u\n", tlb_total_hits + tlb_total_misses);
        printf("TLB:data-hits:%u, data-misses:%u, inst-hits:%u, inst-misses:%u\n", r->tlb_data_hits, r->tlb_data_misses, r->tlb_instruction_hits, r->tlb_instruction_misses);
        printf("TLB:total-hit-rate:%2.2f%%\n", tlb_total_hits / (float)(tlb_total_hits + tlb_total_misses) * 100.0);
    }

    if ( hierarchy_type != tlb_only ) {
        printf("CacheTagBits:%u\n", num_cache_tag_bits);
        printf("CacheOffsetBits:%u\n", cache_offset_bits);
        uint32_t cache_total_hits = r->cache_data_hits + r->cache_instruction_hits;
        uint32_t cache_total_misses = r->cache_data_misses + r->cache_instruction_misses;
        printf("Cache:data-hits:%u, data-misses:%u, inst-hits:%u, inst-misses:%u\n", r->cache_data_hits, r->cache_data_misses, r->cache_instruction_hits, r->cache_instruction_misses);
        printf("Cache:total-hit-rate:%2.2f%%\n", cache_total_hits / (float)(cache_total_hits + cache_total_misses) * 100.0);
    }
}

/*
 *
 * Add any global variables and/or functions here as you wish.
 *
 */
 int page_offset = 0;

 struct Cache_block{
     int valid_bit;
     uint32_t tag;

 };
 struct Tlb_block{
     int valid_bit;
     uint32_t tag;
     uint32_t phy_page_number;
     uint32_t l_r_u;
 };

 void initialise_lru(struct Tlb_block tlb[]){
   int i;
   for(i=0; i<number_of_tlb_entries;i++){
     tlb[i].l_r_u = i;
     tlb[i].valid_bit =0;
   }
 }

 int get_lru_index(struct Tlb_block tlb[])
 { int i = 0;
   int lru_index;
   for(i=0;i<number_of_tlb_entries;i++){
     if(tlb[i].l_r_u==0)
     {
       lru_index = i;
     }
   }
   return lru_index;
 }

 void update_lru(struct Tlb_block tlb[],int index){

   int j;
   for(j = 0;j<number_of_tlb_entries;j++)
   {
     if(tlb[j].l_r_u>tlb[index].l_r_u)
     {
       tlb[j].l_r_u -= 1;
     }

   }


}

 void cache_simulator(struct Cache_block cache[],uint32_t page_address,access_t i_or_d){
   uint32_t block_address = page_address/cache_block_size;
   uint32_t block_number = block_address%number_of_cache_blocks;
   uint32_t number_of_block_bits = 32 - g_num_cache_tag_bits - g_cache_offset_bits;
   uint32_t tag = page_address>>(g_cache_offset_bits+number_of_block_bits);
     if(cache[block_number].valid_bit == 0 )
     {
           if(i_or_d == data)
           g_result.cache_data_misses++;
           else
           g_result.cache_instruction_misses++;
           cache[block_number].tag = tag;
           cache[block_number].valid_bit = 1;
         }
         else{
               if(cache[block_number].tag != tag)
               {
                 if(i_or_d == data)
                   g_result.cache_data_misses++;
                 else
                 g_result.cache_instruction_misses++;
                 cache[block_number].tag = tag;
               }
               else{
                 if(i_or_d == data)
                 g_result.cache_data_hits++;
                 else
                 g_result.cache_instruction_hits++;
               }
         }

   }

   void tlb_simulator(struct Tlb_block tlb[],struct Cache_block cache[],uint32_t offset_content, uint32_t virtual_page_number,access_t i_or_d,uint32_t no_of_offset_bits){
     int i;
     uint32_t tag = virtual_page_number;
     int found = 0;

       for(i = 0 ; i< number_of_tlb_entries;i++){
           if((tlb[i].valid_bit==1)&&(tlb[i].tag == tag)){
             update_lru(tlb,i);
             tlb[i].l_r_u = number_of_tlb_entries - 1;
             if(i_or_d == data)
             g_result.tlb_data_hits++;
             else
             g_result.tlb_instruction_hits++;
             if(hierarchy_type == tlb_cache)
             {
               uint32_t phy_address = tlb[i].phy_page_number<< no_of_offset_bits;
               phy_address = phy_address|offset_content;
               cache_simulator(cache,phy_address,i_or_d);
             }
             found++;
             break;
           }
         }
             if(found == 0){
                    int set = get_lru_index(tlb);

                    tlb[set].tag = tag;
                    tlb[set].valid_bit = 1;
                    tlb[set].phy_page_number = dummy_translate_virtual_page_num(virtual_page_number);
                    update_lru(tlb,set);
                    tlb[set].l_r_u = number_of_tlb_entries - 1;

                    if(i_or_d == data)
                      g_result.tlb_data_misses++;
                    else
                      g_result.tlb_instruction_misses++;
                      if(hierarchy_type == tlb_cache)
                      {
                        uint32_t phy_address = tlb[set].phy_page_number<< no_of_offset_bits;
                         phy_address = phy_address|offset_content;
                        cache_simulator(cache,phy_address,i_or_d);
                      }
             }



}

int main(int argc, char** argv) {

    /*
     *
     * Read command-line parameters and initialize configuration variables.
     *
     */
    int improper_args = 0;
    char file[10000];
    if ( argc < 2 ) {
        improper_args = 1;
        printf("Usage: ./mem_sim [hierarchy_type: tlb-only cache-only tlb+cache] [number_of_tlb_entries: 8/16] [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
    } else  {
        /* argv[0] is program name, parameters start with argv[1] */
        if ( strcmp(argv[1], "tlb-only") == 0 ) {
            if ( argc != 5 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim tlb-only [number_of_tlb_entries: 8/16] [page_size: 256/4096] mem_trace.txt\n");
            } else {
                hierarchy_type = tlb_only;
                number_of_tlb_entries = atoi(argv[2]);
                page_size = atoi(argv[3]);
                strcpy(file, argv[4]);
            }
        } else if ( strcmp(argv[1], "cache-only") == 0 ) {
            if ( argc != 6 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim cache-only [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
            } else {
                hierarchy_type = cache_only;
                page_size = atoi(argv[2]);
                number_of_cache_blocks = atoi(argv[3]);
                cache_block_size = atoi(argv[4]);
                strcpy(file, argv[5]);
            }
        } else if ( strcmp(argv[1], "tlb+cache") == 0 ) {
            if ( argc != 7 ) {
                improper_args = 1;
                printf("Usage: ./mem_sim tlb+cache [number_of_tlb_entries: 8/16] [page_size: 256/4096] [number_of_cache_blocks: 256/2048] [cache_block_size: 32/64] mem_trace.txt\n");
            } else {
                hierarchy_type = tlb_cache;
                number_of_tlb_entries = atoi(argv[2]);
                page_size = atoi(argv[3]);
                number_of_cache_blocks = atoi(argv[4]);
                cache_block_size = atoi(argv[5]);
                strcpy(file, argv[6]);
            }
        } else {
            printf("Unsupported hierarchy type: %s\n", argv[1]);
            improper_args = 1;
        }
    }
    if ( improper_args ) {
        exit(-1);
    }
    assert(page_size == 256 || page_size == 4096);
    if ( hierarchy_type != cache_only) {
        assert(number_of_tlb_entries == 8 || number_of_tlb_entries == 16);
    }
    if ( hierarchy_type != tlb_only) {
        assert(number_of_cache_blocks == 256 || number_of_cache_blocks == 2048);
        assert(cache_block_size == 32 || cache_block_size == 64);
    }

    printf("input:trace_file: %s\n", file);
    printf("input:hierarchy_type: %s\n", get_hierarchy_type(hierarchy_type));
    printf("input:number_of_tlb_entries: %u\n", number_of_tlb_entries);
    printf("input:page_size: %u\n", page_size);
    printf("input:number_of_cache_blocks: %u\n", number_of_cache_blocks);
    printf("input:cache_block_size: %u\n", cache_block_size);
    printf("\n");

    /* Open the file mem_trace.txt to read memory accesses */
    FILE *ptr_file;
    ptr_file =fopen(file,"r");
    if (!ptr_file) {
        printf("Unable to open the trace file: %s\n", file);
        exit(-1);
    }

    /* result structure is initialized for you. */
    memset(&g_result, 0, sizeof(result_t));

    /* Do not delete any of the lines below.
     * Use the following snippet and add your code to finish the task. */

    /* You may want to setup your TLB and/or Cache structure here. */
    struct Cache_block cache[number_of_cache_blocks];
    struct Tlb_block tlb[number_of_tlb_entries];
    initialise_lru(tlb);
    g_total_num_virtual_pages = pow(2,32 - log2(page_size));
    //cache = malloc(n*sizeof(cache_block_t));
    if ( hierarchy_type != tlb_only) {
      int index = log2(number_of_cache_blocks);
      g_cache_offset_bits= log2(cache_block_size);
      g_num_cache_tag_bits = 32-(g_cache_offset_bits+index);
   }
    if( hierarchy_type == tlb_only || hierarchy_type == tlb_cache) {
       g_tlb_offset_bits  = log2(page_size);
       g_num_tlb_tag_bits = 32-g_tlb_offset_bits;
    }
    int i = 0;
    for(i=0;i<number_of_cache_blocks;i++)
    {
      cache[i].valid_bit = 0;
    }
    int j = 0;
    for(i=0;i<number_of_tlb_entries;i++)
    {
      tlb[i].valid_bit = 0;
    }
    mem_access_t access;
    /* Loop until the whole trace file has been read. */
    while(1) {
        access = read_transaction(ptr_file);
        // If no transactions left, break out of loop.
        if (access.address == 0)
            break;
        /* Add your code here */
        /* Feed the address to your TLB and/or Cache simulator and collect statistics. */
        uint32_t no_of_offset_bits = log2(page_size);
        uint32_t no_of_virtual_page_no_bits = 32-log2(page_size);
        uint32_t virtual_page_number = access.address>>no_of_offset_bits;
          uint32_t phy_page_num;
          if(hierarchy_type == cache_only){
                  phy_page_num = dummy_translate_virtual_page_num(virtual_page_number);}
        phy_page_num = phy_page_num << no_of_offset_bits;
        uint32_t offset_content = access.address << no_of_virtual_page_no_bits;
        offset_content = offset_content >> no_of_virtual_page_no_bits;
        phy_page_num = phy_page_num|offset_content;

        if(hierarchy_type == cache_only)
        {



          cache_simulator(cache,phy_page_num,access.accesstype);
        }
        if(hierarchy_type != cache_only){
          tlb_simulator(tlb,cache,offset_content,virtual_page_number,access.accesstype,no_of_offset_bits);
        }

     }

    /* Do not modify code below. */
    /* Make sure that all the parameters are appropriately populated. */
    print_statistics(g_total_num_virtual_pages, g_num_tlb_tag_bits, g_tlb_offset_bits, g_num_cache_tag_bits, g_cache_offset_bits, &g_result);

    /* Close the trace file. */
    fclose(ptr_file);
    return 0;
}
