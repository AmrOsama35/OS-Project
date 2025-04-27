#include <gtk/gtk.h>
#include <stdio.h> // Required for sprintf
#include <string.h> // Required for strcmp
#include <stdlib.h> // Required for malloc and free
#include <glib.h>   // Required for GList and g_list_free
#include <gtk/gtkmenubutton.h> // Required for GtkMenuButton
#include <gtk/gtkpopover.h>    // Required for GtkPopover
#include <errno.h>
#include <limits.h>
#include <ctype.h>
#include <unistd.h> //

#define DEBUG_MSG(msg) printf("[DEBUG] %s:%d: %s\n", __FILE__, __LINE__, msg) //Define dubugging macro used to print a custom error message with the file name and line number at which it occured
#define MAX_LINE_LENGTH 128 //Define maximum instruction line length
#define MAX_TOKEN_LENGTH 128 //Define maximum word length in an instruction
#define MAX_TOKENS_PER_INSTRUCTION 64 //Define maximum tokens in an instruction
#define MEMORY_SIZE 80 //Define memory size
#define MAX_NUMBER_OF_PROCESSES 8 //Define maximum number of processes
#define WORD_VALUE_SIZE 128 //Define number of characters in the name and value field for each memory word

#define BLOCK_USER_INPUT 1
#define BLOCK_USER_OUTPUT 2
#define BLOCK_FILE 3
#define UNBLOCK_USER_INPUT 4
#define UNBLOCK_USER_OUTPUT 5
#define UNBLOCK_FILE 6

// Structure to hold pointers to widgets needed in the button callback
typedef struct {
    GtkWidget *entry;
    GtkWidget *button;
    GtkWidget *execution_vbox;   // Pointer to the vertical box in R1 C2
    GtkWidget *control_buttons_vbox; // Pointer to the vertical box in R1 C4 C2
    GtkWidget *dropdown_button; // Pointer to the dropdown button in R1 C4 C3 (Renamed)
} ButtonCallbackData;

typedef struct {
    GtkWidget *num_of_processes;
    GtkWidget *clock_cycle;
    GtkWidget *active_scheduler;
} OverviewSectionData;

// typedef struct {
//     GtkWidget *start_button;
//     GtkWidget *stop_button;
//     GtkWidget *reset_button;
// } ControlButtons;

GtkWidget *start_button;
GtkWidget *execution_step_by_step_button;
GtkWidget *execution_auto_button;
GtkWidget *adjust_quantum_button;
GtkWidget *dropdown_button;
GtkWidget *main_vbox;

typedef struct {
    char* process;
    int arrival_time;
} Process;

static int insertionPointer = 0;

Process Processes[3];

// int scheduler = 0;


//ALI NOTES:
//snprintf is more safe than strcup

//===============================================Types START==================================================
typedef struct{
    char* name;
    char* value;
} Memory_Word;

typedef struct{
    char* path;
    int arrival_time;
} Program_Arrival_Pair ;

typedef struct {
    Memory_Word* process_id;
    Memory_Word* state;
    Memory_Word* priority;
    Memory_Word* program_counter;
    Memory_Word* higher;
    Memory_Word* lower;
} PCB_Ref;
//===============================================Types END==================================================



//===============================================Global Variables START==================================================
int clock_time = 0;
int scheduler = 0;

Memory_Word memory[MEMORY_SIZE];
int memory_insertion_ptr = 0; //Holds the next empty index in memory
int memory_search_ptr = 0;
int last_process_id = 0; //Keep incrementing while inserting in memory to know what process id you must write in the Process ID section of the PCB

//Semaphore for each resource :
int user_input_s = 1;
int user_output_s = 1;
int file_s = 1;

char* executing_process_id;
char* cur_executing_instruction;

int rr_quanta;
int counting_q = 0;

int program_queue_pointer = 0;

PCB_Ref running_process; //Holds the PCB reference to the process currently running
    int running_flag = 0; //Indication on whether or not a process is currently running (if 0 then override whatever is in there as it has been deemed as finished execution)
    int running_process_higher_bound; //Holds the running process higher bound to compare the Program Counter with it after each clock cycle to know whether the process finished and should be removed from the ready queue or not

Program_Arrival_Pair program_queue[MAX_NUMBER_OF_PROCESSES]; //Queue where we store the name of the program and at which clock cycle it is supposed to arrive

PCB_Ref ready_queue[MAX_NUMBER_OF_PROCESSES]; //Ready Queue to be used by FIFO and RR schedulers
    int ready_queue_start_pointer = 0;
    int ready_queue_end_pointer = 0;

//MLFQ Ready Queues :
PCB_Ref mlfq_rq_1[MAX_NUMBER_OF_PROCESSES];
    int mlfq_rq_1_start_pointer = 0;
    int mlfq_rq_1_end_pointer = 0;

PCB_Ref mlfq_rq_2[MAX_NUMBER_OF_PROCESSES];
    int mlfq_rq_2_start_pointer = 0;
    int mlfq_rq_2_end_pointer = 0;

PCB_Ref mlfq_rq_3[MAX_NUMBER_OF_PROCESSES];
    int mlfq_rq_3_start_pointer = 0;
    int mlfq_rq_3_end_pointer = 0;

PCB_Ref mlfq_rq_4[MAX_NUMBER_OF_PROCESSES];
    int mlfq_rq_4_start_pointer = 0;
    int mlfq_rq_4_end_pointer = 0;
    
//Blocked queue for each resource :
PCB_Ref input_blocked_queue[MAX_NUMBER_OF_PROCESSES];
    int input_bq_start_pointer = 0;
    int input_bq_end_pointer = 0;

PCB_Ref output_blocked_queue[MAX_NUMBER_OF_PROCESSES];
    int output_bq_start_pointer = 0;
    int output_bq_end_pointer = 0;

PCB_Ref file_blocked_queue[MAX_NUMBER_OF_PROCESSES];
    int file_bq_start_pointer = 0;
    int file_bq_end_pointer = 0;
//===============================================Global Variables END==================================================



//===============================================Memory Operations START==================================================
void malloc_memory_word(int index_in_memory) {

    memory[index_in_memory].name = malloc(WORD_VALUE_SIZE);
    if(!memory[index_in_memory].name){ //Crash if malloc() failed to allocate space in RAM
        DEBUG_MSG("Could not allocate enough memory for this application to run");
        exit(1);
    }

    memory[index_in_memory].value = malloc(WORD_VALUE_SIZE);
    if(!memory[index_in_memory].value){ //Crash if malloc() failed to allocate space in RAM
        DEBUG_MSG("Could not allocate enough memory for this application to run");
        exit(1);
    }

}


void memory_free(Memory_Word memory[]){

    for(int i = 0; i < memory_insertion_ptr ; i++) {
        
        if(memory[i].name) {

            free(memory[i].name);
            memory[i].name = NULL;

        }
        
        if(memory[i].value) {

            free(memory[i].value);
            memory[i].value = NULL;

        }

    }

}


void load_program_to_memory(char* program_path) {

    if(memory_insertion_ptr > MEMORY_SIZE - 6) { //Crash if user tries to load a process when there is no remaining space in memory

        DEBUG_MSG("Memory Slots Exhausted");
        exit(1);

    }


    FILE *file;
    char cur_line[MAX_LINE_LENGTH];

    file = fopen(program_path, "r");
    if (file == NULL) {
        DEBUG_MSG("Error Opening File");
        exit(1);
    }

    //Loads PCB of process into memory :
    for(int i = memory_insertion_ptr ; i < memory_insertion_ptr + 6 ; i++) { //Allocates space for 6 (name , value) pointer pair in memory words

        malloc_memory_word(i);

    }
    snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "%s", "Process ID");
    snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%d", ++last_process_id);
    memory_insertion_ptr++;

    snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "%s", "Process State");
    snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%s", "Ready");
    memory_insertion_ptr++;

    snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "%s", "Current Priority");
    snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%d", 1);
    memory_insertion_ptr++;

    snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "%s", "Program Counter"); 
    snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%d", memory_insertion_ptr + 3);
    memory_insertion_ptr++;

    snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "%s", "Lower");  
    snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%d", memory_insertion_ptr - 4);
    memory_insertion_ptr++;

    int higher_word_index = memory_insertion_ptr; //Saving index where I will initialize the Higher section of the PCB , so that I can do it after knowing how many instructions I have
    memory_insertion_ptr++;
    
    
    //Loads program instructions into memory :
    int i = 1;
    while (fgets(cur_line, sizeof(cur_line), file) != NULL) {

        if(memory_insertion_ptr > MEMORY_SIZE - 1) { //Crash if user tries to load a program instruction when there is no remaining space in memory

            DEBUG_MSG("Memory Slots Exhausted");
            exit(1);
    
        }

        malloc_memory_word(memory_insertion_ptr);

        size_t line_length = strlen(cur_line);
        if (line_length > 0 && cur_line[line_length-1] == '\n') {
            cur_line[line_length-1] = '\0';
        }

        snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "I%d", i);
        snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%s", cur_line);

        i++;
        memory_insertion_ptr++;

    }


    fclose(file);


    //Creates variable spaces in memory :
    if(memory_insertion_ptr > MEMORY_SIZE - 3) { //Crash if user tries to load a program when there is no remaining space for its variables in memory

        DEBUG_MSG("Memory Slots Exhausted");
        exit(1);

    }
    for (int j = 0 ; j < 3 ; j++) {

        malloc_memory_word(memory_insertion_ptr);
        snprintf(memory[memory_insertion_ptr].name, WORD_VALUE_SIZE, "%s", "Var");
        snprintf(memory[memory_insertion_ptr].value, WORD_VALUE_SIZE, "%s", "NULL");
        memory_insertion_ptr++;

    }


    //Initializes the Higher section of the PCB with at the index we have saved : 
    snprintf(memory[higher_word_index].name, WORD_VALUE_SIZE, "%s", "Higher");
    snprintf(memory[higher_word_index].value, WORD_VALUE_SIZE, "%d", memory_insertion_ptr - 1);
    
}


