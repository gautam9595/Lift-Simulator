#include <stdio.h>
#include <sys/ipc.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include <math.h>

struct sembuf pop, vop;

void P(int s)
{
    semop(s, &pop, 1);
}
void V(int s)
{
    semop(s, &vop, 1);
}

#define N 10 //no. of persons
#define F 6  //no. of floors
#define M 3  //no. of lifts
#define UP 1
#define DOWN 0

volatile sig_atomic_t done = 0;                                 /*-------------------TO INDICATE THAT CTRL+Z IS PRESSED -------------------------*/
int ischild = 0;

typedef struct                                                  /*--------------------STRUCTURE FOR SHARED FLOOR VARIABLES------------------------*/                                           
{
    int n;
    int lift_no_to_go_up;
    int lift_no_to_go_down;
    int waiting_to_go_up;
    int waiting_to_go_down;
} floor_info;

typedef struct                                                  /*--------------------STRUCTURE FOR FLOOR SEMAPHORES----------------------------*/
{
    int mutexUP;
    int mutexDOWN;
    int mutex1;
    int mutex2;
    int sem_up_arrow;
    int sem_down_arrow;
} floor_sem;

typedef struct                                                /*--------------------STRUCTURE FOR SHARED LIFT VARIABLES--------------------------*/   
{
    int no;
    int position;
    int direction;                                            //1 --> Moving UP, 0 --> Moving DOWN
    int moving;
    int people_in_lift;
    int stop[F];
    int capacity;
    int getting_on;
    int getting_off;
} lift_info;

typedef struct                                              /*--------------------STRUCTURE FOR LIFT SEMAPHORES----------------------------------*/
{
    int mutex;
    int sem_stopsem[F];
} lift_sem;

int floor_shm;
floor_info *floor_obj;

floor_sem floor_obj_sem[F];

int lift_shm;
lift_info *lift_obj;

lift_sem lift_obj_sem[M];

int generate_random_number()            
{
    /*-----------------------------------Generating a random sleep time for a person using exponential distribution of Mean 5sec-----------------------*/
    int mean = 5;
    double r = (double)rand() / (double)(RAND_MAX);
    int sleep_time = -(mean * log(1 - r)) + 2;
    return sleep_time;
}

