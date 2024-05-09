#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BLOCK_SIZE 512   // Tamanho de cada bloco em bytes.
#define RES_BLCK_COUNT 1 // Numero de blocos reservados.
#define BM_BLCK_COUNT 2  // Numero de blocos para o bitmap.
#define ET_BLCK_COUNT 10 // Numero de blocos para a tabela de entradas.

// ESTRUTURAS:

// BootRecord.
typedef struct
{
    unsigned short bytes_per_block;               // offset 0  - 2 bytes. -> 512 bytes
    unsigned short reserved_blocks_count;         // offset 2  - 2 bytes. -> 1 block
    unsigned int number_of_bitmap_blocks;         // offset 4  - 4 bytes.
    unsigned int number_of_entry_table_blocks;    // offset 8  - 4 bytes.
    unsigned int number_of_data_section_blocks;   // offset 12 - 4 bytes.
    unsigned int number_of_total_system_blocks;   // offset 16 - 4 bytes.
    unsigned int number_of_root_directory_blocks; // offset 20 - 4 bytes.
    unsigned short first_root_directory_block;    // offset 24 - 2 bytes.
    unsigned char empty[6];
} __attribute__((__packed__)) BootRecord;

// Entradas.
typedef struct
{
    unsigned char status;              // offset 0  - 1 byte.
    unsigned char entry_name[12];      // offset 1  - 12 bytes.
    unsigned char entry_extension[4];  // offset 13 - 4 bytes.
    unsigned char file_attribute;      // offset 17 - 1 byte.
    unsigned int first_block;          // offset 18 - 4 bytes.
    unsigned int size_in_bytes;        // offset 22 - 4 bytes.
    unsigned int total_used_blocks;    // offset 26 - 4 bytes.
    unsigned char empty[2];            // offset 30 - 2 bytes.
} __attribute__((__packed__)) Entries; // total size -> 32 bytes.

// Mapa de Bits.
typedef struct
{
    unsigned char *blocks; // Sequencia de bytes representando os blocos.
    unsigned int size;  
} Bitmap;

// Data Section.
typedef struct
{
    unsigned char *data; // Area de dados.
    unsigned int size;
} DataArea;

// Estrutura para os Diretorios
typedef struct Directory
{
    char directory_name[50];
    struct Directory *subdirectories;
    struct Directory *next;
    
} Directory;

// Variaveis Globais.
BootRecord boot_record;
Bitmap bitmap;
Entries *table_entries;
DataArea data_area;
Directory *root_directory = NULL;

// Prototipos de Funcoes.
void loadFileSystemImage(const char *isoName);
void createFileSystemImage(unsigned int partition_size);
void formatFileSystem(unsigned int partition_size);
void navigate();
void clearInputBuffer();
void copyFileToFS(const char *source_file);
void copyFileFromFS(const char *entry_name);
void listFiles();
void deleteFile(const char *entry_name);
void addDirectory(const char *directory_name);
void listDirectoryTree(Directory *root, int level);
void saveFileSystemImage();

int main()
{
    char isoName[30]; // Armazena o nome da imagem do sistema.

    printf("Informe o nome da imagem a ser carregada (Nome invalido criara novo): ");
    fgets(isoName, 30, stdin);
    isoName[strcspn(isoName, "\n")] = '\0'; // Remove a quebra de linha.

    // Ou cria nova imagem ou carrega imagem existente.
    loadFileSystemImage(isoName);
    // Manipulacao do sistema de arquivos.
    navigate();
    // Salva alteracoes realizadas.
    saveFileSystemImage();

    // Libera a memoria das estruturas alocadas dinamicamente.
    free(bitmap.blocks);
    free(table_entries);
    free(data_area.data);

    return 0;
}

// Carrega ou cria a imagem do sistema de arquivos.
void loadFileSystemImage(const char *isoName)
{
    FILE *file = fopen(isoName, "rb");
    if (file == NULL)
    {
        printf("Nenhuma imagem do sistema de arquivos encontrada. Criando uma nova imagem...\n");
        createFileSystemImage(1000); // Tamanho padrao de 1000 blocos.
        return;
    }

    // Leitura do boot record.
    fread(&boot_record, sizeof(BootRecord), 1, file);

    // Inicializacao do bitmap.
    bitmap.blocks = (unsigned char *)malloc(boot_record.number_of_total_system_blocks * sizeof(unsigned char));
    bitmap.size = boot_record.number_of_total_system_blocks * BLOCK_SIZE;

    // Leitura do bitmap.
    fread(bitmap.blocks, sizeof(unsigned char), boot_record.number_of_total_system_blocks, file);

    // Inicializacao para a tabela de entradas.
    table_entries = (Entries *)malloc(ET_BLCK_COUNT * BLOCK_SIZE);
    if (table_entries == NULL)
    {
        printf("Erro ao alocar memoria para a tabela de entradas.\n");
        exit(1);
    }

    // Leitura da tabela de entradas.
    fread(table_entries, sizeof(Entries), ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries), file);

    // Aloca espaco para a area de dados
    data_area.data = (unsigned char *)malloc(boot_record.number_of_data_section_blocks * BLOCK_SIZE);
    if (data_area.data == NULL)
    {
        printf("Erro ao alocar memoria para a area de dados.\n");
        exit(1);
    }
    data_area.size = boot_record.number_of_data_section_blocks * BLOCK_SIZE;

    // Leitura da area de dados.
    fread(data_area.data, sizeof(unsigned char), boot_record.number_of_data_section_blocks * BLOCK_SIZE, file);

    fclose(file);

    printf("Imagem do sistema de arquivos carregada com sucesso.\n");
}

