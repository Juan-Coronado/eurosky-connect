string jsonRecorridoOperativo(const vector<Ruta>& recorrido, int origen, const string& adicional = "") {
    double costoRutasEUR = costoRecorridoEUR(recorrido);
    double ingresoTotalEUR = 0;
    for (size_t i = 0; i < recorrido.size(); ++i) ingresoTotalEUR += ingresoRutaEUR(recorrido[i]);
    double costoTotalEUR = avion.costoDiario + costoRutasEUR;
    ostringstream out;
    out << fixed << setprecision(2);
    out << "{\"ruta\":\"" << codigosRecorrido(recorrido, origen)
        << "\",\"tramos\":" << recorrido.size()
        << ",\"tiempoMin\":" << tiempoRecorrido(recorrido)
        << ",\"costoEUR\":" << costoRutasEUR
        << ",\"costoTotalEUR\":" << costoTotalEUR
        << ",\"ingresoEUR\":" << ingresoTotalEUR
        << ",\"beneficioEUR\":" << beneficioRecorridoEUR(recorrido)
        << ",\"beneficioNetoEUR\":" << (ingresoTotalEUR - costoTotalEUR)
        << ",\"detalle\":[";
    for (size_t i = 0; i < recorrido.size(); ++i) {
        const Ruta& r = recorrido[i];
        if (i) out << ",";
        out << "{\"origen\":\"" << aeropuertos[r.origen].codigo
            << "\",\"destino\":\"" << aeropuertos[r.destino].codigo
            << "\",\"origenCiudad\":\"" << jsonEscape(aeropuertos[r.origen].ciudad)
            << "\",\"destinoCiudad\":\"" << jsonEscape(aeropuertos[r.destino].ciudad)
            << "\",\"lat1\":" << aeropuertos[r.origen].latitud
            << ",\"lng1\":" << aeropuertos[r.origen].longitud
            << ",\"lat2\":" << aeropuertos[r.destino].latitud
            << ",\"lng2\":" << aeropuertos[r.destino].longitud
            << ",\"distanciaKm\":" << r.distanciaKm
            << ",\"tiempoMin\":" << r.tiempoMin
            << ",\"pasajeros\":" << pasajerosAtendidos(r)
            << ",\"costoEUR\":" << costoRutaEUR(r)
            << ",\"ingresoEUR\":" << ingresoRutaEUR(r)
            << ",\"beneficioEUR\":" << beneficioRutaEUR(r) << "}";
    }
    out << "]" << adicional << "}";
    return out.str();
}

string jsonComparacion(const string& origenCodigo) {
    int origen = indicePorCodigo(origenCodigo);
    if (origen < 0) return "{\"error\":\"Aeropuerto de origen no encontrado\"}";
    vector<Ruta> greedy = recorridoGreedy(origen);
    vector<Ruta> dinamica = rutaProgramacionDinamica(origen);
    int destino = greedy.empty() ? origen : greedy.back().destino;
    vector<Ruta> dijkstra = rutaDijkstra(origen, destino);
    bool cicloNegativo = false;
    vector<Ruta> bellman = rutaBellmanFord(origen, destino, cicloNegativo);
    vector<int> bfs = recorridoBFS(origen);
    vector<int> dfs = recorridoDFS(origen);
    vector<bool> alcanzable(grafo.size(), false);
    for (size_t i = 0; i < bfs.size(); ++i) alcanzable[bfs[i]] = true;
    ostringstream out;
    out << fixed << setprecision(2);
    out << "{\"origen\":\"" << aeropuertos[origen].codigo << "\",\"destinoComparacion\":\"" << aeropuertos[destino].codigo
        << "\",\"capacidadAeronave\":" << avion.capacidad << ",\"costoAlquilerEUR\":" << avion.costoDiario << ",";
    out << "\"bfs\":{\"visitados\":" << bfs.size() << ",\"orden\":\"" << codigosExploracion(bfs)
        << "\",\"ordenCompleto\":\"" << codigosExploracionCompleta(bfs) << "\"},";
    out << "\"dfs\":{\"visitados\":" << dfs.size() << ",\"orden\":\"" << codigosExploracion(dfs)
        << "\",\"ordenCompleto\":\"" << codigosExploracionCompleta(dfs) << "\"},";
    out << "\"greedy\":" << jsonRecorridoOperativo(greedy, origen) << ",";
    out << "\"dijkstra\":" << jsonRecorridoOperativo(dijkstra, origen, ",\"estructura\":\"min-heap\"") << ",";
    out << "\"bellmanFord\":{\"ruta\":\"" << codigosRecorrido(bellman, origen) << "\",\"tramos\":" << bellman.size()
        << ",\"cicloNegativo\":" << (cicloNegativo ? "true" : "false")
        << ",\"criterio\":\"costo menos ingreso\",\"rutasNegativas\":[";
    bool primeraNegativa = true;
    for (size_t u = 0; u < grafo.size(); ++u) {
        if (!alcanzable[u]) continue;
        for (size_t i = 0; i < grafo[u].size(); ++i) {
            const Ruta& r = grafo[u][i];
            double pesoNeto = costoRutaEUR(r) - ingresoRutaEUR(r);
            if (pesoNeto < 0) {
                if (!primeraNegativa) out << ",";
                primeraNegativa = false;
                out << "{\"ruta\":\"" << aeropuertos[r.origen].codigo << " -> " << aeropuertos[r.destino].codigo
                    << "\",\"pesoNetoEUR\":" << pesoNeto
                    << ",\"beneficioEUR\":" << beneficioRutaEUR(r) << "}";
            }
        }
    }
    out << "]},";
    out << "\"programacionDinamica\":" << jsonRecorridoOperativo(
        dinamica, origen, ",\"criterio\":\"maximo beneficio con 480 min y 3 tramos\""
    ) << "}";
    return out.str();
}