void print_details()
{
    /*------------------------------------------------------PRINTING THE DETAILS IN THE GRAPHICAL FORM------------------------------------------------*/
    while (1)
    {
        if (done)
            break;
        system("clear");
        /*----------------------------GET ALL SEAMPHORE ACCESS-----------------------*/
        for (int i = 0; i < M; i++)
            P(lift_obj_sem[i].mutex);
        for (int i = 0; i < F; i++)
        {
            P(floor_obj_sem[i].mutexUP);
            P(floor_obj_sem[i].mutexDOWN);
        }
        /*--------------------------------------------------------------------------*/

        printf("**********************************************************************************WELCOME TO THE LIFT SIMULATOR PROGRAM*************************************************************\n");
        printf("*************************************************************************************PRESS CTRL+Z TO TERMINATE SAFELY***************************************************************\n\n");
        printf("---------------------------------------------%d FLOORS-----------------------------------%d PERSONS------------------------------------%d LIFTS-----------------------------------\n\n\n",F-1,N,M);
        
        //---------------------------------------------------DETAILS ABOUT EACH FLOOR
        printf("-------------------------------------------------------------------------Floors----------------------------------------------------------------------------------\n\n");
        printf("                FROM FLOOR     |   G  |");
        for (int i = 0; i < F - 1; i++)
        {
            printf("   |   %d  |", i + 1);
        }
        printf("\n\nPersons waiting to go UP ");
        for (int i = 0; i < F; i++)
        {
            printf("          %d", floor_obj[i].waiting_to_go_up);
        }
        printf("\n\nPersons waiting to go DOWN");
        printf("         %d", floor_obj[0].waiting_to_go_down);
        for (int i = 1; i < F; i++)
        {
            printf("          %d", floor_obj[i].waiting_to_go_down);
        }
        //-------------------------------------------------DETAILS OF EACH LIFT
        printf("\n\n-------------------------------------------------------------------------LIFTS----------------------------------------------------------------------------------\n\n");
        printf("                             ");
        for (int i = 0; i < M; i++)
        {
            printf("____________");
            printf("                                    ");
        }
        printf("\n                            ");
        for (int i = 0; i < M; i++)
        {
            if (lift_obj[i].moving)
            {
                if (lift_obj[i].direction == UP)
                {
                    if (lift_obj[i].position == 0)
                        printf("|___G --> 1__|");
                    else
                        printf("|___%d --> %d__|", lift_obj[i].position, lift_obj[i].position + 1);
                }
                else
                {
                    if (lift_obj[i].position == 1)
                        printf("|___1 --> G__|");
                    else if (lift_obj[i].position == 0)
                        printf("|___G --> 1__|");
                    else
                        printf("|___%d --> %d__|", lift_obj[i].position, lift_obj[i].position - 1);
                }
            }
            else
            {
                if (lift_obj[i].position == 0)
                    printf("|_____G______|");
                else
                    printf("|_____%d______|", lift_obj[i].position);
            }
            printf("                                  ");
        }
        printf("\n\n");
        printf("                  ");
        for (int i = 0; i < M; i++)
        {
            printf("-------------------------------");
            printf("                  ");
        }
        printf("\n");
        for (int i = 0; i < 2; i++)
        {
            printf("                  ");
            for (int j = 0; j < M; j++)
            {
                printf("|                             |");
                printf("                  ");
            }
            printf("\n");
        }
        int pr = 0;
        int printing_moving[M] = {};
        int printing_get_ON_OFF[M] = {};
        for (int k = 0; k < F; k++)
        {

            int f = 0,g = 0;
            for (int j = 0; j < M; j++)
            {
                f = f || (lift_obj[j].stop[k]);
                if(lift_obj[j].moving == 0)                 /*---------------LIFT[j] IS NOT MOVING-----------*/
                {
                    g = g || (lift_obj[j].getting_on) || (lift_obj[j].getting_off);
                }
            }
            if(f || g)
            { 
                pr++;
                printf("                  ");
                
                for(int j=0;j<M;j++)
                {
                    if(lift_obj[j].moving == 0 && (lift_obj[j].getting_on || lift_obj[j].getting_off) && printing_get_ON_OFF[j] == 0)
                    {
                        printing_get_ON_OFF[j] = 1;
                        printf("|Getting ON: %d, Getting OFF: %d|", lift_obj[j].getting_on,lift_obj[j].getting_off);
                        printf("                  ");
                    }
                    else if (lift_obj[j].moving == 1 && lift_obj[j].stop[k])
                    {
                        if (k == 0)
                            printf("| %d Persons going to floor G  |", lift_obj[j].stop[k]);
                        else
                            printf("| %d Persons going to floor %d  |", lift_obj[j].stop[k], k);
                        printf("                  ");
                    }
                    else
                    {
                        printf("|                             |");
                        printf("                  ");
                    }
                }
                printf("\n");
            }
        }
        for (int i = pr; i < F + 10; i++)
        {
            printf("                  ");
            for (int j = 0; j < M; j++)
            {
                printf("|                             |");
                printf("                  ");
            }
            printf("\n");
        }
        printf("                  ");
        for (int i = 0; i < M; i++)
        {
            printf("|         Max Capacity: %d     |", lift_obj[i].capacity);
            printf("                  ");
        }
        printf("\n");
        printf("                  ");
        for (int i = 0; i < M; i++)
        {
            printf("-------------------------------");
            printf("                  ");
        }
        printf("\n");
        //-----------------------------------------------------Release all semaphores
        for (int i = 0; i < F; i++)
        {
            V(floor_obj_sem[i].mutexUP);
            V(floor_obj_sem[i].mutexDOWN);
        }
        for (int i = 0; i < M; i++)
            V(lift_obj_sem[i].mutex);
        /*------------------------------------------------------------------------------*/
        sleep(1);
    }

    /*---------------------------- CTRL+Z COMMAND CAUGHT---------------------------*/
    /*---------------------------------- WAIT FOR ALL CHILD PROCESSES TO TERMINATE*/
    while (wait(NULL) > 0);
    /*-----------------------------------------------------------------------------*/

    /*-------------------------------------------------------REMOVE ALL SEMAPHORES*/
    for (int i = 0; i < F; i++)
    {
        semctl(floor_obj_sem[i].mutex1, 1, IPC_RMID);
        semctl(floor_obj_sem[i].mutex2, 1, IPC_RMID);
        semctl(floor_obj_sem[i].mutexUP, 1, IPC_RMID);
        semctl(floor_obj_sem[i].mutexDOWN, 1, IPC_RMID);
        semctl(floor_obj_sem[i].sem_up_arrow, 1, IPC_RMID);
        semctl(floor_obj_sem[i].sem_down_arrow, 1, IPC_RMID);
    }
    for (int i = 0; i < M; i++)
    {
        semctl(lift_obj_sem[i].mutex, 0, IPC_RMID);
        for (int j = 0; j < F; j++)
            semctl(lift_obj_sem[i].sem_stopsem[j], 1, IPC_RMID);
    }
    /*-----------------------------------------------------------------------------*/

    /*-----------------------------REMOVE ALL SHARED MEMORY-------------------------*/
    shmctl(floor_shm, IPC_RMID, NULL);
    shmctl(lift_shm, IPC_RMID, NULL);
    /*-----------------------------------------------------------------------------*/

    printf("\n\n*************************************************************************************LIFT SIMULATOR PROGRAM ENDED***************************************************************\n\n\n");
}

