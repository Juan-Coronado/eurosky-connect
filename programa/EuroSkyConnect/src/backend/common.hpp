#define NOMINMAX
#include <filesystem>
#include <stdexcept>
#include <cctype>
#include <algorithm>
#include <cmath>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>

using namespace std;

const int CAPACIDAD_MAXIMA = 255;
const int JORNADA_MAXIMA_MIN = 480;
const int ESCALA_MIN = 45;

struct Aeropuerto {
    string codigo;
    string nombre;
    string ciudad;
    string pais;
    double latitud;
    double longitud;
    string moneda;
    string simbolo;
    double tipoCambioEUR;
};

struct Aeronave {
    string codigo;
    string modelo;
    int capacidad;
    double costoDiario;
    int autonomiaKm;
};

struct Ruta {
    int origen;
    int destino;
    int distanciaKm;
    int tiempoMin;
    int demanda;
    double precioPromedio;
    double tasaAeroportuaria;
    double costoOperativoKm;
};

vector<Aeropuerto> aeropuertos;
vector<vector<Ruta> > grafo;
vector<Aeronave> aeronaves;
Aeronave avion;

