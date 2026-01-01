#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>

#define TAILLE_FILE 5           // Taille max de la file d'attente
#define NB_SERVEURS 2           // Nombre de serveurs
#define NB_CUISINIERS 2         // Nombre de cuisiniers
#define COMMANDES_PAR_SERVEUR 5 // Commandes à faire par serveur

int file_commandes[TAILLE_FILE];
int index_ajout = 0;
int index_retrait = 0;

// --- Synchronisation ---
pthread_mutex_t mutex;  // Bloque l'accès au tableau
sem_t places_vides;     // Compte les places libres
sem_t commandes_pretes; // Compte les commandes à cuisiner

// --- SERVEUR (Producteur) ---
void* serveur(void* arg) {
    int id = *(int*)arg;

    for (int i = 0; i < COMMANDES_PAR_SERVEUR; i++) {
        int num_commande = rand() % 100;
        sleep(rand() % 2); // Simule la prise de commande

        sem_wait(&places_vides);    // Attend une place libre
        pthread_mutex_lock(&mutex); // Verrouiller l'accès (Section Critique)

        // Ajout dans la file le numéro de commande
        file_commandes[index_ajout] = num_commande;
        printf("[SERVEUR %d] Ajoute commande %d (Case %d)\n", id, num_commande, index_ajout);

        // Gestion circulaire du tableau
        index_ajout = (index_ajout + 1) % TAILLE_FILE;

        pthread_mutex_unlock(&mutex); // Déverrouiller l'accès
        sem_post(&commandes_pretes);  // Signaler qu'une commande est prête
    }

    printf("--- Serveur %d a fini son service ---\n", id);
    return NULL;
}

// --- CUISINIER (Consommateur) ---
void* cuisinier(void* arg) {
    int id = *(int*)arg;

    while (1) {
        sem_wait(&commandes_pretes); // Attendre une commande
        pthread_mutex_lock(&mutex);  // Verrouiller l'accès

        // Retrait de la file
        int cmd = file_commandes[index_retrait];
        printf("    [CUISINIER %d] Prépare commande %d (Case %d)\n", id, cmd, index_retrait);

        index_retrait = (index_retrait + 1) % TAILLE_FILE;

        pthread_mutex_unlock(&mutex); // Déverrouiller
        sem_post(&places_vides);      // Signaler qu'une place est libre

        // Simulation cuisson
        sleep(2);
        printf("    [CUISINIER %d] Commande %d TERMINEE !\n", id, cmd);
    }
    return NULL;
}

// --- MAIN ---
int main() {
    pthread_t thread_serveurs[NB_SERVEURS];
    pthread_t thread_cuisiniers[NB_CUISINIERS];
    int ids[5] = {1, 2, 3, 4, 5}; // Identifiants pour l'affichage

    // Initialisation
    pthread_mutex_init(&mutex, NULL);
    sem_init(&places_vides, 0, TAILLE_FILE); // Au début, tout est vide
    sem_init(&commandes_pretes, 0, 0);       // Au début, rien n'est prêt

    printf("=== OUVERTURE DU RESTAURANT ===\n");

    // Lancement des threads
    for (int i = 0; i < NB_CUISINIERS; i++)
        pthread_create(&thread_cuisiniers[i], NULL, cuisinier, &ids[i]);

    for (int i = 0; i < NB_SERVEURS; i++)
        pthread_create(&thread_serveurs[i], NULL, serveur, &ids[i]);

    // Attente de la fin des serveurs uniquement
    for (int i = 0; i < NB_SERVEURS; i++)
        pthread_join(thread_serveurs[i], NULL);

    printf("=== Serveurs partis. On vide la cuisine... ===\n");
    sleep(5); // Temps pour finir les dernières commandes

    // Nettoyage final
    pthread_mutex_destroy(&mutex);
    sem_destroy(&places_vides);
    sem_destroy(&commandes_pretes);

    printf("=== FERMETURE DU RESTAURANT ===\n");
    return 0;
}