void catch_suspend()
{
    /*---------------------------- CTRL+Z PRESSED---------------------------*/
    done = 1;
    if (ischild)
    {
        /*-----------------------DETACH ALL CHILD PROCESS FROM SHARED MEMORY*/
        shmdt(floor_obj);
        shmdt(lift_obj);

        /*--------------------------------------------------EXIT THE PROCESS*/
        exit(0);
    }
}

void Person_process()
{
    /*-----------------------------------------------------------PERSON'S PROCESS-------------------------------------------*/
    int current_floor = rand() % (F - 1);
    while (1)
    {
        int sleep_time = generate_random_number();                 
        sleep(sleep_time);                                      /*-------------------WAIT BETWEEN SUCCESSIVE TRIPS-----------*/

        /*------------------------------CHOOSE A DIFFERENT FLOOR TO GO-------------------------------*/
        int dest_floor;
        while (1)
        {
            dest_floor = rand() % (F - 1);
            if (dest_floor != current_floor)
            {
                break;
            }
        }
        if (dest_floor > current_floor)
        {
            /*----------------------------------PERSON WANTS TO GO UP-----------------------------------*/

            P(floor_obj_sem[current_floor].mutexUP);                                
            floor_obj[current_floor].waiting_to_go_up++;                   /*---------INCREMENT THE UP_WAIT_COUNT IN CRITCIAL SECTION------------*/
            V(floor_obj_sem[current_floor].mutexUP);
            P(floor_obj_sem[current_floor].sem_up_arrow);                   /*----------------------------WAIT FOR LIFT--------------------------*/

            /*-------------------------------LIFT ARRIVED AND PERSON CAN GET ON --------------------------------*/
            int get_on = floor_obj[current_floor].lift_no_to_go_up;
            lift_obj[get_on].stop[dest_floor]++;

            V(floor_obj_sem[current_floor].mutex1);
            /*--------------------------------------------------------------------------------------------------*/

            P(lift_obj_sem[get_on].sem_stopsem[dest_floor]);            /*-----------------WAIT IN THE LIFT TO REACH DESTINATION FLOOR------------*/
        }
        else
        {
            /*----------------------------------PERSON WANTS TO GO DOWN-----------------------------------*/

            P(floor_obj_sem[current_floor].mutexDOWN);
            floor_obj[current_floor].waiting_to_go_down++;                       /*---------INCREMENT THE DOWN_WAIT_COUNT IN CRITCIAL SECTION------------*/
            V(floor_obj_sem[current_floor].mutexDOWN);
            P(floor_obj_sem[current_floor].sem_down_arrow);                      /*----------------------------WAIT FOR LIFT--------------------------*/

            /*-------------------------------LIFT ARRIVED AND PERSON CAN GET ON --------------------------------*/
            int get_on = floor_obj[current_floor].lift_no_to_go_down;
            lift_obj[get_on].stop[dest_floor]++;

            V(floor_obj_sem[current_floor].mutex2);
            /*---------------------------------------------------------------------------------------------------*/

            P(lift_obj_sem[get_on].sem_stopsem[dest_floor]);                     /*-----------------WAIT IN THE LIFT TO REACH DESTINATION FLOOR------------*/
        }
        /*-----------------------------------------PERSON REACH THE DESTINATION FLOOR-------------------------------------------*/
        current_floor = dest_floor;
    }
}

