# Producer-Consumer

This program can be a producer or consumer and will run forever unless a SIGINT is sent to the process. The program uses semaphores to protect critical sections. Here are a few command-line arguments that can be passed when running the program:

-c or -p (one required, not both): makes the process act like a consumer or producer respectively
-s or -u (one required, not both): chooses shared memory or Unix sockets respectively as a form of interprocess communication
-m "string" (required, producer only): inputs string that producer will produce
-q int (required): sets the buffer size for the program (please note that consumer's buffer size should not be greater than producer's buffer size)
-e (optional for both producer and consumer): prints the string being produced or consumed

Example Uses of Command-Line Arguments:
./producer-consumer.c -p -s -m "Hello Word!" -e -q 5
./producer-consumer.c -c -s -e -q 5
./producer-consumer.c -p -s -m "Hello Word!" -q 5
./producer-consumer.c -c -s -q 5
