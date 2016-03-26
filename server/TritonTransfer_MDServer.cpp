// This autogenerated skeleton file illustrates how to build a server.
// You should copy it to another filename to avoid overwriting it.

#include "MDServerHandler.cpp"

int main(int argc, char **argv)
{
    // tt-server <server_port> <file_dir>
    if (argc != 2) { // Test for correct number of arguments
        cerr << "Expected parameter(s): <Server Port>\n";
        return 0;
    }
    int port = atoi(argv[1]); // First arg:  local port

    shared_ptr<MDServerHandler> handler(new MDServerHandler());
    shared_ptr<TProcessor> processor(new TritonTransferProcessor(handler));
    shared_ptr<TServerTransport> serverTransport(new TServerSocket(port));
    shared_ptr<TTransportFactory> transportFactory(new TBufferedTransportFactory());
    shared_ptr<TProtocolFactory> protocolFactory(new TBinaryProtocolFactory());

    TSimpleServer server(processor, serverTransport, transportFactory, protocolFactory);
    server.serve();
    return 0;
}