void Lift_process(int i)
{
    /*----------------------------------------------------LIFT'S PROCESS----------------------------------------------*/

    int movement_time = (rand() % 5) + 2;                                       /*------------------LIFT MOVEMENT SPEED BETWEEN TWO FLOORS-------------------*/           
    int lift_no = i;
    int current_floor = lift_obj[lift_no].position, dir = lift_obj[lift_no].direction;

    while (1)
    {
        int f = 0;

        P(lift_obj_sem[lift_no].mutex);
        int persons_to_get_off_on_current_floor = lift_obj[lift_no].stop[current_floor];    /*--------CHECK IF ANY PERSONS HAVE TO GET OFF ON THIS FLOOR--------*/

        /*-------------------------PERSON GETTING OFF ON THIS FLOOR----------------*/
        for (int j = 0; j < persons_to_get_off_on_current_floor; j++)
        {
            lift_obj[lift_no].people_in_lift--;

            lift_obj[lift_no].stop[current_floor]--;

            V(lift_obj_sem[lift_no].sem_stopsem[current_floor]);

            lift_obj[lift_no].moving = 0;
            lift_obj[lift_no].getting_off++;

            f = 1;
            
        }
        /*--------------------------------------------------------------------------*/

        /*---------------------------------CHANGE DIRECTION IF THE LIFT IS ON GROUND OR TOP FLOOR-----------------------*/
        if (current_floor == 0) 
        {
            dir = UP;
            lift_obj[lift_no].direction = dir;
        }
        else if (current_floor == F - 1)
        {
            dir = DOWN;
            lift_obj[lift_no].direction = dir;
        }
        /*-----------------------------------------------------------------------------------------------------------------*/

        int persons_waiting_at_current_floor;
        if (dir)
        {
            /*------------------------------------------------LIFT IS GOING UP----------------------------------------------*/
            while (1)
            {
                /*---------------------------------IF LIFT IS AT MAX CAPACITY, THEN DON'T STOP ON THIS FLOOR TO PICK PERSONS-------------------------------*/
                if (lift_obj[lift_no].people_in_lift == lift_obj[lift_no].capacity)
                {
                    V(lift_obj_sem[lift_no].mutex);
                    break;
                }
                /*-------------------------------------------------------------------------------------------------------------------------------------------*/

                /*-----------------------------------------CHECK IF ANY PERSON IS WAITING TO GO UP AT THIS FLOOR---------------------------------------------*/
                P(floor_obj_sem[current_floor].mutexUP);
                persons_waiting_at_current_floor = floor_obj[current_floor].waiting_to_go_up;

                if (persons_waiting_at_current_floor == 0)
                {
                    /*---------------------------------------NO PERSON ON THIS FLOOR TO GO UP----------------------------------*/
                    V(floor_obj_sem[current_floor].mutexUP);
                    V(lift_obj_sem[lift_no].mutex);
                    break;
                }
                lift_obj[lift_no].moving = 0;                                   /*-------------STOP THE LIFT AT THIS FLOOR--------------------*/

                floor_obj[current_floor].lift_no_to_go_up = lift_no;            
                V(floor_obj_sem[current_floor].sem_up_arrow);                   /*-------------------WAKE UP THE PERSON THAT WILL GET ON THE LIFT---------*/

                P(floor_obj_sem[current_floor].mutex1);                         /*----------------WAIT FOR PERSON TO GET ON THE LIFT-------------------*/
                
                /*--------------------------------------------PERSON HAS ENETERED IN THE LIFT-----------------------------------*/
                floor_obj[current_floor].waiting_to_go_up--;
                lift_obj[lift_no].people_in_lift++;
                lift_obj[lift_no].getting_on++;
                V(floor_obj_sem[current_floor].mutexUP);
                f = 1;
            }
            if (f == 1)
                sleep(3);                           /*------------------------TO SIMULATE THAT LIFT STOP FOR 2SEC TO PICK OR DROP ON THIS FLOOR-------------------------*/

            P(lift_obj_sem[lift_no].mutex);
            lift_obj[lift_no].moving = 1;              /*-------------------------------------------------LIFT STARTS MOVING--------------------------------------------*/
            lift_obj[lift_no].getting_on = 0;
            lift_obj[lift_no].getting_off = 0;
            V(lift_obj_sem[lift_no].mutex);

            sleep(movement_time);                     /*-------------------------LIFT IS MOVING FROM CURRENT FLOOR TO NEXT FLOOR---------------------------------*/

            /*-------------------------------------------LIFT REACHES NEXT FLOOR--------------------------------------*/
            P(lift_obj_sem[lift_no].mutex);
            lift_obj[lift_no].position++;
            current_floor = lift_obj[lift_no].position;                     /*-------------CHANGE THE FLOOR POSITION OF LIFT--------------------*/
            V(lift_obj_sem[lift_no].mutex);
        }
        else
        {
            /*------------------------------------------------LIFT IS GOING DOWN----------------------------------------------*/
            while (1)
            {
                /*---------------------------------IF LIFT IS AT MAX CAPACITY, THEN DON'T STOP ON THIS FLOOR TO PICK PERSONS-------------------------------*/
                if (lift_obj[lift_no].people_in_lift == lift_obj[lift_no].capacity)
                {
                    V(lift_obj_sem[lift_no].mutex);
                    break;
                }
                /*-------------------------------------------------------------------------------------------------------------------------------------------*/
                
                /*-----------------------------------------CHECK IF ANY PERSON IS WAITING TO GO DOWN AT THIS FLOOR---------------------------------------------*/
                P(floor_obj_sem[current_floor].mutexDOWN);
                persons_waiting_at_current_floor = floor_obj[current_floor].waiting_to_go_down;
                
                if (persons_waiting_at_current_floor == 0)
                {
                    /*---------------------------------------NO PERSON ON THIS FLOOR TO GO DOWN----------------------------------*/
                    V(floor_obj_sem[current_floor].mutexDOWN);
                    V(lift_obj_sem[lift_no].mutex);
                    break;
                }
                lift_obj[lift_no].moving = 0;                           /*-------------STOP THE LIFT AT THIS FLOOR--------------------*/

                floor_obj[current_floor].lift_no_to_go_down = lift_no;
                V(floor_obj_sem[current_floor].sem_down_arrow);         /*-------------------WAKE UP THE PERSON THAT WILL GET ON THE LIFT---------*/

                P(floor_obj_sem[current_floor].mutex2);                 /*----------------WAIT FOR PERSON TO GET ON THE LIFT-------------------*/
                
                /*--------------------------------------------PERSON HAS ENETERED IN THE LIFT-----------------------------------*/
                floor_obj[current_floor].waiting_to_go_down--;
                lift_obj[lift_no].people_in_lift++;
                lift_obj[lift_no].getting_on++;
                V(floor_obj_sem[current_floor].mutexDOWN);

                f = 1;
            }
            if (f == 1)
                sleep(3);                       /*------------------------TO SIMULATE THAT LIFT STOP FOR 2SEC TO PICK OR DROP ON THIS FLOOR-------------------------*/

            P(lift_obj_sem[lift_no].mutex);
            lift_obj[lift_no].moving = 1;           /*-------------------------------------------------LIFT STARTS MOVING--------------------------------------------*/
            lift_obj[lift_no].getting_on = 0;
            lift_obj[lift_no].getting_off = 0;
            V(lift_obj_sem[lift_no].mutex);

            sleep(movement_time);               /*-------------------------LIFT IS MOVING FROM CURRENT FLOOR TO NEXT FLOOR---------------------------------*/

            /*-------------------------------------------LIFT REACHES NEXT FLOOR--------------------------------------*/
            P(lift_obj_sem[lift_no].mutex);
            lift_obj[lift_no].position--;
            current_floor = lift_obj[lift_no].position;
            V(lift_obj_sem[lift_no].mutex);
        }
    }
}

