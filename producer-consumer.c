#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/shm.h>
#include <getopt.h>
#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/un.h>

#define SEM_P "/prod_sem_8714241235432"
#define SEM_C "/con_sem_8714241235432"
#define termp "TERMINATION_SEQUENCE_!@#$^&*()_asdfjkewknwmemafsdm,sm,vzcxnjjsa1"
#define termc "TERMINATION_SEQUENCE_!@#$^&*()_asdfjkewknwmemafsdm,sm,vzcxnjjsa2"
#define SOCK_PATH "/tmp/kenneth_142_8794789241312213"
#define com_buff 255
volatile sig_atomic_t keep_running = 1;
struct sockaddr_un address;
int sock = 0, valread, cterm = 0, pterm = 0;
key_t key;
int seg_size = 4096, shmid;
typedef struct {
    char arr[256][256];
} buffer;
buffer* shared_memory;
char buff[256][256];

int readArgs(int argc, char** argv, int* p, int* c, int* u, int* s, int* q, int* e, char str[255]) {
    int option, pflag = 0, cflag = 0, mflag = 0, uflag = 0, sflag = 0, qflag = 0, eflag = 0;
    while ((option = getopt(argc, argv, "pcusem:q:")) != -1) {
        switch (option) {
            case 'p': 
                if (pflag == 1) {
                    printf("You can only have one -p argument.\n");
                    return -1;
                }
                *p = 1;
                pflag = 1;
                break;
            case 'c': 
                if (cflag == 1) {
                    printf("You can only have one -c argument.\n");
                    return -1;
                }
                *c = 1;
                cflag = 1;
                break;
            case 'm': 
                if (mflag == 1) {
                    printf("You can only have one -m argument.\n");
                    return -1;
                }
                if (strlen(optarg) > 255) {
                    printf("The string provided is too long. It must be 255 characters or under..\n");
                    return -1;
                }
                else strcpy(str, optarg);
                mflag = 1;
                break;
            case 'u': 
                if (uflag == 1) {
                    printf("You can only have one -u argument.\n");
                    return -1;
                }
                *u = 1;
                uflag = 1;
                break;
            case 's': 
                if (sflag == 1) {
                    printf("You can only have one -s argument.\n");
                    return -1;
                }
                *s = 1;
                sflag = 1;
                break;
            case 'q': 
                if (qflag == 1) {
                    printf("You can only have one -q argument.\n");
                    return -1;
                }
                *q = atoi(optarg);
                qflag = 1;
                break;
            case 'e':
                if (eflag == 1) {
                    printf("You can only have one -e argument.\n");
                    return -1;
                }
                *e = 1;
                break;
        }
    }
    if ((*c == 1 && *p == 1) || (*c == 0 && *p == 0)) {
        printf("The program must be either a consumer or producer, not both.\n");
        return -1;
    }
    if (*c == 1 && mflag == 1) {
        printf("A consumer cannot take a -m argument.\n");
        return -1;
    }
    if (*p == 1 && mflag == 0) {
        printf("A producer needs a string to produce. To pass a string to produce, do -m \"string\".\n");
        return -1;
    }
    if ((*u == 1 && *s == 1) || (*u == 0 && *s == 0)) {
        printf("The program must communicate using a Unix socket or shared memory, not both.\n");
        return -1;
    }
    if (qflag == 0) {
        printf("The depth of the queue must be passed as an argument using the format -q num.\n");
        return -1;
    }
    if (*q > 255) {
        printf("The depth of the queue must be <= 255.\n");
        return -1;
    }
    if (strcmp(str, termc) == 0 || strcmp(str, termp) == 0 || strstr(str, termp) != NULL || strstr(str, termc) != NULL) {
        printf("The string provided may not be used for production.\n");
        return -1;
    }
    return 0;
}

void sig_handler(int num) {
    keep_running = 0;
}

