#include <stdio.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define nullptr NULL

typedef struct param {
    int NO;
    int NH;
    int TI;
    int TB;
} Tparam;

Tparam loc_param;

sem_t *file_lock;
sem_t *first_hydrogen;
sem_t *destroy_lock;
sem_t *oxygen_bar;
sem_t *hydrogen_bar;
sem_t *wait_for_increasing;
sem_t *molecule_bar;
sem_t *wait_for_hydrogen;
sem_t *wait_for_oxygen;
sem_t *hydrogen_lock;
sem_t *qH_lock;



//------Share memory- declaration -----//

int *molecule_counter=nullptr;      // counting molecules
int *queue_H = nullptr;             // how much hydrogen is in queue
int *queue_O = nullptr;             // how much oxygen is in queue
int *counter = nullptr;             // number of line in file
int *number_O = nullptr;            // number of oxygen in mutex
int *number_H = nullptr;            // number of hydrogen in mutex
int *number_of_processes = nullptr; // number of processes

int shm_molecule_counter = 0;
int shm_queue_H = 0;
int shm_queue_O = 0;
int shm_counter = 0;
int shm_number_O = 0;
int shm_number_H = 0;
int shm_number_of_processes = 0;


FILE *file;


int destroy_sources(){

    //---------destroy semaphores --------//

    sem_destroy(oxygen_bar);                // oxygen molecule bar
    sem_destroy(hydrogen_bar);              // hydrogen molecule bar
    sem_destroy(first_hydrogen);            // semaphore for hydrogen to wait on second one
    sem_destroy(wait_for_oxygen);           // synchronize semaphore for hydrogen
    sem_destroy(wait_for_hydrogen);         // synchronize semaphore for oxygen
    sem_destroy(molecule_bar);              // molecule bar for both of them
    sem_destroy(wait_for_increasing);       // critical section
    sem_destroy(destroy_lock);              // wait main process for destroying sources
    sem_destroy(file_lock);                 // critical section for file and buffer
    sem_destroy(hydrogen_lock);             // wait from oxygen for hydrogen
    sem_destroy(qH_lock);                   // critical section


    //-----------destroy shared -----------//
    // unmapping and unlinking share memory//

    munmap(queue_O, sizeof(int));
    shm_unlink("shm_queue_O");

    munmap(queue_H, sizeof(int));
    shm_unlink("shm_queue_H");


    munmap(counter, sizeof(int));
    shm_unlink("shm_counter");


    munmap(number_H, sizeof(int));
    shm_unlink("shm_number_H");


    munmap(number_O, sizeof (int));
    shm_unlink("shm_number_O");

    munmap(molecule_counter, sizeof (int));
    shm_unlink("shm_molecule_counter");

    munmap(number_of_processes, sizeof (int));
    shm_unlink("shm_number_of_processes");

    //------------- close file -----------//

    fclose(file);


    return 0;
}

sem_t * mmap_init(){
// function for mapping memory for function initialize_memory() //

    sem_t * pom;
    pom = mmap(NULL, sizeof (sem_t), PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_SHARED, 0, 0);
    if (pom == MAP_FAILED) {
        destroy_sources();
        fprintf(file,"Error while initializing memory");
        exit(1);
    }
    else
        return pom;
}