void print_memory(){

    int i = 0;
    while(i < memory_insertion_ptr) {

        printf("%s %s\n" , memory[i].name , memory[i].value);
        i++;

    }

}
//===============================================Memory Operations END==================================================



//===============================================MISC Functions START==================================================
int split_string(const char* input_string, char* output_tokens[], int max_tokens, int max_token_len) {
    
    if (!input_string || !output_tokens || max_tokens <= 0 || max_token_len <= 0) { //Check for invalid input scuh as null pointers for input string or output tokens buffer

        DEBUG_MSG("Could not split string, invalid split_string() arguments");
        return -1;

    }

    int token_counter = 0; //Counter for how many tokens we have stored
    const char* current_character = input_string; //Initializing pointer to iterate over input string

    
    while (*current_character && token_counter < max_tokens) {
        
        while (isspace(*current_character)) {

            current_character++; //Skip leading spaces to find the start of the next word

        } 

        if (*current_character == '\0') break; //If we reached end of string after skipping spaces, exit loop

        const char* start = current_character; //Mark the start of the word after the leading spaces

        while (!isspace(*current_character) && *current_character != '\0') {

            current_character++; //Iterate over the string until we hit a space or end of string

        } 

        int word_length = current_character - start; //Calculate length of the word

        snprintf(output_tokens[token_counter], max_token_len, "%.*s", word_length, start); //Copies characters from the start marker of my word up until the word length into the output tokens buffer

        token_counter++; //Move to the next output token buffer slot

    }

    return token_counter; // Return the number of tokens written to the output
}



int str_to_int(const char *string_to_be_converted) {

    char *endptr; //Pointer to character at which the first non-numerical character is encountered and parsing is stopped
    errno = 0; //Error flag resetting

    long value = strtol(string_to_be_converted, &endptr, 10); //Storing the conversion result in a variable called value (using long since strtol returns a long)

    if (errno == ERANGE || value < INT_MIN || value > INT_MAX) { // The string value that you are trying to convert to int is larger than the int datatype can hold
        DEBUG_MSG("String conversion to int failed because string is too large");
        exit(1);
    }

    if (string_to_be_converted == endptr) { // The string value that you are trying to convert to int does not have any numerical characters
        DEBUG_MSG("String conversion to int failed because no digits were found");
        exit(1);  
    }

    return (int)value; //Returning the converted value but type casting it from a long to an int first

}



