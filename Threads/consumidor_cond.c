/****************************************************************************
 * Problema clássico do produtor e consumidor, no caso de concorrência      *
 *  de multiprocessamento produtores e consumidores acessam a mesma         *
 *  memoria compartilhada, porém sem operações atômicas por causa da        *
 *  mudança de contexto o programa pode tomar rumos e valores indefinidos   *
 *                                                                          *
 * Nessa implementação será utilizado mutex para manipular a sessão critica *
 *  no caso o vetor de produção, no qual através de dois índices faz a      *
 *  leitura (consumo) e escrita (produção), após cada produtores produzirem *
 *  'LIMIT_PROD' eles encerram, os consumidores consumem o resto de produto *
 *  se existir e finalizam, assim esse programa é finalizado.               *
 *                                                                          *
 * ** GCC incluir a biblioteca pthread através do comando '-lpthread'       *
 * ** Este Programa Finaliza.                                               *
 *************************************************************************** */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>   /* time() */
#include <unistd.h> /* usleep() | sleep() */

/* Um elemento será inutilizavel para distinguir vazio e cheio.  */
#define MAX_PROD 21   /* Número de slots disponiveis pra produzir  */
#define LIMIT_PROD 10 /* Limite de produtos produzidos por cada produtor */

#define NUM_PROD 10 /* Número de Thread rodando função 'void *produtor(void)'       */
#define NUM_CONS 7  /* Número de Thread rodando função 'void *consumidor(void)'     */

pthread_mutex_t mutex_m, fim_m;      /* Sessão Critica acesso ao vetor 'produtos' e variáveis de índices */
pthread_cond_t prod_cond, cons_cond; /* Índices de controle dos produtores e consumidores sobre o vetor 'produtos' */

unsigned int len_cons = 0; /* Índice de consumo no vetor 'produtos' (sessão critica) */
unsigned int len_prod = 0; /* Índice de produção no vetor 'produtos' (sessão critica) */

unsigned char fim_flag = 0; /* Flag para encerrar consumidores (fim de todo consumo e fim dos produtores) */

unsigned int prod_cont[NUM_PROD]; /* Número de produtos produzido por cada thread produtora */

int produtos[MAX_PROD]; /* Vetor de produção (sessão critica) */

void *produtor(void *num_thread)
{
    for (;;)
    {
        usleep(((rand() % 5) + 1) * 100000);

        /* Sessão critica (Exclusão Mútua)*/
        pthread_mutex_lock(&mutex_m);

        /* Vetor cheio aguardando por pelo menos um consumidor */
        while ((len_prod + 1) % MAX_PROD == len_cons)
            pthread_cond_wait(&prod_cond, &mutex_m);

        /* Inserindo um valor aleatório entre 1 e 10 (simulando a produção) */
        produtos[len_prod] = rand() % 10 + 1;
        printf("Produzindo: %02d, Pos: %02d, Thread: %02ld (%02d/%02d)\n", produtos[len_prod],
               len_prod, *((size_t *)num_thread) + 1, prod_cont[*((size_t *)num_thread)] + 1, LIMIT_PROD);
        len_prod = (len_prod + 1) % MAX_PROD;

        /* Contador de produção dessa Thread */
        prod_cont[*((size_t *)num_thread)]++;

        /* Produção inserida (libera pelo menos um consumidor) */
        pthread_cond_signal(&cons_cond);

        /* Verifica limite de produção */
        if (prod_cont[*((size_t *)num_thread)] == LIMIT_PROD)
        {
            pthread_mutex_unlock(&mutex_m);
            printf("Fim do produtor: %02ld\n", *((size_t *)num_thread) + 1);
            return NULL;
        }

        /* Fim da sessão critica (Exclusão Mútua)*/
        pthread_mutex_unlock(&mutex_m);
    }
}

void *consumidor(void *num_thread)
{
    for (;;)
    {
        usleep(((rand() % 5) + 1) * 100000);

        /* Sessão critica (Exclusão Mútua)*/
        pthread_mutex_lock(&mutex_m);

        /* Vetor vazio aguardando por pelo menos um produtor */
        while (len_cons == len_prod)
        {
            /* Verficia encerramento dos produtores */
            pthread_mutex_lock(&fim_m);
            if (fim_flag)
            {
                pthread_mutex_unlock(&fim_m);
                pthread_mutex_unlock(&mutex_m);
                printf("Fim do consumidor: %02ld\n", *((size_t *)num_thread) + 1);
                return NULL;
            }
            pthread_mutex_unlock(&fim_m);
            /* Aguarda produção (Produtores existentes ainda) */
            pthread_cond_wait(&cons_cond, &mutex_m);
        }

        /* Consumindo (simulando o consumo) */
        printf("Consumindo: %02d, pos: %02d, Thread: %02ld\n", produtos[len_cons],
               len_cons, *((size_t *)num_thread) + 1);
        len_cons = (len_cons + 1) % MAX_PROD;

        /* Consumido (libera um produtor caso esses já tenham enchido o vetor) */
        pthread_cond_signal(&prod_cond);

        /* Fim sessão critica (Exclusão Mútua)*/
        pthread_mutex_unlock(&mutex_m);
    }
}

int main(int argc, char const *argv[])
{
    /* Threads da produção e consumidores */
    pthread_t prodT[NUM_PROD], consT[NUM_CONS];

    /* Enumera cada Thread produtora pra contar produção de cada */
    size_t num_prod_thread[NUM_PROD], num_cons_thread[NUM_CONS];

    /* Semente aleatória (clock cpu) */
    srand(time(NULL));

    /* Zera contadores dos produtores (opcional, safyte code) */
    memset(prod_cont, 0, sizeof(prod_cont));

    /* Inicialização da Mutex e Mutex condicionais */
    pthread_mutex_init(&mutex_m, NULL);
    pthread_mutex_init(&fim_m, NULL);
    pthread_cond_init(&prod_cond, NULL);
    pthread_cond_init(&cons_cond, NULL);

    printf("Começa\n");

    /* Inicialização das Threads (inicia condições de corrida) */
    for (size_t i = 0; i < NUM_CONS; i++)
    {
        num_cons_thread[i] = i;
        pthread_create((consT + i), NULL, (void *)&consumidor, (void *)(num_cons_thread + i));
    }
    for (size_t i = 0; i < NUM_PROD; i++)
    {
        num_prod_thread[i] = i;
        pthread_create((prodT + i), NULL, (void *)&produtor, (void *)(num_prod_thread + i));
    }

    /* Aguarda fim das Threads produtoras */
    for (size_t i = 0; i < NUM_PROD; i++)
        pthread_join(prodT[i], NULL);

    /* Sinaliza fim da produção para consumidores */
    pthread_mutex_lock(&fim_m);
    fim_flag = 1;
    pthread_mutex_unlock(&fim_m);

    /*
        Contexto: Elas podem ser liberada pois existe um while 'while (len_cons == len_prod)'
        no qual irá mater elas sem consumir o vetor, somente consultando a flag 'fim_flag',
        as que estiverem mantendo o wait condicional 'cons_cond'.
    */
    /* Livra possíveis Thread consumidoras do bloqueio de falta de produtos, necessário para finalizarem */
    pthread_mutex_lock(&mutex_m); // Possivel condição de corrida (Thread dentro da sessão critica porém antes do wait)
    pthread_cond_broadcast(&cons_cond);
    pthread_mutex_unlock(&mutex_m);

    /* Aguarda fim das Threads consumidoras */
    for (size_t i = 0; i < NUM_CONS; i++)
        pthread_join(consT[i], NULL);

    printf("Fim\n");

    return 0;
}
