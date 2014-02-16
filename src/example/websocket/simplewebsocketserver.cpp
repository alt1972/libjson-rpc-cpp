/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file simplewebsocketserver.cpp
 * @date 15.02.2014
 * @author toolmmy
 * @license See attached LICENSE.txt
 ************************************************************************/

#include <stdio.h>
#include <string>
#include <iostream>
#include "jsonrpc/rpc.h"

using namespace jsonrpc;
using namespace std;

class SampleWebsocketServer: public AbstractServer<SampleWebsocketServer>
{
    public:
        SampleWebsocketServer()
                : AbstractServer<SampleWebsocketServer>(new WebsocketServer(9876, "de.td.testprotocol", true))
        {
            this->bindAndAddMethod(new Procedure("sayHello", PARAMS_BY_NAME, JSON_STRING, "name", JSON_STRING, NULL),
                    &SampleWebsocketServer::sayHello);
            this->bindAndAddNotification(new Procedure("notifyServer", PARAMS_BY_NAME, NULL),
                    &SampleWebsocketServer::notifyServer);
        }

        //methodssl_cert.pem
        void sayHello(const Json::Value& request, Json::Value& response)
        {
            response = "Hello: " + request["name"].asString();
        }

        //notification
        void notifyServer(const Json::Value& request)
        {
            cout << "server received some Notification" << endl;
        }

};

int main(int argc, char** argv)
{
    try
    {
        SampleWebsocketServer serv;
        if (serv.StartListening())
        {
            cout << "Server started successfully" << endl;
            getchar();
            serv.StopListening();
        }
        else
        {
            cout << "Error starting Server" << endl;
        }
    }
    catch (jsonrpc::JsonRpcException& e)
    {
        cerr << e.what() << endl;
    }
}