string reporteTexto(const string& origenCodigo) {
    int origen = indicePorCodigo(origenCodigo);
    if (origen < 0) return "ERROR: Aeropuerto de origen no encontrado.\r\n";
    vector<Ruta> greedy = recorridoGreedy(origen);
    vector<Ruta> dinamica = rutaProgramacionDinamica(origen);
    ostringstream out;
    out << fixed << setprecision(2);
    out << "REPORTE EUROSKY CONNECT\r\n";
    out << "Origen: " << aeropuertos[origen].codigo << " - " << aeropuertos[origen].ciudad << "\r\n";
    out << "Aeropuertos disponibles: " << aeropuertos.size() << "\r\n";
    out << "Jornada maxima: " << JORNADA_MAXIMA_MIN << " min | Capacidad: " << CAPACIDAD_MAXIMA << " pasajeros\r\n\r\n";
    out << "GREEDY\r\nRuta: " << codigosRecorrido(greedy, origen) << "\r\nTiempo: " << tiempoRecorrido(greedy)
        << " min\r\nCosto: EUR " << costoRecorridoEUR(greedy) << "\r\nBeneficio: EUR " << beneficioRecorridoEUR(greedy) << "\r\n\r\n";
    out << "PROGRAMACION DINAMICA\r\nRuta: " << codigosRecorrido(dinamica, origen) << "\r\nTiempo: " << tiempoRecorrido(dinamica)
        << " min\r\nCosto: EUR " << costoRecorridoEUR(dinamica) << "\r\nBeneficio: EUR " << beneficioRecorridoEUR(dinamica) << "\r\n";
    return out.str();
}

string registrarAeropuerto(const map<string, string>& p) {
    vector<string> requeridos = {"codigo", "nombre", "ciudad", "pais", "lat", "lng", "moneda", "simbolo", "cambio"};
    for (size_t i = 0; i < requeridos.size(); ++i) {
        if (!p.count(requeridos[i]) || p.at(requeridos[i]).empty()) return "{\"error\":\"Faltan datos del aeropuerto\"}";
    }
    if(aeropuertos.size()>=64)return "{\"error\":\"Limite de 64 nodos de esta version academica\"}";
    if(p.at("codigo").size()!=3 || !all_of(p.at("codigo").begin(),p.at("codigo").end(),[](unsigned char c){return c>='A' && c<='Z';}))return "{\"error\":\"IATA debe tener 3 letras mayusculas\"}";
    if (indicePorCodigo(p.at("codigo")) >= 0) return "{\"error\":\"El codigo IATA ya existe\"}";
    Aeropuerto a;
    a.codigo = p.at("codigo");
    a.nombre = p.at("nombre");
    a.ciudad = p.at("ciudad");
    a.pais = p.at("pais");
    a.latitud = atof(p.at("lat").c_str());
    a.longitud = atof(p.at("lng").c_str());
    a.moneda = p.at("moneda");
    a.simbolo = p.at("simbolo");
    a.tipoCambioEUR = atof(p.at("cambio").c_str());
    if (a.tipoCambioEUR <= 0 || abs(a.latitud)>90 || abs(a.longitud)>180) return "{\"error\":\"El tipo de cambio debe ser positivo\"}";
    ostringstream archivo;
    archivo << "\n" << a.codigo << ";" << a.nombre << ";" << a.ciudad << ";" << a.pais << ";"
        << a.latitud << ";" << a.longitud << ";" << a.moneda << ";" << a.simbolo << ";" << a.tipoCambioEUR;
    if(!agregarFilaCSV("datos/aeropuertos.csv",archivo.str()))return "{\"error\":\"No se pudo guardar aeropuertos.csv\"}";
    aeropuertos.push_back(a);
    grafo.push_back(vector<Ruta>());
    return "{\"ok\":true,\"mensaje\":\"Aeropuerto registrado\"}";
}

