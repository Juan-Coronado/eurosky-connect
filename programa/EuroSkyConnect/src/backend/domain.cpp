Aeronave aeronaveBase() {
    return {"A321-R01", "Airbus A321neo (configuracion academica)", 255, 28500, 6100};
}

string jsonEscape(const string& s) {
    string out;
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == '"') out += "\\\"";
        else if (s[i] == '\\') out += "\\\\";
        else out += s[i];
    }
    return out;
}

double costoRutaLocal(const Ruta& r) {
    return r.distanciaKm * r.costoOperativoKm + r.tasaAeroportuaria;
}

int pasajerosAtendidos(const Ruta& r) {
    return min(r.demanda, min(avion.capacidad, CAPACIDAD_MAXIMA));
}

double ingresoRutaLocal(const Ruta& r) {
    return pasajerosAtendidos(r) * r.precioPromedio;
}

string urlDecode(const string& value) {
    string out;
    for (size_t i = 0; i < value.size(); ++i) {
        if (value[i] == '+') out += ' ';
        else if (value[i] == '%' && i + 2 < value.size()) {
            string hex = value.substr(i + 1, 2);
            out += (char)strtol(hex.c_str(), NULL, 16);
            i += 2;
        } else out += value[i];
    }
    return out;
}

double convertirAEUR(double importeLocal, int aeropuertoOrigen) {
    double cambio = aeropuertos[aeropuertoOrigen].tipoCambioEUR;
    if (cambio <= 0) return importeLocal;
    return importeLocal / cambio;
}

double costoRutaEUR(const Ruta& r) {
    return convertirAEUR(costoRutaLocal(r), r.origen);
}

double ingresoRutaEUR(const Ruta& r) {
    return convertirAEUR(ingresoRutaLocal(r), r.origen);
}

double beneficioRutaEUR(const Ruta& r) {
    return ingresoRutaEUR(r) - costoRutaEUR(r);
}

double eficienciaRuta(const Ruta& r) {
    int tiempoTotal = r.tiempoMin + ESCALA_MIN;
    if (tiempoTotal <= 0) return 0.0;
    return beneficioRutaEUR(r) / tiempoTotal;
}



bool textoCSVValido(const string& value){
    return value.size()<=256 && none_of(value.begin(),value.end(),[](unsigned char c){return c<32 || c==127 || c==';' || c=='<' || c=='>' || c=='"' || c=='&';});
}
bool parametrosValidos(const map<string,string>& params){
    const vector<string> decimales={"lat","lng","cambio","precio","tasa","costoKm","costoDiario"};
    const vector<string> enteros={"distancia","tiempo","demanda","capacidad","autonomia","iteraciones"};
    for(const auto& par:params){
        if(!textoCSVValido(par.second))return false;
        bool decimal=find(decimales.begin(),decimales.end(),par.first)!=decimales.end();
        bool entero=find(enteros.begin(),enteros.end(),par.first)!=enteros.end();
        if(decimal || entero){
            try {size_t end=0;double number=stod(par.second,&end);
                if(end!=par.second.size() || !isfinite(number) || abs(number)>1e9 || (entero && (number<0 || floor(number)!=number)))return false;
            }catch(...){return false;}
        }
    }
    return true;
}
