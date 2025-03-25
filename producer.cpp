//To run this program I used the following lines in the terminal:
//g++ producer.cpp -o producer -lrt -pthread
//g++ consumer.cpp -o consumer -lrt -pthread
//./producer passwords.txt [some number]

//In a second terminal:
//./consumer

//Note: Everything seems to work as planned except for the actual encryption and decryption.
//      At one point I had some (incorrect) output in the decrypted_passwords file but now I am not getting anything.
//      I don't remember changing anything with that and I'm not entirely sure where it is going wrong but this is the best I can come up with.

#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <stdbool.h>
#include <fcntl.h>
#include <cstring>

#define SHM_SIZE 1024
#define SEM_PRODUCER "/producer_semaphore"
#define SEM_CONSUMER "/consumer_semaphore"

struct SharedData 
{
    char passwords[SHM_SIZE];
    bool isDone;
};

int main(int argc, char* argv[]) 
{
    if (argc != 3) 
    {
        std::cerr << "Usage: " << argv[0] << " input_file num_passwords" << std::endl;
        return 1;
    }

    std::string inputFilename = argv[1];
    int numPasswords = std::stoi(argv[2]);

    key_t key = ftok("shmkey", 1); 
    int shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);

    if (shmid == -1)
    {
        std::cerr << "shmget failed" << std::endl;
        return 1;
    }

    SharedData* sharedData = (SharedData*)shmat(shmid, nullptr, 0);

    sem_t* producerSemaphore = sem_open(SEM_PRODUCER, O_CREAT, 0666, 1);
    sem_t* consumerSemaphore = sem_open(SEM_CONSUMER, O_CREAT, 0666, 0);

    // read passwords from the input file
    std::ifstream inputFile(inputFilename);
    if (!inputFile) 
    {
        std::cerr << "Error opening input file" << std::endl;
        return 1;
    }

    // encryption algorithm
    auto encryptPassword = [](std::string password) 
    {
        std::string encryptedPassword = password;

        for (char& c : encryptedPassword) 
        {
            // shift the letter forward by one 
            if (isalpha(c)) 
            {
                if (c == 'z') 
                {
                    c = 'a';
                } 
                else if (c == 'Z') 
                {
                    c = 'A';
                } 
                else 
                {
                    c++;
                }
            }
            if (!encryptedPassword.empty())
            {
                // move the last letter to the first 
                if (c == encryptedPassword.back())
                {
                    c = encryptedPassword.front();
                }
            }
        }

        return encryptedPassword;
    };


    int passwordCount = 0;
    while (passwordCount < numPasswords) 
    {
        std::string password;
        if (inputFile >> password) 
        {
            password = encryptPassword(password);

            strncpy(sharedData->passwords, password.c_str(), SHM_SIZE);
            passwordCount++;
        }
    }

    // signal the consumer
    sem_post(consumerSemaphore);

    // wait for consumer to finish
    sem_wait(producerSemaphore);

    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, nullptr);

    sem_close(producerSemaphore);
    sem_close(consumerSemaphore);

    return 0;
}
