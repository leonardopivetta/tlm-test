#include "file_transfer.h"

#include "connection.h"

#include <math.h>
#include <fstream>

namespace FileTransfer{

    int FileTransferManager::id = -1;

    void cleanup(file_transfer_data& data){
        if(data.file != nullptr && data.file->is_open()){
            data.file->close();
            delete data.file;
        }
    }

    bool setup_chunks_to_file(file_transfer_data& data, std::string filename, int num_chunks){
        data.chunk_current = 0;
        data.filename = filename;
        data.chunk_total = num_chunks;
        data.file = new std::fstream(data.filename, std::fstream::out | std::fstream::binary);
        if(!data.file->is_open()){
            cleanup(data);
            return false;
        }
        return true;
    }

    bool setup_file_to_chunks(file_transfer_data& data, std::string filename, int max_chunk_size){
        data.chunk_current = 0;
        data.filename = filename;
        data.max_chunk_size = max_chunk_size;
        data.file = new std::fstream(filename, std::ios_base::in | std::ios_base::binary);
        if(!data.file->is_open()){
            cleanup(data);
            return false;
        }
        
        data.file->seekg(0, std::ios::end);
        data.chunk_total = ceil(data.file->tellg() / max_chunk_size);
        data.file->seekg(0, std::ios::beg);

        return true;
    }
    
    bool read_next_chunk(file_transfer_data& data, std::string& chunk){
        if(data.chunk_current > data.chunk_total){
            return false;
        }
        chunk.resize(data.max_chunk_size);
        data.file->read(&chunk[0], data.max_chunk_size);
        chunk.resize(data.file->gcount());
        data.chunk_current++;
        return true;
    }

    bool write_next_chunk(file_transfer_data& data, std::string& chunk, int chunk_id){
        if(data.chunk_current > data.chunk_total){
            return false;
        }
        if(data.chunk_current == chunk_id){
            data.file->write(&chunk[0], chunk.size());
            data.chunk_current++;
            while(data.cached_chunks.find(data.chunk_current) != data.cached_chunks.end()){
                data.file->write(
                    &data.cached_chunks[data.chunk_current][0],
                     data.cached_chunks[data.chunk_current].size());
                data.cached_chunks.erase(data.chunk_current);
                data.chunk_current++;
            }
            data.file->flush();
        }
        else{
            data.cached_chunks[chunk_id] = chunk;
        }
        return true;
    }

    FileTransferManager::FileTransferManager(){
    }

    FileTransferManager::~FileTransferManager(){
        for(auto& it : transactions){
            if(it.second.thread_ptr != nullptr){
                it.second.thread_ptr->join();
                delete it.second.thread_ptr;
            }
            cleanup(it.second.data);
        }
    }

    bool FileTransferManager::is_sending(int id){
        auto it = transactions.find(id);
        if(it == transactions.end()){
            return false;
        }
        return it->second.sending;
    }

    int FileTransferManager::send(transaction_t transaction){
        file_transfer_data data;
        if(!setup_file_to_chunks(data, transaction.filename, transaction.max_chunk_size))
            return -1;
        transaction.transaction_hash = (int)hash<std::string>{}(data.filename);
        {
            auto pos = data.filename.rfind('/');
            if(pos != std::string::npos){
                data.filename = data.filename.substr(pos + 1);
            }
        }
        // copying data
        id ++;
        transactions.insert({id, transaction});
        transactions.at(id).id = id;
        transactions.at(id).data = data;
        // creating sender thread
        transactions.at(id).thread_ptr = new std::thread([](transaction_t* trans){
            file_chunk chunk;
            file_end_transaction end {
                .filename = trans->data.filename,
                .dest_path = trans->dest_path,
                .transaction_hash = trans->transaction_hash};
            file_begin_transaction beg {
                .filename = trans->data.filename,
                .dest_path = trans->dest_path,
                .total_chunks = trans->data.chunk_total,
                .transaction_hash = trans->transaction_hash};

            // notifying start send
            trans->sending = true;
            trans->callback(trans->id, TRANSACTION_EVENT::TRANSACTION_OPEN);

            while(trans->conn->getQueueSize() < 5){
                trans->callback(trans->id, TRANSACTION_EVENT::TRANSACTION_QUEUE_FULL);
                usleep(100);
            }
            trans->conn->setData(GenericMessage(trans->topic + "_begin", StructToString(beg)));
            
            // setup json message
            chunk.chunk_total = trans->data.chunk_total;
            chunk.transaction_hash = trans->transaction_hash;
            while(true){
                chunk.chunk_n = trans->data.chunk_current;
                // reding chunks
                if(!read_next_chunk(trans->data, chunk.data))
                    break;
                // if connection queue is full it drops messages, so wait
                while(trans->conn->getQueueSize() < 5){
                    trans->callback(trans->id, TRANSACTION_EVENT::TRANSACTION_QUEUE_FULL);
                    usleep(100);
                }
                // send data
                trans->conn->setData(GenericMessage(trans->topic, StructToString(chunk)));
            }
            // notify and cleanup
            trans->conn->setData(GenericMessage(trans->topic + "_end", StructToString(end)));
            trans->callback(trans->id, TRANSACTION_EVENT::TRANSACTION_END);
            trans->sending = false;
            cleanup(trans->data);
        }, &transactions.at(id));
        
        return id;
    }

    int FileTransferManager::init_receive(transaction_t transaction){
        id ++;
        transaction.receiving = true;
        transaction.data.filename = transaction.dest_path + "/" + transaction.filename;
        if(!setup_chunks_to_file(transaction.data, transaction.data.filename, transaction.chunk_total))
            return -1;

        transaction.id = id;
        transactions.insert({id, transaction});
        return id;
    }

    bool FileTransferManager::receive(int id, const GenericMessage& message){
        auto trans = transactions.find(id);
        if(trans == transactions.end()){
            return false;
        }
        if(trans->second.receiving){
            file_chunk chunk;
            if(!StringToStruct(message.payload, chunk)){
                trans->second.callback(id, TRANSACTION_CHUNK_CURRUPTED);
                return false;
            }
            if(!write_next_chunk(trans->second.data, chunk.data, chunk.chunk_n))
                trans->second.callback(id, TRANSACTION_EXCESSIVE_CHUNK);
        }else{
            return false;
        }
        return true;
    }

    bool FileTransferManager::end_receive(int id){
        auto trans = transactions.find(id);
        if(trans == transactions.end()){
            return false;
        }
        if(trans->second.data.chunk_current-1 == trans->second.data.chunk_total &&
            trans->second.data.cached_chunks.size() == 0){
                trans->second.receiving = false;
                cleanup(trans->second.data);
                transactions.erase(id);
                return true;
        }else{
            trans->second.callback(id, TRANSACTION_CHUNK_MISSING_CHUNKS);
            return false;
        }
    }
};