void check_arrival(Program_Arrival_Pair program_queue[], PCB_Ref ready_queue[]) {

    int i = 0;
    while(i < MAX_NUMBER_OF_PROCESSES && program_queue[i].path != NULL) {

        if(program_queue[i].arrival_time == clock_time) {

            load_program_to_memory(program_queue[i].path);

            for(int j = 0 ; j < memory_insertion_ptr ; j++) {
                
                int process_higher_bound;
                if(strcmp(memory[j].name, "Process ID") == 0) {

                    if(str_to_int(memory[j].value) == last_process_id) {

                        PCB_Ref pcb_reference;

                        pcb_reference.process_id = &memory[j];
                        pcb_reference.state = &memory[j+1];
                        pcb_reference.priority = &memory[j+2];
                        pcb_reference.program_counter = &memory[j+3];
                        pcb_reference.lower = &memory[j+4];
                        pcb_reference.higher = &memory[j+5];

                        if(scheduler == 0 || scheduler == 1) {
                            ready_queue[ready_queue_end_pointer] = pcb_reference;
                            ready_queue_end_pointer = (ready_queue_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                        }
                        else {

                            mlfq_rq_1[mlfq_rq_1_end_pointer] = pcb_reference;
                            mlfq_rq_1_end_pointer = (mlfq_rq_1_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;

                        }

                    }
                    else {

                        process_higher_bound = str_to_int(memory[j+5].value);
                        j = process_higher_bound;

                    }

                }

            }

        }

        i++;

    }

}




void free_instruction_buffer(char* instruction_buffer[]){ //Frees the malloced memory for each token inside the instruction buffer used in decode_and_execute()

    for(int i = 0 ; i < MAX_TOKENS_PER_INSTRUCTION ; i++) {

        free(instruction_buffer[i]);

    }

}



int programs_finished() { //Checks if all programs finished or not

    int process_pc;
    int process_higher_bound;

    int i = 0;
    while(i < memory_insertion_ptr) {

        process_pc = str_to_int(memory[i+3].value);
        process_higher_bound = str_to_int(memory[i+5].value);
        
        if(process_pc != process_higher_bound - 2) {

            return 0;

        }
        
        i = process_higher_bound + 1;

    }


    int max_arrival_time = 0;

    int j = 0;
    while(j < MAX_NUMBER_OF_PROCESSES && program_queue[j].path != NULL) {

        if(program_queue[j].arrival_time > max_arrival_time) {

            max_arrival_time = program_queue[j].arrival_time;

        }

        j++;

    }

    if(clock_time <= max_arrival_time) {

        return  0;

    }
    else {

        return 1;

    }

}



void print_processes_queue(const char* queue_name, PCB_Ref processes[], int start_pointer, int end_pointer) {
    
    printf("%s: [ ", queue_name);

    if (start_pointer == end_pointer && processes[start_pointer].process_id == NULL) { //If queue is empty
        
        printf("(empty) ");

    }
    else {

        int current_index = start_pointer;
        while (current_index != end_pointer) {

            printf("PID %s ", processes[current_index].process_id->value);
            current_index = (current_index + 1) % MAX_NUMBER_OF_PROCESSES;

        }
    }

    printf("]\n");
}


//===============================================MISC Functions END==================================================



//===============================================Decode And Execute START==================================================
int decode_and_execute(char* instruction, char* process_memory_lower_boundary) {

    int decode_result = 0;

    char* instruction_buffer[MAX_TOKENS_PER_INSTRUCTION];
    for(int i = 0 ; i < MAX_TOKENS_PER_INSTRUCTION ; i++) {

        instruction_buffer[i] = malloc(MAX_TOKEN_LENGTH); //Allocate space in RAM for each instruction token in the instruction buffer
        if(!instruction_buffer[i]) { //Crash if malloc() failed to allocate space in RAM

            DEBUG_MSG("Could not allocate enough memory for this application to run");
            exit(1);
        }

        memset(instruction_buffer[i], 0, MAX_TOKEN_LENGTH); //Initializes instruction_buffer[i] as an empty string to prevent decoding garbage data

    }

    split_string(instruction, instruction_buffer, MAX_TOKENS_PER_INSTRUCTION, MAX_TOKEN_LENGTH); //Splits the instruction into tokens placed in the instruction buffer

    int lower_bound = str_to_int(process_memory_lower_boundary); //Lower boundary in memory for the process executing the instruction
    int higher_bound = str_to_int(memory[lower_bound+5].value); //Higher boundary in memory for the process executing the instruction

    if(strcmp(instruction_buffer[0], "print") == 0) {

        int found_variable_flag = 0;  
      
        for(int i = higher_bound - 2 ; i <= higher_bound ; i++){ //Iterate over the data segment of the executing process in memory
           
            if(strcmp(memory[i].name, instruction_buffer[1]) == 0) { //If the variable name after the print token is found in memory

                found_variable_flag = 1; //Indicates that the variable to be printed was found in memory
                printf("%s\n", memory[i].value); //Print the value of that variable
                break;
               
            }
        
        }

        if(!found_variable_flag) {

            DEBUG_MSG("Failed trying to print variable not in memory");
            free_instruction_buffer(instruction_buffer);
            exit(1);

        }

    }
    else if(strcmp(instruction_buffer[0], "assign") == 0) {

        int found_variable_flag = 0;

        
        if(strcmp(instruction_buffer[2], "readFile") == 0) {

            char file_content[WORD_VALUE_SIZE];
            char filename[WORD_VALUE_SIZE];
            for(int i = higher_bound - 2 ; i <= higher_bound  ; i++) {

                if(strcmp(instruction_buffer[3] , memory[i].name)== 0) {
                    snprintf(filename , WORD_VALUE_SIZE ,"%s" ,memory[i].value);
                }

            }


            FILE* file = fopen(filename, "rb");
            if (!file) {

                printf("Error opening file named %s", instruction_buffer[3]);
                DEBUG_MSG("Error opening file");
                free_instruction_buffer(instruction_buffer);
                exit(1);

            }

            fseek(file, 0, SEEK_END); //Places file pointer at the end of the file : In file, move by 0 bytes starting from SEEK_END (end of file)
            long file_size = ftell(file); //Saves current position of the file pointer
            rewind(file); //Returns the file pointer to the start of the file

            if (file_size < 0 || file_size >= (long) sizeof(file_content)) {

                DEBUG_MSG("File is too large or size error\n");
                fclose(file);
                free_instruction_buffer(instruction_buffer);
                exit(1);

            }

            size_t bytes_read = fread(file_content, 1, file_size, file); //Reads up to file_size bytes from 'file' into 'file_content', treating each byte as a 1-byte element and returns number of bytes read

            if(bytes_read < file_size) { //If the bytes read from fread are less than the file size

                DEBUG_MSG("An error occured while trying to read a file");
                free_instruction_buffer(instruction_buffer);
                exit(1);

            }

            fclose(file);

            file_content[bytes_read] = '\0'; // Null-terminate the content read from the file


            for(int i = higher_bound - 2 ; i <= higher_bound ; i++){ //Iterate over the data segment of the executing process in memory
           
                if(strcmp(memory[i].name, instruction_buffer[1]) == 0) { //If the variable name after the assign token is found in memory
    
                    found_variable_flag = 1; //Indicates that the variable to be assigned was found in memory
                    snprintf(memory[i].value, WORD_VALUE_SIZE, "%s", file_content); //Change the value of the variable in memory to the value in the file_content buffer
                    break;
                   
                }
            
            }

            if(!found_variable_flag) {

                for(int i = higher_bound - 2 ; i <= higher_bound ; i++) {

                    if(strcmp(memory[i].name, "Var") == 0) {

                        found_variable_flag = 1; //Indicates empty space was found to house variable
                        snprintf(memory[i].name, WORD_VALUE_SIZE, "%s", instruction_buffer[1]); //Change the empty variable name in memory "Var" to the variable name in the instruction
                        snprintf(memory[i].value, WORD_VALUE_SIZE, "%s", file_content); //Change the value of the variable in memory to the value in the instruction
                        break;

                    }

                }

            }

            if(!found_variable_flag) {

                DEBUG_MSG("Failed assigning variable because no data space remaining for process");
                free_instruction_buffer(instruction_buffer);
                exit(1);

            }

        }
        else {

            for(int i = higher_bound - 2 ; i <= higher_bound ; i++){ //Iterate over the data segment of the executing process in memory
           
                if(strcmp(memory[i].name, instruction_buffer[1]) == 0) { //If the variable name after the assign token is found in memory
    
                    found_variable_flag = 1; //Indicates that the variable to be assigned was found in memory

                    if(strcmp(instruction_buffer[2], "input") == 0) {

                        char input_buffer[WORD_VALUE_SIZE];
                        printf("Please enter value for variable %s :\n", instruction_buffer[1]);
                        fgets(input_buffer, sizeof(input_buffer), stdin); //Takes input from user safely (better than scanf as scanf can cause buffer overflow)
                        input_buffer[strcspn(input_buffer, "\n")] = '\0'; //strcspn() finds the index of first occurance of "\n" and then we change the character at that index into the null terminator to remove the new line from the string
                        snprintf(memory[i].value, WORD_VALUE_SIZE, "%s", input_buffer); //Assigns the variable with the data entered by the user and saved in the input buffer

                    }
                    else {

                    snprintf(memory[i].value, WORD_VALUE_SIZE, "%s", instruction_buffer[2]); //Change the value of the variable in memory to the value in the instruction
                    break;

                    }

                }
            
            }

            if(!found_variable_flag) {

                for(int i = higher_bound - 2 ; i <= higher_bound ; i++) {

                    if(strcmp(memory[i].name, "Var") == 0) {

                        found_variable_flag = 1; //Indicates empty space was found to house variable
                        snprintf(memory[i].name, WORD_VALUE_SIZE, "%s", instruction_buffer[1]); //Change the empty variable name in memory "Var" to the variable name in the instruction

                        if(strcmp(instruction_buffer[2], "input") == 0) {

                            char input_buffer[WORD_VALUE_SIZE];
                            printf("Please enter value for variable %s :\n", instruction_buffer[1]);
                            fgets(input_buffer, sizeof(input_buffer), stdin); //Takes input from user safely (better than scanf as scanf can cause buffer overflow)
                            input_buffer[strcspn(input_buffer, "\n")] = '\0'; //strcspn() finds the index of first occurance of "\n" and then we change the character at that index into the null terminator to remove the new line from the string
                            snprintf(memory[i].value, WORD_VALUE_SIZE, "%s", input_buffer); //Assigns the variable with the data entered by the user and saved in the input buffer
                            break;

                        }
                        else {

                            snprintf(memory[i].value, WORD_VALUE_SIZE, "%s", instruction_buffer[2]); //Change the value of the variable in memory to the value in the instruction
                            break;

                        }
                        
                    }

                }

            }

            if(!found_variable_flag) {

                DEBUG_MSG("Failed assigning variable because no data space remaining for process");
                free_instruction_buffer(instruction_buffer);
                exit(1);

            }

        }
        
    }
    else if(strcmp(instruction_buffer[0], "writeFile") == 0) {
        
        if (instruction_buffer[1] == NULL || instruction_buffer[2] == NULL) { /* Handle missing arguments */ return -1; }
        
        
        // Find the filename
          char* filename = NULL;
         for(int i = higher_bound - 2 ; i <= higher_bound ; i++){
              if(i >= lower_bound && i < memory_insertion_ptr && memory[i].name != NULL && strcmp(memory[i].name, instruction_buffer[1]) == 0) {
                   filename = memory[i].value;
                   break;
              }
         }
        
        // Find the data to write it in the filename
        char* value_to_write = NULL;
         for(int i = higher_bound - 2 ; i <= higher_bound ; i++){
              if(i >= lower_bound && i < memory_insertion_ptr && memory[i].name != NULL && strcmp(memory[i].name, instruction_buffer[2]) == 0) {
                   value_to_write = memory[i].value;
                   break;
              }
         }


        FILE *fp = fopen(filename, "w");
        if (!fp) {
            perror("fopen failed"); 
            return -1;
        }
        if (fputs(value_to_write, fp) == EOF) {
            perror("fputs failed");
            fclose(fp); 
            return -1;
        }
        if (fclose(fp) == EOF) {
            perror("fclose failed");
            return -1;
        }

    }
    else if(strcmp(instruction_buffer[0], "printFromTo") == 0) {

        int from , to;
        
        for(int i = higher_bound - 2 ; i <= higher_bound ; i++){
            if(strcmp(memory[i].name, instruction_buffer[1]) == 0) {
                from = atoi(memory[i].value);
                break;
            }
        }

        for(int j = higher_bound - 2 ; j <= higher_bound ; j++){
            if(strcmp(memory[j].name, instruction_buffer[2]) == 0) {
                to = atoi(memory[j].value);
                break;
            }
        }
        for(int k = from ; k <= to ; k++) {
            printf("%d " , k);
        }
        printf("\n");
        
    }
    else if(strcmp(instruction_buffer[0], "semWait") == 0) {

        if(strcmp(instruction_buffer[1], "userInput") == 0){

            user_input_s--;
            if(user_input_s < 0) {

                decode_result = BLOCK_USER_INPUT;

            }

        }
        else if(strcmp(instruction_buffer[1], "userOutput") == 0){

            user_output_s--;
            if(user_output_s < 0) {

                decode_result = BLOCK_USER_OUTPUT;

            }

        }
        else if(strcmp(instruction_buffer[1], "file") == 0){

            file_s--;
            if(file_s < 0) {

                decode_result = BLOCK_FILE;

            }
        
        }
        else { //If token after semWait not userInput, userOutput or File then crash

            DEBUG_MSG("semWait failed on unknown resource");
            free_instruction_buffer(instruction_buffer);
            exit(1);

        }

    }
    else if(strcmp(instruction_buffer[0], "semSignal") == 0) {
        
        if(strcmp(instruction_buffer[1], "userInput") == 0){

            user_input_s++;
            decode_result = UNBLOCK_USER_INPUT;

        }
        else if(strcmp(instruction_buffer[1], "userOutput") == 0){

           user_output_s++;
           decode_result = UNBLOCK_USER_OUTPUT;

        }
        else if(strcmp(instruction_buffer[1], "file") == 0){

           file_s++;
           decode_result = UNBLOCK_FILE;

        }
        else { //If token after semSignal not userInput, userOutput or File then crash

            DEBUG_MSG("semSignal failed on unknown resource");
            free_instruction_buffer(instruction_buffer);
            exit(1);

        }

    }
    else { //Crash if you recieve an unknown instruction keyword

        DEBUG_MSG("Execution failed because of unknown instruction keyword");
        free_instruction_buffer(instruction_buffer);
        exit(1);

    }

    
    free_instruction_buffer(instruction_buffer);

    int pc =  str_to_int(memory[lower_bound+3].value); //Get the current Program Counter value in memory for the executing process
    snprintf(memory[lower_bound+3].value, WORD_VALUE_SIZE, "%d", pc + 1); //Increment the Program Counter in memory after execution

    return decode_result;

}
//===============================================Decode And Execute END==================================================

//===============================================Round Robin START==================================================
void round_robin() {

    executing_process_id = NULL;
    cur_executing_instruction = NULL;

   
    static PCB_Ref finished_quanta_process; //Holds the PCB reference the should be popped from the ready queue and placed back again at the end in the next clock cycle after the flag was raised
    static int finished_quanta_flag = 0;

    static PCB_Ref blocked_process; //Holds the PCB reference to the process that tried to semWait but was blocked so it can be added again in the next cycle
    static int blocking_flag = 0;

    check_arrival(program_queue, ready_queue);

    if(blocking_flag) {

        switch (blocking_flag) {

            case BLOCK_USER_INPUT:
                input_blocked_queue[input_bq_end_pointer] = blocked_process;
                input_bq_end_pointer = (input_bq_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                break;
        
            case BLOCK_USER_OUTPUT:
                output_blocked_queue[output_bq_end_pointer] = blocked_process;
                output_bq_end_pointer = (output_bq_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                break;
        
            case BLOCK_FILE:
                file_blocked_queue[file_bq_end_pointer] = blocked_process;
                file_bq_end_pointer = (file_bq_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                break;

        }

        blocking_flag = 0;
        counting_q = 0;

    }

    if(finished_quanta_flag) {

        snprintf(finished_quanta_process.state->value, WORD_VALUE_SIZE, "%s", "Ready");
        ready_queue[ready_queue_end_pointer] = finished_quanta_process;
        ready_queue_end_pointer = (ready_queue_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
        finished_quanta_flag = 0;

    }

    if(!running_flag && ready_queue[ready_queue_start_pointer].process_id != NULL) {

        running_process = ready_queue[ready_queue_start_pointer];
        ready_queue_start_pointer = (ready_queue_start_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
        snprintf(running_process.state->value, WORD_VALUE_SIZE, "%s", "Running");
        running_flag = 1;

        running_process_higher_bound = str_to_int(running_process.higher->value);

    }
    
    if(running_flag) {

        int running_process_pc = str_to_int(running_process.program_counter->value);
        cur_executing_instruction = memory[running_process_pc].value;
        int decode_result = decode_and_execute(cur_executing_instruction, running_process.lower->value);

        if(decode_result) {

            switch (decode_result) {

                case BLOCK_USER_INPUT:
                    blocked_process = running_process;
                    blocking_flag = BLOCK_USER_INPUT;
                    running_flag = 0;
                    break;
            
                case BLOCK_USER_OUTPUT:
                    blocked_process = running_process;
                    blocking_flag = BLOCK_USER_OUTPUT;
                    running_flag = 0;
                    break;
            
                case BLOCK_FILE:
                    blocked_process = running_process;
                    blocking_flag = BLOCK_FILE;
                    running_flag = 0;
                    break;

                case UNBLOCK_USER_INPUT:
                    if(input_blocked_queue[input_bq_start_pointer].process_id != NULL) {

                        ready_queue[ready_queue_end_pointer] = input_blocked_queue[input_bq_start_pointer];
                        input_bq_start_pointer = (input_bq_start_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                        ready_queue_end_pointer = (ready_queue_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;

                    }
                    break;
            
                case UNBLOCK_USER_OUTPUT:
                    if(output_blocked_queue[output_bq_start_pointer].process_id != NULL) {

                        ready_queue[ready_queue_end_pointer] = output_blocked_queue[output_bq_start_pointer];
                        output_bq_start_pointer = (output_bq_start_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                        ready_queue_end_pointer = (ready_queue_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;

                    }
                    break;
            
                case UNBLOCK_FILE:
                    if(file_blocked_queue[file_bq_start_pointer].process_id != NULL) {

                        ready_queue[ready_queue_end_pointer] = file_blocked_queue[file_bq_start_pointer];
                        file_bq_start_pointer = (file_bq_start_pointer + 1) % MAX_NUMBER_OF_PROCESSES;
                        ready_queue_end_pointer = (ready_queue_end_pointer + 1) % MAX_NUMBER_OF_PROCESSES;

                    }
                    break;
    
            }

        }

        executing_process_id = running_process.process_id->value;

        counting_q++;

        if(str_to_int(running_process.program_counter->value) == running_process_higher_bound - 2) {

            snprintf(running_process.state->value, WORD_VALUE_SIZE, "%s", "Completed");
            running_flag = 0;
            counting_q = 0;

        }

    }

    if(counting_q == rr_quanta){

        finished_quanta_flag = 1;
        finished_quanta_process = running_process;
        running_flag = 0;
        counting_q = 0;

    }

    
}
//===============================================Round Robin END==================================================


//-------------------------------------------Handlers to connect Backend and Frontend
void add_program_to_program_queue(char *file_path, int arrival_time) {
    program_queue[program_queue_pointer].path = file_path;
    program_queue[program_queue_pointer].arrival_time = arrival_time;

    program_queue_pointer++;
}


//-------------------------------------------------------------- GTK Code


// Function to create a basic placeholder block (e.g., a Frame with a Label)
// This function is used by the column/row functions to create their content.
// We keep this function as it's still used for other blocks.
GtkWidget* create_block(const char* label_text) {
    GtkWidget *frame = gtk_frame_new(NULL); // Create a frame with no title
    // Use an inner shadow for the innermost blocks
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
    GtkWidget *label = gtk_label_new(label_text); // Create a label with the given text
    gtk_container_set_border_width(GTK_CONTAINER(frame), 5); // Add padding inside the frame

    // Add the label to the frame
    gtk_container_add(GTK_CONTAINER(frame), label);

    // Set expand and fill properties so the block tries to fill its allocated space
    // These can be overridden by the packing options in parent containers.
    gtk_widget_set_vexpand(frame, TRUE); // Allow vertical expansion by default
    gtk_widget_set_hexpand(frame, TRUE); // Allow horizontal expansion by default

    gtk_widget_set_halign(frame, GTK_ALIGN_FILL);
    gtk_widget_set_valign(frame, GTK_ALIGN_FILL);

    return frame;
}

// --- Functions for creating content within columns in Row 3 ---

// Function to create Row 3: contains a single block
// It takes the parent vertical box (main_vbox) as an argument
// This function remains the same as before
void create_row3(GtkWidget *parent_vbox) {
    // Row 3 is a single block that spans the width
    GtkWidget *row3_block = create_block("Row 3");

    // Pack the Row 3 block into the parent vertical box (main_vbox).
    // TRUE, TRUE means this row will expand and fill vertically and horizontally.
    gtk_box_pack_start(GTK_BOX(parent_vbox), row3_block, TRUE, TRUE, 5); // 5px padding around the row
}

// --- Functions for creating content within columns in Row 2 ---

// Row 2, Column 1: Split into 3 rows
GtkWidget* create_column1_row2_content() {
    // Create a vertical box to hold the 3 rows, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    // Pack the three blocks (representing rows) into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C1 R1"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C1 R2"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C1 R3"), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the three rows
}

// Row 2, Column 2: Split into 3 rows
GtkWidget* create_column2_row2_content() {
    // Create a vertical box to hold the 3 rows, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    // Pack the three blocks (representing rows) into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C2 R1"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C2 R2"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C2 R3"), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the three rows
}

// Row 2, Column 3: Split into 4 rows
GtkWidget* create_column3_row2_content() {
    // Create a vertical box to hold the 4 rows, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    // Pack the four blocks (representing rows) into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C3 R1"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C3 R2"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C3 R3"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), create_block("R2 C3 R4"), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the four rows
}

// Row 2, Column 4: Split into 60 rows
GtkWidget* create_column4_row2_content() {
    // Create a vertical box to hold the 60 rows, with 2px spacing (less spacing for many rows)
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 2);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow rows to have different sizes

    // Pack the sixty blocks (representing rows) into the vertical box
    for (int i = 1; i <= 60; ++i) {
        char label_text[20]; // Buffer for label text
        sprintf(label_text, "R2 C4 R%d", i); // Format the label text
        // For many rows, pack with expand=FALSE, fill=FALSE so they don't force excessive vertical size
        // This column might require a GtkScrolledWindow if the content exceeds the window height.
        gtk_box_pack_start(GTK_BOX(vbox), create_block(label_text), FALSE, FALSE, 0);
    }

    return vbox; // Return the vertical box containing the sixty rows
}

// Function to create Row 2: contains 4 columns with specified subdivisions, each wrapped in a border frame
// It takes the parent vertical box (main_vbox) as an argument
void create_row2(GtkWidget *parent_vbox) {
    // Create a horizontal box to hold the 4 main columns of Row 2, with 5px spacing between column frames
    GtkWidget *row2_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    // Set homogeneous to FALSE so columns can potentially have different sizes
    gtk_box_set_homogeneous(GTK_BOX(row2_hbox), FALSE);

    // --- Create and pack the content for each of the 4 main columns, wrapped in a Frame for a border ---

    // Column 1
    GtkWidget *col1_frame = gtk_frame_new(NULL);
    // Use an etched out shadow for the main column borders
    gtk_frame_set_shadow_type(GTK_FRAME(col1_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col1_frame), 5); // Padding inside the column frame
    gtk_container_add(GTK_CONTAINER(col1_frame), create_column1_row2_content()); // Add column content to the frame
    gtk_box_pack_start(GTK_BOX(row2_hbox), col1_frame, TRUE, TRUE, 0); // Pack the frame into the row HBox

    // Column 2
    GtkWidget *col2_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col2_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col2_frame), 5);
    gtk_container_add(GTK_CONTAINER(col2_frame), create_column2_row2_content());
    gtk_box_pack_start(GTK_BOX(row2_hbox), col2_frame, TRUE, TRUE, 0);

    // Column 3
    GtkWidget *col3_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col3_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col3_frame), 5);
    gtk_container_add(GTK_CONTAINER(col3_frame), create_column3_row2_content());
    gtk_box_pack_start(GTK_BOX(row2_hbox), col3_frame, TRUE, TRUE, 0);

    // Column 4
    GtkWidget *col4_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col4_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col4_frame), 5);
    gtk_container_add(GTK_CONTAINER(col4_frame), create_column4_row2_content());
     // For the column with many rows, allow its frame to expand horizontally,
     // but its vertical size will be primarily determined by its packed children (which are FALSE, FALSE).
    gtk_box_pack_start(GTK_BOX(row2_hbox), col4_frame, TRUE, TRUE, 0);

    // Pack the Row 2 horizontal box into the parent vertical box (main_vbox).
    gtk_box_pack_start(GTK_BOX(parent_vbox), row2_hbox, TRUE, TRUE, 5); // 5px padding around the row
}


// --- Functions for creating content within columns in Row 3 ---

// Handler for the "destroy" signal of the window
void on_window_destroy(GtkWidget *widget, gpointer data) {
    // Free the callback data structure when the window is destroyed
    // This assumes the callback data is only associated with the window's lifetime
    // and was allocated with malloc.
    if (data != NULL) {
        free(data);
    }
    gtk_main_quit(); // Quit the GTK main loop
}

// void create_start_button() {
//     start_button = gtk_button_new_with_label("Start");
// }

int get_num_of_Processes() {
    int numOfProcesses = 0;
    for (int i = 0; i < insertionPointer; i++)
    {
        if (Processes[i].process != NULL)
        {
            numOfProcesses++;
        }
    }

    return numOfProcesses;
}

void update_sensitivty_of_start_step_auto_button(GtkWidget* button) {
    int numOfProcesses = get_num_of_Processes();
    
    if (numOfProcesses != 0 && scheduler != 0)
    {
        gtk_widget_set_sensitive(button, TRUE);
    } else
    {
        gtk_widget_set_sensitive(button, FALSE);
    }    
}

void update_sensitivity_of_adjust_quantum_button() {
 const char* dropdown_button_label = gtk_button_get_label(GTK_BUTTON(dropdown_button));

 if (strcmp(dropdown_button_label, "Round Robin") == 0)
 {
    gtk_widget_set_sensitive(adjust_quantum_button, TRUE);
 } else
 {
     gtk_widget_set_sensitive(adjust_quantum_button, FALSE);
 }   
 
}

// Handler for the buttons inside the scheduler dropdown menu
void on_scheduler_menu_button_clicked(GtkWidget *widget, gpointer data) {
    // data will be the dropdown button (GtkMenuButton)
    //GtkMenuButton *dropdown_button = GTK_MENU_BUTTON(data);
    const char *button_label = gtk_button_get_label(GTK_BUTTON(widget)); // Get label of the clicked button

    if (strcmp(button_label, "FIFO") == 0) {
        scheduler = 1;
        gtk_widget_set_sensitive(dropdown_button, FALSE);
    } else if (strcmp(button_label, "Round Robin") == 0) {
        scheduler = 2;
    } else {
        scheduler = 3;
    }

    update_sensitivty_of_start_step_auto_button(start_button);
    update_sensitivty_of_start_step_auto_button(execution_step_by_step_button);
    update_sensitivty_of_start_step_auto_button(execution_auto_button);

    // Set the label of the dropdown button to the clicked button's label
    gtk_button_set_label(GTK_BUTTON(dropdown_button), button_label);
    update_sensitivity_of_adjust_quantum_button();

    // Get the popover associated with the menu button and hide it
    GtkPopover *popover = gtk_menu_button_get_popover(GTK_MENU_BUTTON(dropdown_button));
    gtk_popover_popdown(popover);

    // // Optional: You could perform an action based on the selected scheduler here
    // printf("Scheduler selected: %s\n", button_label);
}

// Helper function to create a button for the scheduler dropdown menu
GtkWidget* create_scheduler_menu_button(const char* label_text, GtkMenuButton *dropdown_button) {
    GtkWidget *button = gtk_button_new_with_label(label_text);
    // Connect the clicked signal to the handler, passing the dropdown_button pointer
    g_signal_connect(button, "clicked", G_CALLBACK(on_scheduler_menu_button_clicked), dropdown_button);
    // Ensure the button expands horizontally within the menu
    gtk_widget_set_hexpand(button, TRUE);
    return button;
}

// Button click handler for the "Adjust/Save Value" button
void on_adjust_save_button_clicked(GtkWidget *widget, gpointer data) {
    // Cast the generic data pointer back to our specific structure
    ButtonCallbackData *callback_data = (ButtonCallbackData *)data;

    // Get the current label of the button
    const char *current_label = gtk_button_get_label(GTK_BUTTON(callback_data->button));

    gboolean enable_r1_c4_c2_buttons; // Flag to determine if buttons in R1 C4 C2 should be enabled
    gboolean enable_r1_c4_c3_dropdown; // Flag to determine if the R1 C4 C3 dropdown should be enabled
    gboolean enable_r1_c2_buttons;     // Flag to determine if buttons in R1 C2 should be enabled


    if (strcmp(current_label, "Adjust quantum") == 0) {
        // Current state is "Adjust value", transition to "Save value" state
        gtk_widget_set_sensitive(callback_data->entry, TRUE);
        gtk_button_set_label(GTK_BUTTON(callback_data->button), "Save quantum");
        gtk_widget_grab_focus(GTK_WIDGET(callback_data->entry));

        enable_r1_c4_c2_buttons = FALSE;
        enable_r1_c4_c3_dropdown = FALSE; // Disable R1 C4 C3 dropdown
        enable_r1_c2_buttons = FALSE;

    } else { // Assuming the only other state is "Save value"
        // Current state is "Save value", transition back to "Adjust value" state
        gtk_widget_set_sensitive(callback_data->entry, FALSE);
        gtk_button_set_label(GTK_BUTTON(callback_data->button), "Adjust quantum");

        const char* quantum_string = gtk_entry_get_text(GTK_ENTRY(callback_data->entry));
        int quantum = atoi(quantum_string);
        printf("Adjusted Quantum: %d\n", quantum);

        // // Optional: Retrieve the text from the entry field here if needed
        const char *entered_text = gtk_entry_get_text(GTK_ENTRY(callback_data->entry));
        rr_quanta = str_to_int(entered_text);
        printf("Quantum saved: %s %d\n", entered_text, rr_quanta);

        enable_r1_c4_c2_buttons = TRUE;
        enable_r1_c4_c3_dropdown = TRUE; // Enable R1 C4 C3 dropdown
        enable_r1_c2_buttons = TRUE;
    }

    // Set sensitivity for the R1 C4 C3 dropdown button (New)
    gtk_widget_set_sensitive(callback_data->dropdown_button, enable_r1_c4_c3_dropdown); // Use the new pointer name

    // Set sensitivity for buttons in R1 C2
    GList *children = gtk_container_get_children(GTK_CONTAINER(callback_data->execution_vbox));
    GList *l;
    for (l = children; l != NULL; l = l->next) {
        gtk_widget_set_sensitive(GTK_WIDGET(l->data), enable_r1_c2_buttons);
    }
    g_list_free(children);

    // Set sensitivity for buttons in R1 C4 C2
    children = gtk_container_get_children(GTK_CONTAINER(callback_data->control_buttons_vbox));
    for (l = children; l != NULL; l = l->next) {
        gtk_widget_set_sensitive(GTK_WIDGET(l->data), enable_r1_c4_c2_buttons);
    }
    g_list_free(children);

    // The buttons inside the R1 C4 C3 popover will automatically follow the sensitivity of the GtkMenuButton.
    // No need to iterate through them separately here.
}

// --- Modal Dialog Functions ---

// Creates and runs the "Add Process" modal dialog
void create_add_process_modal(GtkWindow *parent_window) {
    // Create a new modal dialog
    // Arguments: title, parent window, flags (modal, destroy-with-parent), button text, response ID, ... NULL
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Add Process",
                                                    parent_window,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    "Load Process",
                                                    GTK_RESPONSE_ACCEPT, // Use ACCEPT for the Load button
                                                    NULL); // Sentinel NULL

    // Set a default size for the dialog
    gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); // Make dialog not resizable

    // Get the content area of the dialog (where we can pack our widgets)
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Create a vertical box to arrange the label, entry, and button inside the dialog
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10); // 10px spacing

    // Create the label for the input field
    GtkWidget *label_file = gtk_label_new("Enter File Path below:");

    // Create the input field (GtkEntry)
    GtkWidget *entry_file = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_file), "e.g., /path/to/process_file.txt");

    GtkWidget *label_arrival = gtk_label_new("Enter Arrival Time below:");

    GtkWidget *entry_arrival = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry_arrival), "e.g., 1");

    // Pack the label and entry into the vertical box
    gtk_box_pack_start(GTK_BOX(vbox), label_file, FALSE, FALSE, 0); // Label doesn't expand
    gtk_box_pack_start(GTK_BOX(vbox), entry_file, TRUE, TRUE, 0);   // Entry expands and fills
    gtk_box_pack_start(GTK_BOX(vbox), label_arrival, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), entry_arrival, TRUE, TRUE, 0);

    // Add some padding around the content inside the dialog
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);


    // Add the vertical box containing the label and entry to the dialog's content area
    gtk_container_add(GTK_CONTAINER(content_area), vbox);

    // Show all widgets in the dialog before running it
    gtk_widget_show_all(dialog);

    // Run the dialog. This function blocks until the dialog is closed.
    // The return value is the response ID of the button that was clicked.
    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    // Handle the response (e.g., if "Load Process" was clicked)
    if (response == GTK_RESPONSE_ACCEPT) {
        // The "Load Process" button was clicked
        // You could retrieve the text from the entry here if needed
        const char *file_path = gtk_entry_get_text(GTK_ENTRY(entry_file));
        printf("File path entered: %s\n", file_path);

        const char *arrival_time_string = gtk_entry_get_text(GTK_ENTRY(entry_arrival));
        const int arrival_time = atoi(arrival_time_string);
        printf("Arrival Time entered: %d\n", arrival_time);

        char *file_path_copy = g_strdup(file_path);

        add_program_to_program_queue(file_path_copy, arrival_time);
        // In a real application, you would process the file path here

        update_sensitivty_of_start_step_auto_button(start_button);
        update_sensitivty_of_start_step_auto_button(execution_step_by_step_button);
        update_sensitivty_of_start_step_auto_button(execution_auto_button);
    }

    // Destroy the dialog widget when gtk_dialog_run returns
    gtk_widget_destroy(dialog);
}