// Metodo que cria uma imagem do sistema de arquivos.
void createFileSystemImage(unsigned int partition_size)
{
    formatFileSystem(partition_size);

    printf("Sistema de arquivos criado com sucesso.\n");
}

// Metodo que formata o sistema de arquivos.
void formatFileSystem(unsigned int partition_size)
{
    // Preenche o boot record.
    boot_record.bytes_per_block = BLOCK_SIZE;
    boot_record.reserved_blocks_count = RES_BLCK_COUNT;
    boot_record.number_of_bitmap_blocks = BM_BLCK_COUNT;
    boot_record.number_of_entry_table_blocks = ET_BLCK_COUNT;
    boot_record.number_of_data_section_blocks = partition_size - RES_BLCK_COUNT - BM_BLCK_COUNT - ET_BLCK_COUNT;
    boot_record.number_of_total_system_blocks = partition_size;
    boot_record.number_of_root_directory_blocks = 1;
    boot_record.first_root_directory_block = RES_BLCK_COUNT + BM_BLCK_COUNT + ET_BLCK_COUNT;

    // Aloca espaco para o bitmap.
    bitmap.blocks = (unsigned char *)malloc(partition_size * sizeof(unsigned char));
    bitmap.size = partition_size;

    // Preenche o bitmap.
    memset(bitmap.blocks, 0, partition_size * sizeof(unsigned char));

    // Aloca espaco para a tabela de entradas.
    table_entries = (Entries *)malloc(ET_BLCK_COUNT * BLOCK_SIZE);
    if (table_entries == NULL)
    {
        printf("Erro ao alocar memoria para a tabela de entradas.\n");
        exit(1);
    }

    // Preenche as entradas da tabela de diretorios.
    memset(table_entries, 0, ET_BLCK_COUNT * BLOCK_SIZE);

    // Aloca espaco para a area de dados.
    data_area.data = (unsigned char *)malloc((partition_size - RES_BLCK_COUNT - BM_BLCK_COUNT - ET_BLCK_COUNT) * BLOCK_SIZE);
    if (data_area.data == NULL)
    {
        printf("Erro ao alocar memoria para a area de dados.\n");
        exit(1);
    }
    data_area.size = (partition_size - RES_BLCK_COUNT - BM_BLCK_COUNT - ET_BLCK_COUNT) * BLOCK_SIZE;

    // Limpa a area de dados.
    memset(data_area.data, 0, (partition_size - RES_BLCK_COUNT - BM_BLCK_COUNT - ET_BLCK_COUNT) * BLOCK_SIZE);
}

