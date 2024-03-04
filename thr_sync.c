#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <stdarg.h>
#include <stdnoreturn.h>
#include <pthread.h>
#include <string.h>

// Quelques macros classiques, mais qui rendent bien service
#define TCHK(op)        do { if ((errno = (op)) > 0) raler (1, #op); } while (0)
#define CHKN(op)        do { if ((op) == NULL) raler (1, #op); } while (0)

#define BUFSIZE 40

// Une fonction "raler" admettant un nombre variable d'arguments
// L'argument syserr doit être à 1 s'il s'agit d'une erreur système
// ou à 0 si c'est une erreur applicative
noreturn void raler(int syserr, const char *msg, ...) {
    va_list ap;

    va_start(ap, msg);
    vfprintf(stderr, msg, ap);
    fprintf(stderr, "\n");
    va_end(ap);

    if (syserr == 1)
        perror("");

    exit(1);
}

void usage (const char *argv0)
{
    raler (0, "usage: %s délai-max nb-threads taille-côté", argv0, argv0) ;
}

void attente_aleatoire (int delai_max)
{
    int delai ;                 // en ms

    delai = ((double) rand () / RAND_MAX) * delai_max ; 

    usleep (delai * 1000) ;     // usleep attend un délai en micro-secondes
}


struct args {
	int tnum;
	pthread_barrier_t * bar;
    pthread_cond_t * cond;
    pthread_cond_t * cond_main;
	pthread_mutex_t * mtx;
    int delai_max;
    int * nb_symb;
    char * symb;
};

void *thr (void *arg)
{
    int numthr ;
    int nbaff ;                 // nombre de symboles affichés au total
    int delai_max ;
	pthread_barrier_t * bar;
    pthread_cond_t * cond;
    pthread_cond_t * cond_main;
	pthread_mutex_t * mtx;
    int * nb_symb;
    char * symb;

	struct args * a = arg;
                // OK il faudrait sans doute utiliser "arg"
    numthr = a->tnum ;               // OK récupérer le numéro du thread courant
    delai_max = a->delai_max ;          // OK récupérer le délai maximum (en ms)
    bar = a->bar;
	cond = a->cond;
	cond_main = a->cond_main;
	mtx = a->mtx;
    nb_symb = a->nb_symb;
    nbaff = 0;

    // on attend un délai aléatoire avant de démarrer
    attente_aleatoire (delai_max) ;

    // chaque thread s'annonce
    // OK le message ne doit pas être affiché avant celui du thread principal
    TCHK( pthread_barrier_wait(bar) );
    printf ("thread %d prêt\n", numthr) ; fflush (stdout) ;
    TCHK( pthread_barrier_wait(bar) );

    // OK tant que le nombre de symboles à afficher != -1
    TCHK( pthread_mutex_lock(mtx) );
    while (*nb_symb != -1)
    {
        // OK attendre si ce nombre est devenu nul
        while (*nb_symb == 0) 
        {
            TCHK( pthread_cond_wait(cond, mtx) );
        }

        // OK si le nombre est > 0, il faut afficher le symbole
        if (*nb_symb > 0)
        {
            // const char *symb ;  // symbole à afficher encodé en UTF-8

            // OK récupérer la valeur du symbole
                    // attention UTF-8 => peut-être plusieurs octets
            symb = a->symb;
            
            // afficher le symbole (fflush pour le cas où c'est dans un fichier)
            fputs (symb, stdout) ; fflush (stdout) ;
            nbaff++ ;

            // OK décrémenter le nb de symboles à afficher
            (*nb_symb)--;

            // OK avertir thread principal si on est le dernier à décrémenter
            if (*nb_symb == 0)
            {
                TCHK( pthread_cond_signal(cond_main) );
            }
        }
        TCHK( pthread_mutex_unlock(mtx) );

        // Un peu d'attente pour voir les problèmes éventuels apparaître
        // attention : les threads doivent attendre en parallèle
        attente_aleatoire (delai_max) ;

        TCHK( pthread_mutex_lock(mtx) );
    }
    TCHK( pthread_mutex_unlock(mtx) );

    printf ("thread %d fini, %d symboles affichés\n", numthr, nbaff) ;
    fflush (stdout) ;

    return NULL ;
}

// attention : symb est une chaîne de caractères, car le symbole est un
// code Unicode encodé en UTF-8, qui peut donc prendre plusieurs octets.
void dessiner (int nb, const char *symb, struct args * arg)
{
    pthread_cond_t * cond;
    pthread_cond_t * cond_main;
	pthread_mutex_t * mtx;

	struct args * a = arg;
                // OK il faudrait sans doute utiliser "arg"
	cond = a->cond;
	cond_main = a->cond_main;
	mtx = a->mtx;

    // OK communiquer nb et symb aux threads
    TCHK( pthread_mutex_lock(mtx) );
    *a->nb_symb = nb;
    if (nb == -1)
    {
        TCHK( pthread_mutex_unlock(mtx) );
        TCHK( pthread_cond_broadcast(cond) );
        return;
    }

    int len = strlen(symb);
    memset(a->symb, '\0', BUFSIZE);
    strncpy(a->symb, symb, len);
       
    // OK réveiller 1 ou tous les threads suivant nb
    if (nb == 1)
    {
        TCHK( pthread_cond_signal(cond) );
    }
    else
    {
        TCHK( pthread_cond_broadcast(cond) );
    }
    
    // OK attendre qu'ils aient terminé
    while (*a->nb_symb != 0)
    {
        TCHK( pthread_cond_wait(cond_main, mtx) );
    }
    TCHK( pthread_mutex_unlock(mtx) );
}

