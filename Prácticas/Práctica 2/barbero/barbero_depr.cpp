#include <cassert>
#include <condition_variable>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>
#include "HoareMonitor.h"

using namespace std;
using namespace HM;

mutex write_mtx;                    // cerrojo de stdout
constexpr int num_clientes = 10;    // número de clientes

/**
 * @brief Genera un entero aleatorio uniformemente distribuido entre dos valores
 * enteros, ambos incluidos.
 * @param min: primer valor
 * @param max: último valor
 * @note Ambos tienen que ser dos constantes, conocidas en tiempo de compilación
 */
template <int min, int max>
int aleatorio() {
    static default_random_engine generador((random_device())());
    static uniform_int_distribution<int> distribucion_uniforme(min, max);
    return distribucion_uniforme(generador);
}

/**
 * @brief Función de cortar pelo
 * 
 * Realiza una espera de tiempo aleatorio
 */
void cortarPelo() {
    chrono::milliseconds time(aleatorio<20,100>());

    write_mtx.lock();
    cout << "======Cortando el pelo del cliente... (" << time.count() << "ms)" << endl;
    write_mtx.unlock();

    this_thread::sleep_for(time);

    write_mtx.lock();
    cout << "======El pelo del cliente ha sido cortado" << endl;
    write_mtx.unlock();
}

/**
 * @brief Función de crecer pelo (estar fuera de la barbería)
 * @param num_cliente: número de la hebra cliente
 * 
 * Realiza una espera de tiempo aleatorio
 */
void crecerPelo(int num_cliente) {
    chrono::milliseconds time(aleatorio<20,100>());

    write_mtx.lock();
    cout << "======A " << num_cliente << " le está creciendo el pelo ("
         << time.count() << "ms)" << endl;
    write_mtx.unlock();

    this_thread::sleep_for(time);
    
    write_mtx.lock();
    cout << "======A " << num_cliente << " ya le ha crecido el pelo" << endl;
    write_mtx.unlock();
}


/**
 * @brief Monitor de barbería
 * 
 * @author Miguel Ángel Fernández Gutiérrez
 */
class Barberia : public HoareMonitor {
private:
    /**
     * @brief Cola en la que duerme el barbero
     */
    CondVar barbero;

    /**
     * @brief Cola en la que esperan las hebras cliente si la silla está
     * ocupada
     */
    CondVar salaEspera;

    /**
     * @brief Cola que representa si hay alguien pelándose
     */
    CondVar silla;

    /**
     * @brief Número de clientes que están en la barbería
     */
    int num_clientes;
public:
    /**
     * @brief Constructor por defecto
     */
    Barberia();

    /**
     * @brief Acción de cortar el pelo
     */
    void cortarPelo(int num_cliente);

    /**
     * @brief Acción de llamar al siguiente cliente
     */
    void siguienteCliente();

    /**
     * @brief Acción de acabar de pelar a un cliente
     */
    void finCliente();
};

Barberia::Barberia() {
    num_clientes = 0;
    barbero = newCondVar();
    salaEspera = newCondVar();
    silla = newCondVar();
}

void Barberia::cortarPelo(int num_cliente) {
    num_clientes++;

    write_mtx.lock();
    cout << "El cliente " << num_cliente << " entra a que le corten el pelo" << endl;
    write_mtx.unlock();

    // si el barbero está durmiendo
    if (!barbero.empty()) {
        write_mtx.lock();
        cout << "El cliente " << num_cliente << " despierta al barbero" << endl;
        write_mtx.unlock();
        barbero.signal();
    }

    if ( num_clientes > 1 ) {   // esperamos si hay alguien pelándose o en cola
        write_mtx.lock();
        cout << "El cliente " << num_cliente << " acaba de entrar en la sala de espera." << endl;
        write_mtx.unlock();
        salaEspera.wait();
    }

    write_mtx.lock();
    cout << "El cliente " << num_cliente << " entra a pelarse" << endl;
    write_mtx.unlock();
    silla.wait();   // esperamos a que el cliente se pele
    write_mtx.lock();
    cout << "El cliente " << num_cliente << " se ha pelado y se marcha" << endl;
    write_mtx.unlock();
}

void Barberia::siguienteCliente() {
    if ( num_clientes == 0 ) {
        write_mtx.lock();
        cout << "------------El barbero está durmiendo" << endl;
        write_mtx.unlock();
        barbero.wait();
    }

    if ( silla.empty() ) {  // no se está pelando a nadie
        write_mtx.lock();
        cout << "------------El barbero llama a un cliente" << endl;
        write_mtx.unlock();
        salaEspera.signal();
    }
}

void Barberia::finCliente() {
    write_mtx.lock();
    cout << "------------------El barbero ha terminado de pelar al cliente" << endl;
    write_mtx.unlock();
    silla.signal();
    num_clientes--;
}

void hebra_barbero(MRef<Barberia> monitor) {
    while (true) {
        monitor->siguienteCliente();    // espera a un cliente
        cortarPelo();                   // le corta el pelo al cliente (retraso)
        monitor->finCliente();          // avisa al cliente de que le ha cortado el pelo
    }
}

void hebra_clientes(MRef<Barberia> monitor, int num_cliente) {
    while (true) {
        monitor->cortarPelo(num_cliente);   // cliente solicita que le corten el pelo
        crecerPelo(num_cliente);            // al cliente le crece el pelo (retraso)
    }
}


int main() {
    cout << "___________________________________________________" << endl
         << endl << "PROBLEMA DEL BARBERO | MONITOR SU" << endl
         << "Alumno: Miguel Ángel Fernández Gutiérrez" << endl
         << "___________________________________________________" << endl << endl;

    MRef<Barberia> monitor = Create<Barberia>();

    thread hebra_b(hebra_barbero, monitor);
    thread hebra_c[num_clientes];
    for ( int i = 0; i < num_clientes; i++ )
        hebra_c[i] = thread(hebra_clientes, monitor, i);
    
    hebra_b.join();
    for ( int i = 0; i < num_clientes; i++ ) hebra_c[i].join();

    return 0;
}