string registrarRuta(const map<string, string>& p) {
    vector<string> requeridos = {"origen", "destino", "distancia", "tiempo", "demanda", "precio", "tasa", "costoKm"};
    for (size_t i = 0; i < requeridos.size(); ++i) {
        if (!p.count(requeridos[i]) || p.at(requeridos[i]).empty()) return "{\"error\":\"Faltan datos de la ruta\"}";
    }
    int origen = indicePorCodigo(p.at("origen"));
    int destino = indicePorCodigo(p.at("destino"));
    if (origen < 0 || destino < 0 || origen == destino) return "{\"error\":\"Origen o destino invalido\"}";
    Ruta r;
    r.origen = origen;
    r.destino = destino;
    r.distanciaKm = atoi(p.at("distancia").c_str());
    r.tiempoMin = atoi(p.at("tiempo").c_str());
    r.demanda = atoi(p.at("demanda").c_str());
    r.precioPromedio = atof(p.at("precio").c_str());
    r.tasaAeroportuaria = atof(p.at("tasa").c_str());
    r.costoOperativoKm = atof(p.at("costoKm").c_str());
    if (r.distanciaKm <= 0 || r.tiempoMin <= 0 || r.demanda < 0 || r.precioPromedio<0 || r.tasaAeroportuaria<0 || r.costoOperativoKm<0) return "{\"error\":\"Valores de ruta invalidos\"}";
    ostringstream archivo;
    archivo << "\n" << p.at("origen") << ";" << p.at("destino") << ";" << r.distanciaKm << ";" << r.tiempoMin
        << ";" << r.demanda << ";" << r.precioPromedio << ";" << r.tasaAeroportuaria << ";" << r.costoOperativoKm;
    if(!agregarFilaCSV("datos/rutas.csv",archivo.str()))return "{\"error\":\"No se pudo guardar rutas.csv\"}";
    grafo[origen].push_back(r);
    return "{\"ok\":true,\"mensaje\":\"Ruta registrada\"}";
}

string actualizarAeronave(const map<string, string>& p) {
    if (!p.count("modelo") || !p.count("capacidad") || !p.count("costoDiario") || !p.count("autonomia")) {
        return "{\"error\":\"Faltan datos de la aeronave\"}";
    }
    if (aeronaves.empty()) aeronaves.push_back(aeronaveBase());
    int indice = 0;
    if (p.count("codigo") && !p.at("codigo").empty()) {
        indice = indiceAeronavePorCodigo(p.at("codigo"));
        if (indice < 0) return "{\"error\":\"La aeronave no existe\"}";
    } else {
        indice = indiceAeronavePorCodigo(avion.codigo);
        if (indice < 0) indice = 0;
    }
    Aeronave actualizada = aeronaves[indice];
    int capacidad = atoi(p.at("capacidad").c_str());
    actualizada.modelo = p.at("modelo");
    actualizada.capacidad = capacidad;
    actualizada.costoDiario = atof(p.at("costoDiario").c_str());
    actualizada.autonomiaKm = atoi(p.at("autonomia").c_str());
    string error;
    if (!aeronaveValida(actualizada, error)) return "{\"error\":\"" + error + "\"}";
    Aeronave anterior=aeronaves[indice], activaAnterior=avion;
    aeronaves[indice] = actualizada;
    avion = actualizada;
    if (!guardarAeronaves(indice)){aeronaves[indice]=anterior;avion=activaAnterior;return "{\"error\":\"No se pudo actualizar aeronave.csv\"}";}
    return "{\"ok\":true,\"mensaje\":\"Aeronave actualizada\"}";
}

