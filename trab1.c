//
//  main.c
//  ori-trab-1
//
//  Created by Luan Batista on 30/10/17.
//  Copyright © 2017 Luan Batista. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define REGISTER_SIZE  100            // Tamanho de cada registro
#define HEADER_SIZE    30             // Tamanho do Header dos arquivo de dados
#define BLOCK_SIZE     512            // Tamanho do bloco
#define REGISTER_BLOCK 5              // Quantidade de registro por bloco

#define DATABASE_NAME   "banco.txt"
#define INDEX_NAME      "index.txt"
#define LISTA_REGISTROS "/Users/luanbatista/Desktop/Projetos/ori-trab-1/ufscar-ori/listaregistros.txt"

typedef struct HeaderFile {            // Estrutura para o cabeçalho do arquivo de dados
    int block_rrn_last;                // último bloco utilizado
    int database_size;                 // Quantidade de registros válidos
    int free_next_rrn;                 // RRN para o próximo bloco com registros livres para gravação
} HeaderFile;

typedef struct Register {              // Estrutura para cada registro de dados
    unsigned int key;                  // Chave do registro                            | 32bits -  4bytes
    char name[95];                     // Nome                                         |          96bytes
    char null;                         // Controle de registro | Deletedo: 0; Ativo: 1 |           1byte
} Register;

typedef struct Block {                 // Estrutura para cada bloco de registros
    unsigned int free_next_rrn;        // RRN para o próximo bloco com registros livres para gravação
    Register regs[REGISTER_BLOCK];     // Mapeia cada registro do bloco
} Block;

typedef struct HeaderIndex {           // Estrutra para o cabeçalho do arquivo de index
    unsigned int count;                // Total de index contidos no arquivo
} HeaderIndex;

typedef struct Index {                 // Estrutura para cada index
    unsigned int key;                  // Chave primária
    unsigned int rrn;                  // RRN para o bloco do registro
} Index;


FILE *_databese;                       // Ponteiro para o arquivo de dados
FILE *_index;                          // Ponteiro para o arquivo de index
HeaderFile *_buffer_header;            // Define o buffer do header do arquivo de dados
HeaderIndex *_buffer_index_header;     // Define o buffer do header do arquivo de index
Index *_buffer_index;                  // Define o buffer dos de index
int _initialized = 0;                  // Verifica se o arquivo foi carregado

/*
 *  Atualiza o arquivo de dados com o cabeçalho atualizado
 */
void updateBufferHeader() {
    fseek(_databese, 0, SEEK_SET);                                    // Move o ponteiro do arquivo para o inicio
    fwrite(_buffer_header, 1, sizeof(struct HeaderFile), _databese);  // Atualiza o header do arquivo de acordo com o buffer
    rewind(_databese);                                                // Atualiza o arquivo no disco
}

/*
 *  Posiciona o curso do arquivo de dados no inicio no próximo bloco
 *    com espaço disponível, controlando o aproveitamento de espaço
 */
int setBeginCurrentBlock() {
    if(_buffer_header->free_next_rrn > 0) {                                     // Caso houve alguma remoção e ainda tenha espaço para reaproveitar
        fseek(_databese, BLOCK_SIZE * _buffer_header->free_next_rrn, SEEK_SET); // Aponta o ponteiro para o bloco com espaço
        return _buffer_header->free_next_rrn;                                   // retorno o RRN do bloco
    }else{                                                                      // Caso contrário aponta para o último bloco
        fseek(_databese, BLOCK_SIZE * _buffer_header->block_rrn_last, SEEK_SET);// Aponta o ponteiro para o último bloco
        return _buffer_header->block_rrn_last;                                  // retorna o RRN do block
    }
}

/*
 *  Posiciona o curso do arquivo de dados em um dados RRN de bloco
 */
void setBlockRRN(int rrn) {
    fseek(_databese, rrn * BLOCK_SIZE, SEEK_SET);                                // Posiciona o ponteiro de acordo com o RRN informado
}

/*
 *  Atualiza o header do arquivo de index
 */
void updateBufferIndex() {
    fseek(_index, 0, SEEK_SET);                                                  // Aponta para o inicio do arquivo do index
    fwrite(_buffer_index_header, 1, sizeof(struct HeaderIndex), _index);         // Salva o buffer do header no arquivo
    rewind(_index);                                                              // Atualiza o arquivo no disco
}

