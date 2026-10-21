#define main servidor_main
#include "../programa/EuroSkyConnect/backend.cpp"
#undef main
#include <cassert>
int main(){
    aeropuertos={{"AAA","A","A","A",0,0,"EUR","EUR",1},{"BBB","B","B","B",0,0,"EUR","EUR",1},{"CCC","C","C","C",0,0,"EUR","EUR",1}};
    avion={"DEMO","Prueba",100,0,10000};grafo.assign(3,{});
    grafo[0]={{0,1,10,10,10,10,0,1},{0,2,100,10,10,10,0,1}};
    grafo[1]={{1,2,10,10,10,10,0,1}};
    auto minima=rutaDijkstra(0,2);assert(minima.size()==2 && costoRecorridoEUR(minima)==20);
    assert(recorridoBFS(0).size()==3 && recorridoDFS(0).size()==3);
    assert(rutaDijkstra(-1,2).empty());
    assert(tiempoRecorrido(rutaProgramacionDinamica(0))<=JORNADA_MAXIMA_MIN);
    bool ciclo=false;assert(rutaBellmanFord(0,2,ciclo).size()==2 && !ciclo);
    grafo[2].push_back({2,0,1,10,10,10,0,1});rutaBellmanFord(0,2,ciclo);assert(ciclo);
    assert(!parametrosValidos({{"lat","nan"}}));
    assert(!parametrosValidos({{"nombre","<script>"}}));
    assert(!parametrosValidos({{"capacidad","12x"}}));
    assert(parametrosValidos({{"lat","-12.5"},{"capacidad","100"}}));
    std::cout<<"Grafos: Dijkstra, BFS, DFS, DP, Bellman-Ford y validacion comprobados\n";
}