string registrarAeronave(const map<string, string>& p) {
    vector<string> requeridos = {"codigo", "modelo", "capacidad", "costoDiario", "autonomia"};
    for (size_t i = 0; i < requeridos.size(); ++i) {
        if (!p.count(requeridos[i]) || p.at(requeridos[i]).empty()) return "{\"error\":\"Faltan datos de la aeronave\"}";
    }
    if (indiceAeronavePorCodigo(p.at("codigo")) >= 0) return "{\"error\":\"El codigo de aeronave ya existe\"}";
    Aeronave nueva;
    nueva.codigo = p.at("codigo");
    nueva.modelo = p.at("modelo");
    nueva.capacidad = atoi(p.at("capacidad").c_str());
    nueva.costoDiario = atof(p.at("costoDiario").c_str());
    nueva.autonomiaKm = atoi(p.at("autonomia").c_str());
    string error;
    if (!aeronaveValida(nueva, error)) return "{\"error\":\"" + error + "\"}";
    aeronaves.push_back(nueva);
    int activa = indiceAeronavePorCodigo(avion.codigo);
    if (activa < 0) activa = 0;
    if (!guardarAeronaves(activa)){aeronaves.pop_back();return "{\"error\":\"No se pudo actualizar aeronave.csv\"}";}
    return "{\"ok\":true,\"mensaje\":\"Aeronave registrada\"}";
}

string seleccionarAeronave(const map<string, string>& p) {
    if (!p.count("codigo") || p.at("codigo").empty()) return "{\"error\":\"Falta el codigo de aeronave\"}";
    int indice = indiceAeronavePorCodigo(p.at("codigo"));
    if (indice < 0) return "{\"error\":\"La aeronave no existe\"}";
    Aeronave anterior=avion;
    avion = aeronaves[indice];
    if (!guardarAeronaves(indice)){avion=anterior;return "{\"error\":\"No se pudo actualizar aeronave.csv\"}";}
    return "{\"ok\":true,\"mensaje\":\"Aeronave activa seleccionada\"}";
}

string jsonAeropuertos() {
    ostringstream out;
    out << "{\"aeropuertos\":[";
    for (size_t i = 0; i < aeropuertos.size(); ++i) {
        if (i) out << ",";
        out << "{\"codigo\":\"" << aeropuertos[i].codigo
            << "\",\"nombre\":\"" << jsonEscape(aeropuertos[i].nombre)
            << "\",\"ciudad\":\"" << jsonEscape(aeropuertos[i].ciudad)
            << "\",\"pais\":\"" << jsonEscape(aeropuertos[i].pais)
            << "\",\"moneda\":\"" << aeropuertos[i].moneda
            << "\",\"simbolo\":\"" << jsonEscape(aeropuertos[i].simbolo)
            << "\",\"tipoCambioEUR\":" << fixed << setprecision(4) << aeropuertos[i].tipoCambioEUR
            << ",\"lat\":" << fixed << setprecision(6) << aeropuertos[i].latitud
            << ",\"lng\":" << fixed << setprecision(6) << aeropuertos[i].longitud << "}";
    }
    out << "],\"avion\":{\"codigo\":\"" << avion.codigo << "\",\"modelo\":\"" << jsonEscape(avion.modelo)
        << "\",\"capacidad\":" << avion.capacidad << ",\"costoDiario\":" << avion.costoDiario
        << ",\"autonomiaKm\":" << avion.autonomiaKm << "},\"aeronaves\":[";
    for (size_t i = 0; i < aeronaves.size(); ++i) {
        if (i) out << ",";
        out << "{\"codigo\":\"" << aeronaves[i].codigo
            << "\",\"modelo\":\"" << jsonEscape(aeronaves[i].modelo)
            << "\",\"capacidad\":" << aeronaves[i].capacidad
            << ",\"costoDiario\":" << aeronaves[i].costoDiario
            << ",\"autonomiaKm\":" << aeronaves[i].autonomiaKm
            << ",\"activa\":" << (aeronaves[i].codigo == avion.codigo ? "true" : "false") << "}";
    }
    out << "]}";
    return out.str();
}