int initialize_memory(){

    //--------Semaphores-----------//
    //     mapping semaphores      //

    destroy_lock = mmap_init();
    first_hydrogen = mmap_init();
    oxygen_bar = mmap_init();
    hydrogen_bar = mmap_init();
    wait_for_increasing = mmap_init();
    molecule_bar = mmap_init();
    wait_for_hydrogen = mmap_init();
    wait_for_oxygen = mmap_init();
    file_lock = mmap_init();
    hydrogen_lock = mmap_init();
    qH_lock = mmap_init();

    //initializing semaphores //

    if((sem_init(qH_lock, 1, 1)) == -1)
        return -1;

    if((sem_init(hydrogen_lock, 1, 0)) == -1)
        return -1;

    if((sem_init(destroy_lock, 1, 0)) == -1)
        return -1;

    if((sem_init(first_hydrogen, 1, 1)) == -1)
        return -1;

    if((sem_init(oxygen_bar, 1, 1)) == -1)
        return -1;

    if((sem_init(hydrogen_bar, 1, 1)) == -1)
        return -1;

    if((sem_init(wait_for_increasing, 1, 0)) == -1)
        return -1;

    if((sem_init(molecule_bar, 1, 0)) == -1)
        return -1;

    if((sem_init(wait_for_hydrogen, 1, 0)) == -1)
        return -1;

    if((sem_init(wait_for_oxygen, 1, 0)) == -1)
        return -1;

    if((sem_init(file_lock, 1, 1)) == -1)
        return -1;

    //----------Share memory -------//
    // opening smh, truncating and mapping shm//

    if ((shm_queue_O = shm_open("shm_molecule_counter", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_molecule_counter, sizeof(int));

    if ((molecule_counter = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_queue_O, 0)) == MAP_FAILED)
        return -3;



    if ((shm_queue_O = shm_open("shm_queue_O", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_queue_O, sizeof(int));

    if ((queue_O = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_queue_O, 0)) == MAP_FAILED)
        return -3;




    if ((shm_queue_O = shm_open("shm_queue_H", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_queue_H, sizeof(int));

    if ((queue_H = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_queue_O, 0)) == MAP_FAILED)
        return -3;



    if ((shm_counter = shm_open("shm_counter", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_counter,sizeof(int));

    if((counter = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_counter, 0)) == MAP_FAILED)
        return -3;



    if ((shm_number_H = shm_open("shm_number_H", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_number_H,sizeof(int));

    if((number_H = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_number_H, 0)) == MAP_FAILED)
        return -3;



    if ((shm_number_O = shm_open("shm_number_O", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_number_O,sizeof(int));

    if((number_O = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_number_O, 0)) == MAP_FAILED)
        return -3;


    if ((shm_number_of_processes = shm_open("shm_number_of_processes", O_RDWR | O_CREAT, 0666)) == -1)
        return -2;

    ftruncate(shm_number_of_processes,sizeof(int));

    if((number_of_processes = mmap(0, sizeof(int), PROT_READ | PROT_WRITE, MAP_ANONYMOUS|MAP_SHARED, shm_number_O, 0)) == MAP_FAILED)
        return -3;

    return 0;

}


int max_molecules_cnt(Tparam loc_param){
// counting how many molecules can be created //

    int O = loc_param.NO;
    int H = loc_param.NH;
    int cnt = 1;

    if (loc_param.NH <= 1 || loc_param.NO <=0 )
        return 0;

    while(1){
        if (O == 0 || H <= 1) {
            return cnt;
        }
        else{
            O--;
            H-=2;
            cnt++;
        }

    }

}


void generate_oxygen(Tparam loc_param){
// creating oxygen //
    int process_num, i;
    time_t t;
    *queue_O = 0;
    *number_of_processes = 0;

    int max_molecules = max_molecules_cnt(loc_param);

    for (i = 0; i < loc_param.NO; i++) {
        pid_t pid = fork();

        if (!pid)
            break;

        if (i == loc_param.NO - 1)
            return;

    }



    sem_wait(file_lock);
        *number_O=*number_O+1;
        process_num=*number_O;
        *counter = *counter + 1;
        fprintf(file, "%d: O %d: started \n", *counter, *number_O);
        fflush(file);
    sem_post(file_lock);


    if (loc_param.TB != 0) {
        srand((unsigned) time(&t));
        usleep((rand() % loc_param.TI)*1000);
    }
    else{
        usleep(0);
    }

    sem_wait(file_lock);
        *counter = *counter + 1;
        fprintf(file, "%d: O %d: going to queue \n", *counter, process_num);
        fflush(file);
    sem_post(file_lock);


    // MOLECULE BARIER //
    sem_wait(oxygen_bar);
    if (*molecule_counter >= max_molecules ){
        sem_wait(file_lock);
        *counter = *counter +1;
        fprintf(file,"%d: O %d: not enough H \n",*counter,process_num);
        fflush(file);
        sem_post(file_lock);
        sem_post(oxygen_bar);

        sem_wait(file_lock);
        *number_of_processes = *number_of_processes + 1;

        if (*number_of_processes == loc_param.NO + loc_param.NH) {
            sem_post(file_lock);
            sem_post(destroy_lock);
            exit(0);
        }
        sem_post(file_lock);

        exit(1);
    }

    sem_wait(file_lock);
        *queue_O = *queue_O + 1;
        *counter = *counter + 1;
        fprintf(file, "%d: O %d: creating molecule %d \n",*counter, process_num, *molecule_counter);
        fflush(file);
    sem_post(file_lock);

    sem_wait(wait_for_hydrogen);
    sem_post(wait_for_oxygen);

    sem_wait(file_lock);
        *counter = *counter + 1 ;
        fprintf(file, "%d: O %d: molecule %d created \n",*counter,process_num,*molecule_counter);
        fflush(file);
    sem_post(file_lock);


    *queue_O = 0;

    sem_wait(wait_for_increasing);

    *molecule_counter = *molecule_counter + 1;
    sem_post(hydrogen_lock);

    sem_post(oxygen_bar);



    sem_wait(file_lock);
    *number_of_processes = *number_of_processes + 1;
    if (*number_of_processes == loc_param.NO + loc_param.NH) {
        sem_post(file_lock);
        sem_post(destroy_lock);
        exit(0);
    }
    sem_post(file_lock);


    exit (0);

}

void generate_hydrogen(Tparam loc_param){
    // creating hydrogen //
    int process_num, i;
    *queue_H = 0;
    time_t t;

    int max_molecules = max_molecules_cnt(loc_param);


    for (i = 0; i < loc_param.NH; i++) {
        pid_t pid = fork();

        if (!pid) {
            break;
        }
        else if (i == loc_param.NH - 1)
            return;

    }

    sem_wait(file_lock);
        *number_H=*number_H+1;
        process_num = *number_H;
        *counter = *counter + 1;
        fprintf(file, "%d: H %d: started \n", *counter, *number_H);
        fflush(file);
    sem_post(file_lock);

    if(loc_param.TI != 0){
        srand((unsigned) time(&t));
        usleep((rand() % loc_param.TI)*1000);
    }
    else{
        usleep(0);
    }

    sem_wait(file_lock);
        *counter = *counter + 1;
        fprintf(file, "%d: H %d: going to queue \n", *counter, process_num);
        fflush(file);
    sem_post(file_lock);


    sem_wait(hydrogen_bar);                 //BARIER


    if (*molecule_counter >= max_molecules ){
        sem_wait(file_lock);
            *counter = *counter +1;
            fprintf(file,"%d: H %d: not enough O or H \n",*counter,process_num);
            fflush(file);
            sem_post(file_lock);
        sem_post(hydrogen_bar);

        sem_wait(file_lock);
        *number_of_processes = *number_of_processes + 1;

        if (*number_of_processes == loc_param.NO + loc_param.NH) {
            sem_post(file_lock);

            sem_post(destroy_lock);
            exit(0);
        }
        sem_post(file_lock);


        exit(0);
    }

    sem_wait(file_lock);
    *counter = *counter + 1;
    fprintf(file, "%d: H %d: creating molecule %d \n",*counter, process_num, *molecule_counter);
    fflush(file);
    sem_post(file_lock);

    sem_wait(qH_lock);
    *queue_H = *queue_H +1;

    if(*queue_H == 1){
        sem_post(qH_lock);
        sem_post(hydrogen_bar);
        sem_wait(molecule_bar);
    }
    else {;
        *queue_H = 0;
        sem_post(qH_lock);

        if(loc_param.TB != 0){
            srand((unsigned) time(&t));
            usleep((rand() % loc_param.TB)*1000);
        }
        else{
            usleep(0);
        }

        sem_post(wait_for_hydrogen);
        sem_wait(wait_for_oxygen);
        sem_post(molecule_bar);
    }

    sem_wait(file_lock);
    *counter = *counter + 1;
    fprintf(file, "%d: H %d: molecule %d created \n", *counter, process_num, *molecule_counter);
    sem_post(file_lock);

    sem_wait(qH_lock);
    *queue_H = *queue_H +1;

    if(*queue_H == 1){
        sem_post(qH_lock);
        sem_wait(first_hydrogen);
    }
    else{
        *queue_H = 0;
        sem_post(qH_lock);
        sem_post(wait_for_increasing);
        sem_wait(hydrogen_lock);
        sem_post(first_hydrogen);
        sem_post(hydrogen_bar);
    }

    sem_wait(file_lock);
    *number_of_processes = *number_of_processes + 1;    //????

    if (*number_of_processes == loc_param.NO + loc_param.NH) {
        sem_post(file_lock);
        sem_post(destroy_lock);
        exit(0);
    }

    sem_post(file_lock);

    exit (0);

}




int main (int argc, char **argv){

    char *end_ptr = nullptr;

    // ------Initializing params-------//
    if (argc != 5) {
        fprintf(stderr, "Not enough parameters\n");
        return 1;
    }
    else{

        if (strlen(argv[1]) == 0 || strlen(argv[2]) == 0 || strlen(argv[3]) == 0 || strlen(argv[4]) == 0){
            fprintf(stderr, "Params are not in valid value\n");
            return 1;
        }


        loc_param.NO= strtol(argv[1], &end_ptr, 0);
        if (errno != 0 || *end_ptr != 0){
            fprintf(stderr, "Params are not in valid value\n");
            return 1;
        }

        loc_param.NH= strtol(argv[2], &end_ptr, 0);
        if (errno != 0 || *end_ptr != 0){
            fprintf(stderr, "Params are not in valid value\n");
            return 1;
        }

        loc_param.TI= strtol(argv[3], &end_ptr, 0);
        if (errno != 0 || *end_ptr != 0){
            fprintf(stderr, "Params are not in valid value\n");
            return 1;
        }

        loc_param.TB= strtol(argv[4], &end_ptr, 0);
        if (errno != 0 || *end_ptr != 0){
            fprintf(stderr, "Params are not in valid value\n");
            return 1;
        }
    }

    if (loc_param.NO < 0 || loc_param.NH < 0){
        fprintf(stderr, "Params are not in valid value");
        return 1;
    }

    if (loc_param.TI > 1000 || loc_param.TI < 0){
        fprintf(stderr, "Params are not in valid value");
        return 1;
    }

    if (loc_param.TB > 1000 || loc_param.TB < 0){
        fprintf(stderr, "Params are not in valid value");
        return 1;
    }


    //-----------Opening file--------------//
    file = fopen("proj2.out", "w");
    if (file == NULL){
        fprintf(stderr, "Error while opening the file");
        fflush(file);
        return 1;
    }

    setbuf(file, NULL);

    //-------Initializing shared memory---//

    switch (initialize_memory()) {
        case -1 :
            fprintf(file,"Error while initializing semaphores");
            destroy_sources();
            return 1;

        case -2 :
            fprintf(file,"Error while mapping share memory");
            destroy_sources();
            return 1;

        case -3 :
            fprintf(file,"Error while initializing share memory");
            destroy_sources();
            return 1;
    }

    *counter = 0;
    *molecule_counter = 1;

    if (loc_param.NH == 0 || loc_param.NO ==0) {
        fprintf(stderr, "Can not create anything without at least one atom");
        destroy_sources();
        return 1;
    }

    if (loc_param.NH != 0)
        generate_hydrogen(loc_param);

    if (loc_param.NO != 0) {
        generate_oxygen(loc_param);
        sem_wait(destroy_lock);
        destroy_sources();
    }

    return 0;
}