// Metodo que navega pelo sistema de arquivos.
void navigate()
{
    unsigned int partition_size;
    int option;

    do
    {

        printf("\nSistema de Arquivos - Opcoes:\n");
        printf("[1] - Copiar arquivo para o sistema de arquivos\n");
        printf("[2] - Copiar arquivo do sistema de arquivos para o disco rigido\n");
        printf("[3] - Listar arquivos do sistema de arquivos\n");
        printf("[4] - Remover arquivo do sistema de arquivos\n");
        printf("[5] - Adicionar diretorio\n");
        printf("[6] - Listar diretorios\n");
        printf("[7] - Formatar Particao\n");
        printf("[8] - Sair\n");
        printf("Escolha uma opcao: ");
        scanf("%d", &option);
        clearInputBuffer();

        switch (option)
        {
        case 1:
        {
            char file[50];

            printf("Digite o nome do arquivo a ser copiado para o sistema de arquivos: ");
            fgets(file, 50, stdin);
            file[strcspn(file, "\n")] = 0; // Remove a quebra de linha do final.

            // Copia arquivo do disco para o sistema de arquivos.
            copyFileToFS(file);
            break;
        }
        case 2:
        {
            char entry_name[50];

            printf("Digite o nome do arquivo a ser copiado do sistema de arquivos para o disco rigido: ");
            fgets(entry_name, 50, stdin);
            entry_name[strcspn(entry_name, "\n")] = 0; // Remove a quebra de linha do final.

            // Copia o arquivo do sistema de arquivos para o disco rigido.
            copyFileFromFS(entry_name);
            break;
        }
        case 3:
            // Exibe os arquivos.
            listFiles();
            break;
        case 4:
        {
            char entry_name[50];

            printf("Digite o nome do arquivo a ser removido: ");
            fgets(entry_name, 50, stdin);
            entry_name[strcspn(entry_name, "\n")] = 0; // Remove a quebra de linha do final.

            // Remove o arquivo.
            deleteFile(entry_name);
            break;
        }
        case 5:
        {
            char directory_name[50];

            printf("Digite o nome do diretorio a ser adicionado: ");
            fgets(directory_name, 50, stdin);
            directory_name[strcspn(directory_name, "\n")] = 0; // Remove a quebra de linha do final.

            // Cria novo diretorio.
            addDirectory(directory_name);
            break;
        }
        case 6:
            // Exibe todos os diretorios.
            printf("\nArvore de diretorios:\n");
            listDirectoryTree(root_directory, 0);
            break;
        case 7:
            printf("Informe o tamanho em setores da particao: ");
            scanf("%u", &partition_size);

            createFileSystemImage(partition_size);
            break;
        case 8:
            printf("Encerrando...\n");
            break;
        default:
            printf("Opcao invalida.\n");
            break;
        }
    } while (option != 8);
}

// Limpa o buffer de entrada.
void clearInputBuffer()
{
    int c;
    while ((c = getchar()) != '\n' && c != EOF)
        ;
}

// Metodo que copia um arquivo do disco rigido para o sistema de arquivos.
void copyFileToFS(const char *source_file)
{
    // Abre o arquivo fonte.
    FILE *source = fopen(source_file, "rb");
    if (source == NULL)
    {
        printf("Erro ao abrir o arquivo fonte.\n");
        return;
    }

    // Verifica o tamanho do arquivo.
    fseek(source, 0, SEEK_END);
    long file_size = ftell(source);
    rewind(source);

    // Calcula o numero de blocos necessarios.
    int required_blocks = (file_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    // Procura blocos contiguos disponiveis no bitmap.
    int start_block = -1;
    for (int i = 0; i < boot_record.number_of_total_system_blocks - required_blocks + 1; i++)
    {
        int j;
        for (j = 0; j < required_blocks; j++)
        {
            if (bitmap.blocks[i + j] != 0)
                break;
        }
        if (j == required_blocks)
        {
            start_block = i;
            for (int k = 0; k < required_blocks; k++)
            {
                bitmap.blocks[start_block + k] = 1; // Marca os blocos como ocupados.
            }
            break;
        }
    }

    if (start_block == -1)
    {
        printf("Nao ha espaco contiguo disponivel no sistema de arquivos.\n");
        fclose(source);
        return;
    }

    // Copia o conteudo do arquivo para a area de dados.
    fread(&data_area.data[start_block * BLOCK_SIZE], sizeof(unsigned char), file_size, source);

    fclose(source);

    // Adiciona uma entrada para o arquivo na tabela de diretorios.
    int entry_index = 0;
    while (table_entries[entry_index].status != 0)
    {
        entry_index++;
        if (entry_index >= ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries))
        {
            printf("Tabela de entradas cheia.\n");
            return;
        }
    }

    strcpy(table_entries[entry_index].entry_name, source_file);
    table_entries[entry_index].status = 1;
    table_entries[entry_index].file_attribute = 1;
    table_entries[entry_index].first_block = start_block;
    table_entries[entry_index].size_in_bytes = file_size;

    printf("Arquivo copiado para o sistema de arquivos com sucesso.\n");
}

// Metodo que copia um arquivo do sistema de arquivos para o disco rigido.
void copyFileFromFS(const char *entry_name)
{
    // Procura o arquivo na tabela de entradas.
    int entry_index = -1;
    for (int i = 0; i < ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries); i++)
    {
        if (table_entries[i].status != 0 && strcmp(table_entries[i].entry_name, entry_name) == 0)
        {
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1)
    {
        printf("Arquivo nao encontrado no sistema de arquivos.\n");
        return;
    }

    // Abre o arquivo destino no disco rigido.
    char dest_file[50];
    strcpy(dest_file, "copia_");
    strcat(dest_file, entry_name);

    FILE *dest = fopen(dest_file, "wb");
    if (dest == NULL)
    {
        printf("Erro ao abrir o arquivo de destino.\n");
        return;
    }

    // Escreve o conteudo do arquivo na area de dados.
    fwrite(&data_area.data[table_entries[entry_index].first_block * BLOCK_SIZE], sizeof(unsigned char), table_entries[entry_index].size_in_bytes, dest);

    fclose(dest);

    printf("Arquivo copiado para o disco rigido com sucesso.\n");
}