string jsonCalculo(const string& origenCodigo) {
    int origen = indicePorCodigo(origenCodigo);
    if (origen < 0) return "{\"error\":\"Aeropuerto de origen no encontrado\"}";
    vector<Ruta> recorrido = recorridoGreedy(origen);
    double costoEUR = avion.costoDiario;
    double ingresoEUR = 0;
    int tiempo = 0;
    ostringstream out;
    out << fixed << setprecision(2);
    out << "{\"origen\":\"" << aeropuertos[origen].codigo << "\",";
    out << "\"jornadaMaximaMin\":" << JORNADA_MAXIMA_MIN << ",";
    out << "\"capacidadMaxima\":" << CAPACIDAD_MAXIMA << ",";
    out << "\"capacidadAeronave\":" << avion.capacidad << ",";
    out << "\"costoAlquilerEUR\":" << avion.costoDiario << ",";
    out << "\"aeronaveCodigo\":\"" << avion.codigo << "\",";
    out << "\"aeronaveModelo\":\"" << jsonEscape(avion.modelo) << "\",";
    out << "\"monedaTotal\":\"EUR\",";
    out << "\"criterioMonetario\":\"Importes por tramo en moneda local del origen; comparacion y totales normalizados a EUR\",";
    out << "\"algoritmo\":\"Greedy por beneficio por minuto\",";
    out << "\"ruta\":[";
    for (size_t i = 0; i < recorrido.size(); ++i) {
        const Ruta& r = recorrido[i];
        if (i) out << ",";
        costoEUR += costoRutaEUR(r);
        ingresoEUR += ingresoRutaEUR(r);
        tiempo += r.tiempoMin + ESCALA_MIN;
        out << "{\"origen\":\"" << aeropuertos[r.origen].codigo
            << "\",\"destino\":\"" << aeropuertos[r.destino].codigo
            << "\",\"origenCiudad\":\"" << aeropuertos[r.origen].ciudad
            << "\",\"destinoCiudad\":\"" << aeropuertos[r.destino].ciudad
            << "\",\"moneda\":\"" << aeropuertos[r.origen].moneda
            << "\",\"simbolo\":\"" << jsonEscape(aeropuertos[r.origen].simbolo)
            << "\",\"lat1\":" << aeropuertos[r.origen].latitud
            << ",\"lng1\":" << aeropuertos[r.origen].longitud
            << ",\"lat2\":" << aeropuertos[r.destino].latitud
            << ",\"lng2\":" << aeropuertos[r.destino].longitud
            << ",\"distanciaKm\":" << r.distanciaKm
            << ",\"tiempoMin\":" << r.tiempoMin
            << ",\"pasajeros\":" << pasajerosAtendidos(r)
            << ",\"costoLocal\":" << costoRutaLocal(r)
            << ",\"ingresoLocal\":" << ingresoRutaLocal(r)
            << ",\"beneficioLocal\":" << (ingresoRutaLocal(r) - costoRutaLocal(r))
            << ",\"costoEUR\":" << costoRutaEUR(r)
            << ",\"ingresoEUR\":" << ingresoRutaEUR(r)
            << ",\"beneficioEUR\":" << beneficioRutaEUR(r) << "}";
    }
    out << "],";
    out << "\"tiempoTotalMin\":" << tiempo << ",";
    out << "\"costoTotalEUR\":" << costoEUR << ",";
    out << "\"ingresoTotalEUR\":" << ingresoEUR << ",";
    out << "\"beneficioNetoEUR\":" << (ingresoEUR - costoEUR) << "}";
    return out.str();
}

string jsonMonteCarlo(const string& origenCodigo, int iteraciones) {
    int origen = indicePorCodigo(origenCodigo);
    if (origen < 0) return "{\"error\":\"Aeropuerto de origen no encontrado\"}";
    vector<Ruta> recorrido = recorridoGreedy(origen);
    default_random_engine gen((unsigned)time(NULL));
    double sumaBeneficio = 0;
    double minBeneficio = 1e18;
    double maxBeneficio = -1e18;
    for (int it = 0; it < iteraciones; ++it) {
        double costo = avion.costoDiario;
        double ingreso = 0;
        for (size_t i = 0; i < recorrido.size(); ++i) {
            Ruta r = recorrido[i];
            int capacidadOperativa = min(avion.capacidad, CAPACIDAD_MAXIMA);
            int minDemanda = min(capacidadOperativa, max(0, (int)floor(r.demanda * 0.75)));
            int maxDemanda = min(capacidadOperativa, (int)ceil(r.demanda * 1.15));
            uniform_int_distribution<int> dist(minDemanda, maxDemanda);
            int pasajeros = min(dist(gen), capacidadOperativa);
            costo += costoRutaEUR(r);
            ingreso += convertirAEUR(pasajeros * r.precioPromedio, r.origen);
        }
        double beneficio = ingreso - costo;
        sumaBeneficio += beneficio;
        minBeneficio = min(minBeneficio, beneficio);
        maxBeneficio = max(maxBeneficio, beneficio);
    }
    ostringstream out;
    out << fixed << setprecision(2);
    out << "{\"iteraciones\":" << iteraciones
        << ",\"beneficioPromedio\":" << (sumaBeneficio / iteraciones)
        << ",\"beneficioMinimo\":" << minBeneficio
        << ",\"beneficioMaximo\":" << maxBeneficio
        << ",\"moneda\":\"EUR\""
        << ",\"descripcion\":\"Simulacion Monte Carlo de pasajeros con variacion de demanda por ruta, normalizada a EUR\"}";
    return out.str();
}

