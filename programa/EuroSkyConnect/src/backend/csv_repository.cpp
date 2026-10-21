bool guardarCSVAtomico(const string& nombre,const string& contenido){
    string temporal=nombre+".tmp";
    ofstream out(temporal,ios::binary|ios::trunc);if(!out)return false;
    out<<contenido;out.close();
    if(!out){remove(temporal.c_str());return false;}
    if(!MoveFileExA(temporal.c_str(),nombre.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)){remove(temporal.c_str());return false;}
    return true;
}
bool agregarFilaCSV(const string& nombre,const string& fila){
    ifstream in(nombre,ios::binary);if(!in)return false;
    ostringstream contenido;contenido<<in.rdbuf();in.close();
    return guardarCSVAtomico(nombre,contenido.str()+fila);
}

vector<string> splitLine(const string& line, char sep) {
    vector<string> parts;
    string part;
    stringstream ss(line);
    while (getline(ss, part, sep)) parts.push_back(part);
    return parts;
}

int buscarIndiceCodigoLocal(const string& codigo) {
    for (size_t i = 0; i < aeropuertos.size(); ++i) {
        if (aeropuertos[i].codigo == codigo) return (int)i;
    }
    return -1;
}

int indiceAeronavePorCodigo(const string& codigo) {
    for (size_t i = 0; i < aeronaves.size(); ++i) {
        if (aeronaves[i].codigo == codigo) return (int)i;
    }
    return -1;
}

bool guardarAeronaves(int activa) {
    if (activa < 0 || activa >= (int)aeronaves.size()) activa = 0;
    ostringstream archivo;
    archivo << "codigo;modelo;capacidad;costoDiarioEUR;autonomiaKm;activa\n";
    for (size_t i = 0; i < aeronaves.size(); ++i) {
        archivo << aeronaves[i].codigo << ";" << aeronaves[i].modelo << ";" << aeronaves[i].capacidad
            << ";" << aeronaves[i].costoDiario << ";" << aeronaves[i].autonomiaKm << ";"
            << ((int)i == activa ? 1 : 0);
        if (i + 1 < aeronaves.size()) archivo << "\n";
    }
    return guardarCSVAtomico("datos/aeronave.csv",archivo.str());
}

bool aeronaveValida(const Aeronave& a, string& error) {
    if (a.codigo.empty() || a.modelo.empty()) {
        error = "Faltan codigo o modelo de la aeronave";
        return false;
    }
    if (a.capacidad <= 0 || a.capacidad > CAPACIDAD_MAXIMA) {
        error = "La capacidad debe estar entre 1 y 255";
        return false;
    }
    if (!isfinite(a.costoDiario) || a.costoDiario < 0 || a.autonomiaKm <= 0) {
        error = "Costo diario o autonomia invalidos";
        return false;
    }
    return true;
}

bool cargarDatosDesdeArchivos() {
    ifstream fa("datos/aeropuertos.csv");
    ifstream fr("datos/rutas.csv");
    if (!fa || !fr) return false;

    vector<Aeropuerto> nuevosAeropuertos;
    string line;
    getline(fa, line);
    while (getline(fa, line)) {
        if (line.empty()) continue;
        vector<string> c = splitLine(line, ';');
        if (c.size() < 9) continue;
        Aeropuerto a;
        a.codigo = c[0];
        a.nombre = c[1];
        a.ciudad = c[2];
        a.pais = c[3];
        a.latitud = atof(c[4].c_str());
        a.longitud = atof(c[5].c_str());
        a.moneda = c[6];
        a.simbolo = c[7];
        a.tipoCambioEUR = atof(c[8].c_str());
        nuevosAeropuertos.push_back(a);
    }
    if(nuevosAeropuertos.empty() || nuevosAeropuertos.size()>64)return false;
    for(const auto& a:nuevosAeropuertos)if(!textoCSVValido(a.nombre) || !textoCSVValido(a.ciudad) || !textoCSVValido(a.codigo) || !isfinite(a.tipoCambioEUR) || a.tipoCambioEUR<=0)return false;

    aeropuertos = nuevosAeropuertos;
    grafo.assign(aeropuertos.size(), vector<Ruta>());
    avion = aeronaveBase();
    aeronaves.clear();
    ifstream favion("datos/aeronave.csv");
    if (favion) {
        getline(favion, line);
        int activa = -1;
        while (getline(favion, line)) {
            if (line.empty()) continue;
            vector<string> c = splitLine(line, ';');
            if (c.size() >= 5) {
                Aeronave a;
                a.codigo = c[0];
                a.modelo = c[1];
                a.capacidad = atoi(c[2].c_str());
                a.costoDiario = atof(c[3].c_str());
                a.autonomiaKm = atoi(c[4].c_str());
                string error;
                if (!aeronaveValida(a, error)) continue;
                if (c.size() >= 6 && (c[5] == "1" || c[5] == "true" || c[5] == "TRUE")) activa = (int)aeronaves.size();
                aeronaves.push_back(a);
            }
        }
        if (!aeronaves.empty()) avion = aeronaves[activa >= 0 ? activa : 0];
    }
    if (aeronaves.empty()) {
        aeronaves.push_back(avion);
    }

    getline(fr, line);
    while (getline(fr, line)) {
        if (line.empty()) continue;
        vector<string> c = splitLine(line, ';');
        if (c.size() < 8) continue;
        int origen = buscarIndiceCodigoLocal(c[0]);
        int destino = buscarIndiceCodigoLocal(c[1]);
        if (origen < 0 || destino < 0) continue;
        Ruta r;
        r.origen = origen;
        r.destino = destino;
        r.distanciaKm = atoi(c[2].c_str());
        r.tiempoMin = atoi(c[3].c_str());
        r.demanda = atoi(c[4].c_str());
        r.precioPromedio = atof(c[5].c_str());
        r.tasaAeroportuaria = atof(c[6].c_str());
        r.costoOperativoKm = atof(c[7].c_str());
        grafo[origen].push_back(r);
    }
    return true;
}


void cargarDatos() {
    if (cargarDatosDesdeArchivos()) return;
    if(filesystem::exists("datos/aeropuertos.csv") || filesystem::exists("datos/rutas.csv"))throw runtime_error("Datos CSV invalidos; no se sobrescriben");
    aeropuertos.clear();
    aeropuertos.push_back({"MAD", "Adolfo Suarez Madrid-Barajas", "Madrid", "Espana", 40.4983, -3.5676, "EUR", "EUR", 1.0});
    aeropuertos.push_back({"BCN", "Josep Tarradellas Barcelona-El Prat", "Barcelona", "Espana", 41.2974, 2.0833, "EUR", "EUR", 1.0});

    avion = aeronaveBase();
    aeronaves.clear();
    aeronaves.push_back(avion);
    grafo.assign(aeropuertos.size(), vector<Ruta>());

    vector<Ruta> rutas = {
        {0, 1, 505, 75, 240, 110, 2800, 5.5},
        {1, 0, 505, 75, 235, 110, 2800, 5.5}
    };
    for (size_t i = 0; i < rutas.size(); ++i) {
        grafo[rutas[i].origen].push_back(rutas[i]);
    }
}

int indicePorCodigo(const string& codigo) {
    for (size_t i = 0; i < aeropuertos.size(); ++i) {
        if (aeropuertos[i].codigo == codigo) return (int)i;
    }
    return -1;
}

