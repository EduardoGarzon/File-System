/*
    Sistema de arquivos baseado em alocacao contígua e
    mapa de bits para o gerenciamento de espacos livres.
*/

#include <stdio.h>
#include <stdlib.h>
#define BLOCK_SIZE 512           // Tamanho de cada bloco em bytes.
#define TOTAL_SYSTEM_BLOCKS 1000 // Total de blocos no disco.

// Boot Record.
typedef struct BR
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
} __attribute__((__packed__)) BootRecord;         // total size -> 32 bytes.

// BitMap.
typedef struct BM
{
} __attribute__((__packed__)) Bitmap;

// Entries Table.

// Entries Format.
typedef struct EntriesFormat
{
    unsigned char status;              // offset 0 - 1 byte.
    unsigned char file_name[12];       // offset 1 - 12 bytes.
    unsigned char file_extension[4];   // offset 13 - 4 bytes.
    unsigned char file_attribute;      // offset 17 - 1 byte.
    unsigned int first_block;          // offset 18 - 4 bytes.
    unsigned int size_in_bytes;        // offset 22 - 4 bytes.
    unsigned int total_used_blocks;    // offset 26 - 4 bytes.
    unsigned char empty[2];            // offset 30 - 2 bytes.
} __attribute__((__packed__)) Entries; // total size -> 32 bytes.

// Data Section.

// Assinatura de Métodos do Sistema de Arquivos.
BootRecord *initBootRecord();

// Assinatura de Métodos Auxiliares.
int isFileEmpty(FILE *system_img);

int main()
{
    FILE *system_img;
    BootRecord *boot_record;
    boot_record = initBootRecord(system_img);

    return 0;
}

BootRecord *initBootRecord()
{
    FILE *system_img = fopen("./file_system.img", "rb+");
    if (!system_img)
    {        
        // Se o arquivo não existir, cria um novo
        printf("Criando nova imagem do sistema...\n");
        
        system_img = fopen("./file_system.img", "wb");
        if (!system_img)
        {
            printf("Erro ao criar a imagem do sistema!\n");
            return NULL;
        }
    }

    BootRecord *boot_record = malloc(sizeof(BootRecord));
    if (!boot_record)
    {
        printf("Falha ao alocar Boot Record!\n");
        fclose(system_img); // Fechar o arquivo antes de retornar NULL
        return NULL;
    }

    // Verifica se a imagem está vazia ou não.
    if (isFileEmpty(system_img))
    {
        printf("Criando imagem do sistema!\n");
    }
    else
    {
        printf("Lendo BootRecord!\n");
    }

    fclose(system_img); // Fechar o arquivo após o uso
    return boot_record;
}

int isFileEmpty(FILE *system_img)
{
    // Armazena a posição atual do cursor
    long currentPosition = ftell(system_img);

    // Move o cursor para o final do arquivo
    fseek(system_img, 0, SEEK_END);

    // Obtém a posição atual do cursor, que é o tamanho do arquivo
    long system_imgSize = ftell(system_img);

    // Retorna 1 se o tamanho do arquivo for zero, indicando que o arquivo está vazio
    // Retorna 0 caso contrário
    return (system_imgSize == 0);

    // Restaura a posição original do cursor
    fseek(system_img, currentPosition, SEEK_SET);
}