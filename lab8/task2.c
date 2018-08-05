#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <elf.h>
#include <sys/stat.h>
#include <sys/mman.h>

#define MAX_BUF     100

char buf[MAX_BUF];
int current_fd = -1;
void *map_start;
struct stat fd_stat;
Elf64_Ehdr *header;

void examine_elf_file();
void print_section_headers();
void print_symbols();
void quit();
char *sh_type_name(long index);

void (*actions[])() = {examine_elf_file, print_section_headers, print_symbols, quit};

int main(int argc, char **argv)
{
    unsigned int a, i;
    int scanned;
    const char *menu[] = 
    {"Choose action:",
     "1-Examine ELF File",
     "2-Print Section Headers",
     "3-Print Symbols",
     "4-Quit",
     NULL};

    for (;;) {
        
        i = 0;
        while (menu[i]) {
            printf("%s\n", menu[i++]);
        }
        
        scanned = scanf("%d", &a);
        while (getchar() != '\n');

        if (scanned != 1 || (a != 1 && a != 2 && a != 3 && a != 4)) {
            printf("Invalid action...\n");
        } else {
            actions[a - 1]();
        }
    }

    return 0;
}

void examine_elf_file()
{
    printf("Please enter filename\n");
    fgets(buf, MAX_BUF, stdin);
    buf[strlen(buf) - 1] = '\0';

    if (current_fd != -1) {
        close(current_fd);
    }

    if (-1 == (current_fd = open(buf, O_RDWR))) {
        perror("open failed");
    } else if (-1 == fstat(current_fd, &fd_stat)) {
        perror("stat failed");
        close(current_fd);
        current_fd = -1;
    } else if (MAP_FAILED == (map_start = 
      mmap(0, fd_stat.st_size, PROT_READ | PROT_WRITE , MAP_SHARED, current_fd, 0))) {
        perror("mmap failed");
        close(current_fd);
        current_fd = -1;
    } else {
        header = (Elf64_Ehdr *)map_start;
        
        if (ELFMAG1 == header->e_ident[EI_MAG1] && ELFMAG2 == header->e_ident[EI_MAG2] &&
            ELFMAG3 == header->e_ident[EI_MAG3]) {

            printf("\nMagic:\t\t\t\t%x %x %x\n", ELFMAG1, ELFMAG2, ELFMAG3);
            printf("Data:\t\t\t\t%s\n",
                   ELFDATA2LSB == header->e_ident[EI_DATA] ? "2's complement, little endian" :
                   ELFDATA2MSB == header->e_ident[EI_DATA] ? "2's complement, big endian" :
                   "Invalid data");
            printf("Entry point address:\t\t0x%08lx\n", header->e_entry);
            printf("Section header offset:\t\t0x%08lx\n", header->e_shoff);
            printf("Number of section headers:\t%d\n", header->e_shnum);
            printf("Size of section headers:\t%d (bytes)\n", header->e_shentsize);
            printf("Program header offset:\t\t0x%08lx\n", header->e_phoff);
            printf("Number of program headers:\t%d\n", header->e_phnum);
            printf("Size of program headers:\t%d (bytes)\n\n", header->e_phentsize);
            
        } else {
            printf("%s is not an ELF file...\n", buf);
            close(current_fd);
            current_fd = -1;
            munmap(map_start, fd_stat.st_size);
        }
    }
}

void print_section_headers()
{
    unsigned int i;
    char *sh_str;
    Elf64_Shdr *sh_table;
    
    if (-1 == current_fd) {
        printf("Invalid filename...\n");
    } else {
        sh_table = (Elf64_Shdr *)(map_start + header->e_shoff);
        sh_str = map_start + sh_table[header->e_shstrndx].sh_offset;
        
        printf("\nSection Headers:\n");
        printf("  [Nr]\tName\t\t\tAddress\t\t\tOffset\t\tSize\t\t\tType\n");

        for (i = 0; i < header->e_shnum; i++) {
            printf("  [%2d]\t%-24s%016lx\t%08lx\t%016lx\t%s\n",
            i, sh_str + sh_table[i].sh_name, sh_table[i].sh_addr, sh_table[i].sh_offset,
            sh_table[i].sh_size, sh_type_name(sh_table[i].sh_type));
        }
        printf("\n");
    }
}