void on_process_individual_config_button_clicked(GtkWidget* button, GtkWidget* entry) {
    const char *arrival_time_string = gtk_entry_get_text(GTK_ENTRY(entry));
    const int arrival_time = atoi(arrival_time_string);
    printf("Arrival Time entered: %d\n", arrival_time);
}

// Creates and runs the "Process Configuration" modal dialog
void create_process_config_modal(GtkWindow *parent_window) {
     // Create a new modal dialog
    GtkWidget *dialog = gtk_dialog_new_with_buttons("Process Configuration",
                                                    parent_window,
                                                    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                                    NULL, // No standard buttons initially
                                                    NULL); // Sentinel NULL

    // Set a default size for the dialog
    gtk_window_set_default_size(GTK_WINDOW(dialog), 600, 200); // Adjust size as needed
    gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE); // Make dialog not resizable


    // Get the content area of the dialog
    GtkWidget *content_area = gtk_dialog_get_content_area(GTK_DIALOG(dialog));

    // Create a horizontal box to hold the three columns
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10); // 10px spacing between columns
    gtk_box_set_homogeneous(GTK_BOX(hbox), TRUE); // Make columns divide space equally

    // Add some padding around the content inside the dialog
    gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);

    // --- Create and pack the content for each of the three columns ---
    for (int i = 1; i <= 3; ++i) {
        // Create a vertical box for each column
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 5px spacing between items in column
        gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Items in column don't need equal height

        // Create the "Process:" label
        GtkWidget *label = gtk_label_new("Process:");

        // Create the input field (GtkEntry)
        GtkWidget *entry = gtk_entry_new();
        gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Adjust the arrival time");

        // Create the "Adjust Arrival Time" button
        GtkWidget *button = gtk_button_new_with_label("Adjust Arrival Time");

        g_signal_connect(button, "clicked", G_CALLBACK(on_process_individual_config_button_clicked), entry);

        // Pack the widgets into the column's vertical box
        gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0); // Label doesn't expand
        gtk_box_pack_start(GTK_BOX(vbox), entry, FALSE, FALSE, 0);   // Entry doesn't expand vertically much by default
        gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0); // Button doesn't expand vertically

        // Pack the column's vertical box into the main horizontal box
        gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 0); // Column expands and fills horizontally
    }


    // Add the main horizontal box to the dialog's content area
    gtk_container_add(GTK_CONTAINER(content_area), hbox);

    // Show all widgets in the dialog before running it
    gtk_widget_show_all(dialog);

    // Run the dialog. This function blocks until the dialog is closed.
    // GTK_RESPONSE_NONE is returned if the dialog is destroyed without clicking a button
    int response = gtk_dialog_run(GTK_DIALOG(dialog));

    // Handle the response if needed (e.g., if you added other buttons later)
    // if (response == SOME_RESPONSE_ID) { ... }

    // Destroy the dialog widget when gtk_dialog_run returns
    gtk_widget_destroy(dialog);
}