/*
 *  Atualiza o arquivo de index com os dados do buffer
 */
void updateIndex() {
    fseek(_index, sizeof(struct HeaderIndex), SEEK_SET);                                  // Movo o ponteiro para o incio da área de dados do arquivo
    fwrite(_buffer_index, 1, sizeof(struct Index) * _buffer_index_header->count, _index); // Atualiza o arquivo de index de acordo com o buffer
    rewind(_index);                                                                       // Atualiza o arquivo no disco
}

/*
 *  Carrega o header do arquivo de index para o buffer em memória primária
 */
int loadHeaderIndexBuffer() {
    _buffer_index_header = malloc(sizeof(struct HeaderIndex));
    fseek(_index, 0, SEEK_SET);
    //printf("DEBUG || Header Index Lidos: %ld\n", fread(_buffer_index_header, 1, sizeof(struct HeaderIndex), _index));
    return 1;
}

/*
 *  Carrega o arquivo de index para o buffer em memória primária
 */
int loadIndexBuffer() {
    int sizeBuffer = _buffer_index_header->count * sizeof(struct Index);
    _buffer_index = malloc(sizeBuffer);
    fseek(_index, sizeof(struct HeaderIndex), SEEK_SET);
    //printf("DEBUG || Lidos: %ld\n", fread(_buffer_index, 1, sizeBuffer, _index));
    return 1;
}

void debugCurrentCursor() {
    printf("DEBUG || Posição atual: %ld \n", ftell(_databese));
}

void debugCurrentCursorMessage(char message[100]) {
    printf("DEBUG || Posição atual: %ld | %s\n", ftell(_databese), message);
}

void debugMessage(char message[100]) {
    printf("DEBUG || %s\n", message);
}

/*
 *  Inicializa os arquivos de index e de dados inicializando os seus respectivos Header
 */
int initialize () {
    
    _databese = fopen(DATABASE_NAME, "w+b");                  // Deve ser um arquivo binário de escrita e leitura
    _index    = fopen(INDEX_NAME, "w+b");                     // Deve ser um arquivo binário de escrita e leitura
    if (_databese == NULL || _index == NULL) {                // Verifica se os arquivos não foram criados
        _initialized = 0;                                     // Seta a variável de controle como não inicializada
        fclose(_databese);                                    // Fecha o arquivo
        fclose(_index);                                       // Fecha o arquivo
        return -1;                                            // Retorna -1 para indicar que a operação não ocorreu com sucesso
    }
    
    _buffer_header = malloc(sizeof(struct HeaderFile));       // Cria o buffer do header do arquivo de dados
    _buffer_header->block_rrn_last  = 1;                      // Inicia a contagem dos blocos a partir do RRN 1, pois o 0 é para o Header
    _buffer_header->database_size   = 0;                      // Inicia a contagem de registros ativos em 0
    _buffer_header->free_next_rrn   = -1;                     // Inicia com -1 para indicar que não tem blocos para reaproveitamento
    
    updateBufferHeader();                                     // Atualiza o header do arquivo com base no buffer
    
    Block *block = calloc(1, sizeof(struct Block));           // Cria um novo bloco de registros na inicialização
    
    setBeginCurrentBlock();                                   // Posiciona o ponteiro do arquivo para o próximo bloco disponível
    fwrite(block, 1, BLOCK_SIZE, _databese);                  // Escreve o bloco inicializado no arquivo
    rewind(_databese);                                        // Atualiza o arquivo no disco
    
    _buffer_index_header = malloc(sizeof(struct HeaderIndex));// Inicializa o buffer do header do index
    _buffer_index_header->count = 0;                          // Inicia a contagem em 0
    
    free(block);                                              // Elimana o bloco vazio criado da memória
    return _initialized = 1;                                  // retorna 1 para indicar que tudo está ok
}

/*
 *  Busca binária no arquivo de index para localizar um determinado index com base na chave informada
 */
