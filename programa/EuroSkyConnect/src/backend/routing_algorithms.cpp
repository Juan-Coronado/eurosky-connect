vector<Ruta> recorridoGreedy(int origen) {
    if(origen<0 || origen>=static_cast<int>(grafo.size()))return {};
    vector<Ruta> recorrido;
    int actual = origen;
    int tiempoUsado = 0;
    int escalas = 0;
    vector<bool> usado(grafo.size(), false);
    usado[actual] = true;
    while (escalas < 3) {
        int mejor = -1;
        double mejorEficiencia = -1e18;
        for (size_t i = 0; i < grafo[actual].size(); ++i) {
            const Ruta& r = grafo[actual][i];
            int tiempoConEscala = r.tiempoMin + ESCALA_MIN;
            if (!usado[r.destino] && tiempoUsado + tiempoConEscala <= JORNADA_MAXIMA_MIN && beneficioRutaEUR(r) > 0) {
                double eficiencia = eficienciaRuta(r);
                if (eficiencia > mejorEficiencia) {
                    mejorEficiencia = eficiencia;
                    mejor = (int)i;
                }
            }
        }
        if (mejor < 0) break;
        Ruta elegida = grafo[actual][mejor];
        recorrido.push_back(elegida);
        tiempoUsado += elegida.tiempoMin + ESCALA_MIN;
        actual = elegida.destino;
        usado[actual] = true;
        escalas++;
    }
    return recorrido;
}

vector<int> recorridoBFS(int origen) {
    vector<int> orden;
    if (origen < 0 || origen >= (int)grafo.size()) return orden;
    vector<bool> visitado(grafo.size(), false);
    queue<int> pendientes;
    visitado[origen] = true;
    pendientes.push(origen);
    while (!pendientes.empty()) {
        int actual = pendientes.front();
        pendientes.pop();
        orden.push_back(actual);
        for (size_t i = 0; i < grafo[actual].size(); ++i) {
            int siguiente = grafo[actual][i].destino;
            if (!visitado[siguiente]) {
                visitado[siguiente] = true;
                pendientes.push(siguiente);
            }
        }
    }
    return orden;
}

void dfsVisitar(int actual, vector<bool>& visitado, vector<int>& orden) {
    visitado[actual] = true;
    orden.push_back(actual);
    for (size_t i = 0; i < grafo[actual].size(); ++i) {
        int siguiente = grafo[actual][i].destino;
        if (!visitado[siguiente]) dfsVisitar(siguiente, visitado, orden);
    }
}

vector<int> recorridoDFS(int origen) {
    vector<int> orden;
    if (origen < 0 || origen >= (int)grafo.size()) return orden;
    vector<bool> visitado(grafo.size(), false);
    dfsVisitar(origen, visitado, orden);
    return orden;
}

vector<Ruta> reconstruirRuta(int origen, int destino, const vector<int>& padre, const vector<int>& aristaPadre) {
    vector<Ruta> resultado;
    if (destino < 0 || destino >= (int)grafo.size()) return resultado;
    int actual = destino;
    while (actual != origen && padre[actual] >= 0) {
        int anterior = padre[actual];
        int indiceArista = aristaPadre[actual];
        if (indiceArista < 0) break;
        resultado.push_back(grafo[anterior][indiceArista]);
        actual = anterior;
    }
    if (actual != origen) return vector<Ruta>();
    reverse(resultado.begin(), resultado.end());
    return resultado;
}

vector<Ruta> rutaDijkstra(int origen, int destino) {
    if(origen<0 || destino<0 || origen>=static_cast<int>(grafo.size()) || destino>=static_cast<int>(grafo.size()))return {};
    const double INF = numeric_limits<double>::infinity();
    vector<double> distancia(grafo.size(), INF);
    vector<int> padre(grafo.size(), -1);
    vector<int> aristaPadre(grafo.size(), -1);
    priority_queue<pair<double, int>, vector<pair<double, int> >, greater<pair<double, int> > > heap;
    distancia[origen] = 0;
    heap.push(make_pair(0.0, origen));
    while (!heap.empty()) {
        double acumulado = heap.top().first;
        int actual = heap.top().second;
        heap.pop();
        if (acumulado > distancia[actual]) continue;
        for (size_t i = 0; i < grafo[actual].size(); ++i) {
            const Ruta& r = grafo[actual][i];
            double nuevo = acumulado + costoRutaEUR(r);
            if (nuevo < distancia[r.destino]) {
                distancia[r.destino] = nuevo;
                padre[r.destino] = actual;
                aristaPadre[r.destino] = (int)i;
                heap.push(make_pair(nuevo, r.destino));
            }
        }
    }
    return reconstruirRuta(origen, destino, padre, aristaPadre);
}

