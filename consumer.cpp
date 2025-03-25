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
    key_t key = ftok("shmkey", 1); 
    int shmid = shmget(key, sizeof(SharedData), 0666 | IPC_CREAT);

    if (shmid == -1) 
    {
        std::cerr << "shmget failed" << std::endl;
        return 1;
    }

    SharedData* sharedData = (SharedData*)shmat(shmid, nullptr, 0);

    sem_t* producerSemaphore = sem_open(SEM_PRODUCER, 0); 
    sem_t* consumerSemaphore = sem_open(SEM_CONSUMER, 0);

    // output file for decrypted passwords
    std::ofstream outputFile("decrypted_passwords.txt");

    // decryption function
    auto decryptPassword = [](std::string password) 
    {
        std::string decryptedPassword = password;

        for (char& c : decryptedPassword) 
        {
            // undo the shifting
            if (isalpha(c)) 
            {
                if (c == 'a') 
                {
                    c = 'z';
                } 
                else if (c == 'A') 
                {
                    c = 'Z';
                }
                 else 
                {
                    c--;
                }
            }

            if (!decryptedPassword.empty()) 
            {
                // move the first letter to the last 
                if (c == decryptedPassword.front()) 
                {
                    c = decryptedPassword.back();
                }
            }
        }

        return decryptedPassword;
    };

    while (!sharedData->isDone) 
    {
        sem_wait(consumerSemaphore);

        // read passwords
        std::string encryptedPasswords = sharedData->passwords;
        std::string decryptedPasswords = decryptPassword(encryptedPasswords);

        // write passwords to output file
        outputFile << decryptedPasswords << std::endl;

        sem_post(producerSemaphore);
    }

    shmdt(sharedData);
    shmctl(shmid, IPC_RMID, nullptr);

    sem_close(producerSemaphore);
    sem_close(consumerSemaphore);

    outputFile.close();

    return 0;
}