int search(int key, int begin, int end) {
    
    if (_buffer_index_header->count == 0)                  // Caso seja o primeiro index
        return -1;
    
    int div = ceil(((double)end - begin) / 2);             // Calcula a posição central do bloco que vai ser percorrido
    
    int currentKey = (_buffer_index + (begin + div))->key; // Captura no buffer de index a chave na posição central
    //printf("DEBUG || Current Key %i | div %i | begin %i | end %i | teste %f\n", currentKey, div, begin, end, ((double)1/2));
    if (currentKey == key)                                 // Verifica se é a chave procurada
        return (div + begin);                              // Retorna a posição da chave atual
    else if (begin >= end)                                 // Caso busca termine
        return -1;                                         // Retorna -1 para indicar que a busca não encontrou
    else {                                                 // Caso a busca deva continuar
        if (currentKey > key)                              // Caso a chave atual seja maior que a chave pesquisada
            return search(key, begin, (begin+end)/2);      // Avança para a esquerda
        else                                               // Caso a chave atual seja menor que a chave pesquisada
            return search(key, end - ((end - begin) / 2), end); // Avança para a direita
    }
    return -1;                                             // Retorna -1 para indicar que a busca não finalizou
}

/*
 *  Busca um registro com base na chave informada
 */
int findRegistroByKey(int key, Register *reg) {
    
    int position = search(key, 0, _buffer_index_header->count-1);
    
    if (position < 0) return -1;
    
    setBlockRRN((_buffer_index+position)->rrn);
    Block *block = malloc(sizeof(struct Block));
    fread(block, 1, BLOCK_SIZE, _databese);
    
    for(int i=0; i<REGISTER_BLOCK; i++){
        
        if(block->regs[i].null == 1 && block->regs[i].key == key) {
            printf("NULL: %i\n", block->regs[i].null);
            memcpy(reg, &block->regs[i], sizeof(struct Register));
            return 1;
        }
    }
    return -1;
}

/*
 *  Remove um registro com base na chave informada
 */
int removeRegistroByKey(int key, Register *reg) {
    
    int position = search(key, 0, _buffer_index_header->count-1);     // Busca a posição do index no arquivo de Index
    int currentRRN = (_buffer_index+position)->rrn;                   // RRN do bloco onde o registro esta o registro que vai ser rem.
    int tempRRN = _buffer_header->free_next_rrn;                      // Variável responsavel pela troca dos RRN de referencia
    setBlockRRN(currentRRN);                                          // Move o pornteiro do arquivo para o bloco correspondente ao rrn
    Block *block = malloc(sizeof(struct Block));                      // Ponteiro para o bloco de dados na memória primária
    fread(block, 1, BLOCK_SIZE, _databese);                           // Carrega o bloco de dados
    
    for(int i=0; i<REGISTER_BLOCK; i++){                              // Percorre todos os registro do bloco
        
        if(block->regs[i].null == 1 && block->regs[i].key == key) {   // Verifica se o registro é valido e se tem a mesma chave
            memcpy(reg, &block->regs[i], sizeof(struct Register));    // Copia os dados do registro atual para o ponteiro de retorno
            block->regs[i].null = 0;                                  // Atualiza o registro como removido
            block->free_next_rrn = tempRRN;                           // Relaciona os blocos com registros removimos em lista
            _buffer_header->free_next_rrn = currentRRN;               // Relaciona os blocos com registros removidos em lista
            _buffer_header->database_size--;                          // Decrementa a quantidade de registros ativos
            setBlockRRN(currentRRN);                                  // Positiona o ponteiro do arquivo par ao inicio do bloco atual
            fwrite(block, 1, BLOCK_SIZE, _databese);                  // Salva o bloco de dados com a alteração
            updateBufferHeader();                                     // Atualiza o Header do Arquivo de dados
            return 1;
        }
    }
    free(block);                                                      // Remove o bloco da memória primária
    return -1;                                                        // Caso a operação não tenha sucesso
}

/*
 *  Captura a posição no buffer de index onde um novo index deve ser inserido já ordenado
 */
