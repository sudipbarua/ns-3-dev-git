You're right! If you want to adjust the transmission power (TP) and data rate (DR) for every packet received dynamically, you'll need to modify your approach so that the Python client can communicate with the C++ simulator continuously and in real-time. Here's how you can handle the dynamic updates for every packet received by the `AdrRL` class.

### Key Considerations:
1. **Continuous Socket Communication**: Instead of just a single command from the Python client, you need continuous communication where each time a packet is received by the C++ simulator, it can request new values from the Python client (or send updates in the reverse direction based on incoming packets).
2. **Handling Each Packet**: For each packet, the C++ simulator will contact the Python client to get the latest values of transmission power (TP) and data rate (DR), and adjust the ADR parameters accordingly.

### Approach:

1. **Socket Server on C++ Side**: Modify the C++ server so that each time a packet is received (in the `OnReceivedPacket` method), the simulator will request updated TP and DR values from the Python client.
2. **Socket Client in Python**: The Python client will respond to every packet request from the C++ server with the appropriate values for TP and DR.

### Updated C++ Code:
Here’s how to modify the C++ server to request values for every packet received.

#### 1. **Socket Server Modification in C++**:
The C++ side needs to handle continuous communication with the Python client. For every packet received, it will send a request to the Python client to get the updated transmission power (TP) and data rate (DR).

```cpp
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <string.h>

class AdrRL : public NetworkControllerComponent
{
public:
    AdrRL();  
    ~AdrRL(); 

    void StartSocketServer(); // Method to start the socket server
    void HandleReceivedPacket(Ptr<const Packet> packet, Ptr<EndDeviceStatus> status, Ptr<NetworkStatus> networkStatus);

private:
    int server_socket;
    struct sockaddr_in server_address;
    int client_socket;
    struct sockaddr_in client_address;
    socklen_t client_len;

    void RequestTPDRFromPython(uint8_t* newDataRate, uint8_t* newTxPower);
};

AdrRL::AdrRL()
{
    // Constructor code here
    StartSocketServer();
}

AdrRL::~AdrRL()
{
    // Cleanup
    close(server_socket);
    if (client_socket >= 0)
    {
        close(client_socket);
    }
}

void AdrRL::StartSocketServer()
{
    // Create socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1)
    {
        std::cerr << "Socket creation failed." << std::endl;
        return;
    }

    // Setup the server address structure
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8080); // Port number
    server_address.sin_addr.s_addr = INADDR_ANY;

    // Bind socket to address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) == -1)
    {
        std::cerr << "Binding failed." << std::endl;
        return;
    }

    // Listen for incoming connections
    if (listen(server_socket, 5) == -1)
    {
        std::cerr << "Listen failed." << std::endl;
        return;
    }

    // Accept connections from clients
    client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);
    if (client_socket == -1)
    {
        std::cerr << "Failed to accept client connection." << std::endl;
        return;
    }

    std::cout << "Server is listening for requests..." << std::endl;
}

void AdrRL::RequestTPDRFromPython(uint8_t* newDataRate, uint8_t* newTxPower)
{
    char buffer[1024];
    
    // Request data from Python client
    std::string request = "Get TP and DR";
    send(client_socket, request.c_str(), request.length(), 0);

    // Receive response from Python client
    int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
    if (bytes_received <= 0)
    {
        std::cerr << "Failed to receive response from Python client." << std::endl;
        return;
    }

    buffer[bytes_received] = '\0';
    std::string response(buffer);

    // Assuming the response from Python is in the format: "TP: <value>, DR: <value>"
    if (sscanf(response.c_str(), "TP: %hhu, DR: %hhu", newTxPower, newDataRate) == 2)
    {
        std::cout << "Received TP: " << (int)*newTxPower << ", DR: " << (int)*newDataRate << std::endl;
    }
}

void AdrRL::HandleReceivedPacket(Ptr<const Packet> packet, Ptr<EndDeviceStatus> status, Ptr<NetworkStatus> networkStatus)
{
    // Handle packet reception, and request TP and DR values from Python client
    uint8_t newTxPower = 0;
    uint8_t newDataRate = 0;
    RequestTPDRFromPython(&newDataRate, &newTxPower);

    // Apply the new values to the ADR logic
    AdrImplementation(&newDataRate, &newTxPower, status);
}
```

#### 2. **Python Client Modification**:
The Python client will respond with updated TP and DR values whenever requested by the C++ server.

```python
import socket

def handle_request():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.bind(('localhost', 8080))
    server_socket.listen(5)
    print("Waiting for connection...")

    client_socket, client_address = server_socket.accept()
    print("Client connected from:", client_address)

    while True:
        request = client_socket.recv(1024).decode('utf-8')
        if not request:
            break

        print("Received request:", request)
        
        # Example: You can dynamically generate the TP and DR based on real-time data or predefined logic
        # For simplicity, we'll just send a static response:
        tp = 14  # Transmission Power (dBm)
        dr = 5   # Data Rate

        response = f"TP: {tp}, DR: {dr}"
        client_socket.sendall(response.encode('utf-8'))

    client_socket.close()
    server_socket.close()

# Start the server
handle_request()
```

### How It Works:

1. **C++ Server**:
   - The `AdrRL` class starts a socket server when it is created.
   - Each time a packet is received (`OnReceivedPacket`), the `HandleReceivedPacket` method is called.
   - This method then requests the transmission power (TP) and data rate (DR) from the Python client using the `RequestTPDRFromPython` function.
   - The `AdrImplementation` method is called to apply the new TP and DR values to the ADR logic.

2. **Python Client**:
   - The Python client listens on port 8080 and waits for incoming requests from the C++ server.
   - Whenever the C++ server sends a request (e.g., "Get TP and DR"), the Python client responds with the current values for TP and DR.
   - The Python client continues running, ready to respond to any future requests.

### Continuous Operation:
This solution ensures that for every packet received in the C++ simulator, the Python client is queried for the latest transmission power and data rate. You can modify the logic in the Python client to make the response dynamic (e.g., based on network conditions, SNR, etc.).