// --- Functions for creating content within columns in Row 1 ---

// Row 1, Column 1: Remains a single block
GtkWidget* create_overview_section_content(OverviewSectionData *overview_section_data) {
    if (program_queue_pointer != 0)
    {
        char buffer[128];
        GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
        // snprintf(buffer, sizeof(buffer), "Num of processes: %d", program_queue_pointer);
        GtkWidget *label_num_of_processes = gtk_label_new(overview_section_data->num_of_processes);
        gtk_box_pack_start(GTK_BOX(vbox), label_num_of_processes, FALSE, FALSE, 0);
        // snprintf(buffer, sizeof(buffer), "Current Clock Cycle: %d", clock_time);
        GtkWidget *label_clock_cycle = gtk_label_new(buffer);
        gtk_box_pack_start(GTK_BOX(vbox), label_clock_cycle, FALSE, FALSE, 0);
        // if (scheduler == 0)
        // {
        //     // do nothing
        // } else {
        char *sched_algorithm = scheduler == 1 ? "FIFO" : scheduler == 2 ? "Round Robin" : "MLFQ";
        snprintf(buffer, sizeof(buffer), "Active Scheduling Algorithm: %s", sched_algorithm);
        GtkWidget *label_active_scheduler = gtk_label_new(buffer);
        gtk_box_pack_start(GTK_BOX(vbox), label_active_scheduler, FALSE, FALSE, 0);
        // }

        return vbox;
        
    } else {   
        return create_block("There is no processes yet"); 
    }
}