vector<Ruta> rutaBellmanFord(int origen, int destino, bool& cicloNegativo) {
    cicloNegativo=false;
    if(origen<0 || destino<0 || origen>=static_cast<int>(grafo.size()) || destino>=static_cast<int>(grafo.size()))return {};
    const double INF = numeric_limits<double>::infinity();
    vector<double> distancia(grafo.size(), INF);
    vector<int> padre(grafo.size(), -1);
    vector<int> aristaPadre(grafo.size(), -1);
    distancia[origen] = 0;
    for (size_t paso = 1; paso < grafo.size(); ++paso) {
        bool cambio = false;
        for (size_t u = 0; u < grafo.size(); ++u) {
            if (distancia[u] == INF) continue;
            for (size_t i = 0; i < grafo[u].size(); ++i) {
                const Ruta& r = grafo[u][i];
                double pesoNeto = costoRutaEUR(r) - ingresoRutaEUR(r);
                if (distancia[r.destino] > distancia[u] + pesoNeto) {
                    distancia[r.destino] = distancia[u] + pesoNeto;
                    padre[r.destino] = (int)u;
                    aristaPadre[r.destino] = (int)i;
                    cambio = true;
                }
            }
        }
        if (!cambio) break;
    }
    cicloNegativo = false;
    for (size_t u = 0; u < grafo.size(); ++u) {
        if (distancia[u] == INF) continue;
        for (size_t i = 0; i < grafo[u].size(); ++i) {
            const Ruta& r = grafo[u][i];
            double pesoNeto = costoRutaEUR(r) - ingresoRutaEUR(r);
            if (distancia[r.destino] > distancia[u] + pesoNeto) cicloNegativo = true;
        }
    }
    if (cicloNegativo) return vector<Ruta>();
    return reconstruirRuta(origen, destino, padre, aristaPadre);
}

string claveDP(int actual, int tiempo, int tramos, unsigned long long visitados) {
    ostringstream out;
    out << actual << "|" << tiempo << "|" << tramos << "|" << visitados;
    return out.str();
}

double beneficioDPMemo(
    int actual,
    int tiempo,
    int tramos,
    unsigned long long visitados,
    unordered_map<string, double>& memo,
    unordered_map<string, int>& decision
) {
    if (tramos >= 3) return 0;
    string clave = claveDP(actual, tiempo, tramos, visitados);
    if (memo.count(clave)) return memo[clave];
    double mejor = 0;
    int mejorArista = -1;
    for (size_t i = 0; i < grafo[actual].size(); ++i) {
        const Ruta& r = grafo[actual][i];
        unsigned long long bit = 1ULL << r.destino;
        int nuevoTiempo = tiempo + r.tiempoMin + ESCALA_MIN;
        if ((visitados & bit) != 0 || nuevoTiempo > JORNADA_MAXIMA_MIN) continue;
        double candidato = beneficioRutaEUR(r) + beneficioDPMemo(
            r.destino, nuevoTiempo, tramos + 1, visitados | bit, memo, decision
        );
        if (candidato > mejor) {
            mejor = candidato;
            mejorArista = (int)i;
        }
    }
    memo[clave] = mejor;
    decision[clave] = mejorArista;
    return mejor;
}

vector<Ruta> rutaProgramacionDinamica(int origen) {
    if(origen<0 || origen>=static_cast<int>(grafo.size()))return {};
    unordered_map<string, double> memo;
    unordered_map<string, int> decision;
    unsigned long long visitados = 1ULL << origen;
    beneficioDPMemo(origen, 0, 0, visitados, memo, decision);
    vector<Ruta> resultado;
    int actual = origen;
    int tiempo = 0;
    int tramos = 0;
    while (tramos < 3) {
        string clave = claveDP(actual, tiempo, tramos, visitados);
        if (!decision.count(clave) || decision[clave] < 0) break;
        Ruta elegida = grafo[actual][decision[clave]];
        resultado.push_back(elegida);
        tiempo += elegida.tiempoMin + ESCALA_MIN;
        actual = elegida.destino;
        visitados |= 1ULL << actual;
        tramos++;
    }
    return resultado;
}

double costoRecorridoEUR(const vector<Ruta>& recorrido) {
    double total = 0;
    for (size_t i = 0; i < recorrido.size(); ++i) total += costoRutaEUR(recorrido[i]);
    return total;
}

double beneficioRecorridoEUR(const vector<Ruta>& recorrido) {
    double total = 0;
    for (size_t i = 0; i < recorrido.size(); ++i) total += beneficioRutaEUR(recorrido[i]);
    return total;
}

int tiempoRecorrido(const vector<Ruta>& recorrido) {
    int total = 0;
    for (size_t i = 0; i < recorrido.size(); ++i) total += recorrido[i].tiempoMin + ESCALA_MIN;
    return total;
}

string codigosRecorrido(const vector<Ruta>& recorrido, int origen) {
    ostringstream out;
    out << aeropuertos[origen].codigo;
    for (size_t i = 0; i < recorrido.size(); ++i) out << " -> " << aeropuertos[recorrido[i].destino].codigo;
    return out.str();
}

string codigosExploracion(const vector<int>& orden) {
    ostringstream out;
    size_t limite = min((size_t)12, orden.size());
    for (size_t i = 0; i < limite; ++i) {
        if (i) out << " -> ";
        out << aeropuertos[orden[i]].codigo;
    }
    if (orden.size() > limite) out << " -> ...";
    return out.str();
}

string codigosExploracionCompleta(const vector<int>& orden) {
    ostringstream out;
    for (size_t i = 0; i < orden.size(); ++i) {
        if (i) out << " -> ";
        out << aeropuertos[orden[i]].codigo;
    }
    return out.str();
}