int producer(char* str, int s, int u, int e, int q, sem_t* semap, sem_t* semac) {
    int in = 0;
    if (s == 1) strcpy(shared_memory->arr[com_buff], "random_string");
    else strcpy(buff[com_buff], "random_string");
    while (keep_running == 1) {
        if (sem_wait(semap) == -1) {
            fprintf(stderr, "sem_wait on producer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
        if (e == 1) printf("%s\n", str);
        if (s == 1) {
            if (strcmp(shared_memory->arr[com_buff], termc) == 0) {
                cterm = 1;
                keep_running = 0;
                break;
            }
            else strcpy(shared_memory->arr[in], str);
        } 
        else if (u == 1) {
            if ((valread = recv(sock, buff[in], sizeof(buff[in]), MSG_DONTWAIT)) != -1) {
                buff[in][valread] = '\0';
                if (strstr(buff[in], termp) != NULL) {
                    cterm = 1;
                    keep_running = 0;
                    break;
                }
            }
            strcpy(buff[in], str);
            if (send(sock, buff[in], strlen(buff[in]), 0) == -1) {
                fprintf(stderr, "send function on socket failed. Error #%d. %s\n", errno, strerror(errno));
                return -1;
            }
        }
        in = (in + 1) % q;
        if (sem_post(semac) == -1) {
            fprintf(stderr, "sem_post on consumer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    if (cterm !=1) {
        if (s == 1) {
            strcpy(shared_memory->arr[com_buff], termp);
        }
        else {
            if (send(sock, termp, strlen(termp), 0) == -1) {
                fprintf(stderr, "send function on socket failed. Error #%d. %s\n", errno, strerror(errno));
                return -1;
            }
        }
        if (sem_post(semac) == -1) {
            fprintf(stderr, "sem_post on consumer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

int consumer(int s, int u, int e, int q, sem_t* semap, sem_t* semac) {
    int out = 0;
    if (s == 1) strcpy(shared_memory->arr[com_buff], "random_string");
    else strcpy(buff[com_buff], "random_string");
    while (keep_running == 1) {
        if (sem_wait(semac) == -1) {
            fprintf(stderr, "sem_wait on consumer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
        if (s == 1) {
            if (strcmp(shared_memory->arr[com_buff], termp) == 0) {
                pterm = 1;
                keep_running = 0;
                break;
            }
        }
        if (e == 1 && s ==1) printf("%s\n", shared_memory->arr[out]);
        else if (u == 1) {
            valread = recv(sock, buff[out], sizeof(buff[out]), 0);
            if (valread == -1) {
                fprintf(stderr, "recv from socket failed. Error #%d. %s\n", errno, strerror(errno));
                return -1;
            }
            buff[out][valread] = '\0';
            if (strstr(buff[out], termp) != NULL) {
                pterm = 1;
                keep_running = 0;
                break;
            }
            if (e == 1) printf("%s\n", buff[out]);
        }
        out = (out + 1) % q;
        if (sem_post(semap) == -1) {
            fprintf(stderr, "sem_post on producer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    if (pterm != 1) {
        if (s == 1) {
            strcpy(shared_memory->arr[com_buff], termc);
        }
        else {
            if (send(sock, termc, sizeof(termc), 0) == -1) {
                fprintf(stderr, "send function on socket failed. Error #%d. %s\n", errno, strerror(errno));
                return -1;
            }
        }
        if (sem_post(semap) == -1) {
            fprintf(stderr, "sem_post on consumer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

void errorMsg() {
    printf("An error has occurred. Please read the above messages for more details.\n");
}

int init_sm(int *q) {
    key = 0142;
    if ((shmid = shmget(key, sizeof(buffer), 0666 | IPC_CREAT)) == -1) {
        fprintf(stderr, "shmget function failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    if ((shared_memory = (buffer*) shmat(shmid, NULL, 0)) == (void*) -1) {
        fprintf(stderr, "shmmat function failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    return 0;
}

int init_s(int* q) {
    int other_q;
    memset(&address, 0, sizeof(address));
    address.sun_family = AF_UNIX;
    strncpy(address.sun_path, SOCK_PATH, sizeof(address.sun_path) - 1);
    if ((sock = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        fprintf(stderr, "socket function failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    if (connect(sock, (struct sockaddr *) &address, sizeof(address)) == -1) {
        if (bind(sock, (struct sockaddr *) &address, sizeof(address)) == -1) {
            fprintf(stderr, "bind function failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
        if (listen(sock, 1) == -1) {
            fprintf(stderr, "listen function failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
        if ((sock = accept(sock, NULL, NULL)) == -1) {
            fprintf(stderr, "accept function failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    return 0;
}

int main(int argc, char** argv) {
    int p = 0, c = 0, u = 0, s = 0, q = 0, e = 0;
    char str[256];
    if (readArgs(argc, argv, &p, &c, &u, &s, &q, &e, str) == -1) {
        errorMsg();
        return -1;
    }
    if (u == 1) {
        sem_unlink(SEM_P);
        sem_unlink(SEM_C);
    }
    if (s == 1) {
        if (init_sm(&q) == -1) {
            errorMsg();
            return -1;
        }
    }
    else {
        if (init_s(&q) == -1) {
            errorMsg();
            return -1;
        }
    }
    signal(SIGINT, sig_handler);
    sem_t *semap, *semac;
    if ((semap = sem_open(SEM_P, O_CREAT, 0660, q)) == SEM_FAILED) {
        printf("Consumer semaphore open function failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    if((semac = sem_open(SEM_C, O_CREAT, 0660, 0)) == SEM_FAILED) {
        printf("Consumer semaphore open function failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    if (p == 1) {
        if (producer(str, s, u, e, q, semap, semac) == -1) {
            errorMsg();
            return -1;
        }
    }
    else {
        if (consumer(s, u, e, q, semap, semac) == -1) {
            errorMsg();
            return -1;
        }
    }
    if (sem_close(semap) == -1) {
        fprintf(stderr, "sem_close function on producer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    if (sem_close(semac) == -1) {
        fprintf(stderr, "sem_close function on consumer semaphore failed. Error #%d. %s\n", errno, strerror(errno));
        return -1;
    }
    if (s == 1) {
        if (shmdt(shared_memory) == -1) {
            fprintf(stderr, "shmdt function on shared_memmory semaphore failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    if (pterm == 1 || cterm == 1) {
        shmctl(shmid, IPC_RMID, NULL);
        if (sem_unlink(SEM_P) == -1) {
            fprintf(stderr, "sem_unlink function on SEM_P failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
        if (sem_unlink(SEM_C) == -1) {
            fprintf(stderr, "sem_unlink function on SEM_C failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
    }
    if (u == 1) {
        if (close(sock) == -1) {
            fprintf(stderr, "close function on sock_fd failed. Error #%d. %s\n", errno, strerror(errno));
            return -1;
        }
        unlink(SOCK_PATH);
    }
    return 0;
}