// Handler for ExecutionStep
GtkWidget* on_execution_step_by_step() {
    return create_block("Placeholder");
}

// Row 1, Column 2: Split into 2 rows, now using specific Buttons that expand vertically
GtkWidget* create_execution_container_content() {
    // Create a vertical box to hold the 2 buttons, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    // Set homogeneous to TRUE so buttons divide vertical space equally
    gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE); // Changed to TRUE for equal vertical division

    // Create the two buttons
    execution_step_by_step_button = gtk_button_new_with_label("Execution Step-by-Step");
    execution_auto_button = gtk_button_new_with_label("Execution Auto");

    update_sensitivty_of_start_step_auto_button(execution_step_by_step_button);
    update_sensitivty_of_start_step_auto_button(execution_auto_button);

    // Pack the buttons into the vertical box
    // Buttons should expand vertically (TRUE) and fill horizontally (TRUE)
    gtk_box_pack_start(GTK_BOX(vbox), execution_step_by_step_button, TRUE, TRUE, 0); // Changed expand to TRUE
    gtk_box_pack_start(GTK_BOX(vbox), execution_auto_button, TRUE, TRUE, 0); // Changed expand to TRUE

    // Set expand and fill properties for the vbox itself within its parent
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);

    return vbox; // Return the vertical box containing the two buttons
}

