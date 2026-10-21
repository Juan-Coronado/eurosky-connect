string leerArchivo(const string& ruta) {
    ifstream f(ruta.c_str(), ios::in | ios::binary);
    if (!f) return "";
    ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

map<string, string> queryParams(const string& path) {
    map<string, string> params;
    size_t q = path.find('?');
    if (q == string::npos) return params;
    string query = path.substr(q + 1);
    stringstream ss(query);
    string part;
    while (getline(ss, part, '&')) {
        size_t eq = part.find('=');
        if (eq != string::npos) params[urlDecode(part.substr(0, eq))] = urlDecode(part.substr(eq + 1));
    }
    return params;
}

void responder(SOCKET client, const string& content, const string& type, int status = 200) {
    ostringstream headers;
    string estado = status == 200 ? "OK" : (status == 400 ? "Bad Request" : "Not Found");
    headers << "HTTP/1.1 " << status << " " << estado << "\r\n";
    headers << "Content-Type: " << type << "; charset=utf-8\r\n";
    headers << "X-Content-Type-Options: nosniff\r\n";
    headers << "Content-Length: " << content.size() << "\r\n";
    headers << "Connection: close\r\n\r\n";
    string response = headers.str() + content;
    size_t offset=0;
    while(offset<response.size()){
        int sent=send(client,response.data()+offset,static_cast<int>(response.size()-offset),0);
        if(sent<=0)break;
        offset+=sent;
    }
}

void manejarCliente(SOCKET client) {
    char buffer[8192];
    int bytes = recv(client, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return;
    buffer[bytes] = '\0';
    string request(buffer);
    string method, path, version;
    stringstream firstLine(request);
    firstLine >> method >> path >> version;
    map<string, string> params = queryParams(path);
    string cleanPath = path.substr(0, path.find('?'));

    string cabeceras=request;transform(cabeceras.begin(),cabeceras.end(),cabeceras.begin(),[](unsigned char c){return static_cast<char>(tolower(c));});
    bool mutation=cleanPath.find("/registrar")!=string::npos || cleanPath.find("/seleccionar")!=string::npos || cleanPath.find("/actualizar")!=string::npos;
    if((mutation && (method!="POST" || cabeceras.find("x-eurosky-request: local")==string::npos)) || cabeceras.find("sec-fetch-site: cross-site")!=string::npos){
        responder(client,"{\"error\":\"Operacion no permitida\"}","application/json",400);return;
    }
    if(!parametrosValidos(params)){responder(client,"{\"error\":\"Parametros no validos\"}","application/json",400);return;}
    if(cleanPath=="/config.js"){
        const char* key=getenv("GOOGLE_MAPS_API_KEY");
        responder(client,"window.EUROSKY_CONFIG={googleMapsApiKey:\""+jsonEscape(key?key:"")+"\"};","application/javascript");
    } else if (cleanPath == "/" || cleanPath == "/index.html") {
        string html = leerArchivo("web/index.html");
        responder(client, html.empty() ? "<h1>No se encontro web/index.html</h1>" : html, "text/html");
    } else if (cleanPath == "/static/css/style.css") {
        string css = leerArchivo("web/static/css/style.css");
        responder(client, css.empty() ? "/* No se encontro style.css */" : css, "text/css", css.empty() ? 404 : 200);
    } else if (cleanPath == "/static/js/app.js") {
        string js = leerArchivo("web/static/js/app.js");
        responder(client, js.empty() ? "console.error('No se encontro app.js');" : js, "application/javascript", js.empty() ? 404 : 200);
    } else if (cleanPath == "/api/aeropuertos") {
        responder(client, jsonAeropuertos(), "application/json");
    } else if (cleanPath == "/api/rutas/calcular") {
        string origen = params.count("origen") ? params["origen"] : "MAD";
        responder(client, jsonCalculo(origen), "application/json", indicePorCodigo(origen) < 0 ? 400 : 200);
    } else if (cleanPath == "/api/simulacion/montecarlo") {
        string origen = params.count("origen") ? params["origen"] : "MAD";
        int iter = params.count("iteraciones") ? atoi(params["iteraciones"].c_str()) : 1000;
        if (iter < 10) iter = 10;
        if (iter > 10000) iter = 10000;
        responder(client, jsonMonteCarlo(origen, iter), "application/json", indicePorCodigo(origen) < 0 ? 400 : 200);
    } else if (cleanPath == "/api/algoritmos/comparar") {
        string origen = params.count("origen") ? params["origen"] : "MAD";
        responder(client, jsonComparacion(origen), "application/json", indicePorCodigo(origen) < 0 ? 400 : 200);
    } else if (cleanPath == "/api/reporte") {
        string origen = params.count("origen") ? params["origen"] : "MAD";
        responder(client, reporteTexto(origen), "text/plain", indicePorCodigo(origen) < 0 ? 400 : 200);
    } else if (cleanPath == "/api/aeropuertos/registrar") {
        string resultado = registrarAeropuerto(params);
        responder(client, resultado, "application/json", resultado.find("\"error\"") != string::npos ? 400 : 200);
    } else if (cleanPath == "/api/rutas/registrar") {
        string resultado = registrarRuta(params);
        responder(client, resultado, "application/json", resultado.find("\"error\"") != string::npos ? 400 : 200);
    } else if (cleanPath == "/api/aeronaves/registrar") {
        string resultado = registrarAeronave(params);
        responder(client, resultado, "application/json", resultado.find("\"error\"") != string::npos ? 400 : 200);
    } else if (cleanPath == "/api/aeronaves/seleccionar") {
        string resultado = seleccionarAeronave(params);
        responder(client, resultado, "application/json", resultado.find("\"error\"") != string::npos ? 400 : 200);
    } else if (cleanPath == "/api/aeronave/actualizar") {
        string resultado = actualizarAeronave(params);
        responder(client, resultado, "application/json", resultado.find("\"error\"") != string::npos ? 400 : 200);
    } else {
        responder(client, "{\"error\":\"Endpoint no encontrado\"}", "application/json", 404);
    }
}