int getInsertionPosition(int key, int begin,int end) {
    
    if (_buffer_index_header->count == 0)                             // Caso seja o primeiro index
        return 0;                                                     // Retorna a posição zero
    
    int div = ceil(((double)end - begin) / 2);                        // Calcula o posição central da lista
    
    int currentKey = (_buffer_index + (begin + div))->key;            // Captura o valor da chave central
    //printf("DEBUG || Current Key %i | div %i | begin %i | end %i | teste %f\n", currentKey, div, begin, end, ((double)1/2));
    
    if (currentKey == key)                                            // Se a chave atual for a chave procurada
        return -3;                                                    // -3 Index já inserido
    else if (begin >= end)                                            // Caso toda lista sejá percorrida
        return begin;                                                 // Retorna ultima posição pesquisada
    else {                                                            // Se a chave atual for diferente da chave procurada
        if (currentKey > key)                                         // Caso a chave atual for maior que a procurada
            return getInsertionPosition(key, begin, (begin+end)/2);
        else                                                          // Caso a chave atual for maior que a procurada
            return getInsertionPosition(key, end - ((end - begin) / 2), end);
    }
    return -4;                                                        // Fim da execução sem resultado
}

/*
 *  Insere um novo index no buffer de index e no arquivo de index
 */
int insertIndex(Index *newIndex) {
    int postion;                                                                         // Posição onde o index deve ser inserido
    Index *temp = malloc(sizeof(struct Index) * (_buffer_index_header->count + 1));      // Temp para a lista Index com mais espaço
    postion = getInsertionPosition(newIndex->key, 0, _buffer_index_header->count-1);     // Captura a posição onde o index deve ser ins.
    
    if (postion < 0) return -1;                                                          // Posição negativa, a lista ta vazia
    
    if (_buffer_index_header->count > 0 && newIndex->key > (_buffer_index+postion)->key) // Inserir a esquerda ou a diretira
        postion++;                                                                       // Caso for a direita, incrementa a posição
    
    int sizeBegin = sizeof(struct Index) * postion;                                      // Calcula o tamanho a esquerda
    int sizeEnd = sizeof(struct Index) * (_buffer_index_header->count - postion);        // Calcula o tamanho a direita
    
    memcpy(temp, _buffer_index, sizeBegin);                                              // Copia para temp a parte da esquerada
    memcpy(temp + postion, newIndex, sizeof(struct Index));                              // Inseri o novo index
    memcpy(temp + postion + 1, _buffer_index + postion, sizeEnd);                        // Copia para temp a parte da direita
    
    _buffer_index_header->count++;                                                       // Atualiza o Header do index
    
    free(_buffer_index);                                                                 // Limpa o buffer atual dos index
    _buffer_index = malloc(sizeof(struct Index) * (_buffer_index_header->count));        // Recalcula o tamanho da lista de index
    memcpy(_buffer_index, temp, sizeof(struct Index) * _buffer_index_header->count);     // Atualiza o buffer com a nova lista
    
    updateBufferIndex();                                                                 // Atualiza o Header do Index
    updateIndex();                                                                       // Atualiza o Index
    return 1;
}

/*
 *  Insere um novo registro no arquivo de dados
 */
int insert(Register *reg) {
    
    if(!_initialized) return -1;                                     // Verifica se os arquivos foram carregados
    
    Block *block = malloc(sizeof(struct Block));                     // Cria um bloco na memória primária
    
    int currentRRN = setBeginCurrentBlock();                         // Desloca até o último bloco utilizado
    fread(block, 1, BLOCK_SIZE, _databese);                          // Carrega o último bloco utilizado na memória principal
    setBeginCurrentBlock();                                          // Desloca até o inicio do último bloco novamente
    
    for(int i=0;i<REGISTER_BLOCK;i++){                               // Percorre pelos registros do bloco atual
        
        if(block->regs[i].null == 0) {                               // Verifica se o espeço atual do bloco está disponível
            
            Index *index = malloc(sizeof(struct Index));             // Cria o Index do registro que esta sendo inserido
            index->key = reg->key;                                   // Armazena a chave do registro que esta sendo inserido
            index->rrn = _buffer_header->block_rrn_last;             // Armazena no Index o RRN do bloco corrente
            
            if(insertIndex(index) < 0) {                             // Insere o index do novo registro e verif. se esta ok
                free(index);
                free(block);
                return -1;
            }
            reg->null = 1;                                           // Ativa o registro antes de inserir
            memcpy(block->regs+i, reg, sizeof(struct Register));     // Copia o registro inderido para o bloco onde sera salvo
            
            // Verifica se é o último registro do bloco e o ult. bloco é o bloco corrente
            if(i == (REGISTER_BLOCK-1) && _buffer_header->block_rrn_last == currentRRN) {
                
                if (_buffer_header->free_next_rrn == currentRRN) {   // Esta em modo de aproveitamento
                    _buffer_header->free_next_rrn = block->free_next_rrn;
                    block->free_next_rrn = 0;
                }
                fwrite(block, 1, BLOCK_SIZE, _databese);             // Atualiza o bloco de dados no arquivo
                _buffer_header->block_rrn_last++;                    // Caso o bloco for preenchido com o máximo de registros
                Block *nullBlock = calloc(1, sizeof(struct Block));  // Cria um novo block vázio para as próximas inserções
                fwrite(nullBlock, 1, BLOCK_SIZE, _databese);         // Grava o novo bloco vázio no arquivo
                free(nullBlock);                                     // Limpa a memória removendo o nullBlock
            } else if(i == (REGISTER_BLOCK-1)) {
                _buffer_header->free_next_rrn = block->free_next_rrn;
                block->free_next_rrn = 0;
                fwrite(block, 1, BLOCK_SIZE, _databese);             // Atualiza o bloco de dados no arquivo
            } else {
                fwrite(block, 1, BLOCK_SIZE, _databese);             // Atualiza o bloco de dados no arquivo
            }
            _buffer_header->database_size++;                         // Incrementa a quantidade de registros ativos na base
            updateBufferHeader();                                    // Atualiza o Header do arquivo de dados
            free(index);                                             // Limpa a memória removendo o index
            break;                                                   // Finaliza a execução do for
        }
    }
    free(block);                                                     // Limpa a memória removendo o block
    return 1;
}