// Handler for the "Add Process" button click
void on_add_process_button_clicked(GtkWidget *widget, gpointer data) {
    // data will be the main window widget
    GtkWindow *main_window = GTK_WINDOW(data);
    // Call the function to create and run the modal dialog
    create_add_process_modal(main_window);
}

// Handler for the "Process Configuration" button click
void on_process_config_button_clicked(GtkWidget *widget, gpointer data) {
    // data will be the main window widget
    GtkWindow *main_window = GTK_WINDOW(data);
    // Call the function to create and run the process configuration modal dialog
    create_process_config_modal(main_window);
}


// Row 1, Column 3: Split into 2 rows, now using specific Buttons that expand vertically
// The top button ("Add Process") will trigger the modal
GtkWidget* create_process_configuraton_container_content(GtkWindow *main_window) { // Accept main_window pointer
    // Create a vertical box to hold the 2 buttons, with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    // Set homogeneous to TRUE so buttons divide vertical space equally
    gtk_box_set_homogeneous(GTK_BOX(vbox), TRUE); // Changed to TRUE for equal vertical division

    // Create the two buttons
    GtkWidget *button_add = gtk_button_new_with_label("Add Process");
    GtkWidget *button_config = gtk_button_new_with_label("Process Configuration");

    // Connect the "Add Process" button to the modal handler, passing the main window
    g_signal_connect(button_add, "clicked", G_CALLBACK(on_add_process_button_clicked), main_window);
    // Connect the "Process Configuration" button to its modal handler, passing the main window
    g_signal_connect(button_config, "clicked", G_CALLBACK(on_process_config_button_clicked), main_window);


    // Pack the buttons into the vertical box
    // Buttons should expand vertically (TRUE) and fill horizontally (TRUE)
    gtk_box_pack_start(GTK_BOX(vbox), button_add, TRUE, TRUE, 0); // Changed expand to TRUE
    gtk_box_pack_start(GTK_BOX(vbox), button_config, TRUE, TRUE, 0); // Changed expand to TRUE

    // Set expand and fill properties for the vbox itself within its parent
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);

    return vbox; // Return the vertical box containing the two buttons
}

// --- Functions for creating content within sub-columns of Row 1, Column 4 ---

// Row 1, Column 4, Column 1: Contains an input field and a button
// Now accepts pointers to the other two vboxes in R1 C4 and R1 C2
GtkWidget* create_adjust_quantum_container_content(GtkWidget *execution_vbox, GtkWidget *control_buttons_vbox, GtkWidget *dropdown_button) { // Added execution_vbox
    // Create a vertical box to hold the entry and button
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE);

    // Create the input field (GtkEntry)
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(entry), "Enter quantum here...");
    // Initially disable the input field
    gtk_widget_set_sensitive(entry, FALSE);

    // Create the button
    adjust_quantum_button = gtk_button_new_with_label("Adjust quantum");
    update_sensitivity_of_adjust_quantum_button();

    // Create a structure to pass all necessary widgets to the callback
    // Note: In a real application, you might need to free this struct
    // when the widgets are destroyed if they are dynamically created/destroyed.
    ButtonCallbackData *callback_data = malloc(sizeof(ButtonCallbackData));
    callback_data->entry = entry;
    callback_data->button = adjust_quantum_button;
    callback_data->execution_vbox = execution_vbox;       // Store pointer to R1 C2 vbox (New)
    callback_data->control_buttons_vbox = control_buttons_vbox; // Store pointer to C2 vbox
    callback_data->dropdown_button = dropdown_button; // Store pointer to C3 dropdown button


    // Connect the button's "clicked" signal to the handler, passing the struct
    g_signal_connect(adjust_quantum_button, "clicked", G_CALLBACK(on_adjust_save_button_clicked), callback_data);

    // Pack the entry and button into the vertical box
    // Entry should expand and fill
    gtk_box_pack_start(GTK_BOX(vbox), entry, TRUE, TRUE, 0);
    // Button should not necessarily expand vertically, but fill horizontally
    gtk_box_pack_start(GTK_BOX(vbox), adjust_quantum_button, FALSE, TRUE, 0);

    // Set expand and fill properties for the vbox itself within its parent
    gtk_widget_set_vexpand(vbox, TRUE);
    gtk_widget_set_hexpand(vbox, TRUE);


    return vbox; // Return the vertical box containing the entry and button
}



// Row 1, Column 4, Column 2: Split into 3 rows, using Buttons directly
GtkWidget* create_control_buttons_container_content() {
    // Create a vertical box to hold the 3 buttons (representing rows), with 5px spacing
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(vbox), FALSE); // Allow buttons to have different sizes if needed

    // Pack the three buttons directly into the vertical box (no extra frame for padding)
    start_button = gtk_button_new_with_label("Start");
    update_sensitivty_of_start_step_auto_button(start_button);
    gtk_box_pack_start(GTK_BOX(vbox), start_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_button_new_with_label("Stop"), TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), gtk_button_new_with_label("Reset"), TRUE, TRUE, 0);

    return vbox; // Return the vertical box containing the three buttons
}

// Row 1, Column 4, Column 3: Now a dropdown button with scheduler options
void create_dropdown_container_content() {
    // Create the dropdown button
    dropdown_button = gtk_menu_button_new();
    gtk_button_set_label(GTK_BUTTON(dropdown_button), "Choose Scheduler"); // Initial label

    // Create the popover that will contain the menu items
    GtkWidget *popover = gtk_popover_new(dropdown_button); // Popover is attached to the dropdown button

    // Create a vertical box to hold the buttons inside the popover
    GtkWidget *vbox_menu = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5); // 5px spacing

    // Create the three buttons for the scheduler options and pack them into the vbox
    // Pass the dropdown_button pointer to the helper function so the callback can access it
    gtk_box_pack_start(GTK_BOX(vbox_menu), create_scheduler_menu_button("FIFO", GTK_MENU_BUTTON(dropdown_button)), FALSE, FALSE, 0); // Corrected label
    gtk_box_pack_start(GTK_BOX(vbox_menu), create_scheduler_menu_button("Round Robin", GTK_MENU_BUTTON(dropdown_button)), FALSE, FALSE, 0); // Corrected label
    gtk_box_pack_start(GTK_BOX(vbox_menu), create_scheduler_menu_button("MLFQ", GTK_MENU_BUTTON(dropdown_button)), FALSE, FALSE, 0); // Corrected label

    // *** FIX: Show the widgets inside the popover's content area ***
    gtk_widget_show_all(vbox_menu);

    // Set the vbox as the child of the popover
    gtk_container_add(GTK_CONTAINER(popover), vbox_menu);

    // Set the popover for the menu button
    gtk_menu_button_set_popover(GTK_MENU_BUTTON(dropdown_button), popover);

    // Set expand and fill properties for the dropdown button itself within its parent
    gtk_widget_set_vexpand(dropdown_button, TRUE); // Allow vertical expansion
    gtk_widget_set_hexpand(dropdown_button, TRUE); // Allow horizontal expansion

    // return dropdown_button; // Return the dropdown button
}


