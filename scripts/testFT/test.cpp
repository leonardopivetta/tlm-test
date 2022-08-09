using namespace std;

#include <iostream>
#include <string>
#include <unistd.h>

#include "ws_connection.h"
#include "file_transfer.h"


bool is_open = false;

file_chunk chunk;
file_end_transaction tr_end;
file_begin_transaction tr_beg;
FileTransfer::FileTransferManager ftm;
std::unordered_map<int, int> transfers;


file_transfer_callback callback = [](const int &id_, TRANSACTION_EVENT event){
    int id = id_;
    if(event != TRANSACTION_QUEUE_FULL){
        for(auto it : transfers)
            if(it.second == id)
                id = it.first;
        cout << "Transaction <" << id << "> event: " << TRANSACTION_EVENT_STRING[event] << endl;
    }
};

void onMessage(const int &id, const GenericMessage &msg)
{
    if(msg.topic == "file_transfer" && StringToStruct(msg.payload, chunk)){
        auto it = transfers.find(chunk.transaction_hash);
        if(it != transfers.end()){
            if(!ftm.receive(it->second, msg)){
                printf("FileTranferManager did not find transaction");
            }else{
                printf("<%d> received chunk %d of %d: %f\n", chunk.transaction_hash, chunk.chunk_n, chunk.chunk_total, (float)chunk.chunk_n / chunk.chunk_total);
            }
        }else{
            printf("Transaction id unrecognized %d\n\r", chunk.transaction_hash);
        }
    }else if(msg.topic == "file_transfer_begin" && StringToStruct(msg.payload, tr_beg)){
        FileTransfer::FileTransferManager::transaction_t tr(
            tr_beg.filename,
            tr_beg.dest_path,
            tr_beg.total_chunks,
            callback
        );
        int id = ftm.init_receive(tr);
        if(id != -1){
            transfers.insert({tr_beg.transaction_hash,  id});
            printf("<%d> -> %s\n\r", tr_beg.transaction_hash, tr_beg.filename.c_str());
        }else{
            printf("Failed to receive file %s\n\r", tr_beg.filename.c_str());
        }
    }else if(msg.topic == "file_transfer_end" && StringToStruct(msg.payload, tr_end)){
        auto it = transfers.find(tr_end.transaction_hash);
        if(it != transfers.end()){
            if(ftm.end_receive(it->second)){
                transfers.erase(it);
                printf("<%d> -> End\n\r", tr_end.transaction_hash);
            }else{
                printf("<%d> -> FAILED end receive\n\r", tr_end.transaction_hash);
            }
        }else{
            printf("End transaction failed\n\r");
        }
    }
}

void onError(const int &id, const int &code, const string &message)
{
    cout << id << " Error: " << code << " - " << message << endl;
}

void onClose(const int &id, const int &code)
{
    cout << id << " Connection closed: " << code << endl;
}

void onOpen(const int &id)
{
    is_open = true;
    cout << id << " Connection opened" << endl;
}

int main()
{
    WSConnection conn_ws;
    conn_ws.init("telemetry-server.herokuapp.com", "", 0);

    conn_ws.addOnError(onError);
    conn_ws.addOnClose(onClose);
    conn_ws.addOnOpen(onOpen);
    conn_ws.addOnMessage(onMessage);

    thread *pub_thread = conn_ws.start();

    conn_ws.subscribe("file_transfer");
    conn_ws.subscribe("file_transfer_end");
    conn_ws.subscribe("file_transfer_begin");

    Connection& conn = conn_ws;
    FileTransfer::FileTransferManager::transaction_t transaction(
        "bin/telemetry",
        "bin",
        &conn,
        "file_transfer",
        40960,
        callback
    );
    FileTransfer::FileTransferManager::transaction_t transaction2(
        "/home/filippo/Downloads/Telemetry-app-0.1.1-Linux.deb",
        "/home/filippo/Desktop/CSV/08_07_2022",
        &conn,
        "file_transfer",
        409600,
        callback
    );
    while(!is_open){
        usleep(100);
    }
    cout << "Connection Opened" << endl;
    usleep(10000);
    if(ftm.send(transaction) == -1){
        printf("Failed to send file 1\n\r");
    }
    if(ftm.send(transaction2) == -1){
        printf("Failed to send file 2\n\r");
    }

    while (true)
    {
        sleep(1);
    }

    conn_ws.closeConnection();

    return 0;
}