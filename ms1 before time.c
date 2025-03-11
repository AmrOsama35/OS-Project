#define _GNU_SOURCE //Btzawed extensions lel C msh mwgoda 8er f linux m7tagin el extensions di 3shan el sched_setaffinity teshta8al
#include <stdio.h>
#include <unistd.h> //3shan el System calls
#include <stdlib.h>
#include <sched.h> //3shan fiha el sched_setaffinity function
#include <sys/wait.h> //3shan el wait(NULL)
#include <pthread.h> //3shan el threads
#include <sys/syscall.h>
#include <ctype.h>
#include <time.h> //3shan el clock bayna e3ni 
#include <unistd.h>//3shan sysconf(_SC_PAGESIZE)
#include <sys/resource.h>

    //Equations :
    // execution_time = completion_time - start_time;
    // response = start_time - release_time;
    // waiting time = turnaround_time - execution_time;
    // turnaround_time = completion_time - release_time;

clock_t start_T1,start_T2,start_T3;      //start of execution of the threads 
clock_t end_T1,end_T2,end_T3;            //end of execution of the threads 
clock_t release_T1,release_T2,release_T3;//creations of the threads

double start_T1_t,start_T2_t,start_T3_t;        //start of execution of the threads (in time)
double end_T1_t,end_T2_t,end_T3_t;              //end of execution of the threads (in time)
double release_T1_t,release_T2_t,release_T3_t;  //creations of the threads (in time)
    
double exec_T1,exec_T2,exec_T3;     //Execution time of each thread
double resp_T1,resp_T2,resp_T3;     //Response time of each thread
double turn_T1,turn_T2,turn_T3;     //TurnAround time of each thread
double wait_T1,wait_T2,wait_T3;     //Wait time of each thread


//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------
double get_time_ms()
{
 struct timespec ts;
 clock_gettime(CLOCK_MONOTONIC, &ts);
 return ts.tv_sec * 1000.0 + ts.tv_nsec / 1.0e6; // Convert to milliseconds
}

void printing_threads_stats()
{
    //Thread 1
    printf("---------------------------------Thread 1---------------------------------\n");
    printf("Thread 1 execution time = %lf ms\n",exec_T1);// execution_time
    printf("Thread 1 response time = %lf ms\n",resp_T1);// response_time
    printf("Thread 1 turnaround time = %lf ms\n",turn_T1);// turnaround_time
    printf("Thread 1 waiting time = %lf ms\n",wait_T1);// waiting_time 
    
    //Thread 2
    printf("---------------------------------Thread 2---------------------------------\n");
    printf("Thread 2 execution time = %lf ms\n",exec_T2);// execution_time
    printf("Thread 2 response time = %lf ms\n",resp_T2);// response_time
    printf("Thread 2 turnaround time = %lf ms\n",turn_T2);// turnaround_time
    printf("Thread 2 waiting time = %lf ms\n",wait_T2);// waiting_time 

    //Thread 3
    printf("---------------------------------Thread 3---------------------------------\n");
    printf("Thread 3 execution time = %lf ms \n",exec_T3);// execution_time
    printf("Thread 3 response time = %lf ms\n",resp_T3);// response_time
    printf("Thread 3 turnaround time = %lf ms\n",turn_T3);// turnaround_time
    printf("Thread 3 waiting time = %lf ms\n",wait_T3);// waiting_time 
}

void get_memory_usage() {
        printf("Start of memory consumption stats : \n");
        printf("\n");
        long page_size = sysconf(_SC_PAGESIZE); // Get page size in bytes
       
        long size; //size: Total program size in pages.
        long resident;
        long shared;
        long text;
        long lib;
        long data;
        long dt;
        
        FILE *fp = fopen("/proc/self/statm", "r");
        if (!fp) {
            perror("Failed to open /proc/self/statm");
            return;
        }
        fscanf(fp, "%ld %ld %ld %ld %ld %ld %ld", &size, &resident, &shared, &text, &lib, &data, &dt);
        fclose(fp);
    
        printf("Total program size: %ld KB\n", (size * page_size) / 1024);// size(no.of pages)*page size (divide 1024 to convert it to KB)
        printf("Resident set size (RSS): %ld KB (Memory in RAM)\n", (resident * page_size) / 1024);
        printf("Shared pages: %ld KB\n", (shared * page_size) / 1024);
        printf("Code (text) segment: %ld KB\n", (text * page_size) / 1024);
        printf("Data + Stack size: %ld KB\n", (data * page_size) / 1024);
        printf("\n");
}
//---------------------------------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------------------------------

