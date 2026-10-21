int main() {
    try{cargarDatos();}catch(const exception& error){cerr<<error.what()<<"\n";return 1;}
    int port=8080;
    if(const char* value=getenv("EUROSKY_PORT")){try{port=stoi(value);}catch(...){return 1;}}
    if(port<1024 || port>65535)return 1;
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        cerr << "No se pudo iniciar Winsock.\n";
        return 1;
    }

    SOCKET server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) {
        cerr << "No se pudo crear socket.\n";
        WSACleanup();
        return 1;
    }

    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(static_cast<unsigned short>(port));

    if (bind(server, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        cerr << "No se pudo usar el puerto 8080. Cierre otro servidor o cambie el puerto.\n";
        closesocket(server);
        WSACleanup();
        return 1;
    }

    listen(server, SOMAXCONN);
    cout << "EuroSky Connect Web activo en http://127.0.0.1:"<<port<<"\n";
    cout << "Endpoints de consulta, comparacion, reporte y registro activos.\n";

    while (true) {
        SOCKET client = accept(server, NULL, NULL);
        if (client != INVALID_SOCKET) {
            DWORD timeoutMs = 500;
            setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
            setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeoutMs, sizeof(timeoutMs));
            manejarCliente(client);
            closesocket(client);
        }
    }

    closesocket(server);
    WSACleanup();
    return 0;
}



