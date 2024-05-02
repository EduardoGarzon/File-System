#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#if defined(_WIN32) || defined(_WIN64)
#define clearscreen "cls"
#else
#define clearscreen "clear"
#endif

// BPB (BIOS Parameter Block).
typedef struct fat_BS
{
    unsigned char bootjmp[3];             // offset 0  - 3 bytes
    unsigned char oem_name[8];            // offset 3  - 8 bytes
    unsigned short bytes_per_sector;      // offset 11 - 2 bytes
    unsigned char sectors_per_cluster;    // offset 12 - 1 byte
    unsigned short reserved_sector_count; // offset 14 - 2 bytes
    unsigned char table_count;            // offset 16 - 1 byte
    unsigned short root_entry_count;      // offset 17 - 2 bytes
    unsigned short total_sectors_16;      // offset 19 - 2 bytes
    unsigned char media_type;             // offset 21 - 1 byte
    unsigned short table_size_16;         // offset 22 - 2 bytes
    unsigned short sectors_per_track;     // offset 24 - 2 bytes
    unsigned short head_side_count;       // offset 26 - 2 bytes
    unsigned int hidden_sector_count;     // offset 28 - 4 bytes
    unsigned int total_sectors_32;        // offset 32 - 4 bytes
    // this will be cast to it's specific type once the driver actually knows what type of FAT this is.
    unsigned char extended_section[54];
} __attribute__((__packed__)) fat_BS_t;

// Standard 8.3 Format Direcotory.
typedef struct Standard_Dir_Format
{
    unsigned char file_name[8];             // offset 0  - 11 bytes (8 for name)
    unsigned char extension[3];             // offset 0  - 11 bytes (3 for extension)
    unsigned char file_attribute;           // offset 11 - 1 byte
    unsigned char reserved_windows_NT;      // offset 12 - 1 byte
    unsigned char creation_time;            // offset 13 - 1 byte
    unsigned short time_file_created;       // offset 14 - 2 bytes
    unsigned short date_file_created;       // offset 16 - 2 bytes
    unsigned short last_accessed_date;      // offset 18 - 2 bytes
    unsigned short high_bits_first_cluster; // offset 20 - 2 bytes
    unsigned short last_modification_time;  // offset 22 - 2 bytes
    unsigned short last_modificaition_date; // offset 24 - 2 bytes
    unsigned short low_bits_first_cluster;  // offset 26 - 2 bytes
    unsigned int file_size;                 // offset 28 - 4 bytes
} __attribute__((__packed__)) standardDir;

// Posicoes dos diretorios do sistema de arquivos.
typedef struct Directory_Positions
{
    unsigned int boot_record;    // Boot Record
    unsigned int fats[2];        // FAT1 - FAT2
    unsigned int root_directory; // Root Directory
    unsigned int data_area;      // Data
} __attribute__((__packed__)) dirPosition;

void printBootRecord(FILE *fp, fat_BS_t *boot_record);
void setDirectoriesPositions(fat_BS_t *boot_record, dirPosition *position);
void printRootEntries(FILE *fp, dirPosition *positions, standardDir *rootDir);
void printArchive(FILE *fp, fat_BS_t *boot_record, dirPosition *positions, standardDir rootDir, int index);

int main()
{
    FILE *fp;
    fat_BS_t boot_record;
    dirPosition positions;

    fp = fopen("./fat16_4sectorpercluster.img", "rb"); // Abre imagem no modo leitura binaria.

    int archive_index = 0;
    do
    {
        system(clearscreen);

        // Passo 1. Apresentar os dados do boot record.
        printBootRecord(fp, &boot_record);
        printf("---------------------------------------------------------------------------------------------\n\n");

        // Passo 2.	Apresentar as entradas do diretorio raiz.
        setDirectoriesPositions(&boot_record, &positions);
        printf("---------------------------------------------------------------------------------------------\n\n");
        standardDir rootDir[boot_record.root_entry_count];
        printRootEntries(fp, &positions, rootDir);
        printf("---------------------------------------------------------------------------------------------\n\n");

        // Passo 3. Exibindo conteudo do primeiro arquivo encontrado.
        printf("Informe o indice do arquivo que deseja exibir ou [-1] para sair: ");
        scanf("%d", &archive_index);
        getchar();

        system(clearscreen);
        printArchive(fp, &boot_record, &positions, rootDir[archive_index], archive_index);

        getchar();
    } while (archive_index != -1);

    fclose(fp);
    return 0;
}

