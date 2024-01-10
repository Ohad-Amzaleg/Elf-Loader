#include <stdio.h>
#include <stdlib.h>
#include <elf.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>



int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg);

void load_phdr(Elf32_Phdr *phdr, int fd) ;
void headerAligment(Elf32_Phdr *phdr,int fd);

unsigned int get_mmap_prot_flags(int phdr_flags) ;
unsigned int get_mmap_map_flags();


void readElfPrint(void *map_start, int arg) ;
void print_phdr_info(Elf32_Phdr *phdr,int arg) ;
void getMapProtStr(Elf32_Phdr *phdr,char *mmap_prot_str);
void getMapFlagsStr(Elf32_Phdr *phdr,char *mmap_flags_str);
void getElfFlagsStr(Elf32_Phdr *phdr,char *flags_str);
char* getType(unsigned int p_type);
int is_header_aligned(Elf32_Phdr *phdr);
int startup(int argc, char **argv, void (*start)());

struct TYPE {
    int key;
    char* type;
}typedef TYPE ;

const int TYPENUM=10;

const TYPE typeArr[]={
    {0,"NULL"},
    {1,"LOAD"},
    {2,"DYNAMIC"},
    {3,"INTERP"},
    {4,"NOTE"},
    {5,"SHLIB"},
    {6,"PHDR"},
    {1685382480,"GNU_STACK"},
    {1685382481,"GNU_RELRO"},
    {1685382482,"NUM"}
};



int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file_name>\n", argv[0]);
        return 1;
    }

    char *file_name = argv[1];

    int fd = open(file_name, O_RDONLY);
    if (fd == -1) {
        perror("Error opening file");
        return 1;
    }

    struct stat st;
    if (fstat(fd, &st) == -1) {
        perror("Error getting file size");
        close(fd);
        return 1;
    }


void *map_start = mmap(NULL, st.st_size, PROT_READ, MAP_PRIVATE | MAP_FIXED, fd, 0);
    if (map_start == MAP_FAILED) {
         perror("Error mapping file");
            close(fd);
            return 0;
    }

    //getting the entry point 
    Elf32_Ehdr *elf_header = (Elf32_Ehdr *)map_start;
    void (*entry_point)() = (void (*)())elf_header->e_entry;


   // Assemble the command-line arguments for the loaded program
    char **loaded_argv = malloc((argc + 1) * sizeof(char *));
    loaded_argv[0] = argv[1];

    // Copy the remaining arguments
    for (int i = 2; i < argc; i++) {
        loaded_argv[i - 1] = argv[i]; 
    }
    loaded_argv[argc - 1] = NULL; // Set the last argument as NULL to mark the end of the array

    


    //Aplaying the loader to any header that is ptr load
    foreach_phdr(map_start,load_phdr,fd);
    
    //Passing control to the loaded program
    startup(argc-1, loaded_argv, entry_point);

    free(loaded_argv);
    munmap(map_start, st.st_size);
    close(fd);

    return 0;
}




// the function expects to get a start point ,function ,argument 
int foreach_phdr(void *map_start, void (*func)(Elf32_Phdr *, int), int arg) {
    //create a pointer to the start point of the elf
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *)map_start;
    //create a pointer to the start point of the program
    Elf32_Phdr *phdr = (Elf32_Phdr *)((char *)map_start + ehdr->e_phoff);
    //number of the elf headers
    int num_phdr = ehdr->e_phnum;

    for (int i = 0; i < num_phdr; i++) {
        // printf("this is the current header :%u",phdr[i].p_type);
        //apply the given function to each one of the headers 
        func(&phdr[i], arg);
    }

    return 0;
}



//##################################### Printing Functions ###################################



void readElfPrint(void *map_start, int arg) {
    printf("%-4s %-4s %-4s %-4s %-4s %-4s %-4s %-4s %-4s %-4s\n", "Type", "Offset", "VirtAddr", "PhysAddr", "FileSiz", "MemSiz", "Flg", "Align","mmapFlags","protFlags");
    foreach_phdr(map_start, print_phdr_info, arg);
}


void testerStr(unsigned int flg,char *mmap_prot_str){
if (flg & PF_R)
    strcat(mmap_prot_str, "PROT_READ ");
if (flg & PF_W)
    strcat(mmap_prot_str, "PROT_WRITE ");
if (flg & PF_X)
    strcat(mmap_prot_str, "PROT_EXEC ");
}