int main()
{
    /*----------------------------------TRAP THE CTRL+Z (OR SIGSTP SIGNAL)-------------------*/
    signal(SIGTSTP, catch_suspend);

    /*---------------------------------DEFINING BASIC SEMAPHORE STRUCUTE VARIABLES FOR P() AND v()---------------------*/
    pop.sem_num = vop.sem_num = 0;
    pop.sem_flg = vop.sem_flg = SEM_UNDO;
    pop.sem_op = -1;
    vop.sem_op = 1;
    /*-------------------------------------------------------------------------------------------------------------------*/

    srand(time(0));
    /*----------------------------------CREATING THE SHARED FLOOR MEMORY--------------------------------------*/

    floor_shm = shmget(IPC_PRIVATE, F * sizeof(floor_info), 0777 | IPC_CREAT);
    floor_obj = (floor_info *)shmat(floor_shm, 0, 0);                               /*-------------------ATTACHING FLOOR SHARED MEMORY WITH THE PARENT PROCESS-------------------*/

    for (int i = 0; i < F; i++)
    {
        floor_obj[i].n = i;
        floor_obj[i].waiting_to_go_up = floor_obj[i].waiting_to_go_down = 0;
        floor_obj[i].lift_no_to_go_up = floor_obj[i].lift_no_to_go_down = -1;
    }
    /*----------------------------------------------------------------------------------------------------------*/

    /*-----------------------------------------CREATE THE FLOOR SEMAPHORES------------------------------------*/

    for (int i = 0; i < F; i++)
    {
        floor_obj_sem[i].mutex1 = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(floor_obj_sem[i].mutex1, 0, SETVAL, 0);

        floor_obj_sem[i].mutex2 = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(floor_obj_sem[i].mutex2, 0, SETVAL, 0);

        floor_obj_sem[i].mutexUP = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(floor_obj_sem[i].mutexUP, 0, SETVAL, 1);

        floor_obj_sem[i].mutexDOWN = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(floor_obj_sem[i].mutexDOWN, 0, SETVAL, 1);

        floor_obj_sem[i].sem_up_arrow = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(floor_obj_sem[i].sem_up_arrow, 0, SETVAL, 0);

        floor_obj_sem[i].sem_down_arrow = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(floor_obj_sem[i].sem_down_arrow, 0, SETVAL, 0);
    }
    /*-----------------------------------------------------------------------------------------------------------*/

    /*------------------------------------CREATING SHARED LIFT MEMORY-------------------------------------------*/

    lift_shm = shmget(IPC_PRIVATE, M * sizeof(lift_info), 0777 | IPC_CREAT);
    lift_obj = (lift_info *)shmat(lift_shm, 0, 0);                          /*-------------------ATTACHING LIFT SHARED MEMORY WITH THE PARENT PROCESS-------------------*/

    for (int i = 0; i < M; i++)
    {
        lift_obj[i].no = i;
        lift_obj[i].position = rand() % (F - 1);
        lift_obj[i].direction = rand() % 2;
        lift_obj[i].people_in_lift = 0;
        lift_obj[i].moving = 1;
        memset(lift_obj[i].stop, 0, sizeof(lift_obj[i].stop));
        lift_obj[i].getting_on = lift_obj[i].getting_off = 0;
        lift_obj[i].capacity = (rand() % 5) + 2;
    }
    /*-----------------------------------------------------------------------------------------------------------*/

    /*-----------------------------------------CREATING LIFT SEMAPHORES------------------------------------------*/

    for (int i = 0; i < M; i++)
    {
        lift_obj_sem[i].mutex = semget(IPC_PRIVATE, 1, 0777 | IPC_CREAT);
        semctl(lift_obj_sem[i].mutex, 0, SETVAL, 1);

        for (int j = 0; j < F; j++)
        {
            lift_obj_sem[i].sem_stopsem[j] = semget(IPC_PRIVATE, 0, 0777 | IPC_CREAT);
            semctl(lift_obj_sem[i].sem_stopsem[j], 0, SETVAL, 0);
        }
    }
    /*-----------------------------------------------------------------------------------------------------------*/

    /*--------------------------------------CREATING PRERSON'S PROCESS-----------------------------------------*/
    pid_t person_processes[N];
    for (int i = 0; i < N; i++)
    {
        if ((person_processes[i] = fork()) == 0)
        {
            ischild = 1;
            /*--------------------------------ATTACHING SHARED MEMORY WITH PERSON PROCESS------------------------------*/
            floor_obj = (floor_info *)shmat(floor_shm, 0, 0);
            lift_obj = (lift_info *)shmat(lift_shm, 0, 0);
            /*-----------------------------------------------------------------------------------------------------------*/
            srand(getpid());
            Person_process();                   /*-------------CALL TO THE PRESON FUNCTION------------------------------*/
            return 0;
        }
    }

    //creating lift's processes
    pid_t lift_processes[M];
    for (int i = 0; i < M; i++)
    {
        if ((lift_processes[i] = fork()) == 0)
        {

            ischild = 1;
            /*--------------------------------ATTACHING SHARED MEMORY WITH LIFT PROCESS------------------------------*/
            floor_obj = (floor_info *)shmat(floor_shm, 0, 0);
            lift_obj = (lift_info *)shmat(lift_shm, 0, 0);
            /*-----------------------------------------------------------------------------------------------------------*/
            srand(getpid());
            Lift_process(i);                /*-------------CALL TO THE LIFT FUNCTION------------------------------*/
            return 0;
        }
    }
    /*--------------------------------------------PRINT DETAILS--------------------------------------*/
    print_details();
    return 0;
}