//-------------------------------Thread 1 function-------------------------------
void* alphabets(void* args) {
    start_T1=clock();//start of execution
    start_T1_t=get_time_ms();

    char first_letter,second_letter;
    printf("Please enter the first letter and second letter : \n");
    scanf(" %c %c",&first_letter,&second_letter);
    first_letter = tolower(first_letter);
    second_letter = tolower(second_letter);

    while((first_letter < 'a' || first_letter > 'z')||(second_letter < 'a' || second_letter > 'z')){
        printf("Invalid input please enter two letters \n");
        scanf(" %c %c",&first_letter,&second_letter);
        first_letter = tolower(first_letter);
        second_letter = tolower(second_letter);
    }

    
    char alphabets[] = "abcdefghijklmnopqrstuvwxyz";

    int start = first_letter % 'a';
    int end = second_letter % 'a';

    int temp = 0;

    if(start > end) {
        temp = start;
        start = end;
        end = temp;
    }


    for (int i = start ; i < end + 1 ; i++) {

        printf("%c" , alphabets[i]);

        if(i == end) {
            printf("\n");
        }

    }

    end_T1=clock();//end of execution
    end_T1_t=get_time_ms();
    
    exec_T1=1000*(((double)(end_T1-start_T1))/CLOCKS_PER_SEC);
    resp_T1=((double)(start_T1_t-release_T1_t));
    turn_T1=((double)(end_T1_t-release_T1_t));
    wait_T1=((double)turn_T1-exec_T1);
    

    return NULL;


}
//-------------------------------Thread 2 function-------------------------------
void* printing(void* args) {
    start_T2=clock();//start of execution
    start_T2_t=get_time_ms();

    pthread_t thread_ID = pthread_self();

    printf("Thread ID is : %lu\n",(unsigned long)thread_ID);
    printf("We are Team\n");
    printf("We Love OS\n");

    end_T2=clock();//end of execution
    end_T2_t=get_time_ms();

    exec_T2=1000*(((double)(end_T2-start_T2))/CLOCKS_PER_SEC);
    resp_T2=((double)(start_T2_t-release_T2_t));
    turn_T2=((double)(end_T2_t-release_T2_t));
    wait_T2=((double)turn_T2-exec_T2);
}
//-------------------------------Thread 3 function-------------------------------
void* calculate(void* args) {
    start_T3=clock();//start of execution
    start_T3_t=get_time_ms();

    int integer_one,integer_two;
    

    printf("Please enter 2 integers\n");

    scanf("%d %d" , &integer_one , &integer_two);

    int sum = 0;
    int product = 1;
    double average = 0;

    int temp = 0;

    if(integer_one > integer_two) {
        temp = integer_one;
        integer_one = integer_two;
        integer_two = temp;
    }
    
    for(int i = integer_one ; i < integer_two + 1 ; i++) {
        sum += i;
        product *= i;
    }

    average = sum/(integer_two - integer_one + 1);

    printf("For the numbers between the first and second integer : \nSum = %d\nProduct = %d\nAverage = %lf\n" , sum , product , average);
    
    end_T3=clock();//end of execution
    end_T3_t=get_time_ms();

    exec_T3=1000*(((double)(end_T3-start_T3))/CLOCKS_PER_SEC);
    resp_T3=((double)(start_T3_t-release_T3_t));
    turn_T3=((double)(end_T3_t-release_T3_t));
    wait_T3=((double)turn_T3-exec_T3);
    
    return NULL;
    
}

