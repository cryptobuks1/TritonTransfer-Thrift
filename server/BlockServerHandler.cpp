// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "./gen-cpp/TritonTransfer.h"
#include <thrift/protocol/TBinaryProtocol.h>
#include <thrift/server/TSimpleServer.h>
#include <thrift/transport/TServerSocket.h>
#include <thrift/transport/TBufferTransports.h>
#include <thrift/transport/TSocket.h>
#include <thrift/transport/TTransportUtils.h>
#include "../utils/utils.hpp"
#include <unordered_map>

using namespace ::apache::thrift;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::transport;
using namespace ::apache::thrift::server;
using boost::shared_ptr;
using namespace  ::no::podcasts::no::learning;
using std::unordered_map;
static SHA256 sha256;
static const int MAX_BLOCK_SIZE = 16000;

typedef struct MDServer {
    int32_t md_port;
    string md_server_name;
    TritonTransferClient *server;
    boost::shared_ptr<TTransport> transport;
} MDServerStruct;

// struct ServerInfo {
//     1: required list<HashValue> server_hash_list,
//     2: required i32 port,
//     3: required ServerAddr server_name,
//     4: required bool file_exists,
// }

class BlockServerHandler : virtual public TritonTransferIf {
    unordered_map<HashValue, string> *hash_to_block;
    unordered_map<string, vector<HashValue> *> *file_names_to_hashes;
    int32_t my_port;
    ServerAddr my_server_addr;
    MDServerStruct *md_server;
public:

    BlockServerHandler()
    {
        bootstrapBlockServer();
    }

    BlockServerHandler(string mdname, int32_t mdport, int32_t myport)
    {
        md_server = new MDServerStruct();
        md_server->md_server_name = mdname;
        md_server->md_port = mdport;
        my_port = myport;
        bootstrapBlockServer();
    }

    ~BlockServerHandler()
    {
        delete hash_to_block;
        delete file_names_to_hashes;
    }

    void ping()
    {
        // cerr << "ping()" << endl;
    }

    void uploadFile(std::vector<ServerInfo> &_return, const std::string &file_name, const std::vector<HashValue> &hash_list)
    {
        // string path_and_file_name = getRealPath(doc_root + "/" + file_name);
        // cerr << "Entering uploadFile in block server. Trying to upload " << file_name << endl;
        auto file_lookup = file_names_to_hashes->find(file_name);
        if (file_names_to_hashes->end() == file_lookup) {
            // cerr << file_name << " was not found in memory, creating a new entry for it." << endl;
            file_names_to_hashes->insert(std::make_pair(file_name, new vector<HashValue>(hash_list)));
        }
        vector<HashValue> hashes_needed;
        for (int i = 0; i < hash_list.size(); i++) {
            auto block_lookup = hash_to_block->find(hash_list.at(i)); // Check if we already have block, if not add it to return list.
            if (hash_to_block->end() == block_lookup) {
                hashes_needed.push_back(hash_list.at(i));
            }
        }
        // cerr << "Need " << hashes_needed.size() << " blocks for " << file_name << endl;
        ServerInfo server_info;
        server_info.server_hash_list = hashes_needed;
        server_info.port = my_port;
        server_info.server_name = my_server_addr;
        server_info.file_exists = false;
        _return.push_back(server_info);
        // cerr << "Completed uploadFile" << endl;
        return;
    }

    void uploadBlock(std::string &_return, const HashValue &hv, const Block &block)
    {
        // cerr << "Entering uploadBlock in block server.\n";
        if (sha256(block) != hv) {
            // cerr << "ERROR! Block hash at server did not match block hash at client.\n";
            _return = "ERROR";
            return;
        } else if (block.size() > MAX_BLOCK_SIZE) {
            // cerr << "ERROR! Uploaded block size was larger than MAX_BLOCK_SIZE: " << block.size() << endl;
            _return = "ERROR";
            return;
        }
        auto hash_lookup = hash_to_block->find(hv);
        if (hash_to_block->end() == hash_lookup) {
            // cerr << "Inserting block of size: " << block.size() << " into hash_to_block at server.\n";
            hash_to_block->insert(std::make_pair(hv, block));
        }
        _return = "OK";
        return;
    }

