#include <sys/time.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <errno.h>

#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <unistd.h>
#include <assert.h>
#include <time.h>
#include "hiredis.h"
#include "bson.h"
#include "bcon.h"
#include "mongoc.h"
#include "zmq.hpp"
#include "canna_daemon.h"
#include "canna_core.h"
#include "canna_random.h"

#define COMMAND_LEN 256

char command[COMMAND_LEN];

int initialize(int mode);
void signal_usr1(int signal);
void signal_usr2(int signal);

int main(int argc, char **argv)
{
    initialize(1);

    int major, minor, patch;
    zmq::version(&major, &minor, &patch);
    CaRandom random;
    random.SetSeed((unsigned int)(canna_gettime()));
    printf("current zeromq version is %d.%d.%d, timestamp %lu, random %u\n", major, minor, patch, canna_gettime(), random.Random(1024));

    redisContext *context = redisConnect("127.0.0.1", 6379);
    if (context == nullptr || context->err) {
        if (context) {
            printf("Error: %s, Code: %d\n", context->errstr, context->err);
            return 1;
        } else {
            printf("Can't allocate redis context\n");
            return 1;
        }
    } else {
        printf("Success: context init \n");
    }

    mongoc_client_t         *mongoc_client;
    mongoc_database_t       *database;
    mongoc_collection_t     *collection;
    bson_t                  *mongoc_command, reply, *insert;
    bson_error_t            error;
    char                    *str;
    bool                    retval;

    // required to initialize libmongoc's internals
    mongoc_init();
    // create a new client instance
    mongoc_client = mongoc_client_new("mongodb://localhost:27017");
    
    database = mongoc_client_get_database(mongoc_client, "db_name");
    collection = mongoc_client_get_collection(mongoc_client, "db_name", "coll_name");

    mongoc_command = BCON_NEW("ping", BCON_INT32(1));
    retval = mongoc_client_command_simple(mongoc_client, "admin", mongoc_command, nullptr, &reply, &error);
    if (!retval) {
        fprintf(stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }

    str = bson_as_json(&reply, nullptr);
    printf("%s\n", str);
    
    insert = BCON_NEW("hello", BCON_UTF8("world"));
    if (!mongoc_collection_insert(collection, MONGOC_INSERT_NONE, insert, nullptr, &error)) {
        fprintf(stderr, "%s\n", error.message);
    }

    bson_destroy(insert);
    bson_destroy(&reply);
    bson_destroy(mongoc_command);
    bson_free(str);


    //redisReply *reply = (redisReply *)redisCommand(context, "SET foo barbarbar");


    //  Prepare our context and socket
    zmq::context_t zmq_context (1);
    zmq::socket_t socket (zmq_context, ZMQ_REP);
    socket.bind ("tcp://*:5555");

    while (true) {
        zmq::message_t request;

        //  Wait for next request from client
        socket.recv (&request);
        if (request.size() > COMMAND_LEN) {
            continue;
        }

        memset(command, 0, COMMAND_LEN);
        memcpy(command, static_cast<char*>(request.data()), request.size());

        redisReply *result = (redisReply *)redisCommand(context, command);

        //  Send reply back to client
        zmq::message_t reply(result->len);
        memcpy (reply.data(), result->str, result->len);
        socket.send(reply);

        freeReplyObject(result);
    }

    mongoc_collection_destroy(collection);
    mongoc_database_destroy(database);
    mongoc_client_destroy(mongoc_client);
    mongoc_cleanup();

    daemon_exit("./dbserver.pid");
    return 0;
}

void signal_usr1(int signal)
{
}

void signal_usr2(int signal)
{
}

int initialize(int mode)
{
    int result;
    result = daemon_init(mode, "./dbserver.pid");

    if (0 != result)
    {
        exit(0);
    }

    signal(SIGUSR1, signal_usr1);
    signal(SIGUSR2, signal_usr2);

    return result;
}