/*
 *  Insere um novo registro no arquivo de dados
 */
int edit(Register *reg) {
    
    if(!_initialized) return -1;                                     // Verifica se os arquivos foram carregados
  
    int position = search(reg->key, 0, _buffer_index_header->count-1);
    
    if (position < 0) return -1;
      
    setBlockRRN((_buffer_index+position)->rrn);
    Block *block = malloc(sizeof(struct Block));
    fread(block, 1, BLOCK_SIZE, _databese);
    
    for(int i=0; i<REGISTER_BLOCK; i++){
        
        if(block->regs[i].null == 1 && block->regs[i].key == reg->key) {
            memcpy(&block->regs[i], reg, sizeof(struct Register));
            break;
        }
    }
    setBlockRRN((_buffer_index+position)->rrn);
    fwrite(block, 1, BLOCK_SIZE, _databese);                         // Atualiza o bloco de dados no arquivo
    free(block);                                                     // Limpa a memória removendo o block
    return 1;
}

/*
 *   Importa a lista de registros já preenchida
 */
int importList (int number) {
    FILE *import = fopen(LISTA_REGISTROS, "r");
    
    if (import == NULL) {
        printf("Não foi possível abrir o arquivo...\n");
        return -1;
    }
    
    char buffer[200];                                 // Buffer que carrega os bytes de cada linha do arquivo
    int count = 0;                                    // Contador de registros que já foram importados
    int i, v=0, n=0;                                  // Variáveis de controle 
    char tempKey[10];                                 // Variável que guarda o chave antes de converter para inteiro
    
    while (fgets(buffer, sizeof(buffer), import) != NULL && number > count) {                
        
        Register *reg = calloc(1, sizeof(struct Register));

        for(i = 0; i<strlen(buffer); i++){            // Percorre cada byte da linha
            if(buffer[i] == ',')                      // Verifica se encontrou a virgula que separa a chave do nome
                v = i;                                // Registra a posição da virgula
            if(buffer[i] == '\n') {                   // Verifica se encontrou o fim da linha
                n = i-2;                              // Registra a posição do último byte ignorando a quebra de linha
                break;                                // Finaliza a execução do FOR
            }
        }
        printf("%s\n", buffer);
        memcpy(tempKey, buffer, sizeof(char)*v);             // Copia somento a chave para a variável temporária
        reg->key  = atoi(tempKey);                           // Converte a chave para inteiro e armazena no registro
        memcpy(reg->name, buffer+v+1, sizeof(char) * (n-v)); // Armazena somento o nome no registro
        insert(reg);                                         // Insere o registro no arquivo
        free(reg);                                           // Limpa o registro da memória primária
        count++;                                             // Incremente o contador de registros inseridos
    }
    
    fclose(import);                                          // Fecha o arquivo de importação
    return 1;
}