// Metodo que lista os arquivos armazenados no sistema de arquivos.
void listFiles()
{
    printf("\nArquivos armazenados no sistema de arquivos:\n");

    for (int i = 0; i < ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries); i++)
    {
        if (table_entries[i].status == 1 && table_entries[i].file_attribute == 1)
        {
            printf("%s\n", table_entries[i].entry_name);
        }
    }
}

// Metodo que remove um arquivo do sistema de arquivos.
void deleteFile(const char *entry_name)
{
    // Procura o arquivo na tabela de entradas.
    int entry_index = -1;
    for (int i = 0; i < ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries); i++)
    {
        if (table_entries[i].status != 0 && strcmp(table_entries[i].entry_name, entry_name) == 0)
        {
            entry_index = i;
            break;
        }
    }

    if (entry_index == -1)
    {
        printf("Arquivo nao encontrado.\n");
        return;
    }

    // Libera os blocos de dados ocupados pelo arquivo no bitmap.
    int blocks_to_free = (table_entries[entry_index].size_in_bytes + BLOCK_SIZE - 1) / BLOCK_SIZE;
    for (int i = 0; i < blocks_to_free; i++)
    {
        bitmap.blocks[table_entries[entry_index].first_block + i] = 0;
    }

    // Limpa a entrada da tabela de entradas.
    memset(&table_entries[entry_index], 0, sizeof(Entries));

    printf("Arquivo removido.\n");
}

// Metodo que adiciona um diretorio.
void addDirectory(const char *directory_name)
{
    int entry_index = 0;
    while (table_entries[entry_index].status != 0)
    {
        entry_index++;
        if (entry_index >= ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries))
        {
            printf("Tabela de entradas cheia.\n");
            return;
        }
    }

    strcpy(table_entries[entry_index].entry_name, directory_name);
    strcpy(table_entries[entry_index].entry_extension, "");

    table_entries[entry_index].status = 1;
    table_entries[entry_index].file_attribute = 2; // Marca como direterio.
    table_entries[entry_index].first_block = -1;
    table_entries[entry_index].size_in_bytes = 0;

    Directory *new_directory = (Directory *)malloc(sizeof(Directory));
    if (new_directory != NULL)
    {
        strcpy(new_directory->directory_name, directory_name);

        new_directory->subdirectories = NULL;
        new_directory->next = NULL;

        if (root_directory == NULL)
        {
            root_directory = new_directory;
        }
        else
        {
            Directory *temp = root_directory;
            while (temp->next != NULL)
            {
                temp = temp->next;
            }
            temp->next = new_directory;
        }
    }
}

// Metodo que lista os diretorios e arquivos em forma de arvore.
void listDirectoryTree(Directory *root, int level)
{
    // Itera sobre a tabela de entradas/
    for (int i = 0; i < ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries); i++)
    {
        // Verifica se a entrada e valida e se pertence ao diretorio atual.
        if (table_entries[i].status != 0 && table_entries[i].file_attribute == 2)
        {
            if (table_entries[i].file_attribute == 2)
            {
                // Imprime o diretorio.
                for (int j = 0; j < level; j++)
                    printf("  ");
                printf("- %s\n", table_entries[i].entry_name);
            }
            else // Caso contrario, a um arquivo (nao um diretorio).
            {
                // Imprime o arquivo.
                for (int j = 0; j < level; j++)
                    printf("  ");
                printf("- %s.%s\n", table_entries[i].entry_name, table_entries[i].entry_extension);
            }
        }
    }
}

// Salva a imagem do sistema de arquivos em um arquivo binario.
void saveFileSystemImage()
{
    FILE *file = fopen("filesystem.img", "wb");
    if (file == NULL)
    {
        printf("Erro ao salvar a imagem do sistema de arquivos.\n");
        return;
    }

    // Escreve o boot record.
    fwrite(&boot_record, sizeof(BootRecord), 1, file);

    // Escreve o bitmap.
    fwrite(bitmap.blocks, sizeof(unsigned char), bitmap.size, file);

    // Escreve a tabela de entradas.
    fwrite(table_entries, sizeof(Entries), ET_BLCK_COUNT * BLOCK_SIZE / sizeof(Entries), file);

    // Escreve a area de dados.
    fwrite(data_area.data, sizeof(unsigned char), data_area.size, file);

    fclose(file);

    printf("Imagem do sistema de arquivos salva com sucesso.\n");
}