#include <iostream>
#include <vector>
#include <random>
#include <map>

using namespace std;

struct TresEnRaya {
  string tablero;
  string jugador;
  string pass;
  char ficha;
  int id = 0;
  int socket = 0;
  TresEnRaya(string cli);
  bool hayGanador(char turno);
  void movimiento(int posicion, char ficha);
};

TresEnRaya::TresEnRaya(string cli) {
    jugador = cli;
    tablero = "000000000";
}

void TresEnRaya::movimiento(int posicion, char ficha) {
    tablero[posicion-1] = ficha;
}