/*
 *  Lista a estrutura de blocos
 */
void listAllBlocks() {
    
    if(!_initialized) return;                                             // Verifica se os arquivos foram carregados
    Block *block = malloc(sizeof(struct Block));                          // Ponteiro na memória primária para os blocos de dados
    rewind(_databese);                                                    // Atualiza os dados do arquivo e aponta para o início
    fseek(_databese, BLOCK_SIZE, SEEK_SET);                               // Move o ponteiro do arquivo para o primeiro bloco de regis.
    
    printf("    HEADER FILE INFO\n");
    printf("--------------------------------------------------------\n");
    printf("Record Count:  %6i\n", _buffer_header->database_size);
    printf("Last Block:    %6i\n", _buffer_header->block_rrn_last);
    printf("Queue pointer: %6i\n", _buffer_header->free_next_rrn);
    printf("--------------------------------------------------------\n");
    printf("Block   |   Register Status   |   Queue Next Pointer\n");
    for(int i = 1; i <= _buffer_header->block_rrn_last; i++) {            // Percorre até o último bloco utilizado
        fread(block, 1, BLOCK_SIZE, _databese);                           // Carrega na memória primária o bloco corrente
        printf(" %6i |   ", i);
        for(int j=0;j<REGISTER_BLOCK;j++){                                // Percorre todos os registro do bloco em memória primária
            if(block->regs[j].null == 1)                                  // Exibe apenas os registro validos
                printf("[O]");
            else
                printf("[X]");
        }
        printf("   |   %5i\n",block->free_next_rrn);
    }
    free(block);                                                          // Remove da memória primária o bloco
}

/*
 *  Lista todos os registro ativos do arquivo de dados
 */
void listAll() {
    
    if(!_initialized) return;                                             // Verifica se os arquivos foram carregados
    Block *block = malloc(sizeof(struct Block));                          // Ponteiro na memória primária para os blocos de dados
    rewind(_databese);                                                    // Atualiza os dados do arquivo e aponta para o início
    fseek(_databese, BLOCK_SIZE, SEEK_SET);                               // Move o ponteiro do arquivo para o primeiro bloco de regis.
    printf("  Chave |   Nome\n");
    printf("---------------------------------------------------------\n");
    for(int i = 1; i <= _buffer_header->block_rrn_last; i++) {            // Percorre até o último bloco utilizado
        fread(block, 1, BLOCK_SIZE, _databese);                           // Carrega na memória primária o bloco corrente
        for(int j=0;j<REGISTER_BLOCK;j++){                                // Percorre todos os registro do bloco em memória primária
            if(block->regs[j].null == 1) {                                // Exibe apenas os registro validos
                printf("  %5i |   %s \n", block->regs[j].key, block->regs[j].name);
            }
        }
    }
    free(block);                                                          // Remove da memória primária o bloco
}

/*
 *  Lista todos os index do arquivo de index (DEBUG)
 */
void listIndexAll() {
    
    if(!_initialized) return;
    Index *index = malloc(sizeof(struct Index));
    rewind(_index);
    fseek(_index, sizeof(struct HeaderIndex), SEEK_SET);
    
    printf("    HEADER INDEX FILE INFO\n");
    printf("--------------------------------------------------------\n");
    printf("Record Count:  %6i\n", _buffer_index_header->count);
    printf("--------------------------------------------------------\n");
    printf("  Key   |   RRN Block  \n");
    for(int i = 0; i < _buffer_index_header->count; i++) {                // Percorre todos os registro de index em memória primária
        fread(index, 1, sizeof(struct Index), _index);
        printf(" %5i  |   %6i\n", index->key, index->rrn);
    }
    free(index);                                                          // Remove da memória primária o index
}

/*
 *  Exibe o menu de opeções do programa
 */
void showOption() {
    printf("      PROJETO ORI - MANIPULAÇÃO DE ARQUIVOS \n");
    printf("---------------------------------------------------\n");
    printf(" 0 - Novo arquivo vázio\n");
    printf(" 1 - Inserir um novo registro\n");
    printf(" 2 - Remover um registro\n");
    printf(" 3 - Listar Registros\n");
    printf(" 4 - Buscar um registro\n");
    printf(" 5 - Compactar arquivo de dados\n");
    printf(" 6 - Importar Lista\n");
    printf(" 7 - Listar estrutura de blocos\n");
    printf(" 8 - Listar estrutura de indices\n");
    printf(" 9 - Editar um registro\n");
    printf("10 - Sair\n");
}