// Row 1, Column 4: Split into 3 columns (using the sub-column content functions above)
// Now accepts the R1 C2 vbox pointer
GtkWidget* create_settings_container_content(GtkWidget *execution_vbox) {
    // Create a horizontal box to hold the 3 sub-columns, with 5px spacing
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_set_homogeneous(GTK_BOX(hbox), FALSE); // Allow columns to have different sizes

    // Create the vboxes for the button columns first
    GtkWidget *control_buttons_vbox = create_control_buttons_container_content();
    // Create the dropdown button for R1 C4 C3 (New)
    // GtkWidget *dropdown_button = create_dropdown_container_content(); // Call the updated function
    create_dropdown_container_content();
    GtkWidget *dropdown_button = dropdown_button; // Call the updated function

    // Create the content for the first column, passing the other vboxes and the dropdown button
    GtkWidget *adjust_quantum_container_content = create_adjust_quantum_container_content(execution_vbox, control_buttons_vbox, dropdown_button); // Pass dropdown button

    // Pack the three sub-column blocks/containers into the horizontal box
    gtk_box_pack_start(GTK_BOX(hbox), adjust_quantum_container_content, TRUE, TRUE, 0); // Vertical box with Entry and Button
    gtk_box_pack_start(GTK_BOX(hbox), control_buttons_vbox, TRUE, TRUE, 0); // Vertical box with 3 buttons
    gtk_box_pack_start(GTK_BOX(hbox), dropdown_button, TRUE, TRUE, 0); // Pack the dropdown button (New)

    return hbox; // Return the horizontal box containing the three sub-columns
}


// Function to create Row 1: contains 4 columns with specified subdivisions, each wrapped in a border frame
// It takes the parent vertical box (main_vbox) and main_window as arguments
void create_row1(GtkWidget *parent_vbox, GtkWindow *main_window) {
    OverviewSectionData overview_section_data = malloc(sizeof(OverviewSectionData));
    
    char buffer_clock[128];
    snprintf(buffer_clock, sizeof(buffer_clock), "Current Clock Cycle: %d", clock_time);
    overview_section_data.clock_cycle = gtk_label_new(buffer_clock);

    if (memory_insertion_ptr != 0)
    {
        char buffer_processes[128];
        snprintf(buffer_processes, sizeof(buffer_processes), "Num of processes: %d", memory_insertion_ptr);
        overview_section_data.num_of_processes = gtk_label_new(buffer_processes);
    } else {
        overview_section_data.num_of_processes = gtk_label_new("No process added yet");
    }

    char buffer_scheduler[128];
    switch (scheduler)
    {
    case 1:
        snprintf(buffer_scheduler, sizeof(buffer_scheduler), "Active Shceduler: %s", "FIFO");
        overview_section_data.active_scheduler = gtk_label_new(buffer_scheduler);
        break;
    case 2:
        snprintf(buffer_scheduler, sizeof(buffer_scheduler), "Active Shceduler: %s", "Round Robin");
        overview_section_data.active_scheduler = gtk_label_new(buffer_scheduler);
        break;
    case 3:
        snprintf(buffer_scheduler, sizeof(buffer_scheduler), "Active Shceduler: %s", "MLFQ");
        overview_section_data.active_scheduler = gtk_label_new(buffer_scheduler);
        break;
    
    default:
        overview_section_data.active_scheduler = gtk_label_new("Scheduler Not Chosen");
        break;
    }

    
    
    // Create a horizontal box to hold the 4 main columns of Row 1, with 5px spacing between column frames
    GtkWidget *row1_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_widget_set_size_request(row1_hbox, -1, 10);
    // Set homogeneous to FALSE so columns can potentially have different sizes
    gtk_box_set_homogeneous(GTK_BOX(row1_hbox), FALSE);

    // --- Create and pack the content for each of the 4 main columns, wrapped in a Frame for a border ---

    // Column 1
    GtkWidget *col1_frame = gtk_frame_new(NULL);
    // Use an etched out shadow for the main column borders
    gtk_frame_set_shadow_type(GTK_FRAME(col1_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col1_frame), 5); // Padding inside the column frame
    gtk_container_add(GTK_CONTAINER(col1_frame), create_overview_section_content()); // Add column content to the frame
    gtk_box_pack_start(GTK_BOX(row1_hbox), col1_frame, TRUE, TRUE, 0); // Pack the frame into the row HBox

    // Column 2 (Need to get the vbox here to pass to R1 C4 C1)
    GtkWidget *execution_vbox = create_execution_container_content(); // Create the vbox for R1 C2
    GtkWidget *col2_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col2_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col2_frame), 5);
    gtk_container_add(GTK_CONTAINER(col2_frame), execution_vbox); // Add the vbox to the frame
    gtk_box_pack_start(GTK_BOX(row1_hbox), col2_frame, TRUE, TRUE, 0);

    // Column 3
    GtkWidget *col3_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col3_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col3_frame), 5);
    gtk_container_add(GTK_CONTAINER(col3_frame), create_process_configuraton_container_content(main_window)); // Pass main_window to create_column3_row1_content
    gtk_box_pack_start(GTK_BOX(row1_hbox), col3_frame, TRUE, TRUE, 0);

    // Column 4 (Pass the R1 C2 vbox to create_settings_container_content)
    GtkWidget *col4_frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(col4_frame), GTK_SHADOW_ETCHED_OUT);
    gtk_container_set_border_width(GTK_CONTAINER(col4_frame), 5);
    gtk_container_add(GTK_CONTAINER(col4_frame), create_settings_container_content(execution_vbox)); // Passed execution_vbox
    gtk_box_pack_start(GTK_BOX(row1_hbox), col4_frame, TRUE, TRUE, 0);

    // Pack the Row 1 horizontal box into the parent vertical box (main_vbox).
    gtk_box_pack_start(GTK_BOX(parent_vbox), row1_hbox, TRUE, TRUE, 5); // 5px padding around the row
}

// Function to initialize and set up the main GTK application window and its contents
// This function creates the main window, the scrolled window, the primary layout container (main_vbox),
// and then calls the functions to create and pack the rows.
GtkWidget* create_main_window() {
    // Create a new top-level window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Complex Layout with Scheduler Dropdown"); // Updated window title
    // No border width directly on the window's content area when using a scrolled window here.
    // The scrolled window will contain the main_vbox which has its own padding/spacing.
    // gtk_container_set_border_width(GTK_CONTAINER(window), 10); // Removed or set to 0

    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600); // Set a default window size

    // Connect the "destroy" signal to our handler to quit the app when the window is closed
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create a Scrolled Window to make the content scrollable
    // The NULL, NULL arguments mean use default scroll adjustments
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    // Set the scrollbar policy:
    // GTK_POLICY_NEVER: Never show horizontal scrollbar
    // GTK_POLICY_AUTOMATIC: Show vertical scrollbar only when needed (content exceeds height)
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window),
                                   GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    // Add the scrolled window to the main window
    gtk_container_add(GTK_CONTAINER(window), scrolled_window);


    // Create a main vertical box to hold the three rows.
    // 5px spacing between the rows.
    main_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    //gtk_box_set_homogeneous(GTK_BOX(main_vbox), FALSE);
    // Add the main vertical box to the Scrolled Window
    // The scrolled window can only contain one child.
    gtk_container_add(GTK_CONTAINER(scrolled_window), main_vbox);


    // int numOfProcesses = get_num_of_Processes();
    // Create the rows and add them to the main VBox
    // We pass main_vbox as the parent container for the rows.
    // Pass the main window pointer to create_row1 so it can be used for the modal
    create_row1(main_vbox, GTK_WINDOW(window));
    // if (numOfProcesses == 0)
    // {
    //     gtk_box_pack_start(GTK_BOX(main_vbox), create_block("Start By Adding a process"), TRUE, TRUE, 5);
    // } else {   
        create_row2(main_vbox);
        create_row3(main_vbox);
    //}

    const char* dropdown_button_label = gtk_button_get_label(GTK_BUTTON(dropdown_button));
    printf("Dropdown label: %s\n", dropdown_button_label);
    // Return the created window widget
    return window;
}

int main(int argc, char *argv[]) {
    GtkWidget *window; // Declare the main window widget pointer

    // Initialize GTK. This must be called before using any other GTK functions.
    // It processes GTK-specific command-line arguments and initializes the toolkit.
    gtk_init(&argc, &argv);

    // Create and set up the main application window and its contents by calling our setup function.
    window = create_main_window();

    // Show all widgets contained within the window (the window itself, the scrolled window,
    // the vbox, hboxes, frames, labels).
    gtk_widget_show_all(window);

    // Start the GTK main loop. This function enters the main processing loop and
    // waits for events (like user input, window events, timer events).
    // The application will stay running here until gtk_main_quit() is called.
    gtk_main();

    // The program exits when gtk_main() returns.
    return 0;
}