void print_symbols()
{
    unsigned int i, j, total, shndx;
    char *sh_str, *sym_str, *dyn_str;
    Elf64_Shdr *sh_table;
    Elf64_Sym *sym_table;
    
    if (-1 == current_fd) {
        printf("Invalid filename...\n");
    } else {
        sh_table = (Elf64_Shdr *)(map_start + header->e_shoff);
        sh_str = map_start + sh_table[header->e_shstrndx].sh_offset;

        for (i = 0; i < header->e_shnum; i++) {
            if (SHT_STRTAB == sh_table[i].sh_type) {
                if (0 == strcmp(".dynstr", sh_str + sh_table[i].sh_name)) {
                    dyn_str = map_start + sh_table[i].sh_offset;
                } else if (0 == strcmp(".strtab", sh_str + sh_table[i].sh_name)) {
                    sym_str = map_start + sh_table[i].sh_offset;
                }
            }
        }

        for (i = total = 0; i < header->e_shnum; i++) {
            if (SHT_SYMTAB == sh_table[i].sh_type || SHT_DYNSYM == sh_table[i].sh_type) {

                printf("\nSymbol table '%s':\n", sh_str + sh_table[i].sh_name);
                printf(" Num:\tValue\t\t\tNdx\tSection Name\t\tSymbol Name\n");
                
                sym_table = (Elf64_Sym *)(map_start + sh_table[i].sh_offset);

                for (j = 0; j < sh_table[i].sh_size / sizeof(Elf64_Sym); j++, total++) {

                    printf("%4d:\t%016lx\t", j, sym_table[j].st_value);

                    switch((shndx = sym_table[j].st_shndx)) {
                        case SHN_UNDEF:  printf("UND\t%-24s", "");                                      break;
                        case SHN_ABS:    printf("ABS\t%-24s", "");                                      break;
                        case SHN_COMMON: printf("COM\t%-24s", "");                                      break;
                        default:         printf("%3d\t%-24s", shndx, sh_str + sh_table[shndx].sh_name); break;
                    }
                    printf("%s\n", SHT_SYMTAB == sh_table[i].sh_type ? sym_str + sym_table[j].st_name
                                                                     : dyn_str + sym_table[j].st_name);
                }
            }
        }
        printf("\n");

        if (0 == total) {
            printf("No symbols in file...\n");
        }
    }
}

void quit()
{
    if (current_fd != -1) {
        munmap(map_start, fd_stat.st_size);
        close(current_fd);
    }
    exit(0);
}

char *sh_type_name(long index) {
    switch(index) {
        case 0:             return "NULL";
        case 1:             return "PROGBITS";
        case 2:             return "SYMTAB";
        case 3:             return "STRTAB";
        case 4:             return "RELA";
        case 5:             return "HASH";
        case 6:             return "DYNAMIC";
        case 7:             return "NOTE";
        case 8:             return "NOBITS";
        case 9:             return "REL";
        case 10:            return "SHLIB";
        case 11:            return "DYNSYM";
        case 14:            return "INIT_ARRAY";
        case 15:            return "FINI_ARRAY";
        case 16:            return "PREINIT_ARRAY";
        case 17:            return "GROUP";
        case 18:            return "SYMTAB_SHNDX";
        case 19:            return "NUM";
        case 0x60000000:    return "LOOS";
        case 0x6ffffef:     return "LOSUNW";
        case 0x6fffffef:    return "SUNW_capchain";
        case 0x6ffffff0:    return "SUNW_capinfo";
        case 0x6ffffff1:    return "SUNW_symsort";
        case 0x6ffffff2:    return "SUNW_tlssort";
        case 0x6ffffff3:    return "SUNW_LDYNSYM";
        case 0x6ffffff4:    return "SUNW_dof";
        case 0x6ffffff5:    return "SUNW_cap";
        case 0x6ffffff6:    return "GNU_HASH";
        case 0x6ffffff7:    return "GNU_LIBLIST";
        case 0x6ffffff8:    return "CHECKSUM";
        case 0x6ffffff9:    return "SUNW_DEBUG";
        case 0x6ffffffa:    return "LOSUNW";
        case 0x6ffffffb:    return "SUNW_COMDAT";
        case 0x6ffffffc:    return "SUNW_syminfo";
        case 0x6ffffffd:    return "VERDEF";
        case 0x6ffffffe:    return "VERNEED";
        case 0x6fffffff:    return "VERSYM";
        case 0x70000000:    return "LOPROC";
        case 0x70000001:    return "AMD64_UNWIND";
        case 0x7fffffff:    return "HIPROC";
        case 0x80000000:    return "LOUSER";
        case 0xffffffff:    return "HIUSER";
        default:            return "UNKNOWN";
    }
}