// Método para exibir as informacoes do Boot Record.
void printBootRecord(FILE *fp, fat_BS_t *boot_record)
{
    fseek(fp, 0, SEEK_SET);                      // Configura o ponteiro no inicio do arquivo.
    fread(boot_record, sizeof(fat_BS_t), 1, fp); // Realiza a leitura do boot record para a estrutura.

    printf("\nBPB (BIOS Parameter Block) Informations:\n\n");

    printf("Boot Jump -------------------------> %x %x %x\n", boot_record->bootjmp[0], boot_record->bootjmp[1], boot_record->bootjmp[2]);

    printf("Identificador OEM -----------------> ");
    for (int i = 0; i < 8; i++)
        printf("%x ", boot_record->oem_name[i]);
    printf("\n");

    printf("Bytes per sector-------------------> %hd\n", boot_record->bytes_per_sector);
    printf("Sector per cluster-----------------> %x\n", boot_record->sectors_per_cluster);

    printf("Number of reserved sectors---------> %hd\n", boot_record->reserved_sector_count);
    printf("Number of FAT tables---------------> %x\n", boot_record->table_count);
    printf("Number of root entries-------------> %hd\n", boot_record->root_entry_count);

    printf("Number of sectors 16---------------> %hu\n", boot_record->total_sectors_16);
    printf("Media descryptor type--------------> %x\n", boot_record->media_type);
    printf("Number of sectores per FAT table---> %hd\n", boot_record->table_size_16);
    printf("Number of sectors per track--------> %hd\n", boot_record->sectors_per_track);
    printf("Number of heads or sides-----------> %hd\n", boot_record->head_side_count);
    printf("Number of hidden sectors-----------> %d\n", boot_record->hidden_sector_count);
    printf("Number of sectors 32---------------> %u\n\n", boot_record->total_sectors_32);
}

// Método para salvar as posicoes --> Boot Record | FAT1 | FAT2 | Root Dir | Dados
void setDirectoriesPositions(fat_BS_t *boot_record, dirPosition *positions)
{
    // Posicao do Boot Record.
    positions->boot_record = 0;

    // Posicao FAT1 -> 512 * 1
    positions->fats[0] = boot_record->bytes_per_sector * boot_record->sectors_per_cluster;

    // Posicao FAT2 -> 512 + (155 * 512)
    positions->fats[1] = positions->fats[0] + (boot_record->table_size_16 * boot_record->bytes_per_sector);

    // Posicao Root Directory -> 79360 + (155 * 512)
    positions->root_directory = positions->fats[1] + (boot_record->table_size_16 * boot_record->bytes_per_sector);

    printf("Directories Positions:\n");
    printf("Boot Record: 0x%x | FAT1: 0x%x | FAT2: 0x%x | Root Dir: 0x%x\n\n", positions->boot_record, positions->fats[0], positions->fats[1], positions->root_directory);
}

// Metodo que armazena e exibe as entradas validas do Root Dir.
void printRootEntries(FILE *fp, dirPosition *positions, standardDir *rootDir)
{
    // Posicionando o ponteiro no Root Directory.
    fseek(fp, positions->root_directory, SEEK_SET);

    int entry = 0; // Counter para cada entrada
    while (1)
    {
        // Leitura da entrada no formato Standard 8.3 Format
        fread(&rootDir[entry], sizeof(standardDir), 1, fp);

        // Unused Entrie.
        if (rootDir[entry].file_name[0] == 0xe5)
            continue;
        // Long File Name
        if (rootDir[entry].file_attribute == 0x0f)
            continue;
        // Entrada Vazia.
        if (rootDir[entry].file_name[0] == 0x0)
            break;

        // Imprime a entrada.
        printf("Entry: %d | File Name: %s.%s | Attribute: 0x%x | Size: %d | First Cluster: %x\n", entry, rootDir[entry].file_name, rootDir[entry].extension, rootDir[entry].file_attribute, rootDir[entry].file_size, rootDir[entry].low_bits_first_cluster);

        printf("\n\n");

        entry++;
    }
}