/*
 *  Limpa a tela do console
 */
void clearDisplay() {
    for(int i = 0; i < 40; i++)
        printf("\n");
}

int loadLastRegister(Register *reg) {
    int regs_acitve = 0;                                                     // Quantidade de registros ativos no block
    int reg_return = -1;                                                     // Indice do registro que vai ser retornado
    Block *last_block = calloc(1, sizeof(struct Block));
    
    fseek(_databese, BLOCK_SIZE * _buffer_header->block_rrn_last, SEEK_SET); // Posiciona no último bloco de registro
    fread(last_block, 1, BLOCK_SIZE, _databese);                             // Carrega o último bloco na memória primária
    
    for(int i = 0; i < REGISTER_BLOCK; i++) {                                // Percorre todos os registros do bloco
        if(last_block->regs[i].null == 1) {                                  // Verifica se o registro está ativo
            if(reg_return == -1) reg_return = i;                             // Verifica o primeiro indice ativo
            regs_acitve++;                                                   // Conta os registros ativos
        }
    }
    
    if(regs_acitve == 0) {                                                   // Caso o último bloco esteja completamente vazio
        _buffer_header->block_rrn_last--;
        return loadLastRegister(reg);
    }
    
    memcpy(reg, &last_block->regs[reg_return], sizeof(struct Register));     // Copia o registro atual para o retorno
    last_block->regs[reg_return].null = 0;                                   // Remove logicamente o registro atual
    regs_acitve--;                                                           // Decrementa o contador de registros ativos
    
    fseek(_databese, BLOCK_SIZE * _buffer_header->block_rrn_last, SEEK_SET); // Posiciona no último bloco de registro
    
    if(reg_return == 0) {                                                    // Verifica se o bloco foi removido completamente
        fwrite("", 1, BLOCK_SIZE, _databese);                              // Elimina o bloco do arquivo
        _buffer_header->block_rrn_last--;                                    // Decrementa o mapeamento do ultimo bloco no buffer
        updateBufferHeader();                                                // Atualiza o hearde com base no buffer
    } else {
        fwrite(last_block, 1, BLOCK_SIZE, _databese);                        // Salva o bloco atualizado no arquivo
    }

    return regs_acitve;                                                      // Retorno a quantidade de registro ativos no bloco
}

int compactar() {
    int position_index;                                           // Posição do index, do registro movido, no buffer de index
    Block *current_block = calloc(1, sizeof(struct Block));
    Register *reg = calloc(1, sizeof(struct Register));
    
    if(_buffer_header->free_next_rrn <= 0) return -1;             // Arquivo já compactado
    
    for(int i = 1; i < _buffer_header->block_rrn_last; i++) {     // Enquanto houver blocos com espaço para reaproveitamento
        
        fseek(_databese, BLOCK_SIZE * i, SEEK_SET);               // Posiciona o ponteiro nos blocos de forma sequencial
        fread(current_block, 1, BLOCK_SIZE, _databese);           // Carrega o bloco na memória primária
        current_block->free_next_rrn = 0;                         // Zera a fila de blocos com registros removidos
        
        for(int j = 0; j < REGISTER_BLOCK; j++) {                 // Percorre todos os registros do bloco
            if(current_block->regs[j].null != 1) {                // Verifica se o registro foi removido
                loadLastRegister(reg);                            // Carrega o último registro do arquivo
                memcpy(&current_block->regs[j], reg, sizeof(struct Register));
                
                position_index = search(reg->key, 0, _buffer_index_header->count-1); // Captura a posição do registro no buffer de ind.
                (_buffer_index+position_index)->rrn = j;          // Atualiza o rrn do registro no buffer de index
            }
        }
        fseek(_databese, BLOCK_SIZE * i, SEEK_SET);               // Posiciona o ponteiro nos blocos de forma sequencial (volta)
        fwrite(current_block, 1, BLOCK_SIZE, _databese);          // Grava o bloco de volta no arquivo
    }
    _buffer_header->free_next_rrn = 0;                            // Atualiza o header do arquivo limpando a fila de reg. remov.
    updateBufferHeader();                                         // Atualiza o arquivo de dados com o buffer do header atuaizado
    updateIndex();                                                // Atualiza o arquivo de index
    return 1;
}

