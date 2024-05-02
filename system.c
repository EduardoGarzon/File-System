/*
    Sistema de arquivos baseado em alocacao contígua e
    mapa de bits para o gerenciamento de espacos livres.
*/

#include <stdio.h>

// Boot Record.
typedef struct
{
    unsigned short bytes_per_block;               // offset 0  - 2 bytes. -> 512 bytes
    unsigned short reserved_blocks_count;         // offset 2  - 2 bytes. -> 1 block
    unsigned int number_of_bitmap_blocks;         // offset 4  - 4 bytes. -> ???
    unsigned int number_of_entry_table_blocks;    // offset 8  - 4 bytes. -> ???
    unsigned int number_of_data_section_blocks;   // offset 12 - 4 bytes. -> ???
    unsigned int number_of_total_system_blocks;   // offset 16 - 4 bytes. -> ???
    unsigned int number_of_root_directory_blocks; // offset 20 - 4 bytes. -> ???
    unsigned short first_root_directory_block;    // offset 24 - 2 bytes. -> ???
    unsigned char empty[6];                       // offset 26 - 6 bytes.
} __attribute__((__packed__)) BootRecord;         // total size 32 bytes.

int main()
{

    return 0;
}