void print_phdr_info(Elf32_Phdr *phdr,int arg) {
   char flags_str[4] = "";
   char mmap_prot_str[50]= "";
   char mmap_flags_str[50]= "";
   getMapFlagsStr(phdr,mmap_flags_str);
   getMapProtStr(phdr,mmap_prot_str);
   getElfFlagsStr(phdr,flags_str);

    printf("%-6s", getType(phdr->p_type));
    printf("0x%06u ", phdr->p_offset);
    printf("0x%08u ", phdr->p_vaddr);
    printf("0x%08u ", phdr->p_paddr);
    printf("0x%05u ", phdr->p_filesz);
    printf("0x%05u ", phdr->p_memsz);
    printf("%-3s ", flags_str);
    printf("0x%u", phdr->p_align);
    printf("%-3s ", mmap_prot_str);
    printf("%-3s\n ", mmap_flags_str);

}




//######################### Strings Getters ####################

void getElfFlagsStr(Elf32_Phdr *phdr,char *flags_str){
    // Determine flags string based on the flags value
    if (phdr->p_flags & PF_R)
        strcat(flags_str,"R");
    if (phdr->p_flags & PF_W)
        strcat(flags_str,"W");
    if (phdr->p_flags & PF_X)
       strcat(flags_str,"E");
}


void getMapFlagsStr(Elf32_Phdr *phdr,char *mmap_flags_str){

if (phdr->p_flags & PF_R)
    strcat(mmap_flags_str, "MAP_PRIVATE ");
if (phdr->p_flags & PF_W)
    strcat(mmap_flags_str, "MAP_SHARED ");

}

void getMapProtStr(Elf32_Phdr *phdr,char *mmap_prot_str){
if (phdr->p_flags & PF_R)
    strcat(mmap_prot_str, "PROT_READ ");
if (phdr->p_flags & PF_W)
    strcat(mmap_prot_str, "PROT_WRITE ");
if (phdr->p_flags & PF_X)
    strcat(mmap_prot_str, "PROT_EXEC ");
}


char* getType(unsigned int p_type){
    char * type="";

   for (int index=0; index<TYPENUM ; index++) {
    if(p_type == typeArr[index].key){
        type=typeArr[index].type;
    }
   }

    return type;
}












//############################################# Loader Functions ########################################


void load_phdr(Elf32_Phdr *phdr, int fd) {
    off_t offset;
    void *addr;

        //? should add this condition && phdr->p_filesz > 0
    
        if (phdr->p_type == PT_LOAD ) {
            printf("%-6s %-8s %-10s %-10s %-10s %-10s %-5s %-6s %-6s %-6s\n", "Type", "Offset", "VirtAddr", "PhysAddr", "FileSiz", "MemSiz", "Flg", "Align","mmapFlags","protFlags");
            print_phdr_info(phdr,0);

            // Get the protection flags.
            int prot = get_mmap_prot_flags(phdr->p_flags);
            // Get the mapping flags.
            int flags = get_mmap_map_flags();


            if(!is_header_aligned(phdr)){
                uint32_t vaddr = phdr->p_vaddr & 0xfffff000;
                uint32_t offset = phdr->p_offset & 0xfffff000;
                uint32_t padding = phdr->p_vaddr & 0xfff;

                addr = mmap((void *)vaddr, phdr->p_memsz+padding, prot, flags, fd, offset);
                if (addr == MAP_FAILED) {
                    perror("mmap");
                    exit(1);
                }

            }

            else {
                offset = phdr->p_offset;
                addr = mmap((void *)phdr->p_vaddr, phdr->p_memsz, prot, flags, fd, offset);
                if (addr == MAP_FAILED) {
                    perror("mmap");
                    exit(1);
                }
            }
        }
    
}


unsigned int get_mmap_prot_flags(int phdr_flags) {
   unsigned int prot_flags = 0;
    
    //Switched the read and exec because of the bit differ 
    if (phdr_flags & PF_R)
        prot_flags |= PROT_EXEC;
    if (phdr_flags & PF_W)
        prot_flags |= PROT_WRITE;
    if (phdr_flags & PF_X)
        prot_flags |= PROT_READ;

    return prot_flags;
}

unsigned int get_mmap_map_flags() {
    unsigned int map_flags = MAP_PRIVATE | MAP_FIXED;

    return map_flags;
}


int is_header_aligned(Elf32_Phdr *phdr) {
    long page_size = sysconf(_SC_PAGE_SIZE);
    return (phdr->p_vaddr % page_size == 0);
}

