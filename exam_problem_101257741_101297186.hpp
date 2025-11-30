/**
 * @file exam_problem.hpp
 * @author Kyra Fingas
 * @brief main.hpp file for Assignment 3 Part 2 of SYSC4001
 * 
 */

#ifndef INTERRUPTS_HPP_
#define INTERRUPTS_HPP_

#include<iostream>
#include<fstream>
#include<string>
#include<vector>
#include<tuple>
#include<random>
#include<utility>
#include<sstream>
#include<iomanip>
#include<algorithm>
#include<chrono>
#include<thread>
#include<unistd.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <stdlib.h>

std::vector<int> shm_id_exam;

//An enumeration of actions to make assignment easier
enum action {
    READ,
    WRITE,
    ETC,
};

struct exam {
    int                 student_id;
    std::vector<bool*>   questions_marked;
};

struct rubric_line {
    int             exercise;
    char            text;
};

struct ta {
    int             id;
    action          current_action;
    int             rubric_line;
};

union semun {
    int val;
    struct semid_ds *buf;
    ushort * array;
} arg ;

int SemaphoreWait(int semid, int iMayBlock ) {
    struct sembuf sbOperation;
    sbOperation.sem_num = 0;
    sbOperation.sem_op = -1;
    sbOperation.sem_flg = iMayBlock;
    return semop( semid, &sbOperation, 1 );
}

int SemaphoreSignal( int semid ) {
    struct sembuf sbOperation;
    sbOperation.sem_num = 0;
    sbOperation.sem_op = +1;
    sbOperation.sem_flg = 0;
    return semop( semid, &sbOperation, 1 );
}

void SemaphoreRemove( int semid ) {
    if(semid != -1 )
    semctl( semid, 0, IPC_RMID , 0);
}

int SemaphoreCreate(int iInitialValue) {
    int semid;
    union semun suInitData;
    int iError;
    /* get a semaphore */
    semid = semget( IPC_PRIVATE, 1, IPC_CREAT|0600);
    /* check for errors */
    if( semid == -1 )
    return semid;
    /* now initialize the semaphore */
    suInitData.val = iInitialValue;
    if(semctl( semid, 0, SETVAL, suInitData) == -1 )
    { /* error occurred, so remove semaphore */
        SemaphoreRemove(semid);
        return -1;
    }
    return semid;
}

std::vector<std::string> split_delim(std::string input, std::string delim) {
    std::vector<std::string> tokens;
    std::size_t pos = 0;
    std::string token;
    while ((pos = input.find(delim)) != std::string::npos) {
        token = input.substr(0, pos);
        tokens.push_back(token);
        input.erase(0, pos + delim.length());
    }
    tokens.push_back(input);

    return tokens;
}

void clear_exam(){
    if(shm_id_exam.size() > 0){
        for(int i = 0; i<shm_id_exam.size(); i++){
            if (shmctl(shm_id_exam[i], IPC_RMID, (struct shmid_ds *) 0) < 0){
                perror("can’t IPC_RMID shared");
                exit(0);
            }
        }
    }
}

exam add_exam (std::vector<std::string> tokens, int num_questions) {
    std::cout<<"in add exam\n";
    exam examz;
    examz.student_id = std::stoi(tokens[0]);
    examz.questions_marked.resize(num_questions);
    int shm_size1 = sizeof(examz.questions_marked);
    
    for (int i = 0; i < num_questions; i++){
        int shm_idz;
        if ((shm_idz = shmget(IPC_PRIVATE, shm_size1, IPC_CREAT | 0600)) <= 0) {
            perror( "Error in shmget");
        }
        try{
            examz.questions_marked[i] = (bool * ) shmat (shm_idz, (char *)0, 0 );
        }catch(...){
            perror("Error in shmat");
        }
        *(examz.questions_marked[i]) = false;
        shm_id_exam.push_back(shm_idz);
    }

    return examz;
}

rubric_line add_rubric_line (std::vector<std::string> tokens) {
    rubric_line linez;
    linez.exercise = std::stoi(tokens[0]);
    linez.text = tokens[1].at(0);
    return linez;
}

int add_tas (std::vector<std::string> tokens) {
    //std::vector<ta> list_tas;
    int ta_num = std::stoi(tokens[0]);
    //for (int i = 0; i < ta_num; i++){
        //ta new_ta;
        //new_ta.id = i;
        //new_ta.current_action = READ;
        //new_ta.rubric_line = 0;
        //list_tas.push_back(new_ta);
    //}
    return ta_num;
}

void write_output (std::string execution, const char* filename) {
    std::ofstream output_file(filename);

    if (output_file.is_open()) {
        output_file << execution;
        output_file.close();  // Close the file when done
        std::cout << "File content overwritten successfully." << std::endl;
    } else {
        std::cerr << "Error opening file!" << std::endl;
    }

    std::cout << "Output generated in " << filename << ".txt" << std::endl;
}

#endif