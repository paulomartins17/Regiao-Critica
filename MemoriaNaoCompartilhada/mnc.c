#include <stdio.h>      // Para printf()
#include <unistd.h>     // Para fork() e getpid()
#include <sys/types.h>  // Para pid_t
#include <sys/wait.h>   // Para wait()

int main() {
    // 1. A variável é inicializada ANTES do fork.
    //    Ambos os processos começarão com contador = 0.
    int contador = 0;

    pid_t pid = fork(); // Cria o processo filho

    if (pid == 0) {
        // -------------------------------------
        // Bloco do FILHO
        // -------------------------------------
        for (int i = 0; i < 100000; i++) {
            contador++;
        }
        printf("FILHO (PID %d): Terminei! Meu contador é: %d\n", getpid(), contador);

    } else if (pid > 0) {
        // -------------------------------------
        // Bloco do PAI
        // -------------------------------------
        for (int i = 0; i < 100000; i++) {
            contador++;
        }
        printf("PAI (PID %d): Terminei! Meu contador é: %d\n", getpid(), contador);
        
        // Boa prática: O pai espera o filho terminar
        // para não deixar um processo "zumbi".
        wait(NULL); 

    } else {
        // Bloco de ERRO
        perror("Erro no fork!");
        return 1;
    }

    return 0;
}