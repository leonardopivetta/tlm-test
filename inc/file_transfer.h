#pragma once

#include <mutex>
#include <thread>
#include <string>
#include <unordered_map>

#include "json-loader/messages_json.h"

/*
 * This class is used to manage the file transfer.
 * Allows to send and receive files. It is thread safe.
 * It also manages the file transfer queue.
*/

/**
 * First establish a connection between the sender and the receiver.
 * Then instantiate a transaction object: FileTransfer::FileTransferManager::transaction_t;
 * There are two constructors, one for the sender and one for the receiver.
 * Basically you must setup filename and destination path. And max_chunk_size (in bytes), a connection pointer, and a callback function.
 * The callback function is used to notify the user about the events.
 * Then you can start the file transfer.
 * Instantiate a FileTransfer::FileTransferManager object. This can be global as it can handle multiple file transfers (both send and receive).
 * To start a file transfer for sending: Call FileTransfer::FileTransferManager::send().
 * The send function handles all the sending process, it creates a thread and returns the id of the transaction.
 * A receiving transaction is defined by the topic_begin, from that message you can get the filename, destination path and total_chunks.
 * When receiving topic_begin call FileTransfer::FileTransferManager::init_receive().
 * Each time you receive a topic you can get the chunk_id and the data, and call FileTransfer::FileTransferManager::receive().
 * The transaction end is notified by the topic_end message.
 * When the transaction is finished you can call FileTransfer::FileTransferManager::end_receive(). This function can return false if not all chunks were received.
 * 
 * Each transaction is defined by a hash, so each message_chunk contains the hash. The user must handle the linking between the hash and the transaction id returned by the TransactionManager.
 * 
 * The sending thread can fill the queue with messages, but checks the queue rememaining size before sending, to avoid messages being dropped.
 */


struct file_transfer_data{
    int max_chunk_size;
    int chunk_total;
    int chunk_current;

    std::unordered_map<int, std::string> cached_chunks; // used when received chunks are not ordered

    std::string filename;
    std::fstream* file;

    file_transfer_data(){};
};

class Connection;
class GenericMessage;

enum TRANSACTION_EVENT{
    TRANSACTION_END,
    TRANSACTION_ERROR,
    TRANSACTION_OPEN,
    TRANSACTION_QUEUE_FULL,
    TRANSACTION_EXCESSIVE_CHUNK,
    TRANSACTION_CHUNK_CURRUPTED,
    TRANSACTION_CHUNK_MISSING_CHUNKS
};
static std::string TRANSACTION_EVENT_STRING[] = {
    "TRANSACTION_END",
    "TRANSACTION_ERROR",
    "TRANSACTION_OPEN",
    "TRANSACTION_QUEUE_FULL",
    "TRANSACTION_EXCESSIVE_CHUNK",
    "TRANSACTION_CHUNK_CURRUPTED",
    "TRANSACTION_CHUNK_MISSING_CHUNKS"
};

// Used when sending a file has an event 
typedef void (*file_transfer_callback)(const int &id, TRANSACTION_EVENT event);
namespace FileTransfer{
    void cleanup(file_transfer_data& data);
    bool setup_chunks_to_file(file_transfer_data& data, std::string filename, int num_chunks);
    bool setup_file_to_chunks(file_transfer_data& data, std::string filename, int max_chunk_size=4096);
    bool read_next_chunk(file_transfer_data& data, std::string& chunk);
    bool write_next_chunk(file_transfer_data& data, std::string& chunk, int chunk_id);

    class FileTransferManager{
        public:
            FileTransferManager();
            ~FileTransferManager();

            struct transaction_t{
                // Use this for receiving
                transaction_t(std::string filename_, std::string dest_path_, int chunk_total_, file_transfer_callback callback_):
                filename(filename_), dest_path(dest_path_), chunk_total(chunk_total_), callback(callback_){}
                // Use this for sending
                transaction_t(std::string filename_, std::string dest_path_, Connection* conn_, std::string topic_, int max_chunk_size_, file_transfer_callback callback_):
                filename(filename_), dest_path(dest_path_), conn(conn_), topic(topic_), max_chunk_size(max_chunk_size_), callback(callback_){}

                // From user
                Connection* conn;
                std::string topic;
                std::string filename;
                std::string dest_path;
                file_transfer_callback callback;
                int max_chunk_size;
                int chunk_total;

                // internal
                int id;
                bool sending;
                bool receiving;
                int transaction_hash;
                file_transfer_data data;
                std::thread* thread_ptr;
            };
            
            bool is_sending(int id);
            int send(transaction_t transaction);

            int init_receive(transaction_t transaction);
            bool receive(int id, const GenericMessage& message);
            bool end_receive(int id);

        private:
            std::unordered_map<int, transaction_t> transactions;

            static int id;
    };
};