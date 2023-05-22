#define _POSIX_C_SOURCE 199309L
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdatomic.h>

int Id;
int *A;
int M;
int Id2;
atomic_int *pravo;
atomic_int *zas[2];
int j;

void ulaz(int i, int j)
{
    // printf("k.o. ulaz\n");

    *zas[i] = 1;
    while (*zas[j] != 0)
    {
        if (*pravo == j)
        {
            *zas[i] = 0;
            while (*pravo == j)
            {
                /*nista*/
            }
            *zas[i] = 1;
        }
    }
}

// proces koji koristi zajedničke varijable
void proces(int i)
{

    if (i == 1)
        j = 0;
    else if (i == 0)
    {
        j = 1;
    }

    for (int t = 0; t < M; t++)
    {
        ulaz(i, j);

        // krit_odsj();
        *A += 1;
        // printf("%d \n", *A);
        //   printf("\n");

        // izlaz(i, j);
        *pravo = j;
        *zas[i] = 0;
    }
}

int main()
{
    printf("Unesi broj ponavljanja M: ");
    scanf("%d", &M);

    // stvaranje segmenta (blok zajednickog spremnika )
    Id = shmget(IPC_PRIVATE, sizeof(int), 0600);
    if (Id == -1)
    {
        exit(1);
    }
    //  Proces veže segment na svoj adresni prostor sa shmat:
    A = (int *)shmat(Id, NULL, 0);
    *A = 0;

    Id2 = shmget(IPC_PRIVATE, sizeof(atomic_int) * 3, 0600);
    if (Id2 == -1)
    {
        exit(1);
    }
    pravo = (atomic_int *)shmat(Id2, NULL, 0);
    zas[0] = (atomic_int *)pravo + 1;
    zas[1] = (atomic_int *)pravo + 2;

    // pravljenje djece i pozivanje fje
    int N = 2; // stvaranje dva procesa tj. dva
    int i;
    for (i = 0; i < N; i++)
        switch (fork())
        {
        case 0:
            // funkcija koja obavlja posao djeteta
            proces(i);
            exit(0);
        case -1:
            // ispis poruke o nemogućnosti stvaranja procesa;
            printf("Ne mogu stvoriti novi proces!\n");
            exit(0);
        default:
            break;
        }

    // roditelj prikuplja svoju djecu
    while (i--)
    {
        wait(NULL);
    }

    printf("\n%d\n", *A);

    // oslobadanje zajednicke meorije
    // Segment se može otpustiti sustavskim pozivom shmdt
    (void)shmdt((char *)A);
    (void)shmdt((char *)pravo);
    (void)shmdt((char *)zas[0]);
    (void)shmdt((char *)zas[1]);
    //(void) shmdt(segment) ;
    // Uništavanje segmenta zajedničke memorije izvodi se sustavskim pozivom shmctl
    (void)shmctl(Id, IPC_RMID, NULL);
    exit(0);
}