int main () {
    int control = 1;
    char confirm;
    Register *reg = malloc(sizeof(struct Register));
    initialize();

    while (control >= 0) {
        showOption();
        scanf("%i", &control);
        
        switch (control) {
            case 0:
                clearDisplay();
                printf("   REINICAR ARQUIVOS DE INDEX E DE DADOS\n");
                printf("--------------------------------------------------\n");
                printf("Todos os dados serão perdidos. Deseja continuar? (Y/n) ");
                scanf("%c \n", &confirm);
                if(strncmp("Y", &confirm, 1))
                    initialize();
                break;
            case 1:
                clearDisplay();
                printf("   CADASTRAR UM NOVO REGISTRO\n");
                printf("--------------------------------------------------\n");
                printf("Digite a chave: ");
                scanf("%u", &reg->key);
                printf("Digite o nome: ");
                scanf("%s", reg->name);
                printf("--------------------------------------------------\n");
                if(insert(reg) < 0)
                    printf("Chave duplicada. Não foi possível inserir o registro.\n");
                else
                    printf("Registro inserido com sucesso.\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 2:
                clearDisplay();
                printf("   REMOVER UM REGISTRO\n");
                printf("--------------------------------------------------\n");
                printf("Digite a chave: ");
                scanf("%u", &reg->key);
                if(removeRegistroByKey(reg->key, reg) < 0)
                    printf("Não foi possível remover o registro\n");
                else
                    printf("Registro removido\n");
                printf("--------------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                break;
            case 3:
                clearDisplay();
                printf("   LISTA DE REGISTROS ATIVOS\n");
                printf("--------------------------------------------------\n");
                listAll();
                printf("--------------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 4:
                clearDisplay();
                printf("   BUSCAR UM REGISTRO\n");
                printf("--------------------------------------------------\n");
                printf("Digite a chave: ");
                scanf("%u", &reg->key);
                if(findRegistroByKey(reg->key, reg) > 0)
                    printf("Key: %i - Nome: %s\n", reg->key, reg->name);
                else
                    printf("Registro não localizado..");
                printf("--------------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 5:
                clearDisplay();
                printf("   COMPACTAR O ARQUIVO DE DADOS\n");
                printf("--------------------------------------------------\n");
                printf("Isso pode levar um tempo. Deseja continuar? (Y/n) ");
                scanf("%c \n", &confirm);
                if(strncmp("Y", &confirm, 1))
                    compactar();
                break;
            case 6:
                clearDisplay();
                printf("   IMPORTAR LISTA DE REGISTROS (Máx. 1694)\n");
                printf("--------------------------------------------------\n");
                printf("Digite a quantidade: ");
                scanf("%i", &control);
                importList(control);
                printf("--------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 7:
                clearDisplay();
                printf("  LISTA BLOCOS\n");
                printf("--------------------------------------------------------\n");
                listAllBlocks();
                printf("--------------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 8:
                clearDisplay();
                printf("  LISTA INDICES\n");
                printf("--------------------------------------------------------\n");
                listIndexAll();
                printf("--------------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 9:
                clearDisplay();
                printf("  EDIÇÃO DE REGISTRO\n");
                printf("--------------------------------------------------\n");
                printf("Digite a chave: ");
                scanf("%u", &reg->key);
                if(findRegistroByKey(reg->key, reg) > 0) {
                    printf("Key: %i - Nome: %s\n", reg->key, reg->name);
                    printf("Digite o novo nome: ");
                    scanf("%s", reg->name);
                    if(edit(reg) < 0)
                        printf("Não foi possível alterar o registro\n");
                    else
                        printf("Registro alterado\n");
                } else
                    printf("Registro não localizado..");
                printf("--------------------------------------------------------\n");
                printf("Pressione qualquer tecla para sair");
                fpurge(stdin);
                scanf("%c", &confirm);
                break;
            case 10:
                control = -1;
                break;
            default:
                break;
        }
        fpurge(stdin);
        clearDisplay();
    }
    
    fclose(_databese);
    fclose(_index);
    return 1;
}