int main (int argc, const char *argv [])
{
    int delai_max ;             // delai maximum d'attente en ms
    int nthr ;                  // nombre de threads de chaque sorte
    int cote ;                  // taille d'un côté (hors coin)
    int i ;                     // compte-tours de boucle

    /*
     * Étape 1 : vérification sommaire des arguments
     */

    if (argc != 4)
        usage (argv [0]) ;
    delai_max = atoi (argv [1]) ;
    nthr = atoi (argv [2]) ;
    cote = atoi (argv [3]) ;
    if (delai_max < 0 || nthr < 1 || cote < 0)
        usage (argv [0]) ;

    pthread_t *tid ;
    CHKN (tid = calloc (nthr, sizeof *tid)) ;
    struct args *ta ;
    CHKN (ta = calloc (nthr, sizeof *ta)) ;
    pthread_barrier_t bar;
    pthread_cond_t cond;
    pthread_cond_t cond_main;
	pthread_mutex_t mtx;
    int nb_symb = 0;
    char * symb = malloc(BUFSIZE * sizeof(char));

    TCHK( pthread_barrier_init(&bar, NULL, nthr+1) );
    TCHK( pthread_cond_init(&cond, NULL) );
    TCHK( pthread_cond_init(&cond_main, NULL) );
    TCHK( pthread_mutex_init(&mtx, NULL) );

    /*
     * Étape 2 : démarrer les threads
     */

    for (i = 0 ; i < nthr ; i++)
    {
        // OK démarrer le thread avec les bons arguments
        ta[i].tnum = i;
        ta[i].bar = &bar;
		ta[i].cond = &cond;
		ta[i].cond_main = &cond_main;
		ta[i].mtx = &mtx;
		ta[i].nb_symb = &nb_symb;
		ta[i].symb = symb;
		ta[i].delai_max = delai_max;

        TCHK (pthread_create (&tid [i], NULL, thr, &ta[i])) ;     // OK arg ?
    }

    // OK Cet affichage doit être effectué avant tout autre affichage
    printf ("Début, %d threads créés\n", nthr) ; fflush (stdout) ;
    TCHK( pthread_barrier_wait(&bar) );
    // pour l'affichage 'thread pret'
    TCHK( pthread_barrier_wait(&bar) );

    /*
     * Étape 3 : dessiner
     */

    // OK il faut sans doute des arguments en plus à dessiner()
    struct args * arg_dessiner;
    CHKN (arg_dessiner = malloc (sizeof (struct args))) ;

    arg_dessiner->cond = &cond;
    arg_dessiner->cond_main = &cond_main;
    arg_dessiner->mtx = &mtx;
    arg_dessiner->nb_symb = &nb_symb;
    arg_dessiner->symb = symb;

    // ligne du haut
    dessiner (1,    "┌", arg_dessiner) ;            // caractère UTF-8 => plusieurs octets
    dessiner (cote, "─", arg_dessiner) ;
    dessiner (1,    "┐", arg_dessiner) ;
    dessiner (1,   "\n", arg_dessiner) ;

    // les "côté" lignes du milieu
    for (i = 0 ; i < cote ; i++)
    {
        dessiner (1,    "│", arg_dessiner) ; 
        dessiner (cote, " ", arg_dessiner) ; 
        dessiner (1,    "│", arg_dessiner) ; 
        dessiner (1,   "\n", arg_dessiner) ; 
    }

    // ligne du bas
    dessiner (1,    "└", arg_dessiner) ; 
    dessiner (cote, "─", arg_dessiner) ; 
    dessiner (1,    "┘", arg_dessiner) ; 
    dessiner (1,   "\n", arg_dessiner) ; 

    /*
     * étape 4 : terminaison des threads
     */

    // prévenir les threads de s'arrêter
    dessiner (-1, NULL, arg_dessiner) ;

    for (i = 0 ; i < nthr ; i++)
        TCHK (pthread_join (tid [i], NULL)) ;

    // OK destroy free
    free(ta); free(tid);
    free(arg_dessiner);
	TCHK( pthread_mutex_destroy(&mtx) );
	TCHK( pthread_cond_destroy(&cond_main) );
	TCHK( pthread_cond_destroy(&cond) );
	TCHK( pthread_barrier_destroy(&bar) );
    free(symb);

    exit (0);
}