// Metodo para exibir o conteudo do primeiro arquivo do disco.
void printArchive(FILE *fp, fat_BS_t *boot_record, dirPosition *positions, standardDir rootDir, int index)
{
    if (rootDir.file_attribute != 0x20)
    {
        printf("\nArquivo nao Encontrado!\n\n");
        return;
    }

    printf("\n----------------------------------------DADOS DO ARQUIVO----------------------------------------\n\n");

    // Numero de Setores do Root Dir.
    int root_directory_sectors = (boot_record->root_entry_count * 32) / boot_record->bytes_per_sector;

    // Inicio da Area de Dados do primeiro arquivo.
    positions->data_area = ((boot_record->reserved_sector_count + (boot_record->table_count * boot_record->table_size_16) + root_directory_sectors) + ((int)rootDir.low_bits_first_cluster - 2) * boot_record->sectors_per_cluster) * boot_record->bytes_per_sector;

    printf("Entry: %d | File Name: %s.%s | Attribute: 0x%x | Size: %d | First Cluster: %x\n\n", index, rootDir.file_name, rootDir.extension, rootDir.file_attribute, rootDir.file_size, rootDir.low_bits_first_cluster);

    // Posicionando o ponteiro no primeiro cluster do arquivo na FAT1.
    int relative_cluster_position = (positions->fats[0] + ((int)rootDir.low_bits_first_cluster) * 2);
    fseek(fp, relative_cluster_position, SEEK_SET);

    // Armazena o dado da entrada do cluster da FAT1.
    unsigned short next_cluster;

    // Primeira posicao do cluster absoluto que contem o inicio dos dados do arquivo.
    int absolute_cluster_position = positions->data_area;

    // Numero de setores que o arquivo ocupa.
    int total_sector_archive = rootDir.file_size / (boot_record->reserved_sector_count * boot_record->bytes_per_sector);

    // Bytes de setor incompleto.
    int total_incomplete_sector_bytes = rootDir.file_size - (total_sector_archive * (boot_record->reserved_sector_count * boot_record->bytes_per_sector));

    printf("Setores Inteiros: %d | Restante: %d bytes\n", (int)total_sector_archive, total_incomplete_sector_bytes);
    printf("Data Area Starts: %x", positions->data_area);

    // Arquivo ocupa setores completos?
    if (total_sector_archive > 0)
    {
        int cluster_counter = 0;
        int previous_cluster;
        while (cluster_counter < (int)total_sector_archive)
        {
            // Realiza a leitura para o proximo cluster.
            previous_cluster = next_cluster;
            fread(&next_cluster, sizeof(unsigned short), 1, fp);

            printf("\n\nRelative: %d | Proximo Cluster: %x\n", relative_cluster_position, next_cluster);
            printf("Absolute Position: %x\n\n", absolute_cluster_position);

            // Posiciona o ponteiro na posicao inicial do cluster absoluto atual.
            fseek(fp, absolute_cluster_position, SEEK_SET);
            unsigned char dado;
            for (int i = 0; i < (boot_record->bytes_per_sector * boot_record->reserved_sector_count); i++)
            {
                fread(&dado, sizeof(unsigned char), 1, fp);
                printf("%c", dado);
            }

            if (next_cluster == 0xffff)
            {
                printf("\n\nRelative: %d | Proximo Cluster: %x\n\n", relative_cluster_position, next_cluster);
                break;
            }

            // Atualiza a posicao do cluster absoluto.
            if ((next_cluster - previous_cluster) > 1)
            {
                // Clusters nao sequenciais.
                absolute_cluster_position = ((positions->data_area / boot_record->bytes_per_sector) + (next_cluster - 2) - 2) * boot_record->bytes_per_sector;
            }
            else
            {
                // Clusters Sequenciais.
                absolute_cluster_position += boot_record->bytes_per_sector * boot_record->reserved_sector_count;
            }

            // Avanca para o proximo cluster relativo.
            relative_cluster_position = (positions->fats[0] + (next_cluster * 2));
            fseek(fp, relative_cluster_position, SEEK_SET);

            cluster_counter++;
        }
    }

    // Bytes remanescentes em setores nao totalmente utilizados?
    if (total_incomplete_sector_bytes != 0)
    {
        // Realiza a leitura para o proximo cluster.
        fread(&next_cluster, sizeof(unsigned short), 1, fp);

        printf("\n\nRelative: %d | Proximo Cluster: %x\n", relative_cluster_position, next_cluster);
        printf("Absolute Position: %x\n\n", absolute_cluster_position);

        // Posiciona o ponteiro na posicao inicial do cluster absoluto atual.
        fseek(fp, absolute_cluster_position, SEEK_SET);

        unsigned char dado;
        for (int i = 0; i < total_incomplete_sector_bytes; i++)
        {
            fread(&dado, sizeof(unsigned char), 1, fp);
            printf("%c", dado);
        }
    }
}
