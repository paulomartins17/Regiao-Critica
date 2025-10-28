#include <stdio.h>
#include <stdlib.h>     // Para exit()
#include <unistd.h>     // Para fork(), getpid()
#include <sys/wait.h>   // Para wait()
#include <sys/shm.h>    // Para shmget(), shmat(), shmdt(), shmctl()
#include <sys/stat.h>   // Para S_IRUSR, S_IWUSR (permissões)
#include <semaphore.h>  // Para sem_t, sem_init(), sem_wait(), sem_post(), sem_destroy()
#include <fcntl.h>      // Para O_CREAT

// Vamos criar uma estrutura para guardar TUDO
// que precisa ser compartilhado.
struct MemoriaCompartilhada {
    int contador;   // O contador que ambos vão incrementar
    sem_t semaforo; // O semáforo que protege o contador
};

int main() {
    int shmid; // ID da memória compartilhada
    struct MemoriaCompartilhada *mem_comp; // Ponteiro para nossa struct

    // --- 1. CONFIGURANDO A MEMÓRIA COMPARTILHADA ---

    // shmget: "Me dê um segmento de memória"
    // IPC_PRIVATE: Chave única, só nós (pai/filho) vamos usar
    // sizeof(...): O tamanho que eu preciso
    // IPC_CREAT | 0666: Crie se não existir e dê permissão de leitura/escrita
    shmid = shmget(IPC_PRIVATE, sizeof(struct MemoriaCompartilhada), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("Erro no shmget!");
        exit(1);
    }

    // shmat: "Anexa" a memória ao nosso processo
    // O SO nos devolve um ponteiro (void*) que convertemos
    mem_comp = (struct MemoriaCompartilhada *) shmat(shmid, NULL, 0);
    if (mem_comp == (void *) -1) {
        perror("Erro no shmat!");
        exit(1);
    }

    // --- 2. INICIALIZANDO OS DADOS COMPARTILHADOS ---

    // Inicializamos os valores DENTRO da memória compartilhada
    mem_comp->contador = 0; // Contador começa em 0

    // sem_init: Inicializa o semáforo
    // &mem_comp->semaforo: Onde ele está
    // 1: (pshared) 1 = compartilhado entre processos
    // 1: (value)   1 = valor inicial (a "chave" começa disponível)
    if (sem_init(&mem_comp->semaforo, 1, 1) < 0) {
        perror("Erro no sem_init!");
        exit(1);
    }

    // --- 3. O FORK ---

    pid_t pid = fork();

    if (pid == 0) {
        // -------------------------------------
        // Bloco do FILHO
        // -------------------------------------
        printf("FILHO (PID %d): Comecei...\n", getpid());
        for (int i = 0; i < 100000; i++) {
            
            // --- INÍCIO DA REGIÃO CRÍTICA ---
            sem_wait(&mem_comp->semaforo); // Pede a "chave" (espera se necessário)
            
            mem_comp->contador++; // Só eu estou mexendo nisso agora
            
            sem_post(&mem_comp->semaforo); // Devolve a "chave"
            // --- FIM DA REGIÃO CRÍTICA ---
        }
        printf("FILHO: Terminei!\n");
        exit(0); // Filho termina

    } else if (pid > 0) {
        // -------------------------------------
        // Bloco do PAI
        // -------------------------------------
        printf("PAI (PID %d): Comecei...\n", getpid());
        for (int i = 0; i < 100000; i++) {
            
            // --- INÍCIO DA REGIÃO CRÍTICA ---
            sem_wait(&mem_comp->semaforo); // Pede a "chave"
            
            mem_comp->contador++; // Só eu estou mexendo nisso agora
            
            sem_post(&mem_comp->semaforo); // Devolve a "chave"
            // --- FIM DA REGIÃO CRÍTICA ---
        }
        printf("PAI: Terminei!\n");

        // --- 4. ESPERAR E LIMPAR TUDO ---
        
        wait(NULL); // Espera o filho terminar

        printf("\n--- Resultado Final ---\n");
        printf("Valor final do contador compartilhado: %d\n", mem_comp->contador);

        // Destruímos o semáforo
        sem_destroy(&mem_comp->semaforo);

        // Desanexamos a memória
        shmdt(mem_comp);

        // Mandamos o SO marcar a memória para destruição
        shmctl(shmid, IPC_RMID, NULL);

    } else {
        perror("Erro no fork!");
        exit(1);
    }

    return 0;
}