    void downloadFile(std::vector<ServerInfo> &_return, const std::string &file_name)
    {
        // cerr << "Entering downloadFile in server. Client is attempting to download " << file_name << endl;
        auto file_entry = file_names_to_hashes->find(file_name);
        ServerInfo server_info;
        if (file_entry == file_names_to_hashes->end()) {
            // cerr << file_name << " was not present on " << my_server_addr << endl;
            vector<HashValue> dummy_vector;
            server_info.server_hash_list = dummy_vector;
            server_info.file_exists = false;
        } else {
            if (fileHashesExist(file_name)) {
                // cerr << file_name << " was found on " << my_server_addr << endl;
                vector<HashValue> hashes_for_file(*(file_entry->second));
                server_info.server_hash_list = hashes_for_file;
                server_info.file_exists = true;
            } else {
                // cerr << file_name << " was not present on " << my_server_addr << endl;
                vector<HashValue> dummy_vector;
                server_info.server_hash_list = dummy_vector;
                server_info.file_exists = false;
            }
        }
        // cerr << file_name << " has " << server_info.server_hash_list.size() << " total blocks.\n";
        server_info.port = my_port;
        server_info.server_name = my_server_addr;
        _return.push_back(server_info);
        return;
    }

    bool fileHashesExist(string file_name)
    {
        auto file_hashes_list = file_names_to_hashes->find(file_name);
        for (int i = 0; i < file_hashes_list->second->size(); i++) {
            auto curr_hash = hash_to_block->find(file_hashes_list->second->at(i));
            if (curr_hash == hash_to_block->end()) {
                // cerr << "One or more blocks for " << file_name << " were not found on this block server.\n";
                return false;
            }
        }
        return true;
    }

    void downloadBlock(ErrorOrBlock &_return, const HashValue &hv)
    {
        // cerr << "Entering downloadBlock in server.\n";
        if (hash_to_block->end() == hash_to_block->find(hv)) {
            // cerr << "ERROR! Requested block was not found in server.\n";
            _return.error = true;
        } else {
            auto hash_lookup = hash_to_block->find(hv);
            _return.error = false;
            _return.block = hash_lookup->second;
            // cerr << "downloadBlock sent a block of size " << _return.block.length() << " back to the client.\n";
        }
        return;
    }

    void bootstrapBlockServer()
    {
        // cerr << "Bootstrapping block server...\n";
        hash_to_block = new unordered_map<HashValue, string>();
        file_names_to_hashes = new unordered_map<string, vector<HashValue> *>();
        char hostname[256];
        int result = gethostname(hostname, sizeof hostname);
        if (result == -1) {
            // cerr << strerror(errno) << ": gethostname(2)\n";
            exit(1);
        }
        my_server_addr = string(hostname) + ".ucsd.edu";
        // cerr << "hostname of this block server is: " << md_server_addr << endl;
        if (!connectToMDServer()) {
            // cerr << "ERROR: Unable to connect to MD Server!\n";
            exit(0);
        }
        md_server->transport->close();
        // cerr << "Connection with MDServer closed.\n" << endl;
        return;
    }

    bool connectToMDServer()
    {
        try {
            boost::shared_ptr<TTransport> socket(new TSocket(md_server->md_server_name, md_server->md_port));
            boost::shared_ptr<TTransport> transport(new TBufferedTransport(socket));
            boost::shared_ptr<TProtocol> protocol(new TBinaryProtocol(transport));
            TritonTransferClient *tt_md_server = new TritonTransferClient(protocol);
            transport->open();
            tt_md_server->ping();
            // cerr << "Successfully pinged md_server at " << md_server->md_server_name << endl;
            md_server->server = tt_md_server;
            md_server->transport = transport;
            md_server->server->bootstrapBlockServer(my_port, my_server_addr);
            return true;;
        } catch (...) {
            // cerr << "Caught an exception while connecting to MD server.\n";
            return false;
        }
    }

    void bootstrapBlockServer(const int32_t port, const ServerAddr &server_addr)
    {
        // Not needed
    }
};

// TODO remove duplicate servers
// TODO make sure output is correct

// DONE
// TODO deal with doc_root shit here
// TODO close connections with dead servers
// TODO test with empty files
