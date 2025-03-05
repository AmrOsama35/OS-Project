#define _GNU_SOURCE //Btzawed extensions lel C msh mwgoda 8er f linux m7tagin el extensions di 3shan el sched_setaffinity teshta8al
#include <stdio.h>
#include <unistd.h> //3shan el System calls
#include <stdlib.h>
#include <sched.h> //3shan fiha el sched_setaffinity function
#include <sys/wait.h> //3shan el wait(NULL)
#include <pthread.h> //3shan el threads
#include <sys/syscall.h>
#include <ctype.h>



void* alphabets(void* args) {


    char first_letter;
    printf("Please enter the first letter : \n");
    scanf(" %c" , &first_letter);
    first_letter = tolower(first_letter);
    while(first_letter < 'a' || first_letter > 'z') {
        printf("Invalid input please enter a letter\n");
        scanf(" %c" , &first_letter);
        first_letter = tolower(first_letter);
    }
    

    char second_letter;
    printf("Please enter the second letter : \n");
    scanf(" %c" , &second_letter);
    second_letter = tolower(second_letter);
    while(second_letter < 'a' || second_letter > 'z') {
        printf("Invalid input please enter a letter\n");
        scanf(" %c" , &second_letter);
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
        
    return NULL;


}

void* printing(void* args) {

    pthread_t thread_ID = pthread_self();

    printf("Thread ID is : %lu\n",(unsigned long)thread_ID);
    printf("We are Team\n");
    printf("We Love OS\n");


}

void* calculate(void* args) {

    int integer_one;
    int integer_two;

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

    if(scheduler_policy == 0) {
        pthread_attr_setschedpolicy(&thread_attributes , SCHED_FIFO);
    }
    else {
        pthread_attr_setschedpolicy(&thread_attributes , SCHED_RR);
    }

    scheduling_parameters.sched_priority = 10; //1 lowest 99 highest
    pthread_attr_setschedparam(&thread_attributes,&scheduling_parameters);

    pthread_create(&tid1 , &thread_attributes , alphabets , NULL);
    pthread_create(&tid2 , &thread_attributes , printing , NULL);
    pthread_create(&tid3 , &thread_attributes , calculate , NULL);

    pthread_join(tid1 , NULL);
    pthread_join(tid2 , NULL);
    pthread_join(tid3 , NULL);


    return 0;
}
