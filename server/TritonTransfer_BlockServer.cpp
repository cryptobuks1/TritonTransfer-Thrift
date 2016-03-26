// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "BlockServerHandler.cpp"

int main(int argc, char **argv)
{
    //<server_port> <metadata_server> <metadata_port>
    if (argc != 4) { // Test for correct number of arguments
        cerr << "Expected parameter(s): <server_port> <metadata_server> <metadata_port> \n";
        return 0;
    }
    int port = atoi(argv[1]); // First arg:  local port
    string md_server = string(argv[2]);
    int md_port = atoi(argv[3]);
    shared_ptr<BlockServerHandler> handler(new BlockServerHandler(md_server, md_port, port));
    shared_ptr<TProcessor> processor(new TritonTransferProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    server.serve();
    return 0;
}
