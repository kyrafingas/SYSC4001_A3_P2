/**
 * @file exam_problem.cpp
 * @author Kyra Fingas
 * @brief main.cpp file for Assignment 3 Part 2 of SYSC4001
 * 
 */

#include"exam_problem_101257741_101297186.hpp"

exam current_exam;   //The current exam
std::vector<char*> exam_list; //The list of exam file names
int next_exam_number; //the index of the next exam file name
int exam_mutex;
int question_mutex;

std::vector<rubric_line*> rubric;
char* rubric_file;
std::vector<int> shm_id_rubric;
int rubric_mutex;

int ta_num;
pid_t c_pid;

void clear_rubric(){
    if(shm_id_rubric.size() > 0){
        for(int i = 0; i<shm_id_rubric.size(); i++){
            if (shmctl(shm_id_rubric[i], IPC_RMID, (struct shmid_ds *) 0) < 0){
                perror("can’t IPC_RMID shared");
                exit(0);
            }
        }
    }
}

void read_rubric (){
    rubric.clear();

    std::ifstream rubric_filez (rubric_file);

    //Parse the entire rubric input file and populate a vector of rubric lines.
    //To do so, the add_rubric_line() helper function is used (see include file).
    std::string line;
    int i = 0;
    while (std::getline(rubric_filez, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_rubric_line = add_rubric_line(input_tokens);
        auto rubric_line_pointer = &new_rubric_line;

        int shm_idz;
        int shm_size1 = sizeof(new_rubric_line);
        if ((shm_idz = shmget(IPC_PRIVATE, shm_size1, IPC_CREAT | 0600)) <= 0) {
            perror( "Error in shmget");
        }
        try{
            rubric_line_pointer = (rubric_line * ) shmat (shm_idz, (char *)0, 0 );
        }catch(...){
            perror("Error in shmat");
        }  

        rubric.push_back(rubric_line_pointer);
        i++;
    }
    
    rubric_filez.close();
}

void correct_rubric (int index){
    std::cout<<"correcting rubric line...\n";
    (*rubric[index]).text++;

    SemaphoreWait(rubric_mutex, 1);
    // access the critical section here.

    std::ofstream output_file (rubric_file);

    if (output_file.is_open()) {
        for (rubric_line * linez : rubric){
            output_file << std::to_string((*linez).exercise) + ", " + (*linez).text +"\n";
            std::cout << std::to_string((*linez).exercise) + ", " + (*linez).text +"\n";;
        }
        output_file.close();  // Close the file when done
        std::cout << "File content overwritten successfully." << std::endl;
    } else {
        std::cerr << "Error opening file!" << std::endl;
    }

    SemaphoreSignal(rubric_mutex);
}

void load_exam (){
    std::cout<<"loading exam number " +std::to_string(next_exam_number) +"\n";

    SemaphoreWait(exam_mutex, 1);
    // access the critical section here.
    std::cout <<"here\n";

    //Open the exam file
    char* exam_file_name = exam_list[next_exam_number];
    std::ifstream exam_file;
    exam_file.open(exam_file_name);
    std::cout<<"exam file open\n";
    clear_exam();

    //Ensure that the file actually opens
    if (!exam_file.is_open()) {
        std::cerr << "Error: Unable to open file: " << exam_file_name << std::endl;
        return;
    }

    //Parse the entire exam input file and populate a vector of exams.
    //To do so, the add_exam() helper function is used (see include file).
    std::string line;
    while (std::getline(exam_file, line)) {
        std::cout<<"in while loop\n";
        auto input_tokens = split_delim(line, ", ");
        current_exam = add_exam(input_tokens, rubric.size());
    }
    exam_file.close();   
    std::cout<<"exam file read\n";
    next_exam_number++;
    SemaphoreSignal(exam_mutex);
}

void run_simulation() {
    std::cout<<"running simulation... \n";
    using namespace std::this_thread; // sleep_for, sleep_until
    using namespace std::chrono; // nanoseconds, system_clock, seconds
    srand(time(0) + c_pid);

    while (next_exam_number<exam_list.size() && current_exam.student_id != 9999){
        std::cout<<"grading exam number " +std::to_string(next_exam_number) +"\n";
        
        for (int i = 0; i < rubric.size(); i++){//reviewing and possibly correcting rubric
            std::cout<<"checking rubric file line\n";
            bool correct = rand()/ double(RAND_MAX) < 0.5;
            std::cout<<"line correct: " +std::to_string(correct) +"\n";
            int time_period = ((rand() / (double(RAND_MAX) * 2 )) + 0.5) * 1000;
            std::cout<<"time delay: " +std::to_string(time_period) +"\n";
            sleep_for (milliseconds(time_period));
            if (correct){
                std::cout<<"correcting the rubric...\n";
                correct_rubric (i);
            }
        }
        std::cout<<"marking exam... \n";
        bool marked;
        for (int i = 0; i < rubric.size(); i++){
            SemaphoreWait(question_mutex, 1);
            if (*(current_exam.questions_marked[i]) == false){
                if(i == rubric.size()-1) marked = true;
                std::cout<<"marking exam question # " +std::to_string(i) +"...\n";
                *(current_exam.questions_marked[i]) = true;
                SemaphoreSignal(question_mutex);
                int time_period = ((rand()/ double(RAND_MAX)) + 1) * 1000;
                sleep_for(milliseconds(time_period));
                std::cout << "Student Number: " +std::to_string(current_exam.student_id) +", Question Marked: " + std::to_string((*rubric[i]).exercise) +"\n";
            } else {
                SemaphoreSignal(question_mutex);
            }
        }
        if(next_exam_number > exam_list.size() || current_exam.student_id == 9999){
            return;
        }
        if (marked == true){
            load_exam();
        }
    }
    return;
}

//deals with opening and reading file and writing to the execution file
int main (int argc, char** argv) {
    //Get the input file from the user
    if (argc != 21) {
        std::cout << "ERROR!\nExpected 3 argument, received " << argc - 1 << std::endl;
        std::cout << "To run the program, do: ./exam_problem <your_exam_file.txt> <your_rubric_file.txt> <your_ta_file.txt>" << std::endl;
        return -1;
    }

    //open the rubric file
    auto rubric_file_name = argv[1];

    rubric_file = rubric_file_name;

    if (( rubric_mutex = SemaphoreCreate(1)) == -1) {
        SemaphoreRemove(rubric_mutex);
        perror("Error in SemaphoreCreate");
        exit(1);
    }

    read_rubric();

    // Create a semaphore:

    std::cout<<"rubric file read\n";

    //Open the TA file
    auto ta_file_name = argv[2];
    std::ifstream ta_file;
    ta_file.open(ta_file_name);

    std::string line;
    //Parse the ta input file to get # of tas
    //To do so, the add_tas() helper function is used (see include file).
    while (std::getline(ta_file, line)) {
        auto input_tokens = split_delim(line, ", ");
        auto new_tas = add_tas(input_tokens);
        ta_num = new_tas;
    }
    
    ta_file.close();

    std::cout<<"ta file read\n";

    for (int i = 3; i < 21; i++){
        exam_list.push_back(argv[i]);
    }

    // Create exam semaphore:
    if (( exam_mutex = SemaphoreCreate(1)) == -1) {
        SemaphoreRemove(exam_mutex);
        perror("Error in SemaphoreCreate");
        exit(1);
    }

    load_exam();

    if (( question_mutex = SemaphoreCreate(1)) == -1) {
        SemaphoreRemove(question_mutex);
        perror("Error in SemaphoreCreate");
        exit(1);
    }

    std::cout<<"exam loaded\n";

    for(int i = 1; i<ta_num; i++){
        c_pid = fork();
        if(c_pid == 0){
            break;
        }
    }
    //With the list of processes, run the simulation
    run_simulation();

    exit(c_pid);
    //write_output(exec, "execution.txt");
    clear_rubric();
    SemaphoreRemove(rubric_mutex);
    SemaphoreRemove(exam_mutex);
    SemaphoreRemove(question_mutex);

    return 0;
}