int main() {
    

   //Bt3ml define variable lel activation bits bta3et kol cpu 3ndk 3al gehaz
   cpu_set_t cpuActivationMask;

   //btsafar el activation bits le kol el cpus
   CPU_ZERO(&cpuActivationMask);

   //bt3ml set el activation bit bta3et cpu 0 bas to 1 3shan tsha8al only one cpu
   CPU_SET(0,&cpuActivationMask);

   //bt2ol lel scheduler y run process 0 (current process) and its children 7asab el activation bits lel cpus el e7na zabatnaha fo2
   sched_setaffinity(0,sizeof(cpu_set_t),&cpuActivationMask);

//   -----=======-----    -----=======-----    -----=======-----    -----=======-----    -----=======-----     

    
    struct rusage usage;
    struct timeval start_real, end_real;
    gettimeofday(&start_real, NULL);// Start measuring real elapsed time

    // Choose scheduling policy
    int scheduler_policy = 0;
    printf("Choose scheduling policy :\nEnter 0 for first in first out (FIFO)\nEnter 1 for Round Robin\n");
    scanf(" %d", &scheduler_policy);
    while(scheduler_policy != 0 && scheduler_policy != 1) {
        printf("Please choose a valid policy");
        scanf(" %d" , &scheduler_policy);
    }
    
    pthread_t tid1 , tid2 , tid3;
    pthread_attr_t thread_attributes;
    struct sched_param scheduling_parameters;

    pthread_attr_init(&thread_attributes);
    pthread_attr_setinheritsched(&thread_attributes , PTHREAD_EXPLICIT_SCHED);
    char schedule[30];
    if(scheduler_policy == 0) {
        pthread_attr_setschedpolicy(&thread_attributes , SCHED_FIFO);
        char schedule[30]="FIFO";
    }
    else {
        pthread_attr_setschedpolicy(&thread_attributes , SCHED_RR);
        char schedule[30]="Round Robin";
    }

    scheduling_parameters.sched_priority = 10; //1 lowest 99 highest
    pthread_attr_setschedparam(&thread_attributes,&scheduling_parameters);
    
    printf("---------------------Start of the program (%s)---------------------\n",schedule);
    release_T1=clock();//Release time of thread 1
    release_T1_t=get_time_ms();
    pthread_create(&tid1 , &thread_attributes , alphabets , NULL);
    
    release_T2=clock();//Release time of thread 2
    release_T2_t=get_time_ms();
    pthread_create(&tid2 , &thread_attributes , printing , NULL);

    release_T3=clock();//Release time of thread 3
    release_T3_t=get_time_ms();
    pthread_create(&tid3 , &thread_attributes , calculate , NULL);
    
    
    //release_T1/T2/T3=clock() is before creation to avoid starting the execution of the function before calculating the release time 
    
    pthread_join(tid1 , NULL);
    pthread_join(tid2 , NULL);
    pthread_join(tid3 , NULL);
    printf("---------------------END of the program---------------------\n");

    
   
    // Get CPU time usage
    getrusage(RUSAGE_SELF, &usage);
    gettimeofday(&end_real, NULL);
    
    // Compute real elapsed time
    double total_time = (end_real.tv_sec - start_real.tv_sec) + 
                        (end_real.tv_usec - start_real.tv_usec) / 1.0e6;

    // Compute CPU time (User + System)
    double cpu_time_used = (usage.ru_utime.tv_sec + usage.ru_stime.tv_sec) +
                           (usage.ru_utime.tv_usec + usage.ru_stime.tv_usec) / 1.0e6;

    // Calculate CPU Useful_work
    double cpu_Useful_work = (cpu_time_used / total_time) * 100;

    // Print results
    printf("\n");
    printf("Total Execution Time: %.2f seconds\n", total_time);
    printf("CPU Time Consumed: %lf seconds\n", cpu_time_used);
    printf("CPU Useful work: %.2f%%\n", cpu_Useful_work);
    
    //Sum of waiting time of all threads
    double sum_of_waiting_time=wait_T1+wait_T2+wait_T3;
    //Calculate cpu utilization
    double cpu_utilization=(exec_T1+exec_T2+exec_T3)/((exec_T1+exec_T2+exec_T3)+sum_of_waiting_time);
    printf("Cpu utilization : %lf%% \n",cpu_utilization*100);
    
    get_memory_usage(); //Print threads stats
    
    printing_threads_stats();//Print threads stats
    

    return 0;
}
