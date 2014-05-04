/*************************************************************************
 * libjson-rpc-cpp
 *************************************************************************
 * @file websocketserver.h
 * @date 06.12.2013
 * @author toolmmy
 * @license See attached LICENSE.txt
 ************************************************************************/

#ifndef WEBSOCKETSERVER_H_
#define WEBSOCKETSERVER_H_

#include <list>

#include "mongoose.h"
#include "../serverconnector.h"
#include "../debug.h"

namespace jsonrpc
{
    struct WebsocketConnection
    {
            std::string protocol;
            mg_connection* conn;
    };

    /**
     * This class provides an embedded HTTP Server based on Mongoose 3.8 in order to handle incoming JSON-RPC request via websockets.
     * TODO:
     * 	- implement protocol whitelisting
     */
    class WebsocketServer: public AbstractServerConnector
    {
        public:

            typedef std::list<WebsocketConnection> websocketConnectionList;

            /**
             * @brief Constructor that creates a new WebSocketServer instance.
             * @param port - int of the used TCP-Port
             * @param websocketProtocol - string of the supported protocol used for the websocket connection
             * @param enableSpecification - bool that allows the access to the server specification via HTTP GET.
             */
            WebsocketServer(unsigned int port, const std::string& websocketProtocol, bool enableSpecification = true, unsigned int thread = 10);
            virtual ~WebsocketServer();

            /**
             * @brief Starts the WebsocketServer in listening mode.
             */
            virtual bool StartListening();

            /**
             * @brief Stops the WebsocketServer listening mode and closes the embedded Mongoose server.
             */
            virtual bool StopListening();

            /**
             * @brief Sends a 'PING' broadcast to all connected clients. This method is used to maintain the association to the clients in order to keep them alive.
             */
            void SendPingBroadcast();

            /**
             * @brief Sends the response of a JSON-RPC request to the client.
             */
            bool virtual SendResponse(const std::string& response, void* addInfo = NULL);

            bool virtual SendEvent(const std::string& resonse);

        private:

            unsigned int _port;
            std::string _protocol;
            bool _showSpec;

            bool _isRunning;
            unsigned int _numThreads;
            int _connMaintainerThread;
            websocketConnectionList _wsConnections;

            struct mg_context *_ctx;

            bool SendPing(struct mg_connection*);
            bool SendPong(struct mg_connection*);

            bool SendData(struct mg_connection*, const unsigned int opCode, const std::string& data);

            bool IsClientMaintenanceThreadRunning();
            void StartMaintenanceThread();
            void StopMaintenanceThread();

            bool RemoveConnection(struct mg_connection*);

            static void handleWebsocketStatusCode(const int statusCode, struct mg_connection* connection);

            /**
             * @brief Generic mongoose callback that handles all incoming requests (including GET and POST).
             * @param connection - mg_connection that represents the current connection.
             */
            static int websocketConnectionRequestCallback(struct mg_connection *connection);

            /**
             * @brief Callback that indicates an established websocket connection
             * @param connection - mg_connection that represents the current connection.
             */
            static int websocketConnectCallback(const struct mg_connection *connection);

            /**
             * @brief Callback that indicates new data received via the websocket.
             * @param connection - mg_connection that represents the current connection.
             * @param bits - int first byte of the websocket frame, see websocket RFC at http://tools.ietf.org/html/rfc6455, section 5.2
             * @param data - char pointer to the received payload
             * @param data_len - size_t of the payload length
             */
            static int websocketDataCallback(struct mg_connection *, int bits, char *data, size_t data_len);

            /**
             * @brief Callback that indicates a sucessful websocket handshake
             * @param connection - mg_connection that represents the current connection.
             */
            static void websocketReadyCallback(struct mg_connection *connection);

            /**
             * @brief Utility method that verifies a connection in order to distinguish between websocket and a plain HTTP connection.
             * @param connection - mg_connection that represents the current connection.
             */
            static bool isWebsocketConnection(struct mg_connection *connection);

            static void* sendContinuousPing(void* data);
    };

} /* namespace jsonrpc */

#endif /* WEBSOCKETSERVER_H_ */
