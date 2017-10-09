#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define REGISTER_SIZE  100
#define HEADER_SIZE    30
#define BLOCK_SIZE     512
#define REGISTER_BLOCK 5

#define DATABASE_NAME "banco.txt"
#define INDEX_NAME    "index.txt"

FILE *_databese;
int _initialized = 0;    // Verifica se o arquivo foi carregado

typedef struct HeaderFile {
    int block_rrn_last;
    int block_size_used;
    int database_size;
} HeaderFile;

typedef struct Register {
    unsigned int key;    // 16bits - 2bytes
    char name[98];       // 98bytes
} Register;

int initialize () {
    
    _databese = fopen(DATABASE_NAME, "wb"); // Deve ser um arquivo binário
                                            // Deve iniciar um novo arquivo sempre que aberto
    if (_databese == NULL) {
        _initialized = 0;
        return -1;
    }
    
    HeaderFile *header = malloc(sizeof(struct HeaderFile));
    header->block_rrn_last  = 32;
    header->block_size_used = 3;
    header->database_size   = 89;
    
    // Error
    fwrite(header, 1, sizeof(struct HeaderFile), _databese);
    free(header);
    return _initialized = 1;
}

int insert(Register *reg) {
    
    if(!_initialized) return -1;
    
    // Get Header
    HeaderFile *header = malloc(sizeof(struct HeaderFile));
    Register *regs = malloc(sizeof(struct Register)*REGISTER_BLOCK);
    
    // Error
    fread(header, 1, sizeof(struct HeaderFile), _databese);
    
    // Verifica se é possível inserir um novo registro no block
    if(REGISTER_BLOCK < header->block_size_used) {
        
        fread(regs, 1, BLOCK_SIZE, _databese);                     // Carrega todos registros do bloco
        memcpy(&regs[header->block_size_used*REGISTER_SIZE],
                                              reg, REGISTER_SIZE); // Copia o novo registro na última posição livre
        fseek(_databese, -1*BLOCK_SIZE, SEEK_CUR);                 // Posiciona o ponteiro no inicio do bloco
        fwrite(regs, 1, BLOCK_SIZE, _databese);                       // Salva o bloco no disco
        
        header->block_size_used++;                                 // Atualiza a quantidade de registros do bloco
        header->database_size++;                                   // Atualiza a quantidade de registros
        rewind(_databese);                                         // Reposiciona no inicio do arquivo
        fwrite(header, 1, sizeof(struct HeaderFile), _databese);   // Atualiza o Header
 
    }
    
    printf("Block RRN Size: %i", header->block_rrn_last);
    printf("Block Size Used: %i", header->block_size_used);
    printf("Database Size: %i \n", header->database_size);
    
    free(header);
    free(regs);
    return 1;
}


int main () {
    initialize();
    
    Register *reg = malloc(sizeof(struct Register));
    reg->key = 999;
    strcpy(reg->name,"Luan Batista");
    
    insert(